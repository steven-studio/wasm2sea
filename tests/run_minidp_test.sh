#!/bin/bash
NAME=$1
KERNEL=$2
clang --target=wasm32 -O0 -nostdlib -Wl,--no-entry -Wl,--export=$KERNEL \
  -o /tmp/${NAME}.wasm ~/wasm2sea/tests/${NAME}.c
cd /tmp
rm -f /tmp/${NAME}.ir
$HOME/wasm2sea/build/wasm2sea /tmp/${NAME}.wasm --save-ir out.ir > /dev/null 2>&1
$HOME/wasm2sea/third_party/dstogov-ir/ir /tmp/${NAME}.ir --emit-c /tmp/${NAME}_out.c
sed -i "s/$KERNEL/test/g" /tmp/${NAME}_out.c
cc -O2 -include stdint.h -include stdbool.h -include math.h /tmp/${NAME}_out.c ~/wasm2sea/tests/${NAME}_harness.c -o /tmp/${NAME}_bin -lm
/tmp/${NAME}_bin
