module {
  func.func @vcopy(%arg0: memref<?xi32>, %arg1: memref<?xi32>, %arg2: i32) -> i32 {
    %0 = arith.index_cast %arg2 : i32 to index
    %c0 = arith.constant 0 : index
    %c4 = arith.constant 4 : index
    scf.for %arg3 = %c0 to %0 step %c4 {
      %c0_i32_0 = arith.constant 0 : i32
      %1 = arith.subi %0, %arg3 : index
      %2 = arith.minui %1, %c4 : index
      %3 = vector.create_mask %2 : vector<4xi1>
      %4 = vector.transfer_read %arg0[%arg3], %c0_i32_0, %3 : memref<?xi32>, vector<4xi32>
      vector.transfer_write %4, %arg1[%arg3], %3 : vector<4xi32>, memref<?xi32>
    }
    %c0_i32 = arith.constant 0 : i32
    return %c0_i32 : i32
  }
}
