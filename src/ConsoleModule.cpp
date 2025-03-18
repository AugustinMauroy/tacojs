#include <JavaScriptCore/JavaScript.h>
#include <string>
#include <iostream>
#include "ConsoleModule.h"

bool supportAnsiColors() {
    const char* term = getenv("TERM");
    const char* NO_COLOR = getenv("NO_COLOR");
    return term && (std::string(term).find("xterm") != std::string::npos || std::string(term).find("color") != std::string::npos) && !NO_COLOR;
};

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

void ConsoleModule::info(const std::string& message) {
    if (supportAnsiColors()) {
        std::cout << "\033[34m[INFO]: " << message << "\033[0m" << std::endl;
    } else {
        std::cout << "[INFO]: " << message << std::endl;
    }
}

void ConsoleModule::warn(const std::string& message) {
    if (supportAnsiColors()) {
        std::cout << "\033[33m[WARN]: " << message << "\033[0m" << std::endl;
    } else {
        std::cout << "[WARN]: " << message << std::endl;
    }
}

void ConsoleModule::attachToContext(JSContextRef context) {
    JSObjectRef consoleObject = JSObjectMake(context, nullptr, nullptr);

    // Attach log function
    JSStringRef logFunctionName = JSStringCreateWithUTF8CString("log");
    JSObjectRef logFunction = JSObjectMakeFunctionWithCallback(context, logFunctionName, logCallback);
    JSObjectSetProperty(context, consoleObject, logFunctionName, logFunction, kJSPropertyAttributeNone, nullptr);
    JSStringRelease(logFunctionName);

    // Attach info function
    JSStringRef infoFunctionName = JSStringCreateWithUTF8CString("info");
    JSObjectRef infoFunction = JSObjectMakeFunctionWithCallback(context, infoFunctionName, [](JSContextRef context, JSObjectRef function, JSObjectRef thisObject, size_t argumentCount, const JSValueRef arguments[], JSValueRef* exception) -> JSValueRef {
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

        ConsoleModule::info(message);

        return JSValueMakeUndefined(context);
    });
    JSObjectSetProperty(context, consoleObject, infoFunctionName, infoFunction, kJSPropertyAttributeNone, nullptr);
    JSStringRelease(infoFunctionName);

    // Attach warn function
    JSStringRef warnFunctionName = JSStringCreateWithUTF8CString("warn");
    JSObjectRef warnFunction = JSObjectMakeFunctionWithCallback(context, warnFunctionName, [](JSContextRef context, JSObjectRef function, JSObjectRef thisObject, size_t argumentCount, const JSValueRef arguments[], JSValueRef* exception) -> JSValueRef {
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

        ConsoleModule::warn(message);

        return JSValueMakeUndefined(context);
    });
    JSObjectSetProperty(context, consoleObject, warnFunctionName, warnFunction, kJSPropertyAttributeNone, nullptr);
    JSStringRelease(warnFunctionName);
    
    // Attach console object to the global object
    JSObjectSetProperty(context, JSContextGetGlobalObject(context), JSStringCreateWithUTF8CString("console"), consoleObject, kJSPropertyAttributeNone, nullptr);
}