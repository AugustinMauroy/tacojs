#pragma once
#include <JavaScriptCore/JavaScript.h>

void greet(JSContextRef context, JSObjectRef thisObject, size_t argumentCount, const JSValueRef arguments[]);

void attachToContext(JSContextRef context);
