format ELF64

section '.text' executable
public syscall3
syscall3:
  mov rax, rdi
  mov rdi, rsi
  mov rsi, rdx
  mov rdx, rcx
  syscall
  ret
