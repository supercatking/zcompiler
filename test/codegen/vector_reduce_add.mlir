module {
  func.func @vreduce(%arg0: memref<?xi32>, %arg1: i32) -> i32 {
    %c0_i32 = arith.constant 0 : i32
    %0 = arith.index_cast %arg1 : i32 to index
    %c0 = arith.constant 0 : index
    %c4 = arith.constant 4 : index
    %1 = scf.for %arg2 = %c0 to %0 step %c4 iter_args(%arg3 = %c0_i32) -> (i32) {
      %c0_i32_0 = arith.constant 0 : i32
      %2 = arith.subi %0, %arg2 : index
      %3 = arith.minui %2, %c4 : index
      %4 = vector.create_mask %3 : vector<4xi1>
      %5 = vector.transfer_read %arg0[%arg2], %c0_i32_0, %4 : memref<?xi32>, vector<4xi32>
      %6 = vector.reduction <add>, %5, %arg3 : vector<4xi32> into i32
      scf.yield %6 : i32
    }
    return %1 : i32
  }
}

