(module
  (memory 1)
  (func (export "test") (param i64) (result i64)
    i32.const 0
    local.get 0
    i64.store
    i32.const 0
    i64.load)
)
