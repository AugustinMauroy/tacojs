// ConsoleModule.h
#pragma once
#include <JavaScriptCore/JavaScript.h>
#include <string>

class ConsoleModule {
public:
    static void log(const std::string& message);
    static void attachToContext(JSContextRef context);
};
