#pragma once
#include <JavaScriptCore/JavaScript.h>
#include <string>

class ConsoleModule {
public:
    static void log(const std::string& message);
    static void info(const std::string& message);
    static void warn(const std::string& message);
    static void attachToContext(JSContextRef context);
};
