(module
  (func (export "main") (param $a i32) (param $b i32) (result i32)
    ;; Multiplication: a * b
    (i32.mul
      (local.get $a)
      (local.get $b)
    )
  )
)