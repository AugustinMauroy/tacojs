import { ok, equal } from "taco:assert";
import path, { join, dirname, basename, extname, normalize } from "taco:path";

equal(join("/tmp", "taco", "file.txt"), "/tmp/taco/file.txt", "join should combine path segments");
equal(dirname("/tmp/taco/file.txt"), "/tmp/taco", "dirname should return parent directory");
equal(basename("/tmp/taco/file.txt"), "file.txt", "basename should return filename");
equal(extname("/tmp/taco/file.txt"), ".txt", "extname should return extension");

const normalized = normalize("/tmp/taco/../taco/./file.txt");
equal(normalized, "/tmp/taco/file.txt", "normalize should collapse dot segments");

ok(typeof path.join === "function", "default export should expose join");
ok(typeof path.normalize === "function", "default export should expose normalize");

console.log("path_module ok");
