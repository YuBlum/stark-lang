format ELF64

section '.text' executable
public __syscall__
__syscall__:
  mov rax, rdi
  mov rdi, rsi
  mov rsi, rdx
  mov rdx, rcx
  mov r10, r8
  mov r8,  r9
  mov r9,  [rsp+8]
  syscall
  ret
