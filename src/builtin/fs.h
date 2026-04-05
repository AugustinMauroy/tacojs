#ifndef BUILTIN_FS_H
#define BUILTIN_FS_H

#include <JavaScriptCore/JavaScriptCore.h>

namespace BuiltinFs {
    JSObjectRef createModuleExports(JSContextRef ctx);
}

#endif // BUILTIN_FS_H
