#pragma once
#include <JavaScriptCore/JavaScript.h>
#include <string>
#include <unordered_map>

class ModuleLoader {
public:
    static JSValueRef loadModule(JSContextRef context, const char* specifier, JSValueRef* exception);
    static void registerInternalModule(const std::string& name, JSObjectRef moduleExports);
    
private:
    static std::unordered_map<std::string, JSObjectRef> internalModules;
    static bool isTacoProtocol(const std::string& specifier);
    static std::string extractModuleName(const std::string& specifier);
};
