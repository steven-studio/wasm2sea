(module
  (func $max (export "test") (param i32 i32) (result i32)
    ;; 实现: a > b ? a : b
    local.get 0
    local.get 1
    i32.gt_s
    if (result i32)
      local.get 0
    else
      local.get 1
    end
  )
)
