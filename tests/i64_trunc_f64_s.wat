(module
  (func (export "test") (param f64) (result i64)
    local.get 0
    i64.trunc_f64_s)
)
