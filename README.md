# TacoJS

A shitty JS runtime based on JavascriptCore

## Features

- Execute JavaScript files using JavaScriptCore.
- Built-in `console` module with support for `log`, `info`, and `warn` methods.
- ANSI color support for enhanced terminal output.
- Support `WebAssembly` module.

## Getting Started

### Prerequisites

- A C++17-compatible compiler (e.g., `g++`).
- macOS with JavaScriptCore framework available.

### Building the Project

To build the project, follow these steps:

1. Clone the repository.
2. Run the following command to compile the project:

   ```bash
   g++ src/*.cpp src/builtin/*.cpp -std=c++20 -O3 -I /System/Library/Frameworks/JavaScriptCore.framework -framework JavaScriptCore -o taco
   ```

### Running a JavaScript File

Once built, you can run a JavaScript file using:

```bash
./taco <path-to-js-file>
```

For example:

```bash
./taco examples/basic.js
```

## Example

Here's an example JavaScript file:

```javascript
const a = 2;
const b = 3;

console.log(a + b); // => 5
console.info(a * b); // => 6
console.warn(a / b); // => 0.6666666666666666

const sum = (...args) => args.reduce((acc, val) => acc + val, 0);

console.log(sum(1, 2, 3, 4, 5)); // => 15
```

## Runtime API

### Global APIs

#### Console

- `console.log(...args)`
- `console.info(...args)`
- `console.warn(...args)`
- `console.error(...args)`

#### Timers

- `setTimeout(callback, delayMs, ...args)`
- `clearTimeout(id)`
- `setInterval(callback, delayMs, ...args)`
- `clearInterval(id)`

#### WebAssembly

`WebAssembly` is available with:

- `new WebAssembly.Module(uint8Array)`
- `new WebAssembly.Instance(module)`

### ESM Support

TacoJS supports ES modules for `.js` files:

- `import ... from "./module.js"`
- `const mod = await import("./module.js")`
- `export ...`

Current constraints:

- Only `.js` extension is supported for entry files and imported files.
- Bare package specifiers (for example `import x from "pkg"`) are not supported yet.
- Dynamic import supports builtin specifiers (`taco:*`, `tacos:*`) and relative/absolute `.js` module paths.

Dynamic import example:

```javascript
const fsMod = await import("taco:fs");
const localMod = await import("./my-module.js");

console.log(typeof fsMod.readFile === "function"); // true
console.log(localMod.default);
```

### Built-in ESM Modules

#### taco:assert

```javascript
import assert, { ok, equal, notEqual, throws } from "taco:assert";

ok(true, "condition should be truthy");
equal(2 + 2, 4);
notEqual(2 + 2, 5);
throws(() => {
   throw new Error("boom");
});
```

Exports:

- `ok(value, message?)`
- `equal(actual, expected, message?)`
- `notEqual(actual, expected, message?)`
- `throws(fn, message?)`
- default export with the same functions

#### taco:fs

```javascript
import fs, { readFile, writeFile, exists, unlink } from "taco:fs";

writeFile("/tmp/demo.txt", "hello");
console.log(exists("/tmp/demo.txt")); // true
console.log(readFile("/tmp/demo.txt")); // hello
unlink("/tmp/demo.txt");
```

Exports:

- `readFile(path)`
- `writeFile(path, content)`
- `exists(path)`
- `unlink(path)`
- default export with the same functions

#### taco:path

```javascript
import path, { join, dirname, basename, extname, normalize } from "taco:path";

console.log(join("/tmp", "taco", "file.txt")); // /tmp/taco/file.txt
console.log(dirname("/tmp/taco/file.txt")); // /tmp/taco
console.log(basename("/tmp/taco/file.txt")); // file.txt
console.log(extname("/tmp/taco/file.txt")); // .txt
console.log(normalize("/tmp/taco/../taco/file.txt")); // /tmp/taco/file.txt
```

Exports:

- `join(...parts)`
- `dirname(path)`
- `basename(path)`
- `extname(path)`
- `normalize(path)`
- default export with the same functions

#### tacos:test

```javascript
import tests, { describe, it, test, summary, reset } from "tacos:test";

reset();

describe("math", () => {
   it("adds numbers", () => {
      if (2 + 2 !== 4) throw new Error("bad math");
   });

   it("intentional fail", () => {
      throw new Error("boom");
   });
});

test("standalone test", () => {
   // regular test API still works
});

console.log(summary()); // { passed, failed, total }
```

Exports:

- `test(name, fn)`
- `describe(name, fn)`
- `it(name, fn)`
- `summary()`
- `reset()`
- default export with the same functions

## License

This project is licensed under the MIT License. See the [LICENSE](LICENSE) file for details.
