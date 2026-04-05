#include <JavaScriptCore/JavaScriptCore.h>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include "../src/ModuleManager.h"

#if defined(__APPLE__)
namespace fs = std::__fs::filesystem;
#else
namespace fs = std::filesystem;
#endif

namespace {
    bool writeFile(const fs::path& path, const std::string& content) {
        std::ofstream out(path);
        if (!out.is_open()) {
            return false;
        }
        out << content;
        return true;
    }

    std::string jsValueToString(JSContextRef ctx, JSValueRef value) {
        JSValueRef conversionException = nullptr;
        JSStringRef strRef = JSValueToStringCopy(ctx, value, &conversionException);
        if (!strRef) {
            return "";
        }

        size_t bufferSize = JSStringGetMaximumUTF8CStringSize(strRef);
        std::string result(bufferSize, '\0');
        size_t used = JSStringGetUTF8CString(strRef, &result[0], bufferSize);
        JSStringRelease(strRef);

        if (used == 0) {
            return "";
        }

        result.resize(used - 1);
        return result;
    }

    bool assertTrue(bool condition, const std::string& message) {
        if (!condition) {
            std::cerr << "[FAIL] " << message << std::endl;
            return false;
        }
        std::cout << "[PASS] " << message << std::endl;
        return true;
    }

    bool evaluateToInt(JSContextRef ctx, const std::string& expr, int expected, const std::string& message) {
        JSStringRef script = JSStringCreateWithUTF8CString(expr.c_str());
        JSValueRef exception = nullptr;
        JSValueRef value = JSEvaluateScript(ctx, script, nullptr, nullptr, 0, &exception);
        JSStringRelease(script);

        if (exception) {
            std::cerr << "[FAIL] " << message << " (exception: " << jsValueToString(ctx, exception) << ")" << std::endl;
            return false;
        }

        int actual = static_cast<int>(JSValueToNumber(ctx, value, nullptr));
        if (actual != expected) {
            std::cerr << "[FAIL] " << message << " (expected " << expected << ", got " << actual << ")" << std::endl;
            return false;
        }

        std::cout << "[PASS] " << message << std::endl;
        return true;
    }
}

int main() {
    int failures = 0;

    failures += !assertTrue(ModuleManager::looksLikeModule("import { a } from './m.js';"), "looksLikeModule detects import syntax");
    failures += !assertTrue(ModuleManager::looksLikeModule("export const n = 1;"), "looksLikeModule detects export syntax");
    failures += !assertTrue(!ModuleManager::looksLikeModule("const n = 1; console.log(n);"), "looksLikeModule ignores classic scripts");

    JSGlobalContextRef ctx = JSGlobalContextCreate(nullptr);

    fs::path fixtureRoot = fs::temp_directory_path() / "tacojs-module-manager-cpp-test";
    std::error_code ec;
    fs::remove_all(fixtureRoot, ec);
    fs::create_directories(fixtureRoot, ec);
    if (ec) {
        std::cerr << "[FAIL] unable to create fixture directory: " << fixtureRoot << std::endl;
        JSGlobalContextRelease(ctx);
        return 1;
    }

    const fs::path depPath = fixtureRoot / "dep.js";
    const fs::path entryPath = fixtureRoot / "entry.js";
    const fs::path badDepPath = fixtureRoot / "bad_dep.mjs";
    const fs::path badEntryPath = fixtureRoot / "entry_bad_ext.mjs";
    const fs::path barePath = fixtureRoot / "entry_bare.js";
    const fs::path missingPath = fixtureRoot / "entry_missing.js";
    const fs::path badDepImportPath = fixtureRoot / "entry_bad_dep_import.js";
    const fs::path absoluteDepPath = fixtureRoot / "dep_absolute.js";
    const fs::path absoluteEntryPath = fixtureRoot / "entry_absolute.js";
    const fs::path sideEffectDepPath = fixtureRoot / "side_effect_dep.js";
    const fs::path sideEffectEntryPath = fixtureRoot / "entry_side_effect.js";
    const fs::path noExtImportPath = fixtureRoot / "entry_no_ext_import.js";
    const fs::path tacoAssertEntryPath = fixtureRoot / "entry_taco_assert.js";
    const fs::path tacoFsEntryPath = fixtureRoot / "entry_taco_fs.js";
    const fs::path tacoFsDataPath = fixtureRoot / "taco_fs_data.txt";
    const fs::path tacoPathEntryPath = fixtureRoot / "entry_taco_path.js";
    const fs::path dynamicDepPath = fixtureRoot / "dyn_dep.js";
    const fs::path dynamicEntryPath = fixtureRoot / "entry_dynamic.js";
    const fs::path tacosTestEntryPath = fixtureRoot / "entry_tacos_test.js";
    const fs::path tacoUnknownEntryPath = fixtureRoot / "entry_taco_unknown.js";

    if (!writeFile(depPath, "export const base = 5;\n")) {
        std::cerr << "[FAIL] unable to write dep.js fixture" << std::endl;
        JSGlobalContextRelease(ctx);
        return 1;
    }

    if (!writeFile(entryPath,
        "import { base } from './dep.js';\n"
        "globalThis.__module_manager_value = base * 2;\n")) {
        std::cerr << "[FAIL] unable to write entry.js fixture" << std::endl;
        JSGlobalContextRelease(ctx);
        return 1;
    }

    if (!writeFile(badDepPath, "export const x = 1;\n") ||
        !writeFile(badEntryPath, "import { x } from './bad_dep.mjs';\n") ||
        !writeFile(barePath, "import v from 'some-package';\n") ||
        !writeFile(missingPath, "import { noSuch } from './missing.js';\n") ||
        !writeFile(badDepImportPath, "import { x } from './bad_dep.mjs';\n") ||
        !writeFile(absoluteDepPath, "export const abs = 11;\n") ||
        !writeFile(absoluteEntryPath,
            "import { abs } from '" + absoluteDepPath.string() + "';\n"
            "globalThis.__module_manager_abs = abs;\n") ||
        !writeFile(sideEffectDepPath, "globalThis.__module_manager_side_effect = 1;\n") ||
        !writeFile(sideEffectEntryPath,
            "import './side_effect_dep.js';\n"
            "globalThis.__module_manager_side_effect_result = globalThis.__module_manager_side_effect;\n") ||
        !writeFile(noExtImportPath, "import { base } from './dep';\n") ||
        !writeFile(tacoAssertEntryPath,
            "import assert, { ok, equal, notEqual, throws } from 'taco:assert';\n"
            "ok(typeof assert.ok === 'function');\n"
            "equal(4, 4);\n"
            "notEqual(4, 5);\n"
            "throws(() => { throw new Error('x'); });\n"
            "globalThis.__module_manager_assert_ok = 1;\n") ||
        !writeFile(tacoFsEntryPath,
            "import fs, { readFile, writeFile, exists, unlink } from 'taco:fs';\n"
            "const p = '" + tacoFsDataPath.string() + "';\n"
            "writeFile(p, 'abc');\n"
            "if (!exists(p)) { throw new Error('exists false after write'); }\n"
            "const content = readFile(p);\n"
            "globalThis.__module_manager_fs_len = content.length;\n"
            "globalThis.__module_manager_fs_default_ok = (typeof fs.readFile === 'function') ? 1 : 0;\n"
            "unlink(p);\n"
            "globalThis.__module_manager_fs_removed = exists(p) ? 0 : 1;\n") ||
        !writeFile(tacoPathEntryPath,
            "import path, { join, dirname, basename, extname, normalize } from 'taco:path';\n"
            "const joined = join('/tmp', 'taco', 'a.txt');\n"
            "globalThis.__module_manager_path_join_ok = joined === '/tmp/taco/a.txt' ? 1 : 0;\n"
            "globalThis.__module_manager_path_dir_ok = dirname('/tmp/taco/a.txt') === '/tmp/taco' ? 1 : 0;\n"
            "globalThis.__module_manager_path_base_ok = basename('/tmp/taco/a.txt') === 'a.txt' ? 1 : 0;\n"
            "globalThis.__module_manager_path_ext_ok = extname('/tmp/taco/a.txt') === '.txt' ? 1 : 0;\n"
            "globalThis.__module_manager_path_norm_ok = normalize('/tmp/taco/../taco/a.txt') === '/tmp/taco/a.txt' ? 1 : 0;\n"
            "globalThis.__module_manager_path_default_ok = typeof path.join === 'function' ? 1 : 0;\n") ||
        !writeFile(dynamicDepPath,
            "export const value = 7;\n"
            "export default 9;\n") ||
        !writeFile(dynamicEntryPath,
            "const pBuiltin = import('taco:fs');\n"
            "const pRelative = import('./dyn_dep.js');\n"
            "globalThis.__module_manager_dynamic_builtin_promise = (typeof pBuiltin.then === 'function') ? 1 : 0;\n"
            "globalThis.__module_manager_dynamic_relative_promise = (typeof pRelative.then === 'function') ? 1 : 0;\n") ||
        !writeFile(tacosTestEntryPath,
            "import tests, { describe, it, summary, reset } from 'tacos:test';\n"
            "reset();\n"
            "const suite = describe('suite', () => {\n"
            "  globalThis.__module_manager_tacos_it_pass = it('a', () => {});\n"
            "  globalThis.__module_manager_tacos_it_fail = it('b', () => { throw new Error('x'); });\n"
            "});\n"
            "const s = summary();\n"
            "globalThis.__module_manager_tacos_default_ok = (typeof tests.describe === 'function' && typeof tests.it === 'function') ? 1 : 0;\n"
            "globalThis.__module_manager_tacos_ret_ok = (suite === true && globalThis.__module_manager_tacos_it_pass === true && globalThis.__module_manager_tacos_it_fail === false) ? 1 : 0;\n"
            "globalThis.__module_manager_tacos_passed = s.passed;\n"
            "globalThis.__module_manager_tacos_failed = s.failed;\n"
            "globalThis.__module_manager_tacos_total = s.total;\n") ||
        !writeFile(tacoUnknownEntryPath, "import 'taco:does-not-exist';\n")) {
        std::cerr << "[FAIL] unable to write failing fixtures" << std::endl;
        JSGlobalContextRelease(ctx);
        return 1;
    }

    std::string error;
    bool ok = ModuleManager::evaluateEntryModule(ctx, entryPath.string(), error);
    failures += !assertTrue(ok, "evaluateEntryModule loads valid .js entry with relative import");
    if (!ok) {
        std::cerr << "[INFO] error was: " << error << std::endl;
    } else {
        failures += !evaluateToInt(ctx, "globalThis.__module_manager_value", 10, "loaded module side effect is visible in context");
    }

    error.clear();
    ok = ModuleManager::evaluateEntryModule(ctx, badEntryPath.string(), error);
    failures += !assertTrue(!ok, "evaluateEntryModule rejects non-.js entry path");
    failures += !assertTrue(error.find(".js") != std::string::npos, "non-.js entry returns .js restriction error");

    error.clear();
    ok = ModuleManager::evaluateEntryModule(ctx, barePath.string(), error);
    failures += !assertTrue(!ok, "evaluateEntryModule rejects bare module specifiers in phase 1");
    failures += !assertTrue(error.find("Bare specifiers") != std::string::npos, "bare specifier error message is returned");

    error.clear();
    ok = ModuleManager::evaluateEntryModule(ctx, missingPath.string(), error);
    failures += !assertTrue(!ok, "evaluateEntryModule fails when relative module path is missing");
    failures += !assertTrue(error.find("Module not found") != std::string::npos, "missing module returns module-not-found error");

    error.clear();
    ok = ModuleManager::evaluateEntryModule(ctx, badDepImportPath.string(), error);
    failures += !assertTrue(!ok, "evaluateEntryModule rejects imported dependency with non-.js extension");
    failures += !assertTrue(error.find("Only .js modules are supported") != std::string::npos, "non-.js dependency import returns extension error");

    error.clear();
    ok = ModuleManager::evaluateEntryModule(ctx, absoluteEntryPath.string(), error);
    failures += !assertTrue(ok, "evaluateEntryModule accepts absolute .js import specifier");
    if (!ok) {
        std::cerr << "[INFO] error was: " << error << std::endl;
    } else {
        failures += !evaluateToInt(ctx, "globalThis.__module_manager_abs", 11, "absolute import binding is evaluated correctly");
    }

    error.clear();
    ok = ModuleManager::evaluateEntryModule(ctx, sideEffectEntryPath.string(), error);
    failures += !assertTrue(ok, "evaluateEntryModule supports side-effect-only imports");
    if (!ok) {
        std::cerr << "[INFO] error was: " << error << std::endl;
    } else {
        failures += !evaluateToInt(ctx, "globalThis.__module_manager_side_effect_result", 1, "side-effect import executes dependency module body");
    }

    error.clear();
    ok = ModuleManager::evaluateEntryModule(ctx, noExtImportPath.string(), error);
    failures += !assertTrue(!ok, "evaluateEntryModule requires explicit .js extension in import specifier");
    failures += !assertTrue(error.find("Only .js modules are supported") != std::string::npos || error.find("Module not found") != std::string::npos,
        "no-extension import returns path-resolution error");

    error.clear();
    ok = ModuleManager::evaluateEntryModule(ctx, tacoAssertEntryPath.string(), error);
    failures += !assertTrue(ok, "evaluateEntryModule supports taco:assert builtin module");
    if (!ok) {
        std::cerr << "[INFO] error was: " << error << std::endl;
    } else {
        failures += !evaluateToInt(ctx, "globalThis.__module_manager_assert_ok", 1, "taco:assert assertions execute successfully");
    }

    error.clear();
    ok = ModuleManager::evaluateEntryModule(ctx, tacoFsEntryPath.string(), error);
    failures += !assertTrue(ok, "evaluateEntryModule supports taco:fs builtin module");
    if (!ok) {
        std::cerr << "[INFO] error was: " << error << std::endl;
    } else {
        failures += !evaluateToInt(ctx, "globalThis.__module_manager_fs_len", 3, "taco:fs readFile returns written content");
        failures += !evaluateToInt(ctx, "globalThis.__module_manager_fs_default_ok", 1, "taco:fs default export exposes functions");
        failures += !evaluateToInt(ctx, "globalThis.__module_manager_fs_removed", 1, "taco:fs unlink removes file");
    }

    error.clear();
    ok = ModuleManager::evaluateEntryModule(ctx, tacoPathEntryPath.string(), error);
    failures += !assertTrue(ok, "evaluateEntryModule supports taco:path builtin module");
    if (!ok) {
        std::cerr << "[INFO] error was: " << error << std::endl;
    } else {
        failures += !evaluateToInt(ctx, "globalThis.__module_manager_path_join_ok", 1, "taco:path join works");
        failures += !evaluateToInt(ctx, "globalThis.__module_manager_path_dir_ok", 1, "taco:path dirname works");
        failures += !evaluateToInt(ctx, "globalThis.__module_manager_path_base_ok", 1, "taco:path basename works");
        failures += !evaluateToInt(ctx, "globalThis.__module_manager_path_ext_ok", 1, "taco:path extname works");
        failures += !evaluateToInt(ctx, "globalThis.__module_manager_path_norm_ok", 1, "taco:path normalize works");
        failures += !evaluateToInt(ctx, "globalThis.__module_manager_path_default_ok", 1, "taco:path default export exposes functions");
    }

    error.clear();
    ok = ModuleManager::evaluateEntryModule(ctx, dynamicEntryPath.string(), error);
    failures += !assertTrue(ok, "evaluateEntryModule supports dynamic import() transformation");
    if (!ok) {
        std::cerr << "[INFO] error was: " << error << std::endl;
    } else {
        failures += !evaluateToInt(ctx, "globalThis.__module_manager_dynamic_builtin_promise", 1, "dynamic import of builtin returns a Promise");
        failures += !evaluateToInt(ctx, "globalThis.__module_manager_dynamic_relative_promise", 1, "dynamic import of relative module returns a Promise");
    }

    error.clear();
    ok = ModuleManager::evaluateEntryModule(ctx, tacosTestEntryPath.string(), error);
    failures += !assertTrue(ok, "evaluateEntryModule supports tacos:test builtin module");
    if (!ok) {
        std::cerr << "[INFO] error was: " << error << std::endl;
    } else {
        failures += !evaluateToInt(ctx, "globalThis.__module_manager_tacos_default_ok", 1, "tacos:test default export exposes describe/it");
        failures += !evaluateToInt(ctx, "globalThis.__module_manager_tacos_ret_ok", 1, "tacos:test returns pass/fail booleans");
        failures += !evaluateToInt(ctx, "globalThis.__module_manager_tacos_passed", 1, "tacos:test summary tracks passed count");
        failures += !evaluateToInt(ctx, "globalThis.__module_manager_tacos_failed", 1, "tacos:test summary tracks failed count");
        failures += !evaluateToInt(ctx, "globalThis.__module_manager_tacos_total", 2, "tacos:test summary tracks total count");
    }

    error.clear();
    ok = ModuleManager::evaluateEntryModule(ctx, tacoUnknownEntryPath.string(), error);
    failures += !assertTrue(!ok, "evaluateEntryModule rejects unsupported taco:* builtin modules");
    failures += !assertTrue(error.find("Unsupported taco builtin module") != std::string::npos,
        "unsupported taco builtin returns explicit error");

    JSGlobalContextRelease(ctx);
    fs::remove_all(fixtureRoot, ec);

    if (failures > 0) {
        std::cerr << "\nModuleManager C++ test failures: " << failures << std::endl;
        return 1;
    }

    std::cout << "\nModuleManager C++ tests passed" << std::endl;
    return 0;
}
