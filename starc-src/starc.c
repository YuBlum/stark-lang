#include <stdint.h>
typedef uint64_t u64;

enum {
  STDOUT = 1,
  STDERR
};

u64 syscall3(u64 sys_code, u64 arg0, u64 arg1, u64 arg2);

static void
exit(u64 exit_code) {
  (void)syscall3(60, exit_code, 0, 0);
}

static void
write(u64 fd, const char *msg, u64 len) {
  (void)syscall3(1, fd, (u64)msg, len);
}

void
_start(void) {
  write(STDOUT, "Hello, World!\n", 14);
  exit(42);
}
