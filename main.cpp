#include <iostream>
#include <fstream>
#include <string>
#include <JavaScriptCore/JavaScript.h>
#include "ConsoleModule.h"

int main(int argc, const char* argv[]) {
    if (argc != 2) {
        std::cerr << "Usage: " << argv[0] << " <javascript_file.js>" << std::endl;
        return 1;
    }

    // Initialize JavaScriptCore
    JSGlobalContextRef context = JSGlobalContextCreate(nullptr);

    // Create a Logger instance and attach it to the JavaScript context
    ConsoleModule logger;
    logger.attachToContext(context);
    ConsoleModule::attachToContext(context);
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
    JSValueRef result = JSEvaluateScript(context, script, nullptr, nullptr, 0, &exception);

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
