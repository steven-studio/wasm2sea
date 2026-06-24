(module
  (type (;0;) (func))
  (type (;1;) (func (param i32 i32)))
  (type (;2;) (func (param i32 i32 i32 i32 i32 i32 i32 i32 i32) (result i32)))
  (type (;3;) (func (result i32)))
  (type (;4;) (func (param i32)))
  (func (;0;) (type 0)
    call 4)
  (func (;1;) (type 1) (param i32 i32)
    (local i32 i32 i32)
    global.get 0
    i32.const 32
    i32.sub
    local.set 2
    local.get 2
    local.get 0
    i32.store offset=28
    local.get 2
    local.get 1
    i32.store offset=24
    local.get 2
    i32.const 0
    i32.store offset=20
    block  ;; label = @1
      loop  ;; label = @2
        local.get 2
        i32.load offset=20
        local.get 2
        i32.load offset=24
        i32.const 1
        i32.sub
        i32.lt_s
        i32.const 1
        i32.and
        i32.eqz
        br_if 1 (;@1;)
        local.get 2
        i32.const 0
        i32.store offset=16
        block  ;; label = @3
          loop  ;; label = @4
            local.get 2
            i32.load offset=16
            local.get 2
            i32.load offset=24
            local.get 2
            i32.load offset=20
            i32.sub
            i32.const 1
            i32.sub
            i32.lt_s
            i32.const 1
            i32.and
            i32.eqz
            br_if 1 (;@3;)
            block  ;; label = @5
              local.get 2
              i32.load offset=28
              local.get 2
              i32.load offset=16
              i32.const 2
              i32.shl
              i32.add
              i32.load
              local.get 2
              i32.load offset=28
              local.get 2
              i32.load offset=16
              i32.const 1
              i32.add
              i32.const 2
              i32.shl
              i32.add
              i32.load
              i32.gt_s
              i32.const 1
              i32.and
              i32.eqz
              br_if 0 (;@5;)
              local.get 2
              local.get 2
              i32.load offset=28
              local.get 2
              i32.load offset=16
              i32.const 2
              i32.shl
              i32.add
              i32.load
              i32.store offset=12
              local.get 2
              i32.load offset=28
              local.get 2
              i32.load offset=16
              i32.const 1
              i32.add
              i32.const 2
              i32.shl
              i32.add
              i32.load
              local.set 3
              local.get 2
              i32.load offset=28
              local.get 2
              i32.load offset=16
              i32.const 2
              i32.shl
              i32.add
              local.get 3
              i32.store
              local.get 2
              i32.load offset=12
              local.set 4
              local.get 2
              i32.load offset=28
              local.get 2
              i32.load offset=16
              i32.const 1
              i32.add
              i32.const 2
              i32.shl
              i32.add
              local.get 4
              i32.store
            end
            local.get 2
            local.get 2
            i32.load offset=16
            i32.const 1
            i32.add
            i32.store offset=16
            br 0 (;@4;)
          end
        end
        local.get 2
        local.get 2
        i32.load offset=20
        i32.const 1
        i32.add
        i32.store offset=20
        br 0 (;@2;)
      end
    end
    return)
  (func (;2;) (type 2) (param i32 i32 i32 i32 i32 i32 i32 i32 i32) (result i32)
    (local i32 i32 i32)
    global.get 0
    i32.const 80
    i32.sub
    local.set 9
    local.get 9
    global.set 0
    local.get 9
    local.get 0
    i32.store offset=76
    local.get 9
    local.get 1
    i32.store offset=72
    local.get 9
    local.get 2
    i32.store offset=68
    local.get 9
    local.get 3
    i32.store offset=64
    local.get 9
    local.get 4
    i32.store offset=60
    local.get 9
    local.get 5
    i32.store offset=56
    local.get 9
    local.get 6
    i32.store offset=52
    local.get 9
    local.get 7
    i32.store offset=48
    local.get 9
    local.get 8
    i32.store offset=44
    local.get 9
    local.get 9
    i32.load offset=76
    i32.store
    local.get 9
    local.get 9
    i32.load offset=72
    i32.store offset=4
    local.get 9
    local.get 9
    i32.load offset=68
    i32.store offset=8
    local.get 9
    local.get 9
    i32.load offset=64
    i32.store offset=12
    local.get 9
    local.get 9
    i32.load offset=60
    i32.store offset=16
    local.get 9
    local.get 9
    i32.load offset=56
    i32.store offset=20
    local.get 9
    local.get 9
    i32.load offset=52
    i32.store offset=24
    local.get 9
    local.get 9
    i32.load offset=48
    i32.store offset=28
    local.get 9
    i32.const 8
    call 1
    local.get 9
    i32.load offset=44
    local.set 10
    local.get 9
    local.get 10
    i32.const 2
    i32.shl
    i32.add
    i32.load
    local.set 11
    local.get 9
    i32.const 80
    i32.add
    global.set 0
    local.get 11
    return)
  (func (;3;) (type 0)
    block  ;; label = @1
      i32.const 1
      i32.eqz
      br_if 0 (;@1;)
      call 0
    end)
  (func (;4;) (type 0)
    i32.const 65536
    global.set 2
    i32.const 0
    i32.const 15
    i32.add
    i32.const -16
    i32.and
    global.set 1)
  (func (;5;) (type 3) (result i32)
    global.get 0
    global.get 1
    i32.sub)
  (func (;6;) (type 3) (result i32)
    global.get 2)
  (func (;7;) (type 3) (result i32)
    global.get 1)
  (func (;8;) (type 4) (param i32)
    local.get 0
    global.set 0)
  (func (;9;) (type 3) (result i32)
    global.get 0)
  (table (;0;) 2 2 funcref)
  (memory (;0;) 257 257)
  (global (;0;) (mut i32) (i32.const 65536))
  (global (;1;) (mut i32) (i32.const 0))
  (global (;2;) (mut i32) (i32.const 0))
  (export "memory" (memory 0))
  (export "sort_and_get" (func 2))
  (export "_initialize" (func 3))
  (export "__indirect_function_table" (table 0))
  (export "emscripten_stack_init" (func 4))
  (export "emscripten_stack_get_free" (func 5))
  (export "emscripten_stack_get_base" (func 6))
  (export "emscripten_stack_get_end" (func 7))
  (export "_emscripten_stack_restore" (func 8))
  (export "emscripten_stack_get_current" (func 9))
  (elem (;0;) (i32.const 1) func 0))
