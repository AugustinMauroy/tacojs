const assert = (condition, message) => {
  if (!condition) {
    throw new Error(message);
  }
};

const assertEqual = (actual, expected, message) => {
  if (!Object.is(actual, expected)) {
    throw new Error(`${message}: expected ${String(expected)}, got ${String(actual)}`);
  }
};

import defVal, { add, value as namedValue } from "./esm_modules/math.js";
import * as messageNs from "./esm_modules/message.js";
import "./esm_modules/side_effect.js";
import { flagFromSideEffect } from "./esm_modules/side_effect_capture.js";

assertEqual(defVal, 7, "default export should be imported");
assertEqual(add(2, 5), 7, "named function export should be imported");
assertEqual(namedValue, 3, "named const export should be imported");
assertEqual(messageNs.message, "hello-esm", "namespace import should expose module exports");
assert(flagFromSideEffect === true, "side-effect module should run and mutate global state");

console.log("esm ok");
