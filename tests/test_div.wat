(module
  (func (export "main") (param $a i32) (param $b i32) (result i32)
    (i32.div_s (local.get $a) (local.get $b))
  )
)