(async () => {
  const result = await Promise.resolve('async IIFE');
  console.log(result); // async IIFE
})();


const async = async () => {
  const result = await Promise.resolve('async');
  return result;
}

async().then(console.log); // async

function* generator() {
  yield 'generator';
}

const gen = generator();

console.log(gen.next().value); // generator

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
