(module $jacobi1d_kernel.wasm
  (type (;0;) (func (param i32 i32 i32 i32)))
  (func $kernel_jacobi_1d (type 0) (param i32 i32 i32 i32)
    (local i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 f64 i32 i32 i32 i32 i32 f64 f64 i32 i32 i32 i32 i32 i32 i32 f64 f64 f64 f64 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 f64 i32 i32 i32 i32 i32 f64 f64 i32 i32 i32 i32 i32 i32 i32 f64 f64 f64 f64 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32)
    global.get $__stack_pointer
    local.set 4
    i32.const 32
    local.set 5
    local.get 4
    local.get 5
    i32.sub
    local.set 6
    local.get 6
    local.get 0
    i32.store offset=28
    local.get 6
    local.get 1
    i32.store offset=24
    local.get 6
    local.get 2
    i32.store offset=20
    local.get 6
    local.get 3
    i32.store offset=16
    i32.const 0
    local.set 7
    local.get 6
    local.get 7
    i32.store offset=12
    block  ;; label = @1
      loop  ;; label = @2
        local.get 6
        i32.load offset=12
        local.set 8
        local.get 6
        i32.load offset=28
        local.set 9
        local.get 8
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
        i32.const 1
        local.set 15
        local.get 6
        local.get 15
        i32.store offset=8
        block  ;; label = @3
          loop  ;; label = @4
            local.get 6
            i32.load offset=8
            local.set 16
            local.get 6
            i32.load offset=24
            local.set 17
            i32.const 1
            local.set 18
            local.get 17
            local.get 18
            i32.sub
            local.set 19
            local.get 16
            local.set 20
            local.get 19
            local.set 21
            local.get 20
            local.get 21
            i32.lt_s
            local.set 22
            i32.const 1
            local.set 23
            local.get 22
            local.get 23
            i32.and
            local.set 24
            local.get 24
            i32.eqz
            br_if 1 (;@3;)
            local.get 6
            i32.load offset=20
            local.set 25
            local.get 6
            i32.load offset=8
            local.set 26
            i32.const 1
            local.set 27
            local.get 26
            local.get 27
            i32.sub
            local.set 28
            i32.const 3
            local.set 29
            local.get 28
            local.get 29
            i32.shl
            local.set 30
            local.get 25
            local.get 30
            i32.add
            local.set 31
            local.get 31
            f64.load
            local.set 32
            local.get 6
            i32.load offset=20
            local.set 33
            local.get 6
            i32.load offset=8
            local.set 34
            i32.const 3
            local.set 35
            local.get 34
            local.get 35
            i32.shl
            local.set 36
            local.get 33
            local.get 36
            i32.add
            local.set 37
            local.get 37
            f64.load
            local.set 38
            local.get 32
            local.get 38
            f64.add
            local.set 39
            local.get 6
            i32.load offset=20
            local.set 40
            local.get 6
            i32.load offset=8
            local.set 41
            i32.const 1
            local.set 42
            local.get 41
            local.get 42
            i32.add
            local.set 43
            i32.const 3
            local.set 44
            local.get 43
            local.get 44
            i32.shl
            local.set 45
            local.get 40
            local.get 45
            i32.add
            local.set 46
            local.get 46
            f64.load
            local.set 47
            local.get 39
            local.get 47
            f64.add
            local.set 48
            f64.const 0x1.555475a31a4bep-2 (;=0.33333;)
            local.set 49
            local.get 49
            local.get 48
            f64.mul
            local.set 50
            local.get 6
            i32.load offset=16
            local.set 51
            local.get 6
            i32.load offset=8
            local.set 52
            i32.const 3
            local.set 53
            local.get 52
            local.get 53
            i32.shl
            local.set 54
            local.get 51
            local.get 54
            i32.add
            local.set 55
            local.get 55
            local.get 50
            f64.store
            local.get 6
            i32.load offset=8
            local.set 56
            i32.const 1
            local.set 57
            local.get 56
            local.get 57
            i32.add
            local.set 58
            local.get 6
            local.get 58
            i32.store offset=8
            br 0 (;@4;)
          end
        end
        i32.const 1
        local.set 59
        local.get 6
        local.get 59
        i32.store offset=4
        block  ;; label = @3
          loop  ;; label = @4
            local.get 6
            i32.load offset=4
            local.set 60
            local.get 6
            i32.load offset=24
            local.set 61
            i32.const 1
            local.set 62
            local.get 61
            local.get 62
            i32.sub
            local.set 63
            local.get 60
            local.set 64
            local.get 63
            local.set 65
            local.get 64
            local.get 65
            i32.lt_s
            local.set 66
            i32.const 1
            local.set 67
            local.get 66
            local.get 67
            i32.and
            local.set 68
            local.get 68
            i32.eqz
            br_if 1 (;@3;)
            local.get 6
            i32.load offset=16
            local.set 69
            local.get 6
            i32.load offset=4
            local.set 70
            i32.const 1
            local.set 71
            local.get 70
            local.get 71
            i32.sub
            local.set 72
            i32.const 3
            local.set 73
            local.get 72
            local.get 73
            i32.shl
            local.set 74
            local.get 69
            local.get 74
            i32.add
            local.set 75
            local.get 75
            f64.load
            local.set 76
            local.get 6
            i32.load offset=16
            local.set 77
            local.get 6
            i32.load offset=4
            local.set 78
            i32.const 3
            local.set 79
            local.get 78
            local.get 79
            i32.shl
            local.set 80
            local.get 77
            local.get 80
            i32.add
            local.set 81
            local.get 81
            f64.load
            local.set 82
            local.get 76
            local.get 82
            f64.add
            local.set 83
            local.get 6
            i32.load offset=16
            local.set 84
            local.get 6
            i32.load offset=4
            local.set 85
            i32.const 1
            local.set 86
            local.get 85
            local.get 86
            i32.add
            local.set 87
            i32.const 3
            local.set 88
            local.get 87
            local.get 88
            i32.shl
            local.set 89
            local.get 84
            local.get 89
            i32.add
            local.set 90
            local.get 90
            f64.load
            local.set 91
            local.get 83
            local.get 91
            f64.add
            local.set 92
            f64.const 0x1.555475a31a4bep-2 (;=0.33333;)
            local.set 93
            local.get 93
            local.get 92
            f64.mul
            local.set 94
            local.get 6
            i32.load offset=20
            local.set 95
            local.get 6
            i32.load offset=4
            local.set 96
            i32.const 3
            local.set 97
            local.get 96
            local.get 97
            i32.shl
            local.set 98
            local.get 95
            local.get 98
            i32.add
            local.set 99
            local.get 99
            local.get 94
            f64.store
            local.get 6
            i32.load offset=4
            local.set 100
            i32.const 1
            local.set 101
            local.get 100
            local.get 101
            i32.add
            local.set 102
            local.get 6
            local.get 102
            i32.store offset=4
            br 0 (;@4;)
          end
        end
        local.get 6
        i32.load offset=12
        local.set 103
        i32.const 1
        local.set 104
        local.get 103
        local.get 104
        i32.add
        local.set 105
        local.get 6
        local.get 105
        i32.store offset=12
        br 0 (;@2;)
      end
    end
    return)
  (memory (;0;) 2)
  (global $__stack_pointer (mut i32) (i32.const 66560))
  (export "memory" (memory 0))
  (export "kernel_jacobi_1d" (func $kernel_jacobi_1d)))
