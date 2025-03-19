#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <filesystem>
#include <JavaScriptCore/JavaScriptCore.h>
#include "ConsoleManager.h"
#include "ModuleManager.h"
#include "WebAssemblyManager.h"

namespace fs = std::filesystem;

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <path-to-js-file>" << std::endl;
        return EXIT_FAILURE;
    }

    std::string filePath = argv[1];
    if (!fs::exists(filePath)) {
        std::cerr << "File not found: " << filePath << std::endl;
        return EXIT_FAILURE;
    }

    JSGlobalContextRef context = JSGlobalContextCreate(nullptr);
    JSObjectRef globalObj = JSContextGetGlobalObject(context);

    ConsoleManager::setupConsole(context, globalObj);
    ModuleManager::setupModuleLoader(context, globalObj);
    WebAssemblyManager::setupWebAssembly(context, globalObj);

    try {
        std::string jsCode = readFile(filePath);
        JSStringRef scriptJS = JSStringCreateWithUTF8CString(jsCode.c_str());
        JSValueRef exception = nullptr;
        JSValueRef result = JSEvaluateScript(context, scriptJS, nullptr, nullptr, 0, &exception);
        JSStringRelease(scriptJS);

        if (exception) {
            JSStringRef exceptionStr = JSValueToStringCopy(context, exception, nullptr);
            size_t bufferSize = JSStringGetMaximumUTF8CStringSize(exceptionStr);
            char* buffer = new char[bufferSize];
            JSStringGetUTF8CString(exceptionStr, buffer, bufferSize);
            std::cerr << "Exception: " << buffer << std::endl;
            delete[] buffer;
            JSStringRelease(exceptionStr);
        }
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
    }

    JSGlobalContextRelease(context);

    return EXIT_SUCCESS;
}
