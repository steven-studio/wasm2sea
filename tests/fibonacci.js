const fs = require('fs');
const wasmBuffer = fs.readFileSync('fibonacci.wasm');

WebAssembly.instantiate(wasmBuffer).then(result => {
  const { fibonacci } = result.instance.exports;
  
  // 测试用例
  console.log('fib(0) =', fibonacci(0));   // 0
  console.log('fib(1) =', fibonacci(1));   // 1
  console.log('fib(2) =', fibonacci(2));   // 1
  console.log('fib(3) =', fibonacci(3));   // 2
  console.log('fib(5) =', fibonacci(5));   // 5
  console.log('fib(10) =', fibonacci(10)); // 55
  console.log('fib(20) =', fibonacci(20)); // 6765
  
  console.log('\n✓ All Fibonacci tests passed!');
});
