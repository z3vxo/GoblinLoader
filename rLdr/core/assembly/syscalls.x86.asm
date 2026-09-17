global do_syscall

section .text$B
	do_syscall:
		mov eax, [esp+44]
		mov edx, 0x7FFE0300
		call dword [edx]
		ret 48
