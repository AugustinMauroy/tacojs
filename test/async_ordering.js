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

const events = [];

events.push("sync");

Promise.resolve()
  .then(() => {
    events.push("micro1");
  })
  .then(() => {
    events.push("micro2");
  });

setTimeout(() => {
  events.push("timer0");
}, 0);

setTimeout(() => {
  assertEqual(
    JSON.stringify(events.slice(0, 4)),
    JSON.stringify(["sync", "micro1", "micro2", "timer0"]),
    "microtasks should run before timer callbacks"
  );

  const settled = [];

  Promise.resolve(1)
    .then((v) => v + 1)
    .then((v) => settled.push(`value:${v}`));

  Promise.reject("boom")
    .catch((reason) => {
      settled.push(`catch:${reason}`);
      return "ok";
    })
    .finally(() => {
      settled.push("finally");
    });

  setTimeout(() => {
    assert(
      settled.includes("value:2"),
      "resolved promise chain should produce transformed value"
    );
    assert(
      settled.includes("catch:boom"),
      "rejected promise should be handled by catch"
    );
    assert(
      settled.includes("finally"),
      "finally should run after promise settlement"
    );

    console.log("async_ordering ok");
  }, 0);
}, 5);
