#!/bin/bash

echo "Compiling taco..."
g++ src/*.cpp src/builtin/*.cpp -std=c++20 -O3 -I /System/Library/Frameworks/JavaScriptCore.framework -framework JavaScriptCore -o taco
if [ $? -ne 0 ]; then
    echo "FAIL: taco build"
    exit 1
fi
echo "Running tests..."

file=$(find ./test -name "*.js" ! -path "./test/esm_modules/*")
has_fail=0

if [ -z "$file" ]; then
    exit 1
else
    for f in $file; do
        output=$(./taco "$f" 2>&1)
        status=$?

        if [ $status -ne 0 ]; then
            echo "FAIL: $f"
            echo "$output"
            has_fail=1
            continue
        fi

        if echo "$output" | grep -E "JavaScript Syntax Error:|JavaScript Runtime Error:|Timer callback error:|Error setting up promise tracking" > /dev/null; then
            echo "FAIL: $f"
            echo "$output"
            has_fail=1
            continue
        fi

        echo "PASS: $f"
    done
fi

echo "Building C++ ModuleManager test..."
g++ test/module_manager_cpp_test.cpp src/ModuleManager.cpp src/builtin/asserts.cpp src/builtin/fs.cpp src/builtin/path.cpp src/builtin/test.cpp -std=c++20 -I /System/Library/Frameworks/JavaScriptCore.framework -framework JavaScriptCore -o module_manager_cpp_test
if [ $? -ne 0 ]; then
    echo "FAIL: module_manager_cpp_test build"
    exit 1
fi

echo "Running C++ ModuleManager test..."
./module_manager_cpp_test
if [ $? -ne 0 ]; then
    echo "FAIL: module_manager_cpp_test"
    has_fail=1
else
    echo "PASS: module_manager_cpp_test"
fi

rm -f ./module_manager_cpp_test

echo "Building C++ builtin modules test..."
g++ test/builtin_cpp_test.cpp src/builtin/asserts.cpp src/builtin/fs.cpp src/builtin/path.cpp src/builtin/test.cpp -std=c++20 -I /System/Library/Frameworks/JavaScriptCore.framework -framework JavaScriptCore -o builtin_cpp_test
if [ $? -ne 0 ]; then
    echo "FAIL: builtin_cpp_test build"
    exit 1
fi

echo "Running C++ builtin modules test..."
./builtin_cpp_test
if [ $? -ne 0 ]; then
    echo "FAIL: builtin_cpp_test"
    has_fail=1
else
    echo "PASS: builtin_cpp_test"
fi

rm -f ./builtin_cpp_test

if [ $has_fail -ne 0 ]; then
    exit 1
fi

exit 0
