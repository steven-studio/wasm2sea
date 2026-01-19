(module
  ;; Three-way comparison: returns 1 if a > b, -1 if a < b, 0 if a == b
  (func (export "main") (param $a i32) (param $b i32) (result i32)
    ;; Tests: I32GtS, I32LtS, nested If-Else
    (if (result i32)
      (i32.gt_s (local.get $a) (local.get $b))
      (then (i32.const 1))
      (else
        (if (result i32)
          (i32.lt_s (local.get $a) (local.get $b))
          (then (i32.const -1))
          (else (i32.const 0))
        )
      )
    )
  )
)