section .text
	global_start ;must be declared for linker

_start:
	mov edx,len ;the lenght of the message
	mov ecx,msg ;message to write
	mov ebx,1   ;file descriptor for stdout
	mov eax,4   ;system call number (sys_write)
	int 0x80    ;call kernel
	
	mov eax,1   ;system call number(exit)
	int 0x80    ;call kernel

section .data
msg db 'HelloWorld', 0xa ;string to be printed
len equ $ -msg           ; lenght of the string
	
	