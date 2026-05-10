module {
  func.func @add_at(%arg0: memref<?xi32>, %arg1: memref<?xi32>, %arg2: memref<?xi32>, %arg3: i32) -> i32 {
    %0 = arith.index_cast %arg3 : i32 to index
    %1 = memref.load %arg0[%0] : memref<?xi32>
    %2 = arith.index_cast %arg3 : i32 to index
    %3 = memref.load %arg1[%2] : memref<?xi32>
    %4 = arith.addi %1, %3 : i32
    %5 = arith.index_cast %arg3 : i32 to index
    memref.store %4, %arg2[%5] : memref<?xi32>
    return %4 : i32
  }
}

