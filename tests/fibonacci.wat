(module
  (func $test (param $n i32) (result i32)
    (local $a i32)
    (local $b i32)
    (local $temp i32)
    (local $counter i32)
    
    ;; 边界情况: fib(0) = 0
    local.get $n
    i32.const 0
    i32.eq
    if
      i32.const 0
      return
    end
    
    ;; 边界情况: fib(1) = 1
    local.get $n
    i32.const 1
    i32.eq
    if
      i32.const 1
      return
    end
    
    ;; 初始化: a=0, b=1, counter=1
    i32.const 0
    local.set $a
    
    i32.const 1
    local.set $b
    
    i32.const 1
    local.set $counter
    
    ;; 循环: 从 1 到 n-1
    (loop $fib_loop
      ;; temp = a + b
      local.get $a
      local.get $b
      i32.add
      local.set $temp
      
      ;; a = b
      local.get $b
      local.set $a
      
      ;; b = temp
      local.get $temp
      local.set $b
      
      ;; counter++
      local.get $counter
      i32.const 1
      i32.add
      local.set $counter
      
      ;; 如果 counter < n，继续循环
      local.get $counter
      local.get $n
      i32.lt_s
      br_if $fib_loop
    )
    
    ;; 返回 b (即 fib(n))
    local.get $b
  )
  
  (export "test" (func $test))
)