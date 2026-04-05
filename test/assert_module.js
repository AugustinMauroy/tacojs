import assert, { ok, equal, notEqual, throws } from "taco:assert";

ok(typeof assert === "object", "default export should be an object");
ok(typeof assert.ok === "function", "default export should expose ok");

ok(true, "ok should pass on truthy value");
equal(2 + 2, 4, "equal should compare strict equality");
notEqual(2 + 2, 5, "notEqual should compare strict inequality");

throws(() => {
  throw new Error("boom");
}, "throws should pass when callback throws");

let didThrow = false;
try {
  equal(1, 2, "intentional failure");
} catch (error) {
  didThrow = true;
  ok(String(error).includes("intentional failure"), "equal failure should keep custom message");
}

ok(didThrow, "equal should throw when values differ");

console.log("assert_module ok");
