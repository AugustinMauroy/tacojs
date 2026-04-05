const assert = (condition, message) => {
  if (!condition) {
    throw new Error(message);
  }
};

assert(typeof setTimeout === "function", "setTimeout should exist");
assert(typeof clearTimeout === "function", "clearTimeout should exist");
assert(typeof setInterval === "function", "setInterval should exist");
assert(typeof clearInterval === "function", "clearInterval should exist");

const timeoutId = setTimeout(() => {}, 0);
assert(typeof timeoutId === "number", "setTimeout should return a numeric id");
clearTimeout(timeoutId);

const intervalId = setInterval(() => {}, 5);
assert(typeof intervalId === "number", "setInterval should return a numeric id");
clearInterval(intervalId);

assert(typeof Promise === "function", "Promise should exist");
const p = Promise.resolve(42);
assert(typeof p.then === "function", "Promise.resolve should return thenable object");

let thenCalledSynchronously = false;
p.then(() => {
  thenCalledSynchronously = true;
});
assert(thenCalledSynchronously === false, "Promise.then callback must not run synchronously");

assert(typeof WebAssembly === "object", "WebAssembly global should exist");
const wasmCode = new Uint8Array([
  0x00, 0x61, 0x73, 0x6d,
  0x01, 0x00, 0x00, 0x00
]);
const module = new WebAssembly.Module(wasmCode);
const instance = new WebAssembly.Instance(module);
assert(module.type === "WebAssembly.Module", "WebAssembly.Module should expose mock type");
assert(instance.type === "WebAssembly.Instance", "WebAssembly.Instance should expose mock type");

console.log("runtime_api ok");
