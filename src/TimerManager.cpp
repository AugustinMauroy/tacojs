#include "TimerManager.h"
#include <iostream>
#include <vector>
#include <thread>
#include <mutex>
#include <memory>
#include <limits>

namespace TimerManager {
    struct Timer {
        int id;
        std::chrono::time_point<std::chrono::steady_clock> expiry;
        JSObjectRef callback;
        JSValueRef* args;
        size_t argCount;
        bool isRepeating;
        int intervalMs;
        
        Timer(int id, std::chrono::time_point<std::chrono::steady_clock> expiry, 
              JSObjectRef callback, JSValueRef* args, size_t argCount,
              bool isRepeating, int intervalMs)
            : id(id), expiry(expiry), callback(callback), args(args), argCount(argCount),
              isRepeating(isRepeating), intervalMs(intervalMs) {}
        
        ~Timer() {
            delete[] args;
        }
    };
    
    static std::map<int, std::unique_ptr<Timer>> timers;
    static int nextTimerId = 1;
    static std::mutex timerMutex;

    static void unprotectTimerValues(JSContextRef ctx, const Timer& timer) {
        JSValueUnprotect(ctx, timer.callback);
        for (size_t i = 0; i < timer.argCount; i++) {
            JSValueUnprotect(ctx, timer.args[i]);
        }
    }

    static JSValueRef createTimer(JSContextRef ctx, size_t argumentCount,
                                  const JSValueRef arguments[], JSValueRef* exception,
                                  bool isRepeating) {
        if (argumentCount < 2) {
            return JSValueMakeNumber(ctx, 0);
        }
        
        // First argument must be a function
        if (!JSValueIsObject(ctx, arguments[0]) || 
            !JSObjectIsFunction(ctx, JSValueToObject(ctx, arguments[0], exception))) {
            JSStringRef errorMsg = JSStringCreateWithUTF8CString("Timer API requires a function as first argument");
            *exception = JSValueMakeString(ctx, errorMsg);
            JSStringRelease(errorMsg);
            return JSValueMakeNumber(ctx, 0);
        }
        
        // Second argument is a delay in ms (default to 0 if invalid)
        double delayMs = 0;
        if (JSValueIsNumber(ctx, arguments[1])) {
            delayMs = JSValueToNumber(ctx, arguments[1], exception);
            if (delayMs < 0) delayMs = 0;
        }
        int delayIntMs = static_cast<int>(std::min(delayMs, static_cast<double>(std::numeric_limits<int>::max())));
        
        // Protect the callback from garbage collection
        JSObjectRef callback = JSValueToObject(ctx, arguments[0], exception);
        JSValueProtect(ctx, callback);
        
        // Copy any additional arguments to pass to the callback
        size_t callbackArgCount = (argumentCount > 2) ? argumentCount - 2 : 0;
        JSValueRef* callbackArgs = nullptr;
        
        if (callbackArgCount > 0) {
            callbackArgs = new JSValueRef[callbackArgCount];
            for (size_t i = 0; i < callbackArgCount; i++) {
                callbackArgs[i] = arguments[i + 2];
                JSValueProtect(ctx, callbackArgs[i]);
            }
        }
        
        // Calculate expiry time
        auto now = std::chrono::steady_clock::now();
        auto expiry = now + std::chrono::milliseconds(delayIntMs);
        
        // Create and register timer
        std::lock_guard<std::mutex> lock(timerMutex);
        int timerId = nextTimerId++;
        timers[timerId] = std::make_unique<Timer>(
            timerId,
            expiry,
            callback,
            callbackArgs,
            callbackArgCount,
            isRepeating,
            delayIntMs
        );
        
        return JSValueMakeNumber(ctx, timerId);
    }
    
    static JSValueRef setTimeout(JSContextRef ctx, JSObjectRef function, 
                                JSObjectRef thisObject, size_t argumentCount, 
                                const JSValueRef arguments[], JSValueRef* exception) {
        return createTimer(ctx, argumentCount, arguments, exception, false);
    }

    static JSValueRef setInterval(JSContextRef ctx, JSObjectRef function,
                                  JSObjectRef thisObject, size_t argumentCount,
                                  const JSValueRef arguments[], JSValueRef* exception) {
        return createTimer(ctx, argumentCount, arguments, exception, true);
    }
    
    static JSValueRef clearTimer(JSContextRef ctx, JSObjectRef function,
                                 JSObjectRef thisObject, size_t argumentCount,
                                 const JSValueRef arguments[], JSValueRef* exception) {
        if (argumentCount < 1 || !JSValueIsNumber(ctx, arguments[0])) {
            return JSValueMakeUndefined(ctx);
        }
        
        int timerId = static_cast<int>(JSValueToNumber(ctx, arguments[0], exception));
        
        std::lock_guard<std::mutex> lock(timerMutex);
        auto it = timers.find(timerId);
        if (it != timers.end()) {
            unprotectTimerValues(ctx, *it->second);
            timers.erase(it);
        }
        
        return JSValueMakeUndefined(ctx);
    }
    
    void setupTimers(JSContextRef ctx, JSObjectRef globalObj) {
        // Register setTimeout
        JSStringRef setTimeoutStr = JSStringCreateWithUTF8CString("setTimeout");
        JSObjectSetProperty(ctx, globalObj, setTimeoutStr,
            JSObjectMakeFunctionWithCallback(ctx, setTimeoutStr, setTimeout),
            kJSPropertyAttributeNone, nullptr);
        JSStringRelease(setTimeoutStr);

        // Register setInterval
        JSStringRef setIntervalStr = JSStringCreateWithUTF8CString("setInterval");
        JSObjectSetProperty(ctx, globalObj, setIntervalStr,
            JSObjectMakeFunctionWithCallback(ctx, setIntervalStr, setInterval),
            kJSPropertyAttributeNone, nullptr);
        JSStringRelease(setIntervalStr);
        
        // Register clearTimeout
        JSStringRef clearTimeoutStr = JSStringCreateWithUTF8CString("clearTimeout");
        JSObjectSetProperty(ctx, globalObj, clearTimeoutStr,
            JSObjectMakeFunctionWithCallback(ctx, clearTimeoutStr, clearTimer),
            kJSPropertyAttributeNone, nullptr);
        JSStringRelease(clearTimeoutStr);

        // Register clearInterval
        JSStringRef clearIntervalStr = JSStringCreateWithUTF8CString("clearInterval");
        JSObjectSetProperty(ctx, globalObj, clearIntervalStr,
            JSObjectMakeFunctionWithCallback(ctx, clearIntervalStr, clearTimer),
            kJSPropertyAttributeNone, nullptr);
        JSStringRelease(clearIntervalStr);
    }
    
    void processTimers(JSContextRef ctx) {
        auto now = std::chrono::steady_clock::now();
        std::vector<int> expiredTimerIds;
        
        // Find expired timers
        {
            std::lock_guard<std::mutex> lock(timerMutex);
            for (auto& pair : timers) {
                Timer* timer = pair.second.get();
                if (timer->expiry <= now) {
                    expiredTimerIds.push_back(timer->id);
                }
            }
        }
        
        // Execute callbacks outside the lock
        for (int timerId : expiredTimerIds) {
            JSObjectRef callback = nullptr;
            JSValueRef* args = nullptr;
            size_t argCount = 0;
            bool isRepeating = false;
            int intervalMs = 0;

            {
                std::lock_guard<std::mutex> lock(timerMutex);
                auto it = timers.find(timerId);
                if (it == timers.end()) {
                    continue;
                }

                callback = it->second->callback;
                args = it->second->args;
                argCount = it->second->argCount;
                isRepeating = it->second->isRepeating;
                intervalMs = it->second->intervalMs;
            }

            JSValueRef exception = nullptr;
            JSObjectCallAsFunction(ctx, callback, nullptr, argCount, args, &exception);
            
            if (exception) {
                JSStringRef exceptionStr = JSValueToStringCopy(ctx, exception, nullptr);
                size_t bufferSize = JSStringGetMaximumUTF8CStringSize(exceptionStr);
                char* buffer = new char[bufferSize];
                JSStringGetUTF8CString(exceptionStr, buffer, bufferSize);
                std::cerr << "Timer callback error: " << buffer << std::endl;
                delete[] buffer;
                JSStringRelease(exceptionStr);
            }
            
            std::lock_guard<std::mutex> lock(timerMutex);
            auto it = timers.find(timerId);
            if (it == timers.end()) {
                continue;
            }

            if (isRepeating && intervalMs >= 0) {
                it->second->expiry = std::chrono::steady_clock::now() + std::chrono::milliseconds(intervalMs);
            } else {
                unprotectTimerValues(ctx, *it->second);
                timers.erase(it);
            }
        }
    }

    int timeUntilNextTimerMs() {
        std::lock_guard<std::mutex> lock(timerMutex);
        if (timers.empty()) {
            return -1;
        }

        auto now = std::chrono::steady_clock::now();
        auto nearest = std::chrono::milliseconds(std::numeric_limits<int>::max());

        for (const auto& pair : timers) {
            const auto& timer = pair.second;
            if (timer->expiry <= now) {
                return 0;
            }

            auto remaining = std::chrono::duration_cast<std::chrono::milliseconds>(timer->expiry - now);
            if (remaining < nearest) {
                nearest = remaining;
            }
        }

        if (nearest.count() > std::numeric_limits<int>::max()) {
            return std::numeric_limits<int>::max();
        }

        return static_cast<int>(nearest.count());
    }
    
    bool hasActiveTimers() {
        std::lock_guard<std::mutex> lock(timerMutex);
        return !timers.empty();
    }
}
