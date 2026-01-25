(module
  ;; 定义函数：计算 (a+b)*c
  (func $compute (param $a i32) (param $b i32) (param $c i32) (result i32)
    local.get $a    ;; stack: [a]
    local.get $b    ;; stack: [a, b]
    i32.add         ;; stack: [a+b]
    local.get $c    ;; stack: [a+b, c]
    i32.mul         ;; stack: [(a+b)*c]
  )
  
  ;; 导出函数（可选，测试时方便调用）
  (export "compute" (func $compute))
)
