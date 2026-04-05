#include "AsyncManager.h"
#include "TimerManager.h" // Add this include
#include <iostream>
#include <chrono>
#include <thread>
#include <algorithm>
#include <string>

namespace AsyncManager {
    // Global promise counter
    static int pendingPromiseCount = 0;
    static int unhandledRejectionCount = 0;
    static int handledRejectionCount = 0;

    static std::string jsValueToStdString(JSContextRef ctx, JSValueRef value) {
        JSValueRef conversionException = nullptr;
        JSStringRef strRef = JSValueToStringCopy(ctx, value, &conversionException);
        if (!strRef) {
            return "[unprintable value]";
        }

        size_t bufferSize = JSStringGetMaximumUTF8CStringSize(strRef);
        std::string result(bufferSize, '\0');
        size_t used = JSStringGetUTF8CString(strRef, result.data(), bufferSize);
        JSStringRelease(strRef);

        if (used == 0) {
            return "";
        }

        result.resize(used - 1);
        return result;
    }
    
    // Function called when a promise is created
    static JSValueRef promiseCreated(JSContextRef ctx, JSObjectRef function, 
                                    JSObjectRef thisObject, size_t argumentCount, 
                                    const JSValueRef arguments[], JSValueRef* exception) {
        pendingPromiseCount++;
        return JSValueMakeUndefined(ctx);
    }
    
    // Function called when a promise is settled (resolved or rejected)
    static JSValueRef promiseSettled(JSContextRef ctx, JSObjectRef function, 
                                    JSObjectRef thisObject, size_t argumentCount, 
                                    const JSValueRef arguments[], JSValueRef* exception) {
        if (pendingPromiseCount > 0) {
            pendingPromiseCount--;
        }
        return JSValueMakeUndefined(ctx);
    }
    
    // Function to check if there are pending promises
    static JSValueRef hasPendingPromisesFunc(JSContextRef ctx, JSObjectRef function, 
                                          JSObjectRef thisObject, size_t argumentCount, 
                                          const JSValueRef arguments[], JSValueRef* exception) {
        return JSValueMakeBoolean(ctx, pendingPromiseCount > 0);
    }

    static JSValueRef onUnhandledRejection(JSContextRef ctx, JSObjectRef function,
                                           JSObjectRef thisObject, size_t argumentCount,
                                           const JSValueRef arguments[], JSValueRef* exception) {
        unhandledRejectionCount++;
        std::string reason = "undefined";
        if (argumentCount > 0) {
            reason = jsValueToStdString(ctx, arguments[0]);
        }
        std::cerr << "Unhandled Promise Rejection: " << reason << std::endl;
        return JSValueMakeUndefined(ctx);
    }

    static JSValueRef onRejectionHandled(JSContextRef ctx, JSObjectRef function,
                                         JSObjectRef thisObject, size_t argumentCount,
                                         const JSValueRef arguments[], JSValueRef* exception) {
        handledRejectionCount++;
        return JSValueMakeUndefined(ctx);
    }

    static JSValueRef getUnhandledRejectionCount(JSContextRef ctx, JSObjectRef function,
                                                 JSObjectRef thisObject, size_t argumentCount,
                                                 const JSValueRef arguments[], JSValueRef* exception) {
        return JSValueMakeNumber(ctx, unhandledRejectionCount);
    }

    static JSValueRef getHandledRejectionCount(JSContextRef ctx, JSObjectRef function,
                                               JSObjectRef thisObject, size_t argumentCount,
                                               const JSValueRef arguments[], JSValueRef* exception) {
        return JSValueMakeNumber(ctx, handledRejectionCount);
    }
    
    bool hasPendingPromises(JSContextRef ctx) {
        return pendingPromiseCount > 0;
    }
    
    void setupAsyncSupport(JSContextRef ctx, JSObjectRef globalObj) {
        pendingPromiseCount = 0;
        unhandledRejectionCount = 0;
        handledRejectionCount = 0;

        // Add Promise tracking functions
        JSStringRef promiseCreatedStr = JSStringCreateWithUTF8CString("__promiseCreated");
        JSObjectSetProperty(ctx, globalObj, promiseCreatedStr,
            JSObjectMakeFunctionWithCallback(ctx, promiseCreatedStr, promiseCreated),
            kJSPropertyAttributeDontEnum, nullptr);
        JSStringRelease(promiseCreatedStr);
        
        JSStringRef promiseSettledStr = JSStringCreateWithUTF8CString("__promiseSettled");
        JSObjectSetProperty(ctx, globalObj, promiseSettledStr,
            JSObjectMakeFunctionWithCallback(ctx, promiseSettledStr, promiseSettled),
            kJSPropertyAttributeDontEnum, nullptr);
        JSStringRelease(promiseSettledStr);
        
        JSStringRef hasPendingPromisesStr = JSStringCreateWithUTF8CString("__hasPendingPromises");
        JSObjectSetProperty(ctx, globalObj, hasPendingPromisesStr,
            JSObjectMakeFunctionWithCallback(ctx, hasPendingPromisesStr, hasPendingPromisesFunc),
            kJSPropertyAttributeDontEnum, nullptr);
        JSStringRelease(hasPendingPromisesStr);

        JSStringRef onUnhandledRejectionStr = JSStringCreateWithUTF8CString("__onUnhandledRejection");
        JSObjectSetProperty(ctx, globalObj, onUnhandledRejectionStr,
            JSObjectMakeFunctionWithCallback(ctx, onUnhandledRejectionStr, onUnhandledRejection),
            kJSPropertyAttributeDontEnum, nullptr);
        JSStringRelease(onUnhandledRejectionStr);

        JSStringRef onRejectionHandledStr = JSStringCreateWithUTF8CString("__onRejectionHandled");
        JSObjectSetProperty(ctx, globalObj, onRejectionHandledStr,
            JSObjectMakeFunctionWithCallback(ctx, onRejectionHandledStr, onRejectionHandled),
            kJSPropertyAttributeDontEnum, nullptr);
        JSStringRelease(onRejectionHandledStr);

        JSStringRef getUnhandledCountStr = JSStringCreateWithUTF8CString("__getUnhandledRejectionCount");
        JSObjectSetProperty(ctx, globalObj, getUnhandledCountStr,
            JSObjectMakeFunctionWithCallback(ctx, getUnhandledCountStr, getUnhandledRejectionCount),
            kJSPropertyAttributeDontEnum, nullptr);
        JSStringRelease(getUnhandledCountStr);

        JSStringRef getHandledCountStr = JSStringCreateWithUTF8CString("__getHandledRejectionCount");
        JSObjectSetProperty(ctx, globalObj, getHandledCountStr,
            JSObjectMakeFunctionWithCallback(ctx, getHandledCountStr, getHandledRejectionCount),
            kJSPropertyAttributeDontEnum, nullptr);
        JSStringRelease(getHandledCountStr);
        
        // Inject Promise tracking code
        const char* promiseTrackingCode = R"(
            (function() {
                const originalPromise = Promise;
                const metaByPromise = new WeakMap();

                const ensureMeta = (promise) => {
                    if (!metaByPromise.has(promise)) {
                        metaByPromise.set(promise, {
                            handled: false,
                            reported: false,
                            reason: undefined
                        });
                    }
                    return metaByPromise.get(promise);
                };

                const scheduleUnhandledCheck = (promise) => {
                    setTimeout(() => {
                        const meta = metaByPromise.get(promise);
                        if (!meta || meta.handled || meta.reported) {
                            return;
                        }
                        meta.reported = true;
                        __onUnhandledRejection(meta.reason);
                    }, 0);
                };

                const markHandled = (promise) => {
                    const meta = metaByPromise.get(promise);
                    if (!meta || meta.handled) {
                        return;
                    }
                    meta.handled = true;
                    if (meta.reported) {
                        __onRejectionHandled(meta.reason);
                    }
                };

                const originalThen = originalPromise.prototype.then;
                const originalCatch = originalPromise.prototype.catch;

                originalPromise.prototype.then = function(onFulfilled, onRejected) {
                    if (typeof onRejected === 'function') {
                        markHandled(this);
                    }
                    return originalThen.call(this, onFulfilled, onRejected);
                };

                originalPromise.prototype.catch = function(onRejected) {
                    markHandled(this);
                    return originalCatch.call(this, onRejected);
                };
                
                function CustomPromise(executor) {
                    __promiseCreated();

                    let resolveRef;
                    let rejectRef;
                    const promise = new originalPromise((resolve, reject) => {
                        resolveRef = resolve;
                        rejectRef = reject;
                    });
                    ensureMeta(promise);

                    const wrappedResolve = (value) => {
                        __promiseSettled();
                        resolveRef(value);
                    };

                    const wrappedReject = (reason) => {
                        const meta = ensureMeta(promise);
                        meta.reason = reason;
                        __promiseSettled();
                        rejectRef(reason);
                        scheduleUnhandledCheck(promise);
                    };

                    try {
                        executor(wrappedResolve, wrappedReject);
                    } catch (error) {
                        wrappedReject(error);
                    }

                    return promise;
                }
                
                CustomPromise.resolve = function(value) {
                    if (value instanceof originalPromise) {
                        return value;
                    }
                    __promiseCreated();
                    __promiseSettled();
                    const promise = originalPromise.resolve(value);
                    ensureMeta(promise);
                    return promise;
                };
                
                CustomPromise.reject = function(reason) {
                    __promiseCreated();
                    __promiseSettled();
                    const promise = originalPromise.reject(reason);
                    const meta = ensureMeta(promise);
                    meta.reason = reason;
                    scheduleUnhandledCheck(promise);
                    return promise;
                };
                
                CustomPromise.all = originalPromise.all;
                CustomPromise.race = originalPromise.race;
                CustomPromise.allSettled = originalPromise.allSettled;
                CustomPromise.any = originalPromise.any;
                
                // Ensure prototype methods are preserved
                CustomPromise.prototype = originalPromise.prototype;
                
                // Replace global Promise
                Promise = CustomPromise;
            })();
        )";
        
        JSStringRef trackingScript = JSStringCreateWithUTF8CString(promiseTrackingCode);
        JSValueRef exception = nullptr;
        JSEvaluateScript(ctx, trackingScript, nullptr, nullptr, 0, &exception);
        JSStringRelease(trackingScript);
        
        if (exception) {
            std::cerr << "Error setting up promise tracking" << std::endl;
        }
    }
    
    void runEventLoop(JSContextRef ctx, int timeoutMs) {
        // Simple event loop with timeout
        auto startTime = std::chrono::steady_clock::now();
        bool hasTimeout = false;
        
        while (!hasTimeout && (hasPendingPromises(ctx) || TimerManager::hasActiveTimers())) {
            // Process any expired timers
            TimerManager::processTimers(ctx);

            // Sleep adaptively to avoid busy-waiting but still react quickly.
            int nextTimerMs = TimerManager::timeUntilNextTimerMs();
            int sleepMs = 2;
            if (nextTimerMs > 0) {
                sleepMs = std::min(nextTimerMs, 25);
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(sleepMs));
            
            // Check for timeout
            auto currentTime = std::chrono::steady_clock::now();
            auto elapsedMs = std::chrono::duration_cast<std::chrono::milliseconds>(
                currentTime - startTime).count();
            
            if (elapsedMs > timeoutMs) {
                std::cout << "Async operation timeout after " << timeoutMs << "ms" << std::endl;
                hasTimeout = true;
            }
        }
    }
}
