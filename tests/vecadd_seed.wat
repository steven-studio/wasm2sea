(module
  (memory 1)
  (func $vecadd_seed (export "test") (param i32 i32) (result i32)
    (local $a i32) (local $b i32) (local $sum0 i32)
    local.get 0
    local.set $a
    local.get 1
    local.set $b

    ;; sum0 = a[0] + b[0]
    local.get $a
    i32.load offset=0
    local.get $b
    i32.load offset=0
    i32.add
    local.set $sum0

    ;; sum1 = a[4] + b[4]
    local.get $a
    i32.load offset=4
    local.get $b
    i32.load offset=4
    i32.add

    ;; return sum0 + sum1，兩組都活著、都可達Return
    local.get $sum0
    i32.add
  )
)
