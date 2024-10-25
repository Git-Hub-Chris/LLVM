; -mbackchain option
; Inducing double register pressure.
; Test output of setjmp/longjmp with 20 global double variable
; sum(regsiter pressure).
; RUN: clang -o %t %s
; RUN: %t | FileCheck %s

; ModuleID = 'builtin-setjmp-spills-double-01.c'
source_filename = "builtin-setjmp-spills-double-01.c"
target datalayout = "E-m:e-i1:8:16-i8:8:16-i64:64-f128:64-v128:64-a:8:16-n32:64"
target triple = "s390x-unknown-linux-gnu"

@buf = dso_local global [10 x ptr] zeroinitializer, align 8
@t = dso_local local_unnamed_addr global double 0.000000e+00, align 8
@s = dso_local local_unnamed_addr global double 0.000000e+00, align 8
@r = dso_local local_unnamed_addr global double 0.000000e+00, align 8
@q = dso_local local_unnamed_addr global double 0.000000e+00, align 8
@p = dso_local local_unnamed_addr global double 0.000000e+00, align 8
@o = dso_local local_unnamed_addr global double 0.000000e+00, align 8
@n = dso_local local_unnamed_addr global double 0.000000e+00, align 8
@m = dso_local local_unnamed_addr global double 0.000000e+00, align 8
@l = dso_local local_unnamed_addr global double 0.000000e+00, align 8
@k = dso_local local_unnamed_addr global double 0.000000e+00, align 8
@j = dso_local local_unnamed_addr global double 0.000000e+00, align 8
@i = dso_local local_unnamed_addr global double 0.000000e+00, align 8
@h = dso_local local_unnamed_addr global double 0.000000e+00, align 8
@g = dso_local local_unnamed_addr global double 0.000000e+00, align 8
@f = dso_local local_unnamed_addr global double 0.000000e+00, align 8
@e = dso_local local_unnamed_addr global double 0.000000e+00, align 8
@d = dso_local local_unnamed_addr global double 0.000000e+00, align 8
@c = dso_local local_unnamed_addr global double 0.000000e+00, align 8
@b = dso_local local_unnamed_addr global double 0.000000e+00, align 8
@a = dso_local local_unnamed_addr global double 0.000000e+00, align 8
@.str.3 = private unnamed_addr constant [9 x i8] c"a = %lf\0A\00", align 2
@.str.4 = private unnamed_addr constant [9 x i8] c"b = %lf\0A\00", align 2
@.str.5 = private unnamed_addr constant [9 x i8] c"c = %lf\0A\00", align 2
@.str.6 = private unnamed_addr constant [9 x i8] c"d = %lf\0A\00", align 2
@.str.7 = private unnamed_addr constant [9 x i8] c"e = %lf\0A\00", align 2
@.str.8 = private unnamed_addr constant [9 x i8] c"f = %lf\0A\00", align 2
@.str.9 = private unnamed_addr constant [9 x i8] c"g = %lf\0A\00", align 2
@.str.10 = private unnamed_addr constant [9 x i8] c"h = %lf\0A\00", align 2
@.str.11 = private unnamed_addr constant [9 x i8] c"i = %lf\0A\00", align 2
@.str.12 = private unnamed_addr constant [9 x i8] c"j = %lf\0A\00", align 2
@.str.13 = private unnamed_addr constant [9 x i8] c"k = %lf\0A\00", align 2
@.str.14 = private unnamed_addr constant [9 x i8] c"l = %lf\0A\00", align 2
@.str.15 = private unnamed_addr constant [9 x i8] c"m = %lf\0A\00", align 2
@.str.16 = private unnamed_addr constant [9 x i8] c"n = %lf\0A\00", align 2
@.str.17 = private unnamed_addr constant [9 x i8] c"o = %lf\0A\00", align 2
@.str.18 = private unnamed_addr constant [9 x i8] c"p = %lf\0A\00", align 2
@.str.19 = private unnamed_addr constant [9 x i8] c"q = %lf\0A\00", align 2
@.str.20 = private unnamed_addr constant [9 x i8] c"r = %lf\0A\00", align 2
@.str.21 = private unnamed_addr constant [9 x i8] c"s = %lf\0A\00", align 2
@.str.22 = private unnamed_addr constant [9 x i8] c"t = %lf\0A\00", align 2
@.str.23 = private unnamed_addr constant [12 x i8] c"val is %lf\0A\00", align 2
@str = private unnamed_addr constant [8 x i8] c"In func\00", align 1
@str.24 = private unnamed_addr constant [41 x i8] c"Second time through, checking variables:\00", align 1
@str.25 = private unnamed_addr constant [40 x i8] c"First time through, all variables are 1\00", align 1

; Function Attrs: noinline noreturn nounwind
define dso_local void @func1() local_unnamed_addr #0 {
entry:
  %puts = tail call i32 @puts(ptr nonnull dereferenceable(1) @str)
  tail call void @llvm.eh.sjlj.longjmp(ptr nonnull @buf)
  unreachable
}

; Function Attrs: nofree nounwind
declare noundef signext i32 @printf(ptr nocapture noundef readonly, ...) local_unnamed_addr #1

; Function Attrs: noreturn nounwind
declare void @llvm.eh.sjlj.longjmp(ptr) #2

; Function Attrs: noinline nounwind
define dso_local double @func() local_unnamed_addr #3 {
entry:
; CHECK: First time through, all variables are 1
; CHECK: In func
; CHECK: Second time through, checking variables:
; CHECK: a = 1.000000
; CHECK: b = 1.000000
; CHECK: c = 1.000000
; CHECK: d = 1.000000
; CHECK: e = 1.000000
; CHECK: f = 1.000000
; CHECK: g = 1.000000
; CHECK: h = 1.000000
; CHECK: i = 1.000000
; CHECK: j = 1.000000
; CHECK: k = 1.000000
; CHECK: l = 1.000000
; CHECK: m = 1.000000
; CHECK: n = 1.000000
; CHECK: o = 1.000000
; CHECK: p = 1.000000
; CHECK: q = 1.000000
; CHECK: r = 1.000000
; CHECK: s = 1.000000
; CHECK: t = 1.000000

  %0 = tail call i32 @llvm.eh.sjlj.setjmp(ptr nonnull @buf)
  %cmp = icmp eq i32 %0, 0
  br i1 %cmp, label %if.then, label %if.else

if.then:                                          ; preds = %entry
  store double 1.000000e+00, ptr @t, align 8, !tbaa !4
  store double 1.000000e+00, ptr @s, align 8, !tbaa !4
  store double 1.000000e+00, ptr @r, align 8, !tbaa !4
  store double 1.000000e+00, ptr @q, align 8, !tbaa !4
  store double 1.000000e+00, ptr @p, align 8, !tbaa !4
  store double 1.000000e+00, ptr @o, align 8, !tbaa !4
  store double 1.000000e+00, ptr @n, align 8, !tbaa !4
  store double 1.000000e+00, ptr @m, align 8, !tbaa !4
  store double 1.000000e+00, ptr @l, align 8, !tbaa !4
  store double 1.000000e+00, ptr @k, align 8, !tbaa !4
  store double 1.000000e+00, ptr @j, align 8, !tbaa !4
  store double 1.000000e+00, ptr @i, align 8, !tbaa !4
  store double 1.000000e+00, ptr @h, align 8, !tbaa !4
  store double 1.000000e+00, ptr @g, align 8, !tbaa !4
  store double 1.000000e+00, ptr @f, align 8, !tbaa !4
  store double 1.000000e+00, ptr @e, align 8, !tbaa !4
  store double 1.000000e+00, ptr @d, align 8, !tbaa !4
  store double 1.000000e+00, ptr @c, align 8, !tbaa !4
  store double 1.000000e+00, ptr @b, align 8, !tbaa !4
  store double 1.000000e+00, ptr @a, align 8, !tbaa !4
  %puts40 = tail call i32 @puts(ptr nonnull dereferenceable(1) @str.25)
  tail call void @func1()
  unreachable

if.else:                                          ; preds = %entry
  %puts = tail call i32 @puts(ptr nonnull dereferenceable(1) @str.24)
  %1 = load double, ptr @a, align 8, !tbaa !4
  %call2 = tail call signext i32 (ptr, ...) @printf(ptr noundef nonnull dereferenceable(1) @.str.3, double noundef %1)
  %2 = load double, ptr @b, align 8, !tbaa !4
  %call3 = tail call signext i32 (ptr, ...) @printf(ptr noundef nonnull dereferenceable(1) @.str.4, double noundef %2)
  %3 = load double, ptr @c, align 8, !tbaa !4
  %call4 = tail call signext i32 (ptr, ...) @printf(ptr noundef nonnull dereferenceable(1) @.str.5, double noundef %3)
  %4 = load double, ptr @d, align 8, !tbaa !4
  %call5 = tail call signext i32 (ptr, ...) @printf(ptr noundef nonnull dereferenceable(1) @.str.6, double noundef %4)
  %5 = load double, ptr @e, align 8, !tbaa !4
  %call6 = tail call signext i32 (ptr, ...) @printf(ptr noundef nonnull dereferenceable(1) @.str.7, double noundef %5)
  %6 = load double, ptr @f, align 8, !tbaa !4
  %call7 = tail call signext i32 (ptr, ...) @printf(ptr noundef nonnull dereferenceable(1) @.str.8, double noundef %6)
  %7 = load double, ptr @g, align 8, !tbaa !4
  %call8 = tail call signext i32 (ptr, ...) @printf(ptr noundef nonnull dereferenceable(1) @.str.9, double noundef %7)
  %8 = load double, ptr @h, align 8, !tbaa !4
  %call9 = tail call signext i32 (ptr, ...) @printf(ptr noundef nonnull dereferenceable(1) @.str.10, double noundef %8)
  %9 = load double, ptr @i, align 8, !tbaa !4
  %call10 = tail call signext i32 (ptr, ...) @printf(ptr noundef nonnull dereferenceable(1) @.str.11, double noundef %9)
  %10 = load double, ptr @j, align 8, !tbaa !4
  %call11 = tail call signext i32 (ptr, ...) @printf(ptr noundef nonnull dereferenceable(1) @.str.12, double noundef %10)
  %11 = load double, ptr @k, align 8, !tbaa !4
  %call12 = tail call signext i32 (ptr, ...) @printf(ptr noundef nonnull dereferenceable(1) @.str.13, double noundef %11)
  %12 = load double, ptr @l, align 8, !tbaa !4
  %call13 = tail call signext i32 (ptr, ...) @printf(ptr noundef nonnull dereferenceable(1) @.str.14, double noundef %12)
  %13 = load double, ptr @m, align 8, !tbaa !4
  %call14 = tail call signext i32 (ptr, ...) @printf(ptr noundef nonnull dereferenceable(1) @.str.15, double noundef %13)
  %14 = load double, ptr @n, align 8, !tbaa !4
  %call15 = tail call signext i32 (ptr, ...) @printf(ptr noundef nonnull dereferenceable(1) @.str.16, double noundef %14)
  %15 = load double, ptr @o, align 8, !tbaa !4
  %call16 = tail call signext i32 (ptr, ...) @printf(ptr noundef nonnull dereferenceable(1) @.str.17, double noundef %15)
  %16 = load double, ptr @p, align 8, !tbaa !4
  %call17 = tail call signext i32 (ptr, ...) @printf(ptr noundef nonnull dereferenceable(1) @.str.18, double noundef %16)
  %17 = load double, ptr @q, align 8, !tbaa !4
  %call18 = tail call signext i32 (ptr, ...) @printf(ptr noundef nonnull dereferenceable(1) @.str.19, double noundef %17)
  %18 = load double, ptr @r, align 8, !tbaa !4
  %call19 = tail call signext i32 (ptr, ...) @printf(ptr noundef nonnull dereferenceable(1) @.str.20, double noundef %18)
  %19 = load double, ptr @s, align 8, !tbaa !4
  %call20 = tail call signext i32 (ptr, ...) @printf(ptr noundef nonnull dereferenceable(1) @.str.21, double noundef %19)
  %20 = load double, ptr @t, align 8, !tbaa !4
  %call21 = tail call signext i32 (ptr, ...) @printf(ptr noundef nonnull dereferenceable(1) @.str.22, double noundef %20)
  %21 = load double, ptr @a, align 8, !tbaa !4
  %22 = load double, ptr @b, align 8, !tbaa !4
  %add = fadd double %21, %22
  %23 = load double, ptr @c, align 8, !tbaa !4
  %add22 = fadd double %add, %23
  %24 = load double, ptr @d, align 8, !tbaa !4
  %add23 = fadd double %add22, %24
  %25 = load double, ptr @e, align 8, !tbaa !4
  %add24 = fadd double %add23, %25
  %26 = load double, ptr @f, align 8, !tbaa !4
  %add25 = fadd double %add24, %26
  %27 = load double, ptr @g, align 8, !tbaa !4
  %add26 = fadd double %add25, %27
  %28 = load double, ptr @h, align 8, !tbaa !4
  %add27 = fadd double %add26, %28
  %29 = load double, ptr @i, align 8, !tbaa !4
  %add28 = fadd double %add27, %29
  %30 = load double, ptr @j, align 8, !tbaa !4
  %add29 = fadd double %add28, %30
  %31 = load double, ptr @k, align 8, !tbaa !4
  %add30 = fadd double %add29, %31
  %32 = load double, ptr @l, align 8, !tbaa !4
  %add31 = fadd double %add30, %32
  %33 = load double, ptr @m, align 8, !tbaa !4
  %add32 = fadd double %add31, %33
  %34 = load double, ptr @n, align 8, !tbaa !4
  %add33 = fadd double %add32, %34
  %35 = load double, ptr @o, align 8, !tbaa !4
  %add34 = fadd double %add33, %35
  %36 = load double, ptr @p, align 8, !tbaa !4
  %add35 = fadd double %add34, %36
  %37 = load double, ptr @q, align 8, !tbaa !4
  %add36 = fadd double %add35, %37
  %38 = load double, ptr @r, align 8, !tbaa !4
  %add37 = fadd double %add36, %38
  %39 = load double, ptr @s, align 8, !tbaa !4
  %add38 = fadd double %add37, %39
  %40 = load double, ptr @t, align 8, !tbaa !4
  %add39 = fadd double %add38, %40
  ret double %add39
}

; Function Attrs: nounwind
declare i32 @llvm.eh.sjlj.setjmp(ptr) #4

; Function Attrs: nounwind
define dso_local noundef signext i32 @main() local_unnamed_addr #5 {
entry:
; CHECK: val is 20.000000
  %call = tail call double @func()
  %call1 = tail call signext i32 (ptr, ...) @printf(ptr noundef nonnull dereferenceable(1) @.str.23, double noundef %call)
  ret i32 0
}

; Function Attrs: nofree nounwind
declare noundef i32 @puts(ptr nocapture noundef readonly) local_unnamed_addr #6

attributes #0 = { noinline noreturn nounwind "no-trapping-math"="true" "stack-protector-buffer-size"="8" "target-cpu"="z10" }
attributes #1 = { nofree nounwind "no-trapping-math"="true" "stack-protector-buffer-size"="8" "target-cpu"="z10" }
attributes #2 = { noreturn nounwind }
attributes #3 = { noinline nounwind "no-trapping-math"="true" "stack-protector-buffer-size"="8" "target-cpu"="z10" }
attributes #4 = { nounwind }
attributes #5 = { nounwind "no-trapping-math"="true" "stack-protector-buffer-size"="8" "target-cpu"="z10" }
attributes #6 = { nofree nounwind }

!llvm.module.flags = !{!0, !1, !2}
!llvm.ident = !{!3}

!0 = !{i32 1, !"wchar_size", i32 4}
!1 = !{i32 8, !"PIC Level", i32 2}
!2 = !{i32 7, !"PIE Level", i32 2}
!3 = !{!"clang version 20.0.0git (https://github.com/llvm/llvm-project.git 79880371396d6e486bf6bacd6c4087ebdac591f8)"}
!4 = !{!5, !5, i64 0}
!5 = !{!"double", !6, i64 0}
!6 = !{!"omnipotent char", !7, i64 0}
!7 = !{!"Simple C/C++ TBAA"}
