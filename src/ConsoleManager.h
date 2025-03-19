#ifndef CONSOLEMANAGER_H
#define CONSOLEMANAGER_H

#include <JavaScriptCore/JavaScriptCore.h>

namespace ConsoleManager {
    void setupConsole(JSContextRef ctx, JSObjectRef globalObj);
}

#endif // CONSOLEMANAGER_H
