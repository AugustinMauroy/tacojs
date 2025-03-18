# TacoJS

A shitty JS runtime based on JavascriptCore

## Features

- Execute JavaScript files using JavaScriptCore.
- Built-in `console` module with support for `log`, `info`, and `warn` methods.
- ANSI color support for enhanced terminal output.
- Support for importing internal modules with the `taco:` prefix.

## Getting Started

### Prerequisites

- A C++17-compatible compiler (e.g., `g++`).
- macOS with JavaScriptCore framework available.

### Building the Project

To build the project, follow these steps:

1. Clone the repository.
2. Run the following command to compile the project:

   ```bash
   g++ src/**/**.cpp -std=c++23 -o3 -I /System/Library/Frameworks/JavaScriptCore.framework -framework JavaScriptCore -o taco
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

### Using Internal Modules

You can import internal modules using the `taco:` prefix. For example, to use the `shitty` module:

```javascript
import shitty from 'taco:shitty';

shitty.greet(); // => "Hello from the ShittyModule!"
```

## Example

Here's an example JavaScript file (`examples/basic.js`):

```javascript
const a = 2;
const b = 3;

console.log(a + b); // => 5
console.info(a * b); // => 6
console.warn(a / b); // => 0.6666666666666666

const sum = (...args) => args.reduce((acc, val) => acc + val, 0);

console.log(sum(1, 2, 3, 4, 5)); // => 15
```

## License

This project is licensed under the MIT License. See the [LICENSE](LICENSE) file for details.
