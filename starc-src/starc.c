#include <stdint.h>
typedef uint64_t u64;
#define true 1
#define false 0

/* system calls */
#define STDOUT 1
#define STDERR 2
#define PROT_READ 0x1
#define PROT_WRITE 0x2
#define MAP_PRIVATE 0x2
#define MAP_ANONYMOUS 0x20
#define MAP_FAILED ((void *) -1)

u64 __syscall__(u64 sys_code, u64 arg0, u64 arg1, u64 arg2, u64 arg3, u64 arg4, u64 arg5);

static void
exit(u64 exit_code) {
  (void)__syscall__(60, exit_code, 0, 0, 0, 0, 0);
}

static u64
write(u64 fd, const char *msg, u64 len) {
  return __syscall__(1, fd, (u64)msg, len, 0, 0, 0);
}

static void *
mmap(void *addr, u64 len, u64 prot, u64 flags, u64 fildes, u64 off) {
  return (void *)__syscall__(9, (u64)addr, len, prot, flags, fildes, off);
}

static u64
munmap(void *addr, u64 len) {
  return __syscall__(11, (u64)addr, len, 0, 0, 0, 0);
}

/* tape with arena-only allocator */
struct tape_header {
  u64 len;
  u64 cap;
  u64 typ;
};
#define TAPE_HEADER_GET(tape) (((struct tape_header *)tape) - 1)

static void *
tape_make(u64 type_size, u64 capacity) {
  struct tape_header *h;
  if (type_size == 0) type_size = 1;
  if (capacity == 0) capacity = 1ul << 33;
  h = mmap(0, sizeof (struct tape_header) + capacity, PROT_READ|PROT_WRITE, MAP_PRIVATE|MAP_ANONYMOUS, -1, 0);
  if (h == MAP_FAILED) return 0;
  h->len = 0;
  h->cap = capacity;
  h->typ = type_size;
  return h + 1;
}

static u64
tape_push(void *tape) {
  struct tape_header *h;
  if (!tape) return false;
  h = TAPE_HEADER_GET(tape);
  if (h->len * h->typ >= h->cap) return false;
  h->len++;
  return true;
}

static u64
tape_pop(void *tape) {
  struct tape_header *h;
  if (!tape) return false;
  h = TAPE_HEADER_GET(tape);
  if (h->len == 0) return false;
  h->len--;
  return true;
}

static u64
tape_len(void *tape) {
  struct tape_header *h;
  if (!tape) return 0;
  h = TAPE_HEADER_GET(tape);
  return h->len;
}

static u64
tape_cap(void *tape) {
  struct tape_header *h;
  if (!tape) return 0;
  h = TAPE_HEADER_GET(tape);
  return h->cap;
}

static u64
tape_clear(void *tape) {
  struct tape_header *h;
  if (!tape) return false;
  h = TAPE_HEADER_GET(tape);
  h->len = 0;
  return true;
}

static u64
tape_destroy(void *tape) {
  struct tape_header *h;
  if (!tape) return 0;
  h = TAPE_HEADER_GET(tape);
  return munmap(h, h->cap) == 0;
}

void
_start(void) {
  (void)write(STDOUT, "Hello, World!\n", 14);
  exit(0);
}
