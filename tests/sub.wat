(module
  (func $sub (param $x i32) (param $y i32) (result i32)
    local.get $x
    local.get $y
    i32.sub
  )

  ;; 為了後面測試方便，加一個 export
  (export "sub" (func $sub))
)