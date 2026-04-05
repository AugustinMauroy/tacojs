#include "path.h"
#include <filesystem>
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

    static JSValueRef pathJoinCallback(JSContextRef ctx, JSObjectRef function, JSObjectRef thisObject,
                                       size_t argumentCount, const JSValueRef arguments[], JSValueRef* exception) {
        if (argumentCount == 0) {
            return makeJsStringValue(ctx, "");
        }

        fs::path joined;
        for (size_t i = 0; i < argumentCount; i++) {
            std::string part;
            if (!getStringArg(ctx, argumentCount, arguments, i, "path.join", part, exception)) {
                return JSValueMakeUndefined(ctx);
            }
            joined /= fs::path(part);
        }

        return makeJsStringValue(ctx, joined.lexically_normal().generic_string());
    }

    static JSValueRef pathDirnameCallback(JSContextRef ctx, JSObjectRef function, JSObjectRef thisObject,
                                          size_t argumentCount, const JSValueRef arguments[], JSValueRef* exception) {
        std::string input;
        if (!getStringArg(ctx, argumentCount, arguments, 0, "path.dirname", input, exception)) {
            return JSValueMakeUndefined(ctx);
        }

        fs::path p(input);
        std::string out = p.parent_path().generic_string();
        if (out.empty()) {
            out = ".";
        }
        return makeJsStringValue(ctx, out);
    }

    static JSValueRef pathBasenameCallback(JSContextRef ctx, JSObjectRef function, JSObjectRef thisObject,
                                           size_t argumentCount, const JSValueRef arguments[], JSValueRef* exception) {
        std::string input;
        if (!getStringArg(ctx, argumentCount, arguments, 0, "path.basename", input, exception)) {
            return JSValueMakeUndefined(ctx);
        }

        fs::path p(input);
        return makeJsStringValue(ctx, p.filename().generic_string());
    }

    static JSValueRef pathExtnameCallback(JSContextRef ctx, JSObjectRef function, JSObjectRef thisObject,
                                          size_t argumentCount, const JSValueRef arguments[], JSValueRef* exception) {
        std::string input;
        if (!getStringArg(ctx, argumentCount, arguments, 0, "path.extname", input, exception)) {
            return JSValueMakeUndefined(ctx);
        }

        fs::path p(input);
        return makeJsStringValue(ctx, p.extension().generic_string());
    }

    static JSValueRef pathNormalizeCallback(JSContextRef ctx, JSObjectRef function, JSObjectRef thisObject,
                                            size_t argumentCount, const JSValueRef arguments[], JSValueRef* exception) {
        std::string input;
        if (!getStringArg(ctx, argumentCount, arguments, 0, "path.normalize", input, exception)) {
            return JSValueMakeUndefined(ctx);
        }

        fs::path p(input);
        return makeJsStringValue(ctx, p.lexically_normal().generic_string());
    }

    void setFunctionProperty(JSContextRef ctx, JSObjectRef target, const char* name,
                             JSObjectCallAsFunctionCallback callback) {
        JSStringRef propStr = JSStringCreateWithUTF8CString(name);
        JSObjectRef fn = JSObjectMakeFunctionWithCallback(ctx, propStr, callback);
        JSObjectSetProperty(ctx, target, propStr, fn, kJSPropertyAttributeNone, nullptr);
        JSStringRelease(propStr);
    }
}

namespace BuiltinPath {
    JSObjectRef createModuleExports(JSContextRef ctx) {
        JSObjectRef exportsObj = JSObjectMake(ctx, nullptr, nullptr);

        setFunctionProperty(ctx, exportsObj, "join", pathJoinCallback);
        setFunctionProperty(ctx, exportsObj, "dirname", pathDirnameCallback);
        setFunctionProperty(ctx, exportsObj, "basename", pathBasenameCallback);
        setFunctionProperty(ctx, exportsObj, "extname", pathExtnameCallback);
        setFunctionProperty(ctx, exportsObj, "normalize", pathNormalizeCallback);

        JSStringRef defaultStr = JSStringCreateWithUTF8CString("default");
        JSObjectSetProperty(ctx, exportsObj, defaultStr, exportsObj, kJSPropertyAttributeNone, nullptr);
        JSStringRelease(defaultStr);

        return exportsObj;
    }
}
