#include "asserts.h"
#include <string>

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

    static JSValueRef assertOkCallback(JSContextRef ctx, JSObjectRef function, JSObjectRef thisObject,
                                       size_t argumentCount, const JSValueRef arguments[], JSValueRef* exception) {
        bool condition = false;
        if (argumentCount > 0) {
            condition = JSValueToBoolean(ctx, arguments[0]);
        }

        if (!condition) {
            std::string message = "Assertion failed";
            if (argumentCount > 1) {
                message = jsValueToStdString(ctx, arguments[1]);
            }
            setErrorException(ctx, exception, message);
        }

        return JSValueMakeUndefined(ctx);
    }

    static JSValueRef assertEqualCallback(JSContextRef ctx, JSObjectRef function, JSObjectRef thisObject,
                                          size_t argumentCount, const JSValueRef arguments[], JSValueRef* exception) {
        if (argumentCount < 2) {
            setErrorException(ctx, exception, "assert.equal requires 2 arguments");
            return JSValueMakeUndefined(ctx);
        }

        if (!JSValueIsStrictEqual(ctx, arguments[0], arguments[1])) {
            std::string message = "Expected values to be strictly equal";
            if (argumentCount > 2) {
                message = jsValueToStdString(ctx, arguments[2]);
            }
            setErrorException(ctx, exception, message);
        }

        return JSValueMakeUndefined(ctx);
    }

    static JSValueRef assertNotEqualCallback(JSContextRef ctx, JSObjectRef function, JSObjectRef thisObject,
                                             size_t argumentCount, const JSValueRef arguments[], JSValueRef* exception) {
        if (argumentCount < 2) {
            setErrorException(ctx, exception, "assert.notEqual requires 2 arguments");
            return JSValueMakeUndefined(ctx);
        }

        if (JSValueIsStrictEqual(ctx, arguments[0], arguments[1])) {
            std::string message = "Expected values to be different";
            if (argumentCount > 2) {
                message = jsValueToStdString(ctx, arguments[2]);
            }
            setErrorException(ctx, exception, message);
        }

        return JSValueMakeUndefined(ctx);
    }

    static JSValueRef assertThrowsCallback(JSContextRef ctx, JSObjectRef function, JSObjectRef thisObject,
                                           size_t argumentCount, const JSValueRef arguments[], JSValueRef* exception) {
        if (argumentCount < 1 || !JSValueIsObject(ctx, arguments[0])) {
            setErrorException(ctx, exception, "assert.throws requires a function argument");
            return JSValueMakeUndefined(ctx);
        }

        JSObjectRef fn = JSValueToObject(ctx, arguments[0], nullptr);
        if (!JSObjectIsFunction(ctx, fn)) {
            setErrorException(ctx, exception, "assert.throws requires a function argument");
            return JSValueMakeUndefined(ctx);
        }

        JSValueRef callException = nullptr;
        JSObjectCallAsFunction(ctx, fn, nullptr, 0, nullptr, &callException);
        if (callException == nullptr) {
            std::string message = "Expected function to throw";
            if (argumentCount > 1) {
                message = jsValueToStdString(ctx, arguments[1]);
            }
            setErrorException(ctx, exception, message);
        }

        return JSValueMakeUndefined(ctx);
    }

    void setFunctionProperty(JSContextRef ctx, JSObjectRef target, const char* name,
                             JSObjectCallAsFunctionCallback callback) {
        JSStringRef propStr = JSStringCreateWithUTF8CString(name);
        JSObjectRef fn = JSObjectMakeFunctionWithCallback(ctx, propStr, callback);
        JSObjectSetProperty(ctx, target, propStr, fn, kJSPropertyAttributeNone, nullptr);
        JSStringRelease(propStr);
    }
}

namespace BuiltinAsserts {
    JSObjectRef createModuleExports(JSContextRef ctx) {
        JSObjectRef exportsObj = JSObjectMake(ctx, nullptr, nullptr);

        setFunctionProperty(ctx, exportsObj, "ok", assertOkCallback);
        setFunctionProperty(ctx, exportsObj, "equal", assertEqualCallback);
        setFunctionProperty(ctx, exportsObj, "notEqual", assertNotEqualCallback);
        setFunctionProperty(ctx, exportsObj, "throws", assertThrowsCallback);

        JSStringRef defaultStr = JSStringCreateWithUTF8CString("default");
        JSObjectSetProperty(ctx, exportsObj, defaultStr, exportsObj, kJSPropertyAttributeNone, nullptr);
        JSStringRelease(defaultStr);

        return exportsObj;
    }
}
