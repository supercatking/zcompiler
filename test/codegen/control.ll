; ModuleID = 'zcompiler'
source_filename = "zcompiler"

define i32 @main() {
entry:
  %0 = icmp slt i32 1, 2
  br i1 %0, label %if.then.0, label %if.else.1
if.then.0:
  ret i32 7
if.else.1:
  ret i32 3
if.end.2:
  ret i32 0
}

