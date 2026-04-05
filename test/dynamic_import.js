import { ok, equal } from "taco:assert";

(async () => {
  const fs = await import("taco:fs");
  ok(typeof fs.readFile === "function", "dynamic import should resolve taco:fs builtin");

  const path = await import("taco:path");
  equal(path.join("/tmp", "taco", "dynamic.txt"), "/tmp/taco/dynamic.txt", "dynamic import should resolve taco:path builtin");

  const rel = await import("./esm_modules/dynamic_value.js");
  equal(rel.default, 42, "dynamic import should resolve relative module default export");
  equal(rel.value, 41, "dynamic import should resolve relative module named export");

  console.log("dynamic_import ok");
})();
