(module
  (func (export "main") (param $a i32) (param $b i32) (param $c i32) (result i32)
    ;; Compute (a + b) * c - (a - b)
    (i32.sub
      (i32.mul
        (i32.add (local.get $a) (local.get $b))
        (local.get $c)
      )
      (i32.sub (local.get $a) (local.get $b))
    )
  )
)