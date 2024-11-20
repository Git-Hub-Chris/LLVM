# clang++ main.cpp -c -o
# #define MY_CONST const
# extern int FooVar;
# extern int BarVar;
# [[clang::noinline]]
# MY_CONST int fooMul(int a, int b) {
#   return a * b;
# }
# [[clang::noinline]]
# MY_CONST int barMul(int a, int b) {
#   return a * b;
# }
# [[clang::noinline]]
# MY_CONST int fooAdd(int a, int b) {
#   return a + b;
# }
# [[clang::noinline]]
# MY_CONST int barAdd(int a, int b) {
#   return a + b;
# }
# [[clang::noinline]]
# MY_CONST int helper1(MY_CONST int (*func)(int, int), int a, int b) {
#   if (func == barAdd)
#     return 1;
#   return func(a, b) - 4;
# }
# [[clang::noinline]]
# MY_CONST int helper2(MY_CONST int (*func)(int, int), MY_CONST int (*func2)(int, int), int a, int b) {
#   if (func == func2)
#     return 2;
#   return func(a, b) + func2(a, b);
# }
# MY_CONST static int (*MY_CONST funcGlobalBarAdd)(int, int) = barAdd;
# MY_CONST int (*MY_CONST funcGlobalBarMul)(int, int) = barMul;
# int main(int argc, char **argv) {
#   int temp = helper1(funcGlobalBarAdd, FooVar, BarVar) +
#              helper2(fooMul, funcGlobalBarMul, FooVar, BarVar)
#              fooAdd(FooVar, BarVar);
#   MY_PRINTF("val: %d", temp);
#   return temp;
# }
	.text
	.file	"main.cpp"
	.globl	_Z6fooMulii                     # -- Begin function _Z6fooMulii
	.p2align	4, 0x90
	.type	_Z6fooMulii,@function
_Z6fooMulii:                            # @_Z6fooMulii
	.cfi_startproc
# %bb.0:
	pushq	%rbp
	.cfi_def_cfa_offset 16
	.cfi_offset %rbp, -16
	movq	%rsp, %rbp
	.cfi_def_cfa_register %rbp
	movl	%edi, -4(%rbp)
	movl	%esi, -8(%rbp)
	movl	-4(%rbp), %eax
	imull	-8(%rbp), %eax
	popq	%rbp
	.cfi_def_cfa %rsp, 8
	retq
.Lfunc_end0:
	.size	_Z6fooMulii, .Lfunc_end0-_Z6fooMulii
	.cfi_endproc
                                        # -- End function
	.globl	_Z6barMulii                     # -- Begin function _Z6barMulii
	.p2align	4, 0x90
	.type	_Z6barMulii,@function
_Z6barMulii:                            # @_Z6barMulii
	.cfi_startproc
# %bb.0:
	pushq	%rbp
	.cfi_def_cfa_offset 16
	.cfi_offset %rbp, -16
	movq	%rsp, %rbp
	.cfi_def_cfa_register %rbp
	movl	%edi, -4(%rbp)
	movl	%esi, -8(%rbp)
	movl	-4(%rbp), %eax
	imull	-8(%rbp), %eax
	popq	%rbp
	.cfi_def_cfa %rsp, 8
	retq
.Lfunc_end1:
	.size	_Z6barMulii, .Lfunc_end1-_Z6barMulii
	.cfi_endproc
                                        # -- End function
	.globl	_Z6fooAddii                     # -- Begin function _Z6fooAddii
	.p2align	4, 0x90
	.type	_Z6fooAddii,@function
_Z6fooAddii:                            # @_Z6fooAddii
	.cfi_startproc
# %bb.0:
	pushq	%rbp
	.cfi_def_cfa_offset 16
	.cfi_offset %rbp, -16
	movq	%rsp, %rbp
	.cfi_def_cfa_register %rbp
	movl	%edi, -4(%rbp)
	movl	%esi, -8(%rbp)
	movl	-4(%rbp), %eax
	addl	-8(%rbp), %eax
	popq	%rbp
	.cfi_def_cfa %rsp, 8
	retq
.Lfunc_end2:
	.size	_Z6fooAddii, .Lfunc_end2-_Z6fooAddii
	.cfi_endproc
                                        # -- End function
	.globl	_Z6barAddii                     # -- Begin function _Z6barAddii
	.p2align	4, 0x90
	.type	_Z6barAddii,@function
_Z6barAddii:                            # @_Z6barAddii
	.cfi_startproc
# %bb.0:
	pushq	%rbp
	.cfi_def_cfa_offset 16
	.cfi_offset %rbp, -16
	movq	%rsp, %rbp
	.cfi_def_cfa_register %rbp
	movl	%edi, -4(%rbp)
	movl	%esi, -8(%rbp)
	movl	-4(%rbp), %eax
	addl	-8(%rbp), %eax
	popq	%rbp
	.cfi_def_cfa %rsp, 8
	retq
.Lfunc_end3:
	.size	_Z6barAddii, .Lfunc_end3-_Z6barAddii
	.cfi_endproc
                                        # -- End function
	.globl	_Z7helper1PFKiiiEii             # -- Begin function _Z7helper1PFKiiiEii
	.p2align	4, 0x90
	.type	_Z7helper1PFKiiiEii,@function
_Z7helper1PFKiiiEii:                    # @_Z7helper1PFKiiiEii
	.cfi_startproc
# %bb.0:
	pushq	%rbp
	.cfi_def_cfa_offset 16
	.cfi_offset %rbp, -16
	movq	%rsp, %rbp
	.cfi_def_cfa_register %rbp
	subq	$32, %rsp
	movq	%rdi, -16(%rbp)
	movl	%esi, -20(%rbp)
	movl	%edx, -24(%rbp)
	movabsq	$_Z6barAddii, %rax
	cmpq	%rax, -16(%rbp)
	jne	.LBB4_2
# %bb.1:
	movl	$1, -4(%rbp)
	jmp	.LBB4_3
.LBB4_2:
	movq	-16(%rbp), %rax
	movl	-20(%rbp), %edi
	movl	-24(%rbp), %esi
	callq	*%rax
	subl	$4, %eax
	movl	%eax, -4(%rbp)
.LBB4_3:
	movl	-4(%rbp), %eax
	addq	$32, %rsp
	popq	%rbp
	.cfi_def_cfa %rsp, 8
	retq
.Lfunc_end4:
	.size	_Z7helper1PFKiiiEii, .Lfunc_end4-_Z7helper1PFKiiiEii
	.cfi_endproc
                                        # -- End function
	.globl	_Z7helper2PFKiiiES1_ii          # -- Begin function _Z7helper2PFKiiiES1_ii
	.p2align	4, 0x90
	.type	_Z7helper2PFKiiiES1_ii,@function
_Z7helper2PFKiiiES1_ii:                 # @_Z7helper2PFKiiiES1_ii
	.cfi_startproc
# %bb.0:
	pushq	%rbp
	.cfi_def_cfa_offset 16
	.cfi_offset %rbp, -16
	movq	%rsp, %rbp
	.cfi_def_cfa_register %rbp
	subq	$48, %rsp
	movq	%rdi, -16(%rbp)
	movq	%rsi, -24(%rbp)
	movl	%edx, -28(%rbp)
	movl	%ecx, -32(%rbp)
	movq	-16(%rbp), %rax
	cmpq	-24(%rbp), %rax
	jne	.LBB5_2
# %bb.1:
	movl	$2, -4(%rbp)
	jmp	.LBB5_3
.LBB5_2:
	movq	-16(%rbp), %rax
	movl	-28(%rbp), %edi
	movl	-32(%rbp), %esi
	callq	*%rax
	movl	%eax, -36(%rbp)                 # 4-byte Spill
	movq	-24(%rbp), %rax
	movl	-28(%rbp), %edi
	movl	-32(%rbp), %esi
	callq	*%rax
	movl	%eax, %ecx
	movl	-36(%rbp), %eax                 # 4-byte Reload
	addl	%ecx, %eax
	movl	%eax, -4(%rbp)
.LBB5_3:
	movl	-4(%rbp), %eax
	addq	$48, %rsp
	popq	%rbp
	.cfi_def_cfa %rsp, 8
	retq
.Lfunc_end5:
	.size	_Z7helper2PFKiiiES1_ii, .Lfunc_end5-_Z7helper2PFKiiiES1_ii
	.cfi_endproc
                                        # -- End function
	.globl	main                            # -- Begin function main
	.p2align	4, 0x90
	.type	main,@function
main:                                   # @main
	.cfi_startproc
# %bb.0:
	pushq	%rbp
	.cfi_def_cfa_offset 16
	.cfi_offset %rbp, -16
	movq	%rsp, %rbp
	.cfi_def_cfa_register %rbp
	subq	$32, %rsp
	movl	$0, -4(%rbp)
	movl	%edi, -8(%rbp)
	movq	%rsi, -16(%rbp)
	movl	FooVar, %esi
	movl	BarVar, %edx
	movabsq	$_Z6barAddii, %rdi
	callq	_Z7helper1PFKiiiEii
	movl	%eax, -28(%rbp)                 # 4-byte Spill
	movl	FooVar, %edx
	movl	BarVar, %ecx
	movabsq	$_Z6fooMulii, %rdi
	movabsq	$_Z6barMulii, %rsi
	callq	_Z7helper2PFKiiiES1_ii
	movl	%eax, %ecx
	movl	-28(%rbp), %eax                 # 4-byte Reload
	addl	%ecx, %eax
	movl	%eax, -24(%rbp)                 # 4-byte Spill
	movl	FooVar, %edi
	movl	BarVar, %esi
	callq	_Z6fooAddii
	movl	%eax, %ecx
	movl	-24(%rbp), %eax                 # 4-byte Reload
	addl	%ecx, %eax
	movl	%eax, -20(%rbp)
	movl	-20(%rbp), %esi
	movabsq	$.L.str, %rdi
	movb	$0, %al
	callq	printf
	movl	-20(%rbp), %eax
	addq	$32, %rsp
	popq	%rbp
	.cfi_def_cfa %rsp, 8
	retq
.Lfunc_end6:
	.size	main, .Lfunc_end6-main
	.cfi_endproc
                                        # -- End function
	.type	.L.str,@object                  # @.str
	.section	.rodata.str1.1,"aMS",@progbits,1
.L.str:
	.asciz	"val: %d\n"
	.size	.L.str, 9

	.ident	"clang version 20.0.0git"
	.section	".note.GNU-stack","",@progbits
	.addrsig
	.addrsig_sym _Z6fooMulii
	.addrsig_sym _Z6barMulii
	.addrsig_sym _Z6fooAddii
	.addrsig_sym _Z6barAddii
	.addrsig_sym _Z7helper1PFKiiiEii
	.addrsig_sym _Z7helper2PFKiiiES1_ii
	.addrsig_sym printf
	.addrsig_sym FooVar
	.addrsig_sym BarVar
