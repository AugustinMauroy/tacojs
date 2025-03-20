#include "AsyncManager.h"
#include "TimerManager.h" // Add this include
#include <iostream>
#include <chrono>
#include <thread>

namespace AsyncManager {
    // Global promise counter
    static int pendingPromiseCount = 0;
    
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
    
    bool hasPendingPromises(JSContextRef ctx) {
        return pendingPromiseCount > 0;
    }
    
    void setupAsyncSupport(JSContextRef ctx, JSObjectRef globalObj) {
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
        
        // Inject Promise tracking code
        const char* promiseTrackingCode = R"(
            (function() {
                const originalPromise = Promise;
                
                function CustomPromise(executor) {
                    __promiseCreated();
                    
                    return new originalPromise((resolve, reject) => {
                        const wrappedResolve = (value) => {
                            __promiseSettled();
                            resolve(value);
                        };
                        
                        const wrappedReject = (reason) => {
                            __promiseSettled();
                            reject(reason);
                        };
                        
                        try {
                            executor(wrappedResolve, wrappedReject);
                        } catch (error) {
                            wrappedReject(error);
                        }
                    });
                }
                
                CustomPromise.resolve = function(value) {
                    if (value instanceof originalPromise) {
                        return value;
                    }
                    __promiseCreated();
                    const promise = originalPromise.resolve(value);
                    promise.then(() => __promiseSettled(), () => __promiseSettled());
                    return promise;
                };
                
                CustomPromise.reject = function(reason) {
                    __promiseCreated();
                    const promise = originalPromise.reject(reason);
                    promise.catch(() => __promiseSettled());
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
            
            // Give control back to the OS to process events
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            
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
