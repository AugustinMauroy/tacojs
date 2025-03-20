#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <filesystem>
#include <JavaScriptCore/JavaScriptCore.h>
#include "ConsoleManager.h"
#include "WebAssemblyManager.h"
#include "AsyncManager.h"
#include "TimerManager.h"

namespace fs = std::filesystem;

std::string readFile(const std::string& path) {
    std::ifstream file(path);
    if (!file.is_open()) {
        throw std::runtime_error("Could not open file: " + path);
    }

    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

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

    AsyncManager::setupAsyncSupport(context, globalObj);
    ConsoleManager::setupConsole(context, globalObj);
    TimerManager::setupTimers(context, globalObj);
    WebAssemblyManager::setupWebAssembly(context, globalObj);

    try {
        std::string jsCode = readFile(filePath);
        JSStringRef scriptJS = JSStringCreateWithUTF8CString(jsCode.c_str());
        JSStringRef sourceURL = JSStringCreateWithUTF8CString(filePath.c_str());
        JSValueRef exception = nullptr;
        JSCheckScriptSyntax(context, scriptJS, sourceURL, 0, &exception);

        if (exception) {
            JSStringRef exString = JSValueToStringCopy(context, exception, nullptr);
            size_t bufferSize = JSStringGetMaximumUTF8CStringSize(exString);
            char* buffer = new char[bufferSize];
            JSStringGetUTF8CString(exString, buffer, bufferSize);
            std::cerr << "JavaScript Error: " << buffer << std::endl;
            delete[] buffer;
            JSStringRelease(exString);
            return EXIT_FAILURE;
        } else {
            JSEvaluateScript(context, scriptJS, nullptr, sourceURL, 0, &exception);
            JSStringRelease(scriptJS);
            // Run the event loop to process async operations
            AsyncManager::runEventLoop(context);
        }
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
    }

    JSGlobalContextRelease(context);

    return EXIT_SUCCESS;
}
