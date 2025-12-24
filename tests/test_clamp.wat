(module
  (func (export "main") (param $value i32) (param $min i32) (param $max i32) (result i32)
    (local $temp i32)
    
    ;; First clamp to min
    (if (result i32)
      (i32.lt_s (local.get $value) (local.get $min))
      (then (local.get $min))
      (else
        ;; Then clamp to max
        (if (result i32)
          (i32.gt_s (local.get $value) (local.get $max))
          (then (local.get $max))
          (else (local.get $value))
        )
      )
    )
  )
)