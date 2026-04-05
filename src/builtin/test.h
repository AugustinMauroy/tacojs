#ifndef BUILTIN_TEST_H
#define BUILTIN_TEST_H

#include <JavaScriptCore/JavaScriptCore.h>

namespace BuiltinTest {
    JSObjectRef createModuleExports(JSContextRef ctx);
}

#endif // BUILTIN_TEST_H
