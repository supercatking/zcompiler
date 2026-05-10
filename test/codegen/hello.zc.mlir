module {
  zc.func @main() -> i32 {
    %0 = zc.constant 1 : i32
    %1 = zc.constant 2 : i32
    %2 = zc.constant 3 : i32
    %3 = zc.mul %1, %2 : i32
    %4 = zc.add %0, %3 : i32
    zc.return %4 : i32
  }
}
