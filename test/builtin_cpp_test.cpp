#include <JavaScriptCore/JavaScriptCore.h>
#include <iostream>
#include <string>
#include "../src/builtin/asserts.h"
#include "../src/builtin/fs.h"
#include "../src/builtin/path.h"
#include "../src/builtin/test.h"

namespace {
    std::string jsValueToString(JSContextRef ctx, JSValueRef value) {
        JSValueRef conversionException = nullptr;
        JSStringRef strRef = JSValueToStringCopy(ctx, value, &conversionException);
        if (!strRef) {
            return "";
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

    bool assertTrue(bool condition, const std::string& message) {
        if (!condition) {
            std::cerr << "[FAIL] " << message << std::endl;
            return false;
        }
        std::cout << "[PASS] " << message << std::endl;
        return true;
    }

    bool setGlobalObject(JSContextRef ctx, const char* name, JSObjectRef value) {
        JSStringRef prop = JSStringCreateWithUTF8CString(name);
        JSObjectSetProperty(ctx, JSContextGetGlobalObject(ctx), prop, value, kJSPropertyAttributeNone, nullptr);
        JSStringRelease(prop);
        return true;
    }

    bool evalTruthy(JSContextRef ctx, const std::string& expr, const std::string& message) {
        JSStringRef script = JSStringCreateWithUTF8CString(expr.c_str());
        JSValueRef exception = nullptr;
        JSValueRef result = JSEvaluateScript(ctx, script, nullptr, nullptr, 0, &exception);
        JSStringRelease(script);

        if (exception) {
            std::cerr << "[FAIL] " << message << " (exception: " << jsValueToString(ctx, exception) << ")" << std::endl;
            return false;
        }

        return assertTrue(JSValueToBoolean(ctx, result), message);
    }

    bool evalNumberEquals(JSContextRef ctx, const std::string& expr, int expected, const std::string& message) {
        JSStringRef script = JSStringCreateWithUTF8CString(expr.c_str());
        JSValueRef exception = nullptr;
        JSValueRef result = JSEvaluateScript(ctx, script, nullptr, nullptr, 0, &exception);
        JSStringRelease(script);

        if (exception) {
            std::cerr << "[FAIL] " << message << " (exception: " << jsValueToString(ctx, exception) << ")" << std::endl;
            return false;
        }

        int actual = static_cast<int>(JSValueToNumber(ctx, result, nullptr));
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

    JSGlobalContextRef ctx = JSGlobalContextCreate(nullptr);

    JSObjectRef assertExports = BuiltinAsserts::createModuleExports(ctx);
    JSObjectRef fsExports = BuiltinFs::createModuleExports(ctx);
    JSObjectRef pathExports = BuiltinPath::createModuleExports(ctx);
    JSObjectRef testExports = BuiltinTest::createModuleExports(ctx);

    setGlobalObject(ctx, "assert", assertExports);
    setGlobalObject(ctx, "fs", fsExports);
    setGlobalObject(ctx, "path", pathExports);
    setGlobalObject(ctx, "tests", testExports);

    failures += !evalTruthy(ctx, "typeof assert.ok === 'function' && typeof assert.equal === 'function'", "assert exports include ok/equal");
    failures += !evalTruthy(ctx, "typeof assert.default === 'object' && typeof assert.default.throws === 'function'", "assert default export is wired");
    failures += !evalTruthy(ctx, "(function(){ assert.ok(true); assert.equal(2 + 2, 4); assert.notEqual(2, 3); return true; })()", "assert happy-path calls succeed");
    failures += !evalTruthy(ctx, "(function(){ try { assert.equal(1, 2, 'x'); return false; } catch (e) { return String(e).includes('x'); } })()", "assert.equal throws with custom message");
    failures += !evalTruthy(ctx, "(function(){ try { assert.throws(() => {}); return false; } catch (e) { return String(e).includes('Expected function to throw'); } })()", "assert.throws fails when callback does not throw");

    const std::string path = "/tmp/tacojs_builtin_cpp_test_file.txt";
    failures += !evalTruthy(ctx, "fs.unlink('/tmp/tacojs_builtin_cpp_test_file.txt') || true", "fs.unlink on possibly missing file does not crash");
    failures += !evalTruthy(ctx, "fs.writeFile('/tmp/tacojs_builtin_cpp_test_file.txt', 'abc') === true", "fs.writeFile returns true on success");
    failures += !evalTruthy(ctx, "fs.exists('/tmp/tacojs_builtin_cpp_test_file.txt') === true", "fs.exists detects written file");
    failures += !evalNumberEquals(ctx, "fs.readFile('/tmp/tacojs_builtin_cpp_test_file.txt').length", 3, "fs.readFile returns written content");
    failures += !evalTruthy(ctx, "fs.default && typeof fs.default.readFile === 'function'", "fs default export is wired");
    failures += !evalTruthy(ctx, "(function(){ fs.unlink('/tmp/tacojs_builtin_cpp_test_file.txt'); return !fs.exists('/tmp/tacojs_builtin_cpp_test_file.txt'); })()", "fs.unlink removes file");
    failures += !evalTruthy(ctx, "(function(){ try { fs.readFile('/tmp/tacojs_builtin_cpp_test_missing.txt'); return false; } catch (e) { return String(e).includes('Failed to read file'); } })()", "fs.readFile throws on missing file");

    failures += !evalTruthy(ctx, "typeof path.join === 'function' && typeof path.normalize === 'function'", "path exports include join/normalize");
    failures += !evalTruthy(ctx, "typeof path.default === 'object' && typeof path.default.basename === 'function'", "path default export is wired");
    failures += !evalTruthy(ctx, "path.join('/tmp', 'taco', 'x.js') === '/tmp/taco/x.js'", "path.join works");
    failures += !evalTruthy(ctx, "path.dirname('/tmp/taco/x.js') === '/tmp/taco'", "path.dirname works");
    failures += !evalTruthy(ctx, "path.basename('/tmp/taco/x.js') === 'x.js'", "path.basename works");
    failures += !evalTruthy(ctx, "path.extname('/tmp/taco/x.js') === '.js'", "path.extname works");
    failures += !evalTruthy(ctx, "path.normalize('/tmp/taco/../taco/x.js') === '/tmp/taco/x.js'", "path.normalize works");

    failures += !evalTruthy(ctx, "typeof tests.test === 'function' && typeof tests.describe === 'function' && typeof tests.it === 'function' && typeof tests.summary === 'function' && typeof tests.reset === 'function'", "tests exports include test/describe/it/summary/reset");
    failures += !evalTruthy(ctx,
        "(function(){ "
        "tests.reset(); "
        "const d = tests.describe('suite', () => { "
        "  globalThis.__itA = tests.it('a', () => {}); "
        "  globalThis.__itB = tests.it('b', () => { throw new Error('x'); }); "
        "}); "
        "const s = tests.summary(); "
        "return d === true && globalThis.__itA === true && globalThis.__itB === false && s.passed === 1 && s.failed === 1 && s.total === 2; "
        "})()",
        "tests module tracks pass/fail summary");

    JSGlobalContextRelease(ctx);

    if (failures > 0) {
        std::cerr << "\nBuiltin C++ test failures: " << failures << std::endl;
        return 1;
    }

    std::cout << "\nBuiltin C++ tests passed" << std::endl;
    return 0;
}
