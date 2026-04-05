import { ok, equal } from "taco:assert";
import tests, { describe, it, summary, reset } from "tacos:test";

ok(typeof tests.test === "function", "default export should expose test function");
ok(typeof tests.describe === "function", "default export should expose describe function");
ok(typeof tests.it === "function", "default export should expose it function");

reset();

describe("math", () => {
  const pass = it("simple pass", () => {
    const x = 2 + 2;
    if (x !== 4) {
      throw new Error("bad math");
    }
  });

  const fail = it("simple fail", () => {
    throw new Error("boom");
  });

  ok(pass === true, "it should return true for passing test");
  ok(fail === false, "it should return false for failing test");
});

const stats = summary();
equal(stats.passed, 1, "summary.passed should be tracked");
equal(stats.failed, 1, "summary.failed should be tracked");
equal(stats.total, 2, "summary.total should be tracked");

console.log("tacos_test_module ok");
