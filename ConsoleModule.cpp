// ConsoleModule.cpp
#include <JavaScriptCore/JavaScript.h>
#include <string>
#include <iostream>
#include "ConsoleModule.h"

// Définition de la fonction logCallback
JSValueRef logCallback(JSContextRef context, JSObjectRef function, JSObjectRef thisObject, size_t argumentCount, const JSValueRef arguments[], JSValueRef* exception) {
    if (argumentCount == 0) {
        return JSValueMakeUndefined(context);
    }

    JSValueRef firstArgument = arguments[0];
    JSStringRef firstArgumentString = JSValueToStringCopy(context, firstArgument, nullptr);
    size_t bufferSize = JSStringGetMaximumUTF8CStringSize(firstArgumentString);
    char* buffer = new char[bufferSize];
    JSStringGetUTF8CString(firstArgumentString, buffer, bufferSize);
    std::string message(buffer);
    delete[] buffer;
    JSStringRelease(firstArgumentString);

    ConsoleModule::log(message);

    return JSValueMakeUndefined(context);
}

void ConsoleModule::log(const std::string& message) {
    std::cout << message << std::endl;
}

void ConsoleModule::attachToContext(JSContextRef context) {
    JSObjectRef consoleObject = JSObjectMake(context, nullptr, nullptr);
    JSStringRef logFunctionName = JSStringCreateWithUTF8CString("log");
    JSObjectRef logFunction = JSObjectMakeFunctionWithCallback(context, logFunctionName, logCallback);
    JSObjectSetProperty(context, consoleObject, logFunctionName, logFunction, kJSPropertyAttributeNone, nullptr);
    JSStringRelease(logFunctionName);
    JSObjectSetProperty(context, JSContextGetGlobalObject(context), JSStringCreateWithUTF8CString("console"), consoleObject, kJSPropertyAttributeNone, nullptr);
}