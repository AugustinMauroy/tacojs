#ifndef MODULEMANAGER_H
#define MODULEMANAGER_H

#include <JavaScriptCore/JavaScriptCore.h>
#include <string>


std::string readFile(const std::string& filePath);

namespace ModuleManager {
    void setupModuleLoader(JSContextRef ctx, JSObjectRef globalObj);
}

#endif // MODULEMANAGER_H
