














































































	.text
	.align	5, 0x90
	.globl	___gmpn_mod_34lsub1
	
	
___gmpn_mod_34lsub1:

	

	mov	$0x0000FFFFFFFFFFFF, %r11

	mov	(%rdi), %rax

	cmp	$2, %rsi
	ja	Lgt2

	jb	Lone

	mov	8(%rdi), %rsi
	mov	%rax, %rdx
	shr	$48, %rax		

	and	%r11, %rdx		
	add	%rdx, %rax
	mov	%esi, %edx

	shr	$32, %rsi		
	add	%rsi, %rax

	shl	$16, %rdx		
	add	%rdx, %rax
Lone:	
	ret





Lgt2:	mov	8(%rdi), %rcx
	mov	16(%rdi), %rdx
	xor	%r9, %r9
	add	$24, %rdi
	sub	$12, %rsi
	jc	Lend
	.align	4, 0x90
Ltop:
	add	(%rdi), %rax
	adc	8(%rdi), %rcx
	adc	16(%rdi), %rdx
	adc	$0, %r9
	add	24(%rdi), %rax
	adc	32(%rdi), %rcx
	adc	40(%rdi), %rdx
	adc	$0, %r9
	add	48(%rdi), %rax
	adc	56(%rdi), %rcx
	adc	64(%rdi), %rdx
	adc	$0, %r9
	add	$72, %rdi
	sub	$9, %rsi
	jnc	Ltop

Lend:
	lea	Ltab(%rip), %r8
	movslq	36(%r8,%rsi,4), %r10
	add	%r10, %r8
	jmp	*%r8

	.text
	.align	3, 0x90
Ltab:	.set	L0_tmp, L0-Ltab
	.long	L0_tmp

	.set	L1_tmp, L1-Ltab
	.long	L1_tmp

	.set	L2_tmp, L2-Ltab
	.long	L2_tmp

	.set	L3_tmp, L3-Ltab
	.long	L3_tmp

	.set	L4_tmp, L4-Ltab
	.long	L4_tmp

	.set	L5_tmp, L5-Ltab
	.long	L5_tmp

	.set	L6_tmp, L6-Ltab
	.long	L6_tmp

	.set	L7_tmp, L7-Ltab
	.long	L7_tmp

	.set	L8_tmp, L8-Ltab
	.long	L8_tmp

	.text

L6:	add	(%rdi), %rax
	adc	8(%rdi), %rcx
	adc	16(%rdi), %rdx
	adc	$0, %r9
	add	$24, %rdi
L3:	add	(%rdi), %rax
	adc	8(%rdi), %rcx
	adc	16(%rdi), %rdx
	jmp	Lcj1

L7:	add	(%rdi), %rax
	adc	8(%rdi), %rcx
	adc	16(%rdi), %rdx
	adc	$0, %r9
	add	$24, %rdi
L4:	add	(%rdi), %rax
	adc	8(%rdi), %rcx
	adc	16(%rdi), %rdx
	adc	$0, %r9
	add	$24, %rdi
L1:	add	(%rdi), %rax
	adc	$0, %rcx
	jmp	Lcj2

L8:	add	(%rdi), %rax
	adc	8(%rdi), %rcx
	adc	16(%rdi), %rdx
	adc	$0, %r9
	add	$24, %rdi
L5:	add	(%rdi), %rax
	adc	8(%rdi), %rcx
	adc	16(%rdi), %rdx
	adc	$0, %r9
	add	$24, %rdi
L2:	add	(%rdi), %rax
	adc	8(%rdi), %rcx

Lcj2:	adc	$0, %rdx
Lcj1:	adc	$0, %r9
L0:	add	%r9, %rax
	adc	$0, %rcx
	adc	$0, %rdx
	adc	$0, %rax

	mov	%rax, %rdi		
	shr	$48, %rax		

	and	%r11, %rdi		
	mov	%ecx, %r10d	

	shr	$32, %rcx		

	add	%rdi, %rax		
	movzwl	%dx, %edi		
	shl	$16, %r10		

	add	%rcx, %rax		
	shr	$16, %rdx		

	add	%r10, %rax		
	shl	$32, %rdi		

	add	%rdx, %rax		
	add	%rdi, %rax		

	
	ret
	
