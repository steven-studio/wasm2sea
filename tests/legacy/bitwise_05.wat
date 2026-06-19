(module
  ;; Test 5: Arithmetic shift right (sign-extending)
  ;; Returns: a >> b (preserves sign bit)
  (func (export "main") (param $a i32) (param $b i32) (result i32)
    (i32.shr_s (local.get $a) (local.get $b))
  )
)
