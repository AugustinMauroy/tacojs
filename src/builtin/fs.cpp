#include "fs.h"
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>

#if defined(__APPLE__)
namespace fs = std::__fs::filesystem;
#else
namespace fs = std::filesystem;
#endif

namespace {
    std::string jsValueToStdString(JSContextRef ctx, JSValueRef value) {
        JSValueRef conversionException = nullptr;
        JSStringRef strRef = JSValueToStringCopy(ctx, value, &conversionException);
        if (!strRef) {
            return "[unprintable value]";
        }

        size_t maxSize = JSStringGetMaximumUTF8CStringSize(strRef);
        std::string result(maxSize, '\0');
        size_t used = JSStringGetUTF8CString(strRef, &result[0], maxSize);
        JSStringRelease(strRef);

        if (used == 0) {
            return "";
        }

        result.resize(used - 1);
        return result;
    }

    JSValueRef makeJsStringValue(JSContextRef ctx, const std::string& text) {
        JSStringRef str = JSStringCreateWithUTF8CString(text.c_str());
        JSValueRef value = JSValueMakeString(ctx, str);
        JSStringRelease(str);
        return value;
    }

    void setErrorException(JSContextRef ctx, JSValueRef* exception, const std::string& message) {
        JSValueRef msg = makeJsStringValue(ctx, message);
        JSValueRef args[] = { msg };
        *exception = JSObjectMakeError(ctx, 1, args, nullptr);
    }

    bool getStringArg(JSContextRef ctx, size_t argumentCount, const JSValueRef arguments[],
                      size_t index, const std::string& funcName, std::string& out,
                      JSValueRef* exception) {
        if (argumentCount <= index || !JSValueIsString(ctx, arguments[index])) {
            setErrorException(ctx, exception, funcName + " requires a string argument");
            return false;
        }

        out = jsValueToStdString(ctx, arguments[index]);
        return true;
    }

    static JSValueRef fsReadFileCallback(JSContextRef ctx, JSObjectRef function, JSObjectRef thisObject,
                                         size_t argumentCount, const JSValueRef arguments[], JSValueRef* exception) {
        std::string path;
        if (!getStringArg(ctx, argumentCount, arguments, 0, "fs.readFile", path, exception)) {
            return JSValueMakeUndefined(ctx);
        }

        std::ifstream file(path);
        if (!file.is_open()) {
            setErrorException(ctx, exception, "Failed to read file: " + path);
            return JSValueMakeUndefined(ctx);
        }

        std::stringstream buffer;
        buffer << file.rdbuf();
        return makeJsStringValue(ctx, buffer.str());
    }

    static JSValueRef fsWriteFileCallback(JSContextRef ctx, JSObjectRef function, JSObjectRef thisObject,
                                          size_t argumentCount, const JSValueRef arguments[], JSValueRef* exception) {
        std::string path;
        std::string content;

        if (!getStringArg(ctx, argumentCount, arguments, 0, "fs.writeFile", path, exception)) {
            return JSValueMakeUndefined(ctx);
        }
        if (!getStringArg(ctx, argumentCount, arguments, 1, "fs.writeFile", content, exception)) {
            return JSValueMakeUndefined(ctx);
        }

        std::ofstream file(path);
        if (!file.is_open()) {
            setErrorException(ctx, exception, "Failed to write file: " + path);
            return JSValueMakeUndefined(ctx);
        }

        file << content;
        if (!file.good()) {
            setErrorException(ctx, exception, "Failed to write file: " + path);
            return JSValueMakeUndefined(ctx);
        }

        return JSValueMakeBoolean(ctx, true);
    }

    static JSValueRef fsExistsCallback(JSContextRef ctx, JSObjectRef function, JSObjectRef thisObject,
                                       size_t argumentCount, const JSValueRef arguments[], JSValueRef* exception) {
        std::string path;
        if (!getStringArg(ctx, argumentCount, arguments, 0, "fs.exists", path, exception)) {
            return JSValueMakeUndefined(ctx);
        }

        return JSValueMakeBoolean(ctx, fs::exists(fs::path(path)));
    }

    static JSValueRef fsUnlinkCallback(JSContextRef ctx, JSObjectRef function, JSObjectRef thisObject,
                                       size_t argumentCount, const JSValueRef arguments[], JSValueRef* exception) {
        std::string path;
        if (!getStringArg(ctx, argumentCount, arguments, 0, "fs.unlink", path, exception)) {
            return JSValueMakeUndefined(ctx);
        }

        std::error_code ec;
        bool removed = fs::remove(fs::path(path), ec);
        if (ec) {
            setErrorException(ctx, exception, "Failed to remove file: " + path);
            return JSValueMakeUndefined(ctx);
        }

        return JSValueMakeBoolean(ctx, removed);
    }

    void setFunctionProperty(JSContextRef ctx, JSObjectRef target, const char* name,
                             JSObjectCallAsFunctionCallback callback) {
        JSStringRef propStr = JSStringCreateWithUTF8CString(name);
        JSObjectRef fn = JSObjectMakeFunctionWithCallback(ctx, propStr, callback);
        JSObjectSetProperty(ctx, target, propStr, fn, kJSPropertyAttributeNone, nullptr);
        JSStringRelease(propStr);
    }
}

namespace BuiltinFs {
    JSObjectRef createModuleExports(JSContextRef ctx) {
        JSObjectRef exportsObj = JSObjectMake(ctx, nullptr, nullptr);

        setFunctionProperty(ctx, exportsObj, "readFile", fsReadFileCallback);
        setFunctionProperty(ctx, exportsObj, "writeFile", fsWriteFileCallback);
        setFunctionProperty(ctx, exportsObj, "exists", fsExistsCallback);
        setFunctionProperty(ctx, exportsObj, "unlink", fsUnlinkCallback);

        JSStringRef defaultStr = JSStringCreateWithUTF8CString("default");
        JSObjectSetProperty(ctx, exportsObj, defaultStr, exportsObj, kJSPropertyAttributeNone, nullptr);
        JSStringRelease(defaultStr);

        return exportsObj;
    }
}
