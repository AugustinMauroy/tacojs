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

const arr = Array.from({ length: 5 }, (_, i) => i + 1);
assertEqual(arr.length, 5, "Array.from should create expected length");
assertEqual(arr[4], 5, "last element should be 5");

const evenSquares = arr.filter((n) => n % 2 === 0).map((n) => n * n);
assertEqual(JSON.stringify(evenSquares), "[4,16]", "filter+map pipeline should match expected values");

const flattened = [[1], [2, 3], [4]].flat();
assertEqual(JSON.stringify(flattened), "[1,2,3,4]", "Array.flat should flatten one level");

const obj = Object.fromEntries([
  ["a", 1],
  ["b", 2]
]);
assertEqual(obj.a, 1, "Object.fromEntries should map key a");
assertEqual(obj.b, 2, "Object.fromEntries should map key b");

const entries = Object.entries(obj);
assert(entries.some(([k, v]) => k === "b" && v === 2), "Object.entries should include key/value pairs");

const text = "tacojs runtime";
assert(text.startsWith("taco"), "startsWith should work");
assert(text.includes("run"), "includes should find substrings");

const match = /t(aco)js/.exec("tacojs");
assert(match !== null, "RegExp.exec should produce a match");
assertEqual(match[1], "aco", "capturing group should contain expected value");

const date = new Date("2020-01-02T03:04:05Z");
assertEqual(date.getUTCFullYear(), 2020, "Date parsing should keep UTC year");
assertEqual(date.getUTCMonth(), 0, "Date month should be January (0)");

const encoded = encodeURIComponent("a b+c");
assertEqual(decodeURIComponent(encoded), "a b+c", "URI encode/decode should round-trip");

console.log("builtins ok");
