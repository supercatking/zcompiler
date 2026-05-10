module {
  func.func @add(%arg0: i32, %arg1: i32) -> i32 {
    %0 = arith.addi %arg0, %arg1 : i32
    return %0 : i32
  }
  func.func @main() -> i32 {
    %c2_i32 = arith.constant 2 : i32
    %c3_i32 = arith.constant 3 : i32
    %0 = call @add(%c2_i32, %c3_i32) : (i32, i32) -> i32
    %c4_i32 = arith.constant 4 : i32
    %1 = arith.addi %0, %c4_i32 : i32
    return %1 : i32
  }
}

