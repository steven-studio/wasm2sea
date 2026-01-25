(module
  (func (export "main") (param $x i32) (result i32)
    (if (result i32)
      (i32.lt_s (local.get $x) (i32.const 0))
      (then (i32.sub (i32.const 0) (local.get $x)))
      (else (local.get $x))
    )
  )
)