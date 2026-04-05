#ifndef MODULEMANAGER_H
#define MODULEMANAGER_H

#include <JavaScriptCore/JavaScriptCore.h>
#include <string>

namespace ModuleManager {
    bool looksLikeModule(const std::string& source);
    bool evaluateEntryModule(JSContextRef ctx, const std::string& entryPath, std::string& errorOut);
}

#endif // MODULEMANAGER_H
