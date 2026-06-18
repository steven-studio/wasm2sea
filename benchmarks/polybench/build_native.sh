#!/bin/bash
# benchmarks/polybench/build_native.sh
# 用法：在 wasm2sea repo 根目錄執行
#   bash benchmarks/polybench/build_native.sh

POLY=benchmarks/polybench/original
OUT=benchmarks/polybench/native
mkdir -p $OUT

declare -A BENCHMARKS=(
  [correlation]="datamining/correlation"
  [covariance]="datamining/covariance"
  [gemm]="linear-algebra/blas/gemm"
  [gemver]="linear-algebra/blas/gemver"
  [gesummv]="linear-algebra/blas/gesummv"
  [symm]="linear-algebra/blas/symm"
  [syr2k]="linear-algebra/blas/syr2k"
  [syrk]="linear-algebra/blas/syrk"
  [trmm]="linear-algebra/blas/trmm"
  [2mm]="linear-algebra/kernels/2mm"
  [3mm]="linear-algebra/kernels/3mm"
  [atax]="linear-algebra/kernels/atax"
  [bicg]="linear-algebra/kernels/bicg"
  [doitgen]="linear-algebra/kernels/doitgen"
  [mvt]="linear-algebra/kernels/mvt"
  [cholesky]="linear-algebra/solvers/cholesky"
  [durbin]="linear-algebra/solvers/durbin"
  [gramschmidt]="linear-algebra/solvers/gramschmidt"
  [lu]="linear-algebra/solvers/lu"
  [ludcmp]="linear-algebra/solvers/ludcmp"
  [trisolv]="linear-algebra/solvers/trisolv"
  [adi]="stencils/adi"
  [fdtd-2d]="stencils/fdtd-2d"
  [heat-3d]="stencils/heat-3d"
  [jacobi-1d]="stencils/jacobi-1d"
  [jacobi-2d]="stencils/jacobi-2d"
  [seidel-2d]="stencils/seidel-2d"
  [deriche]="medley/deriche"
  [floyd-warshall]="medley/floyd-warshall"
  [nussinov]="medley/nussinov"
)

PASS=0
FAIL=0

for NAME in "${!BENCHMARKS[@]}"; do
  DIR=${BENCHMARKS[$NAME]}
  echo -n "Building $NAME ... "
  clang -O0 \
    -I $POLY/utilities \
    -I $POLY/$DIR \
    $POLY/$DIR/$NAME.c $POLY/utilities/polybench.c \
    -DPOLYBENCH_DUMP_ARRAYS \
    -lm \
    -o $OUT/$NAME 2>/dev/null \
    && { echo "OK"; ((PASS++)); } \
    || { echo "FAILED"; ((FAIL++)); }
done

echo ""
echo "Done: $PASS OK, $FAIL FAILED"
