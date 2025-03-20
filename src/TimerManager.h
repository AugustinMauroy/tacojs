#ifndef TIMERMANAGER_H
#define TIMERMANAGER_H

#include <JavaScriptCore/JavaScriptCore.h>
#include <map>
#include <chrono>
#include <functional>

namespace TimerManager {
    void setupTimers(JSContextRef ctx, JSObjectRef globalObj);
    void processTimers(JSContextRef ctx);
    bool hasActiveTimers();
}

#endif // TIMERMANAGER_H