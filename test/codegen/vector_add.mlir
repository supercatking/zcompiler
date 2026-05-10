module {
  func.func @vadd(%arg0: memref<?xi32>, %arg1: memref<?xi32>, %arg2: memref<?xi32>, %arg3: i32) -> i32 {
    %0 = arith.index_cast %arg3 : i32 to index
    %c0 = arith.constant 0 : index
    %c4 = arith.constant 4 : index
    scf.for %arg4 = %c0 to %0 step %c4 {
      %c0_i32_0 = arith.constant 0 : i32
      %1 = vector.transfer_read %arg0[%arg4], %c0_i32_0 : memref<?xi32>, vector<4xi32>
      %2 = vector.transfer_read %arg1[%arg4], %c0_i32_0 : memref<?xi32>, vector<4xi32>
      %3 = arith.addi %1, %2 : vector<4xi32>
      vector.transfer_write %3, %arg2[%arg4] : vector<4xi32>, memref<?xi32>
    }
    %c0_i32 = arith.constant 0 : i32
    return %c0_i32 : i32
  }
}

