#include "ModuleManager.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>

// Helper function to read file content into string
std::string readFile(const std::string& path) {
    std::ifstream file(path);
    if (!file.is_open()) {
        throw std::runtime_error("Could not open file: " + path);
    }

    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

namespace ModuleManager {
    static JSValueRef importModule(JSContextRef ctx, JSObjectRef function, JSObjectRef thisObject,
                                  size_t argumentCount, const JSValueRef arguments[], JSValueRef* exception) {
        if (argumentCount < 1) {
            return JSValueMakeUndefined(ctx);
        }

        JSStringRef pathStr = JSValueToStringCopy(ctx, arguments[0], exception);
        size_t bufferSize = JSStringGetMaximumUTF8CStringSize(pathStr);
        char* buffer = new char[bufferSize];
        JSStringGetUTF8CString(pathStr, buffer, bufferSize);
        std::string modulePath(buffer);
        delete[] buffer;
        JSStringRelease(pathStr);

        std::string moduleCode;
        if (modulePath.substr(0, 5) == "taco:") {
            std::string internalModule = modulePath.substr(5);
            moduleCode = "export default {}";
        } else if (modulePath.substr(0, 7) == "file://") {
            std::string filePath = modulePath.substr(7);
            try {
                moduleCode = readFile(filePath);
            } catch (const std::exception& e) {
                JSStringRef errorMsg = JSStringCreateWithUTF8CString(e.what());
                *exception = JSValueMakeString(ctx, errorMsg);
                JSStringRelease(errorMsg);
                return JSValueMakeUndefined(ctx);
            }
        } else {
            JSStringRef errorMsg = JSStringCreateWithUTF8CString(
                ("Unsupported module format: " + modulePath).c_str());
            *exception = JSValueMakeString(ctx, errorMsg);
            JSStringRelease(errorMsg);
            return JSValueMakeUndefined(ctx);
        }

        JSStringRef scriptJS = JSStringCreateWithUTF8CString(moduleCode.c_str());
        JSValueRef result = JSEvaluateScript(ctx, scriptJS, nullptr, nullptr, 0, exception);
        JSStringRelease(scriptJS);

        return result;
    }

    void setupModuleLoader(JSContextRef ctx, JSObjectRef globalObj) {
        JSStringRef importStr = JSStringCreateWithUTF8CString("import");
        JSObjectSetProperty(ctx, globalObj, importStr,
            JSObjectMakeFunctionWithCallback(ctx, importStr, importModule),
            kJSPropertyAttributeNone, nullptr);
        JSStringRelease(importStr);
    }
}
