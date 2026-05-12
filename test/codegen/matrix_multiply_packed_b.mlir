module {
  func.func @matmul_packed_b_i32(%arg0: memref<?xi32>, %arg1: memref<?xi32>, %arg2: memref<?xi32>, %arg3: i32, %arg4: i32, %arg5: i32) -> i32 {
    %0 = arith.index_cast %arg3 : i32 to index
    %1 = arith.index_cast %arg4 : i32 to index
    %2 = arith.index_cast %arg5 : i32 to index
    %c0 = arith.constant 0 : index
    %c1 = arith.constant 1 : index
    %c0_i32 = arith.constant 0 : i32
    scf.for %arg6 = %c0 to %0 step %c1 {
      scf.for %arg7 = %c0 to %1 step %c1 {
        %3 = scf.for %arg8 = %c0 to %2 step %c1 iter_args(%arg9 = %c0_i32) -> (i32) {
          %6 = arith.muli %arg6, %2 : index
          %7 = arith.addi %6, %arg8 : index
          %8 = arith.muli %arg7, %2 : index
          %9 = arith.addi %8, %arg8 : index
          %10 = memref.load %arg1[%7] : memref<?xi32>
          %11 = memref.load %arg2[%9] : memref<?xi32>
          %12 = arith.muli %10, %11 : i32
          %13 = arith.addi %arg9, %12 : i32
          scf.yield %13 : i32
        }
        %4 = arith.muli %arg6, %1 : index
        %5 = arith.addi %4, %arg7 : index
        memref.store %3, %arg0[%5] : memref<?xi32>
      }
    }
    %c0_i32_0 = arith.constant 0 : i32
    return %c0_i32_0 : i32
  }
}

