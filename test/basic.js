const a = 2;
const b = 3;

console.log(a + b); // => 5
console.info(a * b); // => 6
console.warn(a / b); // => 0.6666666666666666
console.error(a - b); // => -1

const sum = (...args) => args.reduce((acc, val) => acc + val, 0);

console.log(sum(1, 2, 3, 4, 5)); // => 15

const greet = (name) => `Hello, ${name}!`;

console.log(greet('World')); // => Hello, World!

const Person = function (name) {
    this.name = name;
}

Person.prototype.greet = function () {
    return `Hello, ${this.name}!`;
}

const person = new Person('Jane');

console.log(person.greet()); // => Hello, World!


class Animal {
    constructor(name) {
        this.name = name;
    }
    
    greet() {
        return `Hello, ${this.name}!`;
    }
}

const animal = new Animal('Dog');

console.log(animal.greet()); // => Hello, Dog!

const map = new Map();
map.set('key1', 'value1');
map.set('key2', 'value2');

console.log(map.get('key1')); // => value1
console.log(map.has('key2')); // => true
map.delete('key1');
console.log(map.has('key1')); // => false

const set = new Set();
set.add(1);
set.add(2);
set.add(3);

console.log(set.has(2)); // => true
set.delete(2);
console.log(set.has(2)); // => false
console.log([...set]); // => [1, 3]
