(module
  (func (export "test") (param i64) (result i32)
    local.get 0
    i32.wrap_i64)
)
