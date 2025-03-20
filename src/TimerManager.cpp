#include "TimerManager.h"
#include <iostream>
#include <vector>
#include <thread>
#include <mutex>

namespace TimerManager {
    struct Timer {
        int id;
        std::chrono::time_point<std::chrono::steady_clock> expiry;
        JSObjectRef callback;
        JSValueRef* args;
        size_t argCount;
        bool fired;
        
        Timer(int id, std::chrono::time_point<std::chrono::steady_clock> expiry, 
              JSObjectRef callback, JSValueRef* args, size_t argCount)
            : id(id), expiry(expiry), callback(callback), args(args), argCount(argCount), fired(false) {}
        
        ~Timer() {
            delete[] args;
        }
    };
    
    static std::map<int, Timer*> timers;
    static int nextTimerId = 1;
    static std::mutex timerMutex;
    
    static JSValueRef setTimeout(JSContextRef ctx, JSObjectRef function, 
                                JSObjectRef thisObject, size_t argumentCount, 
                                const JSValueRef arguments[], JSValueRef* exception) {
        if (argumentCount < 2) {
            return JSValueMakeNumber(ctx, 0);
        }
        
        // First argument must be a function
        if (!JSValueIsObject(ctx, arguments[0]) || 
            !JSObjectIsFunction(ctx, JSValueToObject(ctx, arguments[0], exception))) {
            JSStringRef errorMsg = JSStringCreateWithUTF8CString("setTimeout requires a function as first argument");
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
        auto expiry = now + std::chrono::milliseconds(static_cast<int>(delayMs));
        
        // Create and register timer
        std::lock_guard<std::mutex> lock(timerMutex);
        int timerId = nextTimerId++;
        timers[timerId] = new Timer(timerId, expiry, callback, callbackArgs, callbackArgCount);
        
        return JSValueMakeNumber(ctx, timerId);
    }
    
    static JSValueRef clearTimeout(JSContextRef ctx, JSObjectRef function, 
                                   JSObjectRef thisObject, size_t argumentCount, 
                                   const JSValueRef arguments[], JSValueRef* exception) {
        if (argumentCount < 1 || !JSValueIsNumber(ctx, arguments[0])) {
            return JSValueMakeUndefined(ctx);
        }
        
        int timerId = static_cast<int>(JSValueToNumber(ctx, arguments[0], exception));
        
        std::lock_guard<std::mutex> lock(timerMutex);
        auto it = timers.find(timerId);
        if (it != timers.end()) {
            // Clean up the timer
            Timer* timer = it->second;
            
            // Unprotect the callback and arguments
            JSValueUnprotect(ctx, timer->callback);
            for (size_t i = 0; i < timer->argCount; i++) {
                JSValueUnprotect(ctx, timer->args[i]);
            }
            
            delete timer;
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
        
        // Register clearTimeout
        JSStringRef clearTimeoutStr = JSStringCreateWithUTF8CString("clearTimeout");
        JSObjectSetProperty(ctx, globalObj, clearTimeoutStr,
            JSObjectMakeFunctionWithCallback(ctx, clearTimeoutStr, clearTimeout),
            kJSPropertyAttributeNone, nullptr);
        JSStringRelease(clearTimeoutStr);
    }
    
    void processTimers(JSContextRef ctx) {
        auto now = std::chrono::steady_clock::now();
        std::vector<Timer*> expiredTimers;
        
        // Find expired timers
        {
            std::lock_guard<std::mutex> lock(timerMutex);
            for (auto& pair : timers) {
                Timer* timer = pair.second;
                if (!timer->fired && timer->expiry <= now) {
                    timer->fired = true;
                    expiredTimers.push_back(timer);
                }
            }
        }
        
        // Execute callbacks outside the lock
        for (Timer* timer : expiredTimers) {
            JSValueRef exception = nullptr;
            JSObjectCallAsFunction(ctx, timer->callback, nullptr, timer->argCount, timer->args, &exception);
            
            if (exception) {
                JSStringRef exceptionStr = JSValueToStringCopy(ctx, exception, nullptr);
                size_t bufferSize = JSStringGetMaximumUTF8CStringSize(exceptionStr);
                char* buffer = new char[bufferSize];
                JSStringGetUTF8CString(exceptionStr, buffer, bufferSize);
                std::cerr << "Timer callback error: " << buffer << std::endl;
                delete[] buffer;
                JSStringRelease(exceptionStr);
            }
            
            // Clean up the timer
            {
                std::lock_guard<std::mutex> lock(timerMutex);
                // Unprotect the callback and arguments
                JSValueUnprotect(ctx, timer->callback);
                for (size_t i = 0; i < timer->argCount; i++) {
                    JSValueUnprotect(ctx, timer->args[i]);
                }
                
                timers.erase(timer->id);
                delete timer;
            }
        }
    }
    
    bool hasActiveTimers() {
        std::lock_guard<std::mutex> lock(timerMutex);
        return !timers.empty();
    }
}
