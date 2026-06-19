(module
  ;; Test 4: Bitwise shift left
  ;; Returns: a << b
  ;; Note: shift amount is masked to 5 bits (0-31)
  (func (export "main") (param $a i32) (param $b i32) (result i32)
    (i32.shl (local.get $a) (local.get $b))
  )
)
