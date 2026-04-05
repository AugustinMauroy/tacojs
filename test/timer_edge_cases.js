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

// Should be a no-op and never throw.
clearTimeout(999999);
clearInterval(999999);

const canceledId = setTimeout(() => {
  events.push("canceled-timeout-fired");
}, 0);
clearTimeout(canceledId);

setTimeout(() => {
  events.push("surviving-timeout-fired");
}, 0);

let ticks = 0;
const intervalId = setInterval(() => {
  ticks++;
  events.push(`tick-${ticks}`);

  if (ticks === 2) {
    clearInterval(intervalId);
  }

  if (ticks > 2) {
    throw new Error("clearInterval did not stop the interval after tick 2");
  }
}, 1);

setTimeout(() => {
  assert(
    !events.includes("canceled-timeout-fired"),
    "cleared timeout should not execute"
  );

  assert(
    events.includes("surviving-timeout-fired"),
    "non-cleared timeout should execute"
  );

  assertEqual(ticks, 2, "interval should run exactly twice before being cleared");

  console.log("timer_edge_cases ok");
}, 30);
