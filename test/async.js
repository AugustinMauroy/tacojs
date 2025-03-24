(async () => {
  const result = await Promise.resolve('async IIFE');
  console.log(result); // async IIFE
})();


const async = async () => {
  const result = await Promise.resolve('async');
  return result;
}

async().then(console.log); // async

function* fibonacci() {
  let a = 0;
  let b = 1;
  while (true) {
    yield a;
    [a, b] = [b, a + b];
  }
}

const gen = fibonacci();

console.log(gen.next().value); // 0
console.log(gen.next().value); // 1
console.log(gen.next().value); // 1
console.log(gen.next().value); // 2
console.log(gen.next().value); // 3
console.log(gen.next().value); // 5

gen.return('done');

console.log("gen.next().value",gen.next().value); // undefined

const fetchData = async () => {
  return new Promise((resolve) => {
    setTimeout(() => {
      resolve('Data fetched after 2 seconds');
    }, 2000);
  });
};


const processData = async () => {
  try {
    const data = await fetchData();
    console.log(`Processed: ${data}`);
  } catch (error) {
    console.error('Error:', error);
  }
};

processData(); // Processed: Data fetched after 2 seconds

setTimeout(() => {
  console.log('setTimeout');
}, 0);

console.log("Promise:", Promise);
