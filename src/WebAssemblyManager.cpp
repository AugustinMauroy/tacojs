#include "WebAssemblyManager.h"
#include <iostream>
#include <vector>

namespace WebAssemblyManager {
    // Helper to convert JSValue to uint8 array
    static std::vector<uint8_t> getUint8ArrayData(JSContextRef ctx, JSValueRef value, JSValueRef* exception) {
        std::vector<uint8_t> result;
        
        if (!JSValueIsObject(ctx, value)) {
            JSStringRef errorMsg = JSStringCreateWithUTF8CString("Argument must be a Uint8Array");
            *exception = JSValueMakeString(ctx, errorMsg);
            JSStringRelease(errorMsg);
            return result;
        }
        
        JSObjectRef arrayObj = JSValueToObject(ctx, value, exception);
        if (*exception) return result;
        
        // Check if it's a Uint8Array by checking for certain properties
        bool isTypedArray = false;
        JSStringRef byteLength = JSStringCreateWithUTF8CString("byteLength");
        JSValueRef byteLengthVal = JSObjectGetProperty(ctx, arrayObj, byteLength, exception);
        JSStringRelease(byteLength);
        
        if (!*exception && JSValueIsNumber(ctx, byteLengthVal)) {
            isTypedArray = true;
        }
        
        if (!isTypedArray) {
            JSStringRef errorMsg = JSStringCreateWithUTF8CString("Argument must be a Uint8Array");
            *exception = JSValueMakeString(ctx, errorMsg);
            JSStringRelease(errorMsg);
            return result;
        }
        
        // Get array data
        int length = (int)JSValueToNumber(ctx, byteLengthVal, exception);
        if (*exception) return result;
        
        // In a real implementation, you would access the buffer data
        // For this simple mock implementation, we just create a vector of the right size
        result.resize(length);
        
        return result;
    }
    
    // Changed return type from JSValueRef to JSObjectRef to match JSObjectCallAsConstructorCallback
    static JSObjectRef moduleConstructor(JSContextRef ctx, JSObjectRef constructor, 
                                        size_t argumentCount, const JSValueRef arguments[], JSValueRef* exception) {
        if (argumentCount < 1) {
            JSStringRef errorMsg = JSStringCreateWithUTF8CString("WebAssembly.Module constructor requires 1 argument");
            *exception = JSValueMakeString(ctx, errorMsg);
            JSStringRelease(errorMsg);
            return nullptr;  // Return nullptr instead of undefined
        }
        
        std::vector<uint8_t> wasmBytes = getUint8ArrayData(ctx, arguments[0], exception);
        if (*exception) return nullptr;  // Return nullptr instead of undefined
        
        std::cout << "WebAssembly.Module created with " << wasmBytes.size() << " bytes" << std::endl;
        
        // Create a simple object to represent a module
        JSObjectRef moduleObj = JSObjectMake(ctx, nullptr, nullptr);
        
        // Add some properties to make it look like a Module
        JSStringRef typeStr = JSStringCreateWithUTF8CString("type");
        JSStringRef typeValue = JSStringCreateWithUTF8CString("WebAssembly.Module");
        JSObjectSetProperty(ctx, moduleObj, typeStr, JSValueMakeString(ctx, typeValue), 
                           kJSPropertyAttributeReadOnly, nullptr);
        JSStringRelease(typeStr);
        JSStringRelease(typeValue);
        
        return moduleObj;
    }
    
    // Changed return type from JSValueRef to JSObjectRef to match JSObjectCallAsConstructorCallback
    static JSObjectRef instanceConstructor(JSContextRef ctx, JSObjectRef constructor, 
                                         size_t argumentCount, const JSValueRef arguments[], JSValueRef* exception) {
        if (argumentCount < 1) {
            JSStringRef errorMsg = JSStringCreateWithUTF8CString("WebAssembly.Instance constructor requires at least 1 argument");
            *exception = JSValueMakeString(ctx, errorMsg);
            JSStringRelease(errorMsg);
            return nullptr;  // Return nullptr instead of undefined
        }
        
        // Create a simple object to represent an instance
        JSObjectRef instanceObj = JSObjectMake(ctx, nullptr, nullptr);
        
        // Add some properties to make it look like an Instance
        JSStringRef typeStr = JSStringCreateWithUTF8CString("type");
        JSStringRef typeValue = JSStringCreateWithUTF8CString("WebAssembly.Instance");
        JSObjectSetProperty(ctx, instanceObj, typeStr, JSValueMakeString(ctx, typeValue), 
                           kJSPropertyAttributeReadOnly, nullptr);
        JSStringRelease(typeStr);
        JSStringRelease(typeValue);
        
        // Create an exports object
        JSObjectRef exportsObj = JSObjectMake(ctx, nullptr, nullptr);
        JSStringRef exportsStr = JSStringCreateWithUTF8CString("exports");
        JSObjectSetProperty(ctx, instanceObj, exportsStr, exportsObj, 
                           kJSPropertyAttributeReadOnly, nullptr);
        JSStringRelease(exportsStr);
        
        std::cout << "WebAssembly.Instance created" << std::endl;
        
        return instanceObj;
    }
    
    void setupWebAssembly(JSContextRef ctx, JSObjectRef globalObj) {
        // Create the WebAssembly object
        JSObjectRef wasmObj = JSObjectMake(ctx, nullptr, nullptr);
        
        // Create WebAssembly.Module constructor
        JSClassDefinition moduleClassDef = kJSClassDefinitionEmpty;
        moduleClassDef.className = "WebAssemblyModule";
        moduleClassDef.callAsConstructor = moduleConstructor;  // Set the constructor directly in class definition
        JSClassRef moduleClass = JSClassCreate(&moduleClassDef);
        JSObjectRef moduleConstructorObj = JSObjectMake(ctx, moduleClass, nullptr);
        JSStringRef moduleStr = JSStringCreateWithUTF8CString("Module");
        JSObjectSetProperty(ctx, wasmObj, moduleStr, moduleConstructorObj, 
                           kJSPropertyAttributeReadOnly, nullptr);
        JSStringRelease(moduleStr);
        
        // Create WebAssembly.Instance constructor
        JSClassDefinition instanceClassDef = kJSClassDefinitionEmpty;
        instanceClassDef.className = "WebAssemblyInstance";
        instanceClassDef.callAsConstructor = instanceConstructor;  // Set the constructor directly in class definition
        JSClassRef instanceClass = JSClassCreate(&instanceClassDef);
        JSObjectRef instanceConstructorObj = JSObjectMake(ctx, instanceClass, nullptr);
        JSStringRef instanceStr = JSStringCreateWithUTF8CString("Instance");
        JSObjectSetProperty(ctx, wasmObj, instanceStr, instanceConstructorObj, 
                           kJSPropertyAttributeReadOnly, nullptr);
        JSStringRelease(instanceStr);
        
        // Add the WebAssembly object to global
        JSStringRef wasmStr = JSStringCreateWithUTF8CString("WebAssembly");
        JSObjectSetProperty(ctx, globalObj, wasmStr, wasmObj, 
                           kJSPropertyAttributeReadOnly, nullptr);
        JSStringRelease(wasmStr);
    }
}
