(module
  (func (export "test") (param i32 i32) (result i64)
    local.get 0
    i64.extend_i32_s
    local.get 1
    i64.extend_i32_s
    i64.mul))
