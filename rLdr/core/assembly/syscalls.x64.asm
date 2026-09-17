global do_syscall

section .text$B
	do_syscall:
		mov r10, rcx
		mov eax, [rsp+0x58]
		jmp qword [rsp+0x60]
