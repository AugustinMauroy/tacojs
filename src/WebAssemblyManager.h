#ifndef WEBASSEMBLYMANAGER_H
#define WEBASSEMBLYMANAGER_H

#include <JavaScriptCore/JavaScriptCore.h>

namespace WebAssemblyManager {
    void setupWebAssembly(JSContextRef ctx, JSObjectRef globalObj);
}

#endif // WEBASSEMBLYMANAGER_H
