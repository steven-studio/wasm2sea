(module
  ;; Test 1: Bitwise AND operation
  ;; Returns: a & b
  (func (export "main") (param $a i32) (param $b i32) (result i32)
    (i32.and (local.get $a) (local.get $b))
  )
)