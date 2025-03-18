#include "ModuleLoader.h"
#include <iostream>
#include <fstream>
#include <string>

std::unordered_map<std::string, JSObjectRef> ModuleLoader::internalModules;

bool ModuleLoader::isTacoProtocol(const std::string& specifier) {
    return specifier.substr(0, 5) == "taco:";
}

std::string ModuleLoader::extractModuleName(const std::string& specifier) {
    if (isTacoProtocol(specifier)) {
        return specifier.substr(5);
    }
    return specifier;
}

void ModuleLoader::registerInternalModule(const std::string& name, JSObjectRef moduleExports) {
    internalModules[name] = moduleExports;
}

JSValueRef ModuleLoader::loadModule(JSContextRef context, const char* specifierCStr, JSValueRef* exception) {
    std::string specifier(specifierCStr);
    
    if (isTacoProtocol(specifier)) {
        std::string moduleName = extractModuleName(specifier);
        auto it = internalModules.find(moduleName);
        if (it != internalModules.end()) {
            return it->second;
        } else {
            std::string error = "Cannot find module '" + specifier + "'";
            JSStringRef errorMsg = JSStringCreateWithUTF8CString(error.c_str());
            *exception = JSValueMakeString(context, errorMsg);
            JSStringRelease(errorMsg);
            return JSValueMakeUndefined(context);
        }
    } else {
        // Handle external file modules (not implemented in this example)
        std::string error = "External module loading not implemented: " + specifier;
        JSStringRef errorMsg = JSStringCreateWithUTF8CString(error.c_str());
        *exception = JSValueMakeString(context, errorMsg);
        JSStringRelease(errorMsg);
        return JSValueMakeUndefined(context);
    }
}
