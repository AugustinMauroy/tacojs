#include "test.h"
#include <iostream>
#include <string>
#include <vector>

namespace {
    static int passedCount = 0;
    static int failedCount = 0;
    static std::vector<std::string> suiteStack;

    std::string currentSuitePrefix() {
        if (suiteStack.empty()) {
            return "";
        }

        std::string prefix;
        for (size_t i = 0; i < suiteStack.size(); i++) {
            if (i > 0) {
                prefix += " > ";
            }
            prefix += suiteStack[i];
        }
        return prefix;
    }

    std::string qualifiedName(const std::string& name) {
        std::string prefix = currentSuitePrefix();
        if (prefix.empty()) {
            return name;
        }
        return prefix + " > " + name;
    }

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

    JSValueRef runTestCase(JSContextRef ctx, size_t argumentCount, const JSValueRef arguments[], JSValueRef* exception,
                           const std::string& apiName) {
        if (argumentCount < 2 || !JSValueIsString(ctx, arguments[0]) || !JSValueIsObject(ctx, arguments[1])) {
            setErrorException(ctx, exception, apiName + "(name, fn) requires a string name and function");
            return JSValueMakeUndefined(ctx);
        }

        JSObjectRef fn = JSValueToObject(ctx, arguments[1], nullptr);
        if (!JSObjectIsFunction(ctx, fn)) {
            setErrorException(ctx, exception, apiName + "(name, fn) requires fn to be a function");
            return JSValueMakeUndefined(ctx);
        }

        std::string name = qualifiedName(jsValueToStdString(ctx, arguments[0]));

        JSValueRef callException = nullptr;
        JSObjectCallAsFunction(ctx, fn, nullptr, 0, nullptr, &callException);

        if (callException) {
            failedCount++;
            std::cerr << "[tacos:test] FAIL " << name << ": " << jsValueToStdString(ctx, callException) << std::endl;
            return JSValueMakeBoolean(ctx, false);
        }

        passedCount++;
        std::cout << "[tacos:test] PASS " << name << std::endl;
        return JSValueMakeBoolean(ctx, true);
    }

    static JSValueRef testCallback(JSContextRef ctx, JSObjectRef function, JSObjectRef thisObject,
                                   size_t argumentCount, const JSValueRef arguments[], JSValueRef* exception) {
        return runTestCase(ctx, argumentCount, arguments, exception, "test");
    }

    static JSValueRef itCallback(JSContextRef ctx, JSObjectRef function, JSObjectRef thisObject,
                                 size_t argumentCount, const JSValueRef arguments[], JSValueRef* exception) {
        return runTestCase(ctx, argumentCount, arguments, exception, "it");
    }

    static JSValueRef describeCallback(JSContextRef ctx, JSObjectRef function, JSObjectRef thisObject,
                                       size_t argumentCount, const JSValueRef arguments[], JSValueRef* exception) {
        if (argumentCount < 2 || !JSValueIsString(ctx, arguments[0]) || !JSValueIsObject(ctx, arguments[1])) {
            setErrorException(ctx, exception, "describe(name, fn) requires a string name and function");
            return JSValueMakeUndefined(ctx);
        }

        JSObjectRef fn = JSValueToObject(ctx, arguments[1], nullptr);
        if (!JSObjectIsFunction(ctx, fn)) {
            setErrorException(ctx, exception, "describe(name, fn) requires fn to be a function");
            return JSValueMakeUndefined(ctx);
        }

        std::string suiteName = jsValueToStdString(ctx, arguments[0]);
        suiteStack.push_back(suiteName);

        JSValueRef callException = nullptr;
        JSObjectCallAsFunction(ctx, fn, nullptr, 0, nullptr, &callException);

        suiteStack.pop_back();

        if (callException) {
            failedCount++;
            std::cerr << "[tacos:test] FAIL describe(" << suiteName << "): "
                      << jsValueToStdString(ctx, callException) << std::endl;
            return JSValueMakeBoolean(ctx, false);
        }

        return JSValueMakeBoolean(ctx, true);
    }

    static JSValueRef summaryCallback(JSContextRef ctx, JSObjectRef function, JSObjectRef thisObject,
                                      size_t argumentCount, const JSValueRef arguments[], JSValueRef* exception) {
        JSObjectRef summary = JSObjectMake(ctx, nullptr, nullptr);

        JSStringRef passedStr = JSStringCreateWithUTF8CString("passed");
        JSObjectSetProperty(ctx, summary, passedStr, JSValueMakeNumber(ctx, passedCount), kJSPropertyAttributeNone, nullptr);
        JSStringRelease(passedStr);

        JSStringRef failedStr = JSStringCreateWithUTF8CString("failed");
        JSObjectSetProperty(ctx, summary, failedStr, JSValueMakeNumber(ctx, failedCount), kJSPropertyAttributeNone, nullptr);
        JSStringRelease(failedStr);

        JSStringRef totalStr = JSStringCreateWithUTF8CString("total");
        JSObjectSetProperty(ctx, summary, totalStr, JSValueMakeNumber(ctx, passedCount + failedCount), kJSPropertyAttributeNone, nullptr);
        JSStringRelease(totalStr);

        return summary;
    }

    static JSValueRef resetCallback(JSContextRef ctx, JSObjectRef function, JSObjectRef thisObject,
                                    size_t argumentCount, const JSValueRef arguments[], JSValueRef* exception) {
        passedCount = 0;
        failedCount = 0;
        suiteStack.clear();
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

namespace BuiltinTest {
    JSObjectRef createModuleExports(JSContextRef ctx) {
        passedCount = 0;
        failedCount = 0;
        suiteStack.clear();

        JSObjectRef exportsObj = JSObjectMake(ctx, nullptr, nullptr);

        setFunctionProperty(ctx, exportsObj, "test", testCallback);
        setFunctionProperty(ctx, exportsObj, "describe", describeCallback);
        setFunctionProperty(ctx, exportsObj, "it", itCallback);
        setFunctionProperty(ctx, exportsObj, "summary", summaryCallback);
        setFunctionProperty(ctx, exportsObj, "reset", resetCallback);

        JSStringRef defaultStr = JSStringCreateWithUTF8CString("default");
        JSObjectSetProperty(ctx, exportsObj, defaultStr, exportsObj, kJSPropertyAttributeNone, nullptr);
        JSStringRelease(defaultStr);

        return exportsObj;
    }
}
