#include <iostream>
#include <fstream>
#include <string>
#include <JavaScriptCore/JavaScript.h>

class Logger {
public:
    Logger() : context(nullptr), consoleObject(nullptr) {}

    void log(const std::string& message) {
        std::cout << message << std::endl;
    }

    static JSValueRef logCallback(JSContextRef context, JSObjectRef function, JSObjectRef thisObject, size_t argumentCount, const JSValueRef arguments[], JSValueRef* exception) {
        if (argumentCount == 0) {
            return JSValueMakeUndefined(context);
        }

        JSValueRef firstArgument = arguments[0];
        JSStringRef firstArgumentString = JSValueToStringCopy(context, firstArgument, nullptr);
        size_t bufferSize = JSStringGetMaximumUTF8CStringSize(firstArgumentString);
        char* buffer = new char[bufferSize];
        JSStringGetUTF8CString(firstArgumentString, buffer, bufferSize);
        std::string message(buffer);
        delete[] buffer;
        JSStringRelease(firstArgumentString);

        Logger* logger = static_cast<Logger*>(JSObjectGetPrivate(function));
        logger->log(message);

        return JSValueMakeUndefined(context); 
    }

    void attachToContext(JSContextRef context) {
        this->context = context;
        consoleObject = JSObjectMake(context, nullptr, nullptr);
        JSObjectSetPrivate(consoleObject, this);

        JSStringRef logFunctionName = JSStringCreateWithUTF8CString("log");
        JSObjectRef logFunction = JSObjectMakeFunctionWithCallback(context, logFunctionName, &Logger::logCallback);
        JSObjectSetProperty(context, consoleObject, logFunctionName, logFunction, kJSPropertyAttributeNone, nullptr);
        JSStringRelease(logFunctionName);

        JSObjectSetProperty(context, JSContextGetGlobalObject(context), JSStringCreateWithUTF8CString("console"), consoleObject, kJSPropertyAttributeNone, nullptr);
    }

private:
    JSContextRef context;
    JSObjectRef consoleObject;
};

int main(int argc, const char* argv[]) {
    if (argc != 2) {
        std::cerr << "Usage: " << argv[0] << " <javascript_file.js>" << std::endl;
        return 1;
    }

    // Initialize JavaScriptCore
    JSGlobalContextRef context = JSGlobalContextCreate(nullptr);

    // Create a Logger instance and attach it to the JavaScript context
    Logger logger;
    logger.attachToContext(context);

    // Load and execute the JavaScript file
    const char* filename = argv[1];
    std::ifstream file(filename);
    if (!file) {
        std::cerr << "Failed to open the JavaScript file: " << filename << std::endl;
        return 1;
    }

    std::string scriptContent((std::istreambuf_iterator<char>(file)), (std::istreambuf_iterator<char>()));
    JSStringRef script = JSStringCreateWithUTF8CString(scriptContent.c_str());
    JSValueRef exception = nullptr;
    JSValueRef result = JSEvaluateScript(context, script, nullptr, nullptr, 0, &exception);

    if (exception) {
        JSStringRef exceptionStr = JSValueToStringCopy(context, exception, nullptr);
        size_t bufferSize = JSStringGetMaximumUTF8CStringSize(exceptionStr);
        char* buffer = new char[bufferSize];
        JSStringGetUTF8CString(exceptionStr, buffer, bufferSize);
        std::cerr << "JavaScript exception: " << buffer << std::endl;
        delete[] buffer;
        JSStringRelease(exceptionStr);
    }

    // Cleanup
    JSStringRelease(script);
    JSGlobalContextRelease(context);

    return 0;
}
