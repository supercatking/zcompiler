; ModuleID = 'zcompiler'
source_filename = "zcompiler"

define i32 @main() {
entry:
  %0 = mul i32 2, 3
  %1 = add i32 1, %0
  ret i32 %1
}

