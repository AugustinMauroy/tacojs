#include "ConsoleManager.h"
#include <iostream>

namespace ConsoleManager {
    static std::string valueToString(JSContextRef ctx, JSValueRef value, JSValueRef* exception) {
        if (JSValueIsObject(ctx, value)) {
            JSStringRef jsonName = JSStringCreateWithUTF8CString("JSON");
            JSValueRef jsonValue = JSObjectGetProperty(ctx, JSContextGetGlobalObject(ctx), jsonName, exception);
            JSStringRelease(jsonName);

            if (exception && *exception) return "[error]";

            JSObjectRef jsonObj = JSValueToObject(ctx, jsonValue, exception);
            if (exception && *exception) return "[error]";

            JSStringRef stringifyName = JSStringCreateWithUTF8CString("stringify");
            JSValueRef stringifyValue = JSObjectGetProperty(ctx, jsonObj, stringifyName, exception);
            JSStringRelease(stringifyName);

            if (exception && *exception) return "[error]";

            JSObjectRef stringifyFunc = JSValueToObject(ctx, stringifyValue, exception);
            if (exception && *exception) return "[error]";

            JSValueRef args[] = { value };
            JSValueRef jsonString = JSObjectCallAsFunction(ctx, stringifyFunc, nullptr, 1, args, exception);

            if (exception && *exception) return "[error]";

            JSStringRef resultStr = JSValueToStringCopy(ctx, jsonString, exception);
            size_t bufferSize = JSStringGetMaximumUTF8CStringSize(resultStr);
            char* buffer = new char[bufferSize];
            JSStringGetUTF8CString(resultStr, buffer, bufferSize);
            std::string result(buffer);
            delete[] buffer;
            JSStringRelease(resultStr);

            return result;
        } else {
            JSStringRef string = JSValueToStringCopy(ctx, value, exception);
            size_t bufferSize = JSStringGetMaximumUTF8CStringSize(string);
            char* buffer = new char[bufferSize];
            JSStringGetUTF8CString(string, buffer, bufferSize);
            std::string result(buffer);
            delete[] buffer;
            JSStringRelease(string);

            return result;
        }
    }

    static JSValueRef consoleLog(JSContextRef ctx, JSObjectRef function, JSObjectRef thisObject,
                                size_t argumentCount, const JSValueRef arguments[], JSValueRef* exception) {
        for (size_t i = 0; i < argumentCount; i++) {
            std::string str = valueToString(ctx, arguments[i], exception);
            std::cout << str;
            if (i < argumentCount - 1) std::cout << " ";
        }
        std::cout << std::endl;
        return JSValueMakeUndefined(ctx);
    }

    static JSValueRef consoleError(JSContextRef ctx, JSObjectRef function, JSObjectRef thisObject,
                                  size_t argumentCount, const JSValueRef arguments[], JSValueRef* exception) {
        std::cerr << "\033[31m"; // Red color
        for (size_t i = 0; i < argumentCount; i++) {
            std::string str = valueToString(ctx, arguments[i], exception);
            std::cerr << str;
            if (i < argumentCount - 1) std::cerr << " ";
        }
        std::cerr << "\033[0m" << std::endl; // Reset color
        return JSValueMakeUndefined(ctx);
    }

    static JSValueRef consoleWarn(JSContextRef ctx, JSObjectRef function, JSObjectRef thisObject,
                                 size_t argumentCount, const JSValueRef arguments[], JSValueRef* exception) {
        std::cerr << "\033[33m"; // Yellow color
        for (size_t i = 0; i < argumentCount; i++) {
            std::string str = valueToString(ctx, arguments[i], exception);
            std::cerr << str;
            if (i < argumentCount - 1) std::cerr << " ";
        }
        std::cerr << "\033[0m" << std::endl; // Reset color
        return JSValueMakeUndefined(ctx);
    }

    static JSValueRef consoleInfo(JSContextRef ctx, JSObjectRef function, JSObjectRef thisObject,
                                 size_t argumentCount, const JSValueRef arguments[], JSValueRef* exception) {
        std::cerr << "\033[36m"; // Cyan color
        for (size_t i = 0; i < argumentCount; i++) {
            std::string str = valueToString(ctx, arguments[i], exception);
            std::cerr << str;
            if (i < argumentCount - 1) std::cerr << " ";
        }
        std::cerr << "\033[0m" << std::endl; // Reset color
        return JSValueMakeUndefined(ctx);
    }

    void setupConsole(JSContextRef ctx, JSObjectRef globalObj) {
        JSObjectRef consoleObj = JSObjectMake(ctx, nullptr, nullptr);

        JSStringRef logStr = JSStringCreateWithUTF8CString("log");
        JSObjectSetProperty(ctx, consoleObj, logStr,
            JSObjectMakeFunctionWithCallback(ctx, logStr, consoleLog),
            kJSPropertyAttributeNone, nullptr);
        JSStringRelease(logStr);

        JSStringRef errorStr = JSStringCreateWithUTF8CString("error");
        JSObjectSetProperty(ctx, consoleObj, errorStr,
            JSObjectMakeFunctionWithCallback(ctx, errorStr, consoleError),
            kJSPropertyAttributeNone, nullptr);
        JSStringRelease(errorStr);

        JSStringRef warnStr = JSStringCreateWithUTF8CString("warn");
        JSObjectSetProperty(ctx, consoleObj, warnStr,
            JSObjectMakeFunctionWithCallback(ctx, warnStr, consoleWarn),
            kJSPropertyAttributeNone, nullptr);
        JSStringRelease(warnStr);

        JSStringRef infoStr = JSStringCreateWithUTF8CString("info");
        JSObjectSetProperty(ctx, consoleObj, infoStr,
            JSObjectMakeFunctionWithCallback(ctx, infoStr, consoleInfo),
            kJSPropertyAttributeNone, nullptr);
        JSStringRelease(infoStr);

        JSStringRef consoleStr = JSStringCreateWithUTF8CString("console");
        JSObjectSetProperty(ctx, globalObj, consoleStr, consoleObj,
            kJSPropertyAttributeNone, nullptr);
        JSStringRelease(consoleStr);
    }
}
