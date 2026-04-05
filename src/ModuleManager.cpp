#include "ModuleManager.h"
#include "builtin/asserts.h"
#include "builtin/fs.h"
#include "builtin/path.h"
#include "builtin/test.h"
#include <filesystem>
#include <fstream>
#include <sstream>
#include <unordered_map>
#include <regex>
#include <vector>
#include <algorithm>
#include <cctype>
#include <stdexcept>

#if defined(__APPLE__)
namespace fs = std::__fs::filesystem;
#else
namespace fs = std::filesystem;
#endif

namespace {
    struct ModuleRecord {
        JSObjectRef exports;
        bool loading;
        bool loaded;
    };

    static std::unordered_map<std::string, ModuleRecord> moduleCache;

    std::string trim(const std::string& input) {
        size_t start = 0;
        while (start < input.size() && std::isspace(static_cast<unsigned char>(input[start]))) {
            start++;
        }

        size_t end = input.size();
        while (end > start && std::isspace(static_cast<unsigned char>(input[end - 1]))) {
            end--;
        }

        return input.substr(start, end - start);
    }

    bool isValidIdentifier(const std::string& name) {
        static const std::regex identifierRegex(R"(^[A-Za-z_$][A-Za-z0-9_$]*$)");
        return std::regex_match(name, identifierRegex);
    }

    std::vector<std::string> splitLines(const std::string& source) {
        std::vector<std::string> lines;
        std::stringstream ss(source);
        std::string line;
        while (std::getline(ss, line)) {
            lines.push_back(line);
        }
        return lines;
    }

    std::string readFile(const std::string& path) {
        std::ifstream file(path);
        if (!file.is_open()) {
            throw std::runtime_error("Could not open file: " + path);
        }

        std::stringstream buffer;
        buffer << file.rdbuf();
        return buffer.str();
    }

    std::string jsValueToStdString(JSContextRef ctx, JSValueRef value) {
        JSValueRef conversionException = nullptr;
        JSStringRef strRef = JSValueToStringCopy(ctx, value, &conversionException);
        if (!strRef) {
            return "[unprintable value]";
        }

        size_t maxSize = JSStringGetMaximumUTF8CStringSize(strRef);
        std::string result(maxSize, '\0');
        size_t used = JSStringGetUTF8CString(strRef, &result[0], maxSize);
        JSStringRelease(strRef);

        if (used == 0) {
            return "";
        }

        result.resize(used - 1);
        return result;
    }

    JSValueRef makeJsStringValue(JSContextRef ctx, const std::string& text) {
        JSStringRef str = JSStringCreateWithUTF8CString(text.c_str());
        JSValueRef value = JSValueMakeString(ctx, str);
        JSStringRelease(str);
        return value;
    }

    bool isTacoBuiltinSpecifier(const std::string& specifier) {
        return specifier.rfind("taco:", 0) == 0 || specifier.rfind("tacos:", 0) == 0;
    }

    JSObjectRef loadBuiltinModule(JSContextRef ctx, const std::string& specifier, std::string& errorOut) {
        std::string cacheKey = "builtin:" + specifier;
        auto existing = moduleCache.find(cacheKey);
        if (existing != moduleCache.end()) {
            return existing->second.exports;
        }

        JSObjectRef exportsObj = nullptr;
        if (specifier == "taco:assert") {
            exportsObj = BuiltinAsserts::createModuleExports(ctx);
        } else if (specifier == "taco:fs") {
            exportsObj = BuiltinFs::createModuleExports(ctx);
        } else if (specifier == "taco:path") {
            exportsObj = BuiltinPath::createModuleExports(ctx);
        } else if (specifier == "tacos:test") {
            exportsObj = BuiltinTest::createModuleExports(ctx);
        } else {
            errorOut = "Unsupported taco builtin module: " + specifier;
            return nullptr;
        }

        JSValueProtect(ctx, exportsObj);

        moduleCache[cacheKey] = ModuleRecord{exportsObj, false, true};
        return exportsObj;
    }

    JSObjectRef resolveImportedModule(JSContextRef ctx, const std::string& specifier,
                                      const std::string& parentPath, std::string& errorOut);

    JSValueRef makePromiseFromValue(JSContextRef ctx, bool isReject, JSValueRef value, JSValueRef* exception) {
        JSObjectRef globalObj = JSContextGetGlobalObject(ctx);

        JSStringRef promiseStr = JSStringCreateWithUTF8CString("Promise");
        JSValueRef promiseValue = JSObjectGetProperty(ctx, globalObj, promiseStr, exception);
        JSStringRelease(promiseStr);
        if (exception && *exception) {
            return JSValueMakeUndefined(ctx);
        }

        if (!JSValueIsObject(ctx, promiseValue)) {
            *exception = makeJsStringValue(ctx, "Promise global is not available");
            return JSValueMakeUndefined(ctx);
        }

        JSObjectRef promiseObj = JSValueToObject(ctx, promiseValue, exception);
        if (exception && *exception) {
            return JSValueMakeUndefined(ctx);
        }

        JSStringRef methodStr = JSStringCreateWithUTF8CString(isReject ? "reject" : "resolve");
        JSValueRef methodValue = JSObjectGetProperty(ctx, promiseObj, methodStr, exception);
        JSStringRelease(methodStr);
        if (exception && *exception) {
            return JSValueMakeUndefined(ctx);
        }

        if (!JSValueIsObject(ctx, methodValue)) {
            *exception = makeJsStringValue(ctx, "Promise resolver method is not available");
            return JSValueMakeUndefined(ctx);
        }

        JSObjectRef methodFn = JSValueToObject(ctx, methodValue, exception);
        if (exception && *exception) {
            return JSValueMakeUndefined(ctx);
        }

        JSValueRef args[] = { value };
        return JSObjectCallAsFunction(ctx, methodFn, promiseObj, 1, args, exception);
    }

    std::vector<std::string> splitCommaList(const std::string& input) {
        std::vector<std::string> parts;
        std::string current;
        for (char c : input) {
            if (c == ',') {
                std::string piece = trim(current);
                if (!piece.empty()) {
                    parts.push_back(piece);
                }
                current.clear();
            } else {
                current.push_back(c);
            }
        }

        std::string piece = trim(current);
        if (!piece.empty()) {
            parts.push_back(piece);
        }

        return parts;
    }

    std::string parseNamedImports(const std::string& clause, const std::string& moduleVar) {
        std::string content = trim(clause);
        if (content.size() < 2 || content.front() != '{' || content.back() != '}') {
            throw std::runtime_error("Unsupported named import syntax: " + clause);
        }

        content = trim(content.substr(1, content.size() - 2));
        if (content.empty()) {
            return "";
        }

        std::vector<std::string> parts = splitCommaList(content);
        std::stringstream out;

        for (const std::string& partRaw : parts) {
            std::string part = trim(partRaw);
            std::string imported;
            std::string local;

            size_t asPos = part.find(" as ");
            if (asPos == std::string::npos) {
                imported = trim(part);
                local = imported;
            } else {
                imported = trim(part.substr(0, asPos));
                local = trim(part.substr(asPos + 4));
            }

            if (!isValidIdentifier(imported) || !isValidIdentifier(local)) {
                throw std::runtime_error("Invalid import binding: " + part);
            }

            out << "const " << local << " = " << moduleVar << "." << imported << ";\n";
        }

        return out.str();
    }

    std::string transformImportClause(const std::string& clauseRaw, const std::string& moduleVar) {
        std::string clause = trim(clauseRaw);
        std::stringstream out;

        if (clause.empty()) {
            return "";
        }

        if (clause.front() == '{') {
            out << parseNamedImports(clause, moduleVar);
            return out.str();
        }

        if (clause.rfind("* as ", 0) == 0) {
            std::string nsName = trim(clause.substr(5));
            if (!isValidIdentifier(nsName)) {
                throw std::runtime_error("Invalid namespace import identifier: " + nsName);
            }
            out << "const " << nsName << " = " << moduleVar << ";\n";
            return out.str();
        }

        size_t commaPos = clause.find(',');
        if (commaPos != std::string::npos) {
            std::string defaultName = trim(clause.substr(0, commaPos));
            std::string rest = trim(clause.substr(commaPos + 1));
            if (!isValidIdentifier(defaultName)) {
                throw std::runtime_error("Invalid default import identifier: " + defaultName);
            }
            out << "const " << defaultName << " = " << moduleVar << ".default;\n";

            if (rest.rfind("* as ", 0) == 0) {
                std::string nsName = trim(rest.substr(5));
                if (!isValidIdentifier(nsName)) {
                    throw std::runtime_error("Invalid namespace import identifier: " + nsName);
                }
                out << "const " << nsName << " = " << moduleVar << ";\n";
                return out.str();
            }

            out << parseNamedImports(rest, moduleVar);
            return out.str();
        }

        if (!isValidIdentifier(clause)) {
            throw std::runtime_error("Unsupported import clause: " + clause);
        }

        out << "const " << clause << " = " << moduleVar << ".default;\n";
        return out.str();
    }

    std::string transformDynamicImportCalls(const std::string& line) {
        static const std::regex dynamicImportRegex(R"(\bimport\s*\()");
        return std::regex_replace(line, dynamicImportRegex, "__dynamicImportLocal(");
    }

    std::string transformModuleSource(const std::string& source) {
        static const std::regex importFromRegex(R"(^\s*import\s+(.+)\s+from\s+['\"]([^'\"]+)['\"]\s*;?\s*$)");
        static const std::regex importSideEffectRegex(R"(^\s*import\s+['\"]([^'\"]+)['\"]\s*;?\s*$)");
        static const std::regex exportVarRegex(R"(^\s*export\s+(const|let|var)\s+([A-Za-z_$][A-Za-z0-9_$]*)\b(.*)$)");
        static const std::regex exportNamedRegex(R"(^\s*export\s*\{(.+)\}\s*;?\s*$)");
        static const std::regex exportDefaultExprRegex(R"(^\s*export\s+default\s+(.+)\s*;?\s*$)");
        static const std::regex exportFunctionRegex(R"(^\s*export\s+function\s+([A-Za-z_$][A-Za-z0-9_$]*)\s*\()");
        static const std::regex exportClassRegex(R"(^\s*export\s+class\s+([A-Za-z_$][A-Za-z0-9_$]*)\b)");
        static const std::regex exportDefaultNamedFunctionRegex(R"(^\s*export\s+default\s+function\s+([A-Za-z_$][A-Za-z0-9_$]*)\s*\()");
        static const std::regex exportDefaultNamedClassRegex(R"(^\s*export\s+default\s+class\s+([A-Za-z_$][A-Za-z0-9_$]*)\b)");

        std::vector<std::string> lines = splitLines(source);
        std::vector<std::string> exportPostlude;
        int importIndex = 0;

        std::stringstream output;

        for (std::string line : lines) {
            std::smatch match;
            std::string trimmed = trim(line);

            if (trimmed.empty()) {
                output << "\n";
                continue;
            }

            if (std::regex_match(line, match, importFromRegex)) {
                std::string clause = trim(match[1].str());
                std::string specifier = match[2].str();
                std::string moduleVar = "__module" + std::to_string(importIndex++);
                output << "const " << moduleVar << " = __import(\"" << specifier << "\", __filename);\n";
                output << transformImportClause(clause, moduleVar);
                continue;
            }

            if (std::regex_match(line, match, importSideEffectRegex)) {
                std::string specifier = match[1].str();
                output << "__import(\"" << specifier << "\", __filename);\n";
                continue;
            }

            if (std::regex_match(line, match, exportDefaultNamedFunctionRegex)) {
                std::string functionName = match[1].str();
                size_t exportPos = line.find("export default ");
                line.erase(exportPos, std::string("export default ").size());
                output << line << "\n";
                exportPostlude.push_back("__exports.default = " + functionName + ";");
                continue;
            }

            if (std::regex_match(line, match, exportDefaultNamedClassRegex)) {
                std::string className = match[1].str();
                size_t exportPos = line.find("export default ");
                line.erase(exportPos, std::string("export default ").size());
                output << line << "\n";
                exportPostlude.push_back("__exports.default = " + className + ";");
                continue;
            }

            if (std::regex_match(line, match, exportDefaultExprRegex)) {
                std::string expr = trim(match[1].str());
                if (!expr.empty() && expr.back() == ';') {
                    expr.pop_back();
                }
                output << "__exports.default = (" << expr << ");\n";
                continue;
            }

            if (std::regex_match(line, match, exportVarRegex)) {
                std::string kind = match[1].str();
                std::string name = match[2].str();
                std::string rest = match[3].str();
                output << kind << " " << name << rest << "\n";
                output << "__exports." << name << " = " << name << ";\n";
                continue;
            }

            if (std::regex_match(line, match, exportNamedRegex)) {
                std::vector<std::string> pieces = splitCommaList(match[1].str());
                for (const std::string& pieceRaw : pieces) {
                    std::string piece = trim(pieceRaw);
                    size_t asPos = piece.find(" as ");
                    std::string local;
                    std::string exported;
                    if (asPos == std::string::npos) {
                        local = piece;
                        exported = piece;
                    } else {
                        local = trim(piece.substr(0, asPos));
                        exported = trim(piece.substr(asPos + 4));
                    }

                    if (!isValidIdentifier(local) || !isValidIdentifier(exported)) {
                        throw std::runtime_error("Invalid export binding: " + piece);
                    }

                    output << "__exports." << exported << " = " << local << ";\n";
                }
                continue;
            }

            if (std::regex_search(line, match, exportFunctionRegex)) {
                std::string functionName = match[1].str();
                size_t exportPos = line.find("export ");
                line.erase(exportPos, std::string("export ").size());
                output << line << "\n";
                exportPostlude.push_back("__exports." + functionName + " = " + functionName + ";");
                continue;
            }

            if (std::regex_search(line, match, exportClassRegex)) {
                std::string className = match[1].str();
                size_t exportPos = line.find("export ");
                line.erase(exportPos, std::string("export ").size());
                output << line << "\n";
                exportPostlude.push_back("__exports." + className + " = " + className + ";");
                continue;
            }

            line = transformDynamicImportCalls(line);
            output << line << "\n";
        }

        for (const std::string& postlude : exportPostlude) {
            output << postlude << "\n";
        }

        return output.str();
    }

    std::string resolveModulePath(const std::string& specifier, const std::string& parentPath) {
        if (specifier.empty()) {
            throw std::runtime_error("Empty module specifier");
        }

        fs::path resolved;
        if (specifier[0] == '/') {
            resolved = fs::path(specifier);
        } else if (specifier.rfind("./", 0) == 0 || specifier.rfind("../", 0) == 0) {
            fs::path parent(parentPath);
            resolved = parent.parent_path() / specifier;
        } else {
            throw std::runtime_error("Bare specifiers are not supported in phase 1: " + specifier);
        }

        resolved = fs::weakly_canonical(resolved);

        if (resolved.extension() != ".js") {
            throw std::runtime_error("Only .js modules are supported: " + resolved.string());
        }

        if (!fs::exists(resolved)) {
            throw std::runtime_error("Module not found: " + resolved.string());
        }

        return resolved.string();
    }

    JSObjectRef loadModuleByPath(JSContextRef ctx, const std::string& canonicalPath, std::string& errorOut);

    JSObjectRef resolveImportedModule(JSContextRef ctx, const std::string& specifier,
                                      const std::string& parentPath, std::string& errorOut) {
        if (isTacoBuiltinSpecifier(specifier)) {
            return loadBuiltinModule(ctx, specifier, errorOut);
        }

        std::string resolvedPath = resolveModulePath(specifier, parentPath);
        return loadModuleByPath(ctx, resolvedPath, errorOut);
    }

    JSValueRef importCallback(JSContextRef ctx, JSObjectRef function, JSObjectRef thisObject,
                              size_t argumentCount, const JSValueRef arguments[], JSValueRef* exception) {
        if (argumentCount < 2) {
            *exception = makeJsStringValue(ctx, "__import expects (specifier, parentPath)");
            return JSValueMakeUndefined(ctx);
        }

        if (!JSValueIsString(ctx, arguments[0]) || !JSValueIsString(ctx, arguments[1])) {
            *exception = makeJsStringValue(ctx, "__import expects string arguments");
            return JSValueMakeUndefined(ctx);
        }

        std::string specifier = jsValueToStdString(ctx, arguments[0]);
        std::string parentPath = jsValueToStdString(ctx, arguments[1]);

        try {
            std::string error;
            JSObjectRef moduleExports = resolveImportedModule(ctx, specifier, parentPath, error);
            if (!moduleExports) {
                *exception = makeJsStringValue(ctx, error.empty() ? "Failed to import module" : error);
                return JSValueMakeUndefined(ctx);
            }
            return moduleExports;
        } catch (const std::exception& e) {
            *exception = makeJsStringValue(ctx, e.what());
            return JSValueMakeUndefined(ctx);
        }
    }

    JSValueRef dynamicImportCallback(JSContextRef ctx, JSObjectRef function, JSObjectRef thisObject,
                                     size_t argumentCount, const JSValueRef arguments[], JSValueRef* exception) {
        if (argumentCount < 2) {
            JSValueRef err = makeJsStringValue(ctx, "__dynamicImport expects (specifier, parentPath)");
            return makePromiseFromValue(ctx, true, err, exception);
        }

        if (!JSValueIsString(ctx, arguments[0]) || !JSValueIsString(ctx, arguments[1])) {
            JSValueRef err = makeJsStringValue(ctx, "__dynamicImport expects string arguments");
            return makePromiseFromValue(ctx, true, err, exception);
        }

        std::string specifier = jsValueToStdString(ctx, arguments[0]);
        std::string parentPath = jsValueToStdString(ctx, arguments[1]);

        try {
            std::string error;
            JSObjectRef moduleExports = resolveImportedModule(ctx, specifier, parentPath, error);
            if (!moduleExports) {
                JSValueRef err = makeJsStringValue(ctx, error.empty() ? "Failed to import module" : error);
                return makePromiseFromValue(ctx, true, err, exception);
            }

            return makePromiseFromValue(ctx, false, moduleExports, exception);
        } catch (const std::exception& e) {
            JSValueRef err = makeJsStringValue(ctx, e.what());
            return makePromiseFromValue(ctx, true, err, exception);
        }
    }

    JSObjectRef loadModuleByPath(JSContextRef ctx, const std::string& canonicalPath, std::string& errorOut) {
        auto it = moduleCache.find(canonicalPath);
        if (it != moduleCache.end()) {
            return it->second.exports;
        }

        JSObjectRef exportsObj = JSObjectMake(ctx, nullptr, nullptr);
        JSValueProtect(ctx, exportsObj);
        moduleCache[canonicalPath] = ModuleRecord{exportsObj, true, false};

        std::string source;
        try {
            source = readFile(canonicalPath);
        } catch (const std::exception& e) {
            errorOut = e.what();
            JSValueUnprotect(ctx, exportsObj);
            moduleCache.erase(canonicalPath);
            return nullptr;
        }

        std::string transformed;
        try {
            transformed = transformModuleSource(source);
        } catch (const std::exception& e) {
            errorOut = std::string("ESM transform error in ") + canonicalPath + ": " + e.what();
            JSValueUnprotect(ctx, exportsObj);
            moduleCache.erase(canonicalPath);
            return nullptr;
        }

        std::string wrapperSource = "(function(__import, __dynamicImport, __exports, __filename) {\n"
            "const __dynamicImportLocal = (specifier) => __dynamicImport(specifier, __filename);\n"
            + transformed + "\nreturn __exports;\n})";

        JSStringRef wrapperScript = JSStringCreateWithUTF8CString(wrapperSource.c_str());
        JSStringRef sourceUrl = JSStringCreateWithUTF8CString(canonicalPath.c_str());
        JSValueRef evalException = nullptr;
        JSValueRef wrapperValue = JSEvaluateScript(ctx, wrapperScript, nullptr, sourceUrl, 0, &evalException);
        JSStringRelease(wrapperScript);
        JSStringRelease(sourceUrl);

        if (evalException) {
            errorOut = "ESM compile error in " + canonicalPath + ": " + jsValueToStdString(ctx, evalException);
            JSValueUnprotect(ctx, exportsObj);
            moduleCache.erase(canonicalPath);
            return nullptr;
        }

        if (!JSValueIsObject(ctx, wrapperValue)) {
            errorOut = "Internal ESM error: wrapper is not an object for " + canonicalPath;
            JSValueUnprotect(ctx, exportsObj);
            moduleCache.erase(canonicalPath);
            return nullptr;
        }

        JSObjectRef wrapperFn = JSValueToObject(ctx, wrapperValue, nullptr);
        if (!JSObjectIsFunction(ctx, wrapperFn)) {
            errorOut = "Internal ESM error: wrapper is not a function for " + canonicalPath;
            JSValueUnprotect(ctx, exportsObj);
            moduleCache.erase(canonicalPath);
            return nullptr;
        }

        JSStringRef importName = JSStringCreateWithUTF8CString("__import");
        JSObjectRef importFn = JSObjectMakeFunctionWithCallback(ctx, importName, importCallback);
        JSStringRelease(importName);

        JSStringRef dynamicImportName = JSStringCreateWithUTF8CString("__dynamicImport");
        JSObjectRef dynamicImportFn = JSObjectMakeFunctionWithCallback(ctx, dynamicImportName, dynamicImportCallback);
        JSStringRelease(dynamicImportName);

        JSValueRef filenameValue = makeJsStringValue(ctx, canonicalPath);
        JSValueRef args[] = { importFn, dynamicImportFn, exportsObj, filenameValue };

        JSValueRef callException = nullptr;
        JSObjectCallAsFunction(ctx, wrapperFn, nullptr, 4, args, &callException);

        if (callException) {
            errorOut = "ESM runtime error in " + canonicalPath + ": " + jsValueToStdString(ctx, callException);
            JSValueUnprotect(ctx, exportsObj);
            moduleCache.erase(canonicalPath);
            return nullptr;
        }

        moduleCache[canonicalPath].loading = false;
        moduleCache[canonicalPath].loaded = true;
        return exportsObj;
    }
}

namespace ModuleManager {
    bool looksLikeModule(const std::string& source) {
        static const std::regex moduleSyntaxRegex(R"((^|\n)\s*(import\s+.+from\s+['\"].+['\"]\s*;?|import\s+['\"].+['\"]\s*;?|import\s*\(|export\s+))");
        return std::regex_search(source, moduleSyntaxRegex);
    }

    bool evaluateEntryModule(JSContextRef ctx, const std::string& entryPath, std::string& errorOut) {
        fs::path path(entryPath);
        path = fs::weakly_canonical(path);

        if (path.extension() != ".js") {
            errorOut = "ESM entry must be a .js file: " + path.string();
            return false;
        }

        moduleCache.clear();

        JSObjectRef exportsObj = loadModuleByPath(ctx, path.string(), errorOut);
        return exportsObj != nullptr;
    }
}
