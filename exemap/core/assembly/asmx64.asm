extern loaderEntry

global start


section .text$A
    start:
        push rsi
        mov rsi, rsp
        and rsp, 0FFFFFFFFFFFFFFF0h
        sub rsp, 020h
        call getLocation
    getLocation:
        pop rcx
        mov rdx, rsi
        call loaderEntry
        mov rsp, rsi
        pop rsi
        ret
		 
		
	

