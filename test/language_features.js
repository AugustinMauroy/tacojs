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

{
  let scoped = 1;
  {
    let scoped = 2;
    assertEqual(scoped, 2, "block scope should shadow outer variable");
  }
  assertEqual(scoped, 1, "outer variable should keep original value");
}

const person = { name: "Ada", meta: { age: 37 } };
const { name, meta: { age } } = person;
assertEqual(name, "Ada", "destructuring should extract name");
assertEqual(age, 37, "nested destructuring should extract age");

const values = [1, 2, 3];
const add = (...args) => args.reduce((acc, n) => acc + n, 0);
assertEqual(add(...values, 4), 10, "rest and spread should work together");

const maybe = { nested: { value: 0 } };
assertEqual(maybe?.nested?.value ?? 12, 0, "optional chaining should read nested value");
assertEqual((null)?.missing ?? "fallback", "fallback", "nullish coalescing fallback should work");

class Animal {
  constructor(name) {
    this.name = name;
  }

  greet() {
    return `Hello ${this.name}`;
  }
}

class Dog extends Animal {
  greet() {
    return `${super.greet()} woof`;
  }
}

const dog = new Dog("Nina");
assertEqual(dog.greet(), "Hello Nina woof", "class inheritance and super should work");

const symA = Symbol("id");
const symB = Symbol("id");
assert(symA !== symB, "symbols should be unique");

const big = 9007199254740991n + 10n;
assertEqual(big, 9007199254741001n, "bigint arithmetic should be correct");

console.log("language_features ok");
