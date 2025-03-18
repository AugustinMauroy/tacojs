#include "ShittyModule.h"
#include <JavaScriptCore/JavaScript.h>
#include <iostream>

void ShittyModule::greet(JSContextRef context, JSObjectRef thisObject, size_t argumentCount, const JSValueRef arguments[]) {
    std::cout << "Hello from the ShittyModule!" << std::endl;
}

void ShittyModule::attachToContext(JSContextRef context) {
    JSObjectRef globalObject = JSContextGetGlobalObject(context);
    JSObjectRef shittyModuleObject = JSObjectMake(context, nullptr, nullptr);

    JSStringRef greetFunctionName = JSStringCreateWithUTF8CString("greet");
    JSObjectRef greetFunction = JSObjectMakeFunctionWithCallback(context, greetFunctionName, [](JSContextRef context, JSObjectRef function, JSObjectRef thisObject, size_t argumentCount, const JSValueRef arguments[], JSValueRef* exception) -> JSValueRef {
        greet(context, thisObject, argumentCount, arguments);
        return JSValueMakeUndefined(context);
    });
    JSObjectSetProperty(context, shittyModuleObject, greetFunctionName, greetFunction, kJSPropertyAttributeNone, nullptr);
    JSStringRelease(greetFunctionName);

    JSStringRef moduleName = JSStringCreateWithUTF8CString("shitty");
    JSObjectSetProperty(context, globalObject, moduleName, shittyModuleObject, kJSPropertyAttributeNone, nullptr);
    JSStringRelease(moduleName);
}
