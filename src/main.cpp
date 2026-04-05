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
#include "ModuleManager.h"

namespace fs = std::filesystem;

namespace {
    std::string jsValueToStdString(JSContextRef context, JSValueRef value) {
        JSValueRef conversionException = nullptr;
        JSStringRef strRef = JSValueToStringCopy(context, value, &conversionException);
        if (!strRef) {
            return "[unprintable exception]";
        }

        size_t bufferSize = JSStringGetMaximumUTF8CStringSize(strRef);
        std::string result(bufferSize, '\0');
        size_t used = JSStringGetUTF8CString(strRef, &result[0], bufferSize);
        JSStringRelease(strRef);

        if (used == 0) {
            return "";
        }

        result.resize(used - 1);
        return result;
    }
}

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

    int exitCode = EXIT_SUCCESS;

    try {
        std::string jsCode = readFile(filePath);
        bool isModuleCandidate = (fs::path(filePath).extension() == ".js") && ModuleManager::looksLikeModule(jsCode);

        if (isModuleCandidate) {
            std::string esmError;
            if (!ModuleManager::evaluateEntryModule(context, filePath, esmError)) {
                std::cerr << "JavaScript Runtime Error: " << esmError << std::endl;
                exitCode = EXIT_FAILURE;
            } else {
                AsyncManager::runEventLoop(context);
            }
        } else {
            JSStringRef scriptJS = JSStringCreateWithUTF8CString(jsCode.c_str());
            JSStringRef sourceURL = JSStringCreateWithUTF8CString(filePath.c_str());
            JSValueRef exception = nullptr;
            JSCheckScriptSyntax(context, scriptJS, sourceURL, 0, &exception);

            if (exception) {
                std::cerr << "JavaScript Syntax Error: " << jsValueToStdString(context, exception) << std::endl;
                exitCode = EXIT_FAILURE;
            } else {
                JSEvaluateScript(context, scriptJS, nullptr, sourceURL, 0, &exception);
                if (exception) {
                    std::cerr << "JavaScript Runtime Error: " << jsValueToStdString(context, exception) << std::endl;
                    exitCode = EXIT_FAILURE;
                } else {
                    // Run the event loop to process async operations
                    AsyncManager::runEventLoop(context);
                }
            }

            JSStringRelease(scriptJS);
            JSStringRelease(sourceURL);
        }
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        exitCode = EXIT_FAILURE;
    }

    JSGlobalContextRelease(context);

    return exitCode;
}
