module {
  func.func @main() -> i32 {
    %0 = zc.constant 1 : i32
    %1 = zc.constant 2 : i32
    %2 = zc.add %0, %1 : i32
    return %2 : i32
  }
}

