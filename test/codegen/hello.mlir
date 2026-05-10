module {
  func.func @main() -> i32 {
    %c1_i32 = arith.constant 1 : i32
    %c2_i32 = arith.constant 2 : i32
    %c3_i32 = arith.constant 3 : i32
    %0 = arith.muli %c2_i32, %c3_i32 : i32
    %1 = arith.addi %c1_i32, %0 : i32
    return %1 : i32
  }
}
