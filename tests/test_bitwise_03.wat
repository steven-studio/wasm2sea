(module
  ;; Test 3: Bitwise XOR operation
  ;; Returns: a ^ b
  (func (export "main") (param $a i32) (param $b i32) (result i32)
    (i32.xor (local.get $a) (local.get $b))
  )
)
