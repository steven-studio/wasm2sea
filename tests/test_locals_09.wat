(module
  ;; Test 9: Swap two parameters using a temporary local
  ;; Swaps a and b, returns new value of a (original b)
  (func (export "main") (param $a i32) (param $b i32) (result i32)
    (local $temp i32)
    (local.set $temp (local.get $a))
    (local.set $a (local.get $b))
    (local.set $b (local.get $temp))
    (local.get $a)
  )
)
