#ifndef ASYNCMANAGER_H
#define ASYNCMANAGER_H

#include <JavaScriptCore/JavaScriptCore.h>

namespace AsyncManager {
    void setupAsyncSupport(JSContextRef ctx, JSObjectRef globalObj);
    void runEventLoop(JSContextRef ctx, int timeoutMs = 5000);
    bool hasPendingPromises(JSContextRef ctx);
}

#endif // ASYNCMANAGER_H
