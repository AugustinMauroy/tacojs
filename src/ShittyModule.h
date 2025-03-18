#pragma once
#include <JavaScriptCore/JavaScript.h>

class ShittyModule {
public:
    static void greet(JSContextRef context, JSObjectRef thisObject, size_t argumentCount, const JSValueRef arguments[]);
    static void attachToContext(JSContextRef context);
};
