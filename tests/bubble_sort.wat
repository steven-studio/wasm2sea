(module $bubble_sort.wasm
  (type (;0;) (func))
  (type (;1;) (func (param i32 i32)))
  (type (;2;) (func (param i32 i32 i32 i32 i32 i32 i32 i32 i32) (result i32)))
  (func $__wasm_call_ctors (type 0))
  (func $bubble_sort (type 1) (param i32 i32)
    (local i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32)
    global.get $__stack_pointer
    local.set 2
    i32.const 32
    local.set 3
    local.get 2
    local.get 3
    i32.sub
    local.set 4
    local.get 4
    local.get 0
    i32.store offset=28
    local.get 4
    local.get 1
    i32.store offset=24
    i32.const 0
    local.set 5
    local.get 4
    local.get 5
    i32.store offset=20
    block  ;; label = @1
      loop  ;; label = @2
        local.get 4
        i32.load offset=20
        local.set 6
        local.get 4
        i32.load offset=24
        local.set 7
        i32.const 1
        local.set 8
        local.get 7
        local.get 8
        i32.sub
        local.set 9
        local.get 6
        local.set 10
        local.get 9
        local.set 11
        local.get 10
        local.get 11
        i32.lt_s
        local.set 12
        i32.const 1
        local.set 13
        local.get 12
        local.get 13
        i32.and
        local.set 14
        local.get 14
        i32.eqz
        br_if 1 (;@1;)
        i32.const 0
        local.set 15
        local.get 4
        local.get 15
        i32.store offset=16
        block  ;; label = @3
          loop  ;; label = @4
            local.get 4
            i32.load offset=16
            local.set 16
            local.get 4
            i32.load offset=24
            local.set 17
            local.get 4
            i32.load offset=20
            local.set 18
            local.get 17
            local.get 18
            i32.sub
            local.set 19
            i32.const 1
            local.set 20
            local.get 19
            local.get 20
            i32.sub
            local.set 21
            local.get 16
            local.set 22
            local.get 21
            local.set 23
            local.get 22
            local.get 23
            i32.lt_s
            local.set 24
            i32.const 1
            local.set 25
            local.get 24
            local.get 25
            i32.and
            local.set 26
            local.get 26
            i32.eqz
            br_if 1 (;@3;)
            local.get 4
            i32.load offset=28
            local.set 27
            local.get 4
            i32.load offset=16
            local.set 28
            i32.const 2
            local.set 29
            local.get 28
            local.get 29
            i32.shl
            local.set 30
            local.get 27
            local.get 30
            i32.add
            local.set 31
            local.get 31
            i32.load
            local.set 32
            local.get 4
            i32.load offset=28
            local.set 33
            local.get 4
            i32.load offset=16
            local.set 34
            i32.const 1
            local.set 35
            local.get 34
            local.get 35
            i32.add
            local.set 36
            i32.const 2
            local.set 37
            local.get 36
            local.get 37
            i32.shl
            local.set 38
            local.get 33
            local.get 38
            i32.add
            local.set 39
            local.get 39
            i32.load
            local.set 40
            local.get 32
            local.set 41
            local.get 40
            local.set 42
            local.get 41
            local.get 42
            i32.gt_s
            local.set 43
            i32.const 1
            local.set 44
            local.get 43
            local.get 44
            i32.and
            local.set 45
            block  ;; label = @5
              local.get 45
              i32.eqz
              br_if 0 (;@5;)
              local.get 4
              i32.load offset=28
              local.set 46
              local.get 4
              i32.load offset=16
              local.set 47
              i32.const 2
              local.set 48
              local.get 47
              local.get 48
              i32.shl
              local.set 49
              local.get 46
              local.get 49
              i32.add
              local.set 50
              local.get 50
              i32.load
              local.set 51
              local.get 4
              local.get 51
              i32.store offset=12
              local.get 4
              i32.load offset=28
              local.set 52
              local.get 4
              i32.load offset=16
              local.set 53
              i32.const 1
              local.set 54
              local.get 53
              local.get 54
              i32.add
              local.set 55
              i32.const 2
              local.set 56
              local.get 55
              local.get 56
              i32.shl
              local.set 57
              local.get 52
              local.get 57
              i32.add
              local.set 58
              local.get 58
              i32.load
              local.set 59
              local.get 4
              i32.load offset=28
              local.set 60
              local.get 4
              i32.load offset=16
              local.set 61
              i32.const 2
              local.set 62
              local.get 61
              local.get 62
              i32.shl
              local.set 63
              local.get 60
              local.get 63
              i32.add
              local.set 64
              local.get 64
              local.get 59
              i32.store
              local.get 4
              i32.load offset=12
              local.set 65
              local.get 4
              i32.load offset=28
              local.set 66
              local.get 4
              i32.load offset=16
              local.set 67
              i32.const 1
              local.set 68
              local.get 67
              local.get 68
              i32.add
              local.set 69
              i32.const 2
              local.set 70
              local.get 69
              local.get 70
              i32.shl
              local.set 71
              local.get 66
              local.get 71
              i32.add
              local.set 72
              local.get 72
              local.get 65
              i32.store
            end
            local.get 4
            i32.load offset=16
            local.set 73
            i32.const 1
            local.set 74
            local.get 73
            local.get 74
            i32.add
            local.set 75
            local.get 4
            local.get 75
            i32.store offset=16
            br 0 (;@4;)
          end
        end
        local.get 4
        i32.load offset=20
        local.set 76
        i32.const 1
        local.set 77
        local.get 76
        local.get 77
        i32.add
        local.set 78
        local.get 4
        local.get 78
        i32.store offset=20
        br 0 (;@2;)
      end
    end
    return)
  (func $sort_and_get (type 2) (param i32 i32 i32 i32 i32 i32 i32 i32 i32) (result i32)
    (local i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32)
    global.get $__stack_pointer
    local.set 9
    i32.const 80
    local.set 10
    local.get 9
    local.get 10
    i32.sub
    local.set 11
    local.get 11
    global.set $__stack_pointer
    local.get 11
    local.get 0
    i32.store offset=76
    local.get 11
    local.get 1
    i32.store offset=72
    local.get 11
    local.get 2
    i32.store offset=68
    local.get 11
    local.get 3
    i32.store offset=64
    local.get 11
    local.get 4
    i32.store offset=60
    local.get 11
    local.get 5
    i32.store offset=56
    local.get 11
    local.get 6
    i32.store offset=52
    local.get 11
    local.get 7
    i32.store offset=48
    local.get 11
    local.get 8
    i32.store offset=44
    local.get 11
    i32.load offset=76
    local.set 12
    local.get 11
    local.get 12
    i32.store
    local.get 11
    i32.load offset=72
    local.set 13
    local.get 11
    local.get 13
    i32.store offset=4
    local.get 11
    i32.load offset=68
    local.set 14
    local.get 11
    local.get 14
    i32.store offset=8
    local.get 11
    i32.load offset=64
    local.set 15
    local.get 11
    local.get 15
    i32.store offset=12
    local.get 11
    i32.load offset=60
    local.set 16
    local.get 11
    local.get 16
    i32.store offset=16
    local.get 11
    i32.load offset=56
    local.set 17
    local.get 11
    local.get 17
    i32.store offset=20
    local.get 11
    i32.load offset=52
    local.set 18
    local.get 11
    local.get 18
    i32.store offset=24
    local.get 11
    i32.load offset=48
    local.set 19
    local.get 11
    local.get 19
    i32.store offset=28
    local.get 11
    local.set 20
    i32.const 8
    local.set 21
    local.get 20
    local.get 21
    call $bubble_sort
    local.get 11
    i32.load offset=44
    local.set 22
    local.get 11
    local.set 23
    i32.const 2
    local.set 24
    local.get 22
    local.get 24
    i32.shl
    local.set 25
    local.get 23
    local.get 25
    i32.add
    local.set 26
    local.get 26
    i32.load
    local.set 27
    i32.const 80
    local.set 28
    local.get 11
    local.get 28
    i32.add
    local.set 29
    local.get 29
    global.set $__stack_pointer
    local.get 27
    return)
  (memory (;0;) 2)
  (global $__stack_pointer (mut i32) (i32.const 66560))
  (global (;1;) i32 (i32.const 1024))
  (global (;2;) i32 (i32.const 1024))
  (global (;3;) i32 (i32.const 1024))
  (global (;4;) i32 (i32.const 66560))
  (global (;5;) i32 (i32.const 1024))
  (global (;6;) i32 (i32.const 66560))
  (global (;7;) i32 (i32.const 131072))
  (global (;8;) i32 (i32.const 0))
  (global (;9;) i32 (i32.const 1))
  (export "memory" (memory 0))
  (export "__wasm_call_ctors" (func $__wasm_call_ctors))
  (export "bubble_sort" (func $bubble_sort))
  (export "sort_and_get" (func $sort_and_get))
  (export "__dso_handle" (global 1))
  (export "__data_end" (global 2))
  (export "__stack_low" (global 3))
  (export "__stack_high" (global 4))
  (export "__global_base" (global 5))
  (export "__heap_base" (global 6))
  (export "__heap_end" (global 7))
  (export "__memory_base" (global 8))
  (export "__table_base" (global 9)))
