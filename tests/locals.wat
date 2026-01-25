(module
  ;; Test 1: Basic LocalSet + LocalGet
  (func (export "test_01_basic") (param $x i32) (result i32)
    (local $temp i32)
    (local.set $temp (i32.const 10))
    (local.get $temp)
  )
  
  ;; Test 2: Reassignment
  (func (export "test_02_reassign") (param $x i32) (result i32)
    (local $temp i32)
    (local.set $temp (i32.const 10))
    (local.set $temp (i32.add (local.get $temp) (local.get $x)))
    (local.get $temp)
  )
  
  ;; Test 3: LocalTee
  (func (export "test_03_tee") (param $x i32) (result i32)
    (local $temp i32)
    (i32.add (local.tee $temp (i32.const 5)) (local.get $temp))
  )
  
  ;; Test 4: Parameter as local (parameter can be modified)
  (func (export "test_04_param") (param $x i32) (result i32)
    (local.set $x (i32.add (local.get $x) (i32.const 100)))
    (local.get $x)
  )
  
  ;; Test 5: Multiple locals
  (func (export "test_05_multi") (param $x i32) (result i32)
    (local $a i32)
    (local $b i32)
    (local.set $a (i32.const 10))
    (local.set $b (i32.const 20))
    (i32.add (local.get $a) (local.get $b))
  )
  
  ;; Test 6: Sparse locals (declare 10, use only 2)
  (func (export "test_06_sparse") (param $x i32) (result i32)
    (local i32) (local i32) (local i32) (local i32) (local i32)
    (local i32) (local i32) (local i32) (local i32) (local i32)
    (local.set 5 (i32.const 99))
    (local.get 5)
  )
  
  ;; Test 7: Local with arithmetic
  (func (export "test_07_arith") (param $x i32) (result i32)
    (local $result i32)
    (local.set $result (i32.mul (local.get $x) (i32.const 2)))
    (local.set $result (i32.add (local.get $result) (i32.const 5)))
    (local.get $result)
  )
  
  ;; Test 8: Local in nested operations
  (func (export "test_08_nested") (param $x i32) (result i32)
    (local $temp i32)
    (local.set $temp (i32.const 3))
    (i32.add 
      (i32.mul (local.get $temp) (local.get $x))
      (local.get $temp)
    )
  )
  
  ;; Test 9: Swap using locals
  (func (export "test_09_swap") (param $a i32) (param $b i32) (result i32)
    (local $temp i32)
    (local.set $temp (local.get $a))
    (local.set $a (local.get $b))
    (local.set $b (local.get $temp))
    (local.get $a)
  )
  
  ;; Test 10: Complex - all operations
  (func (export "test_10_complex") (param $x i32) (result i32)
    (local $a i32)
    (local $b i32)
    (local.set $a (i32.const 5))
    (local.tee $b (i32.add (local.get $a) (local.get $x)))
  )
)