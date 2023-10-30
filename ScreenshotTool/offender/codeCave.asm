


.code
ALIGN 16

extern __imp_BitBlt:qword
EXTERN sendBitmap: PROC


codeCave PROC
	xor edx,edx
	mov rcx,qword ptr [rbp + 28h]
	mov r15, [rsp] ; save return address to unused register
	add rsp, 8h ; restore stack state to what it was before the call to the code cave
	call __imp_bitblt
	push rcx
	push rdx
	push r8
	push r9
	sub rsp, 20h
	mov rcx, [rbp + 88h]
	call sendBitmap
	add rsp, 20h
	pop r9
	pop r8
	pop rdx
	pop rcx
	push r15 ; push return address back to the stack
	mov r15, 0h ; restore r15 state
	ret
codeCave ENDP


END