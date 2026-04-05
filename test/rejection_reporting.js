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

assert(typeof __getUnhandledRejectionCount === "function", "runtime should expose unhandled rejection counter");
assert(typeof __getHandledRejectionCount === "function", "runtime should expose handled rejection counter");

Promise.reject("handled-now").catch(() => {});

setTimeout(() => {
  assertEqual(
    __getUnhandledRejectionCount(),
    0,
    "immediately handled rejection should not be reported as unhandled"
  );

  const late = Promise.reject("handled-late");

  setTimeout(() => {
    late.catch(() => {});
  }, 5);

  setTimeout(() => {
    const unhandled = __getUnhandledRejectionCount();
    const handled = __getHandledRejectionCount();

    assert(unhandled >= 1, "late-handled rejection should be reported as unhandled first");
    assert(handled >= 1, "late catch should be tracked as handled after being reported");

    console.log("rejection_reporting ok");
  }, 25);
}, 10);
