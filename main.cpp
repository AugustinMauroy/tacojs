#include <iostream>
#include <fstream>
#include <string>
#include <JavaScriptCore/JavaScript.h>

JSValueRef JSLogCallback(JSContextRef context, JSObjectRef function, JSObjectRef thisObject, size_t argumentCount, const JSValueRef arguments[], JSValueRef* exception) {
    if (argumentCount > 0) {
        JSStringRef message = JSValueToStringCopy(context, arguments[0], nullptr);
        size_t bufferSize = JSStringGetMaximumUTF8CStringSize(message);
        char* buffer = new char[bufferSize];
        JSStringGetUTF8CString(message, buffer, bufferSize);
        std::cout << buffer << std::endl;
        delete[] buffer;
        JSStringRelease(message);
    }

    return JSValueMakeUndefined(context);
}

int main(int argc, const char * argv[]) {
    if (argc != 2) {
        std::cerr << "Usage: " << argv[0] << " <javascript_file.js>" << std::endl;
        return 1;
    }

    // Initialize JavaScriptCore
    JSGlobalContextRef context = JSGlobalContextCreate(nullptr);
    JSObjectRef globalObject = JSContextGetGlobalObject(context);

    // Add a console.log() function to the JavaScript context
    JSStringRef functionName = JSStringCreateWithUTF8CString("console");
    JSObjectRef consoleObject = JSObjectMake(context, nullptr, nullptr);
    JSObjectSetProperty(context, globalObject, functionName, consoleObject, kJSPropertyAttributeNone, nullptr);

    JSStringRef logFunctionName = JSStringCreateWithUTF8CString("log");
    JSObjectRef logFunction = JSObjectMakeFunctionWithCallback(context, logFunctionName, &JSLogCallback);
    JSObjectSetProperty(context, consoleObject, logFunctionName, logFunction, kJSPropertyAttributeNone, nullptr);

    // Load and execute the JavaScript file
    const char* filename = argv[1];
    std::ifstream file(filename);
    if (!file) {
        std::cerr << "Failed to open the JavaScript file: " << filename << std::endl;
        return 1;
    }

    std::string scriptContent((std::istreambuf_iterator<char>(file)), (std::istreambuf_iterator<char>()));
    JSStringRef script = JSStringCreateWithUTF8CString(scriptContent.c_str());
    JSValueRef exception = nullptr;
    JSEvaluateScript(context, script, nullptr, nullptr, 0, &exception);

    if (exception) {
        JSStringRef exceptionStr = JSValueToStringCopy(context, exception, nullptr);
        size_t bufferSize = JSStringGetMaximumUTF8CStringSize(exceptionStr);
        char* buffer = new char[bufferSize];
        JSStringGetUTF8CString(exceptionStr, buffer, bufferSize);
        std::cerr << "JavaScript exception: " << buffer << std::endl;
        delete[] buffer;
        JSStringRelease(exceptionStr);
    }

    // Cleanup
    JSStringRelease(script);
    JSGlobalContextRelease(context);

    return 0;
}
