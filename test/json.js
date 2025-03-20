const foo = {
    bar: 'baz',
    array: [1, 2, 3]
}

console.log(JSON.stringify(foo)) // => {"bar":"baz"}

const json = '{"bar":"baz"}'
console.log(JSON.parse(json)) // => { bar: 'baz' }
