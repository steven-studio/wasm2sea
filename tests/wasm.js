#!/usr/bin/env node
/**
 * wasm - Simple WASM runner that shows output
 * Usage: node wasm.js <wasm_file> <function> <args...>
 * Example: node wasm.js test_div.wasm main 10 2
 */

const fs = require('fs');

async function run() {
    const args = process.argv.slice(2);
    
    if (args.length < 2) {
        console.log('Usage: node wasm.js <wasm_file> <function> [args...]');
        console.log('Example: node wasm.js test_div.wasm main 10 2');
        process.exit(1);
    }
    
    const wasmFile = args[0];
    const funcName = args[1];
    const funcArgs = args.slice(2).map(x => parseInt(x, 10));
    
    try {
        // Load WASM file
        const wasmBuffer = fs.readFileSync(wasmFile);
        
        // Compile and instantiate
        const wasmModule = await WebAssembly.compile(wasmBuffer);
        const wasmInstance = await WebAssembly.instantiate(wasmModule);
        
        // Get function
        const func = wasmInstance.exports[funcName];
        
        if (!func) {
            console.error(`Error: Function '${funcName}' not found`);
            console.error('Available functions:', Object.keys(wasmInstance.exports).join(', '));
            process.exit(1);
        }
        
        // Call function and print result
        const result = func(...funcArgs);
        console.log(result);
        
    } catch (error) {
        console.error('Error:', error.message);
        process.exit(1);
    }
}

run();