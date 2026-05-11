module {
  func.func @masked_add_le(%arg0: memref<?xi32>, %arg1: memref<?xi32>, %arg2: memref<?xi32>, %arg3: memref<?xi32>, %arg4: memref<?xi32>, %arg5: memref<?xi32>, %arg6: i32) -> i32 {
    %0 = arith.index_cast %arg6 : i32 to index
    %c0 = arith.constant 0 : index
    %c4 = arith.constant 4 : index
    scf.for %arg7 = %c0 to %0 step %c4 {
      %c0_i32_0 = arith.constant 0 : i32
      %1 = arith.subi %0, %arg7 : index
      %2 = arith.minui %1, %c4 : index
      %3 = vector.create_mask %2 : vector<4xi1>
      %4 = vector.transfer_read %arg0[%arg7], %c0_i32_0, %3 : memref<?xi32>, vector<4xi32>
      %5 = vector.transfer_read %arg1[%arg7], %c0_i32_0, %3 : memref<?xi32>, vector<4xi32>
      %6 = arith.cmpi sle, %4, %5 : vector<4xi32>
      %7 = vector.transfer_read %arg2[%arg7], %c0_i32_0, %3 : memref<?xi32>, vector<4xi32>
      %8 = vector.transfer_read %arg3[%arg7], %c0_i32_0, %3 : memref<?xi32>, vector<4xi32>
      %9 = vector.transfer_read %arg4[%arg7], %c0_i32_0, %3 : memref<?xi32>, vector<4xi32>
      %10 = arith.addi %7, %8 : vector<4xi32>
      %11 = arith.select %6, %10, %9 : vector<4xi1>, vector<4xi32>
      vector.transfer_write %11, %arg5[%arg7], %3 : vector<4xi32>, memref<?xi32>
    }
    %c0_i32 = arith.constant 0 : i32
    return %c0_i32 : i32
  }
}
