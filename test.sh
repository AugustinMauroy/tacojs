#!/bin/bash


file=$(find ./test -name "*.js")



if [ -z "$file" ]; then
    exit 1
else
    for f in $file; do
        if ./taco "$f" > /dev/null; then
            echo "PASS: $f"
        else
            echo "FAIL: $f"
        fi
    done
fi

exit 0
