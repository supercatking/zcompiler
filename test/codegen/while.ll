; ModuleID = 'zcompiler'
source_filename = "zcompiler"

define i32 @main() {
entry:
  br label %while.cond.0
while.cond.0:
  %0 = icmp ne i32 0, 0
  br i1 %0, label %while.body.1, label %while.end.2
while.body.1:
  ret i32 1
while.end.2:
  ret i32 0
}

