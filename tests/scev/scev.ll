; ModuleID = 'scev.c'
source_filename = "scev.c"
target datalayout = "e-m:e-p270:32:32-p271:32:32-p272:64:64-i64:64-f80:128-n8:16:32:64-S128"
target triple = "x86_64-unknown-linux-gnu"

@Z = dso_local global i32 5, align 4
@.str = private unnamed_addr constant [3 x i8] c"%d\00", align 1

; Function Attrs: noinline nounwind uwtable
define dso_local i32 @main() #0 {
entry:
  %retval = alloca i32, align 4
  %C = alloca i32, align 4
  %A = alloca [10 x i32], align 16
  %N = alloca i32, align 4
  %B = alloca [12 x i32], align 16
  %I = alloca i32, align 4
  %i = alloca i32, align 4
  store i32 0, i32* %retval, align 4
  store i32 10, i32* %C, align 4
  store i32 5, i32* %N, align 4
  store i32 0, i32* %I, align 4
  br label %for.cond

for.cond:                                         ; preds = %for.inc, %entry
  %0 = load i32, i32* %I, align 4
  %1 = load i32, i32* %N, align 4
  %cmp = icmp slt i32 %0, %1
  br i1 %cmp, label %for.body, label %for.end

for.body:                                         ; preds = %for.cond
  %2 = load i32, i32* %C, align 4
  %3 = load i32, i32* %I, align 4
  %add = add nsw i32 %2, %3
  %4 = load i32, i32* %I, align 4
  %idxprom = sext i32 %4 to i64
  %arrayidx = getelementptr inbounds [10 x i32], [10 x i32]* %A, i64 0, i64 %idxprom
  store i32 %add, i32* %arrayidx, align 4
  %5 = load i32, i32* %C, align 4
  %6 = load i32, i32* %I, align 4
  %add1 = add nsw i32 %5, %6
  %add2 = add nsw i32 %add1, 1
  %7 = load i32, i32* %I, align 4
  %add3 = add nsw i32 %7, 1
  %idxprom4 = sext i32 %add3 to i64
  %arrayidx5 = getelementptr inbounds [10 x i32], [10 x i32]* %A, i64 0, i64 %idxprom4
  store i32 %add2, i32* %arrayidx5, align 4
  %8 = load i32, i32* %C, align 4
  %9 = load i32, i32* %I, align 4
  %add6 = add nsw i32 %8, %9
  %add7 = add nsw i32 %add6, 2
  %10 = load i32, i32* %I, align 4
  %add8 = add nsw i32 %10, 2
  %idxprom9 = sext i32 %add8 to i64
  %arrayidx10 = getelementptr inbounds [10 x i32], [10 x i32]* %A, i64 0, i64 %idxprom9
  store i32 %add7, i32* %arrayidx10, align 4
  %11 = load i32, i32* %C, align 4
  %12 = load i32, i32* %I, align 4
  %add11 = add nsw i32 %11, %12
  %add12 = add nsw i32 %add11, 3
  %13 = load i32, i32* %I, align 4
  %add13 = add nsw i32 %13, 3
  %idxprom14 = sext i32 %add13 to i64
  %arrayidx15 = getelementptr inbounds [10 x i32], [10 x i32]* %A, i64 0, i64 %idxprom14
  store i32 %add12, i32* %arrayidx15, align 4
  br label %for.inc

for.inc:                                          ; preds = %for.body
  %14 = load i32, i32* %I, align 4
  %add16 = add nsw i32 %14, 4
  store i32 %add16, i32* %I, align 4
  br label %for.cond, !llvm.loop !4

for.end:                                          ; preds = %for.cond
  store i32 0, i32* %i, align 4
  br label %for.cond17

for.cond17:                                       ; preds = %for.inc22, %for.end
  %15 = load i32, i32* %i, align 4
  %16 = load i32, i32* %N, align 4
  %cmp18 = icmp slt i32 %15, %16
  br i1 %cmp18, label %for.body19, label %for.end23

for.body19:                                       ; preds = %for.cond17
  %17 = load i32, i32* %i, align 4
  %idxprom20 = sext i32 %17 to i64
  %arrayidx21 = getelementptr inbounds [10 x i32], [10 x i32]* %A, i64 0, i64 %idxprom20
  %18 = load i32, i32* %arrayidx21, align 4
  %call = call i32 (i8*, ...) @printf(i8* noundef getelementptr inbounds ([3 x i8], [3 x i8]* @.str, i64 0, i64 0), i32 noundef %18)
  br label %for.inc22

for.inc22:                                        ; preds = %for.body19
  %19 = load i32, i32* %i, align 4
  %inc = add nsw i32 %19, 1
  store i32 %inc, i32* %i, align 4
  br label %for.cond17, !llvm.loop !6

for.end23:                                        ; preds = %for.cond17
  ret i32 0
}

declare dso_local i32 @printf(i8* noundef, ...) #1

attributes #0 = { noinline nounwind uwtable "frame-pointer"="all" "min-legal-vector-width"="0" "no-trapping-math"="true" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+cx8,+fxsr,+mmx,+sse,+sse2,+x87" "tune-cpu"="generic" }
attributes #1 = { "frame-pointer"="all" "no-trapping-math"="true" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+cx8,+fxsr,+mmx,+sse,+sse2,+x87" "tune-cpu"="generic" }

!llvm.module.flags = !{!0, !1, !2}
!llvm.ident = !{!3}

!0 = !{i32 1, !"wchar_size", i32 4}
!1 = !{i32 7, !"uwtable", i32 1}
!2 = !{i32 7, !"frame-pointer", i32 2}
!3 = !{!"clang version 14.0.6 (https://github.com/shravankumar0811/llvm-project.git 973cbf86551cd15a9cb85bbb0ad01f2406ccb62d)"}
!4 = distinct !{!4, !5}
!5 = !{!"llvm.loop.mustprogress"}
!6 = distinct !{!6, !5}
