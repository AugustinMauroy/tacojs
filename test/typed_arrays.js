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

const bytes = new Uint8Array([1, 2, 3, 4]);
assertEqual(bytes.byteLength, 4, "Uint8Array byteLength should match input length");

bytes[2] = 9;
assertEqual(bytes[2], 9, "element assignment should update typed array");

const view = new DataView(bytes.buffer);
assertEqual(view.getUint8(2), 9, "DataView should observe typed array writes");

view.setUint16(0, 0x1234, false);
assertEqual(bytes[0], 0x12, "DataView big-endian write high byte");
assertEqual(bytes[1], 0x34, "DataView big-endian write low byte");

const slice = bytes.slice(1, 3);
assertEqual(slice.length, 2, "slice should return requested number of elements");
assertEqual(slice[0], 0x34, "slice should preserve element order");
assertEqual(slice[1], 9, "slice should include the second selected element");

const sum = bytes.reduce((acc, n) => acc + n, 0);
assert(sum > 0, "reduce should work on typed arrays");

console.log("typed_arrays ok");
