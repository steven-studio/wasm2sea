(module
  (func (export "test") (param f64) (result i32)
    local.get 0
    i32.trunc_f64_s)
)
