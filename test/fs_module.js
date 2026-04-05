import { ok, equal, throws } from "taco:assert";
import fs, { readFile, writeFile, exists, unlink } from "taco:fs";

const path = "/tmp/tacojs_fs_module_test.txt";

const removedBefore = unlink(path);
ok(removedBefore === true || removedBefore === false, "unlink should return a boolean");

ok(writeFile(path, "hello-taco-fs") === true, "writeFile should return true on success");
ok(exists(path), "exists should return true after writing file");

equal(readFile(path), "hello-taco-fs", "readFile should read written content");
ok(typeof fs.readFile === "function", "default export should expose readFile");

throws(() => {
  readFile("/tmp/tacojs_missing_fs_module_file.txt");
}, "readFile should throw when file does not exist");

ok(unlink(path), "unlink should remove existing file");
ok(!exists(path), "exists should return false after unlink");

console.log("fs_module ok");
