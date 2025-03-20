#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <filesystem>
#include <JavaScriptCore/JavaScriptCore.h>
#include "ConsoleManager.h"
#include "ModuleManager.h"
#include "WebAssemblyManager.h"
#include "AsyncManager.h"
#include "TimerManager.h"

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
    AsyncManager::setupAsyncSupport(context, globalObj);
    TimerManager::setupTimers(context, globalObj);

    try {
        std::string jsCode = readFile(filePath);
        JSStringRef scriptJS = JSStringCreateWithUTF8CString(jsCode.c_str());
        JSStringRef sourceURL = JSStringCreateWithUTF8CString(filePath.c_str());
        JSValueRef exception = nullptr;
        JSCheckScriptSyntax(context, scriptJS, sourceURL, 0, &exception);

        if (exception) {
            // Error handling
            JSStringRelease(scriptJS);
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
