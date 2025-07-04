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

void
exit(u64 exit_code) {
  (void)__syscall__(60, exit_code, 0, 0, 0, 0, 0);
}

u64
write(u64 fd, const char *msg, u64 len) {
  return __syscall__(1, fd, (u64)msg, len, 0, 0, 0);
}

void *
mmap(void *addr, u64 len, u64 prot, u64 flags, u64 fildes, u64 off) {
  return (void *)__syscall__(9, (u64)addr, len, prot, flags, fildes, off);
}

u64
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

void *
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

void *
tape_grow(void *tape, u64 amount) {
  struct tape_header *h;
  void *out = 0;
  if (!tape) return out;
  h = TAPE_HEADER_GET(tape);
  if ((h->len + amount) * h->typ > h->cap) return out;
  out = (char *)tape + (h->len * h->typ);
  h->len += amount;
  return out;
}

u64
tape_shrink(void *tape, u64 amount) {
  struct tape_header *h;
  if (!tape) return false;
  h = TAPE_HEADER_GET(tape);
  if (amount > h->len) return false;
  h->len -= amount;
  return true;
}

#define tape_push(tape) tape_grow(tape, 1)
#define tape_pop(tape) tape_shrink(tape, 1)

u64
tape_len(const void *tape) {
  struct tape_header *h;
  if (!tape) return 0;
  h = TAPE_HEADER_GET(tape);
  return h->len;
}

u64
tape_cap(const void *tape) {
  struct tape_header *h;
  if (!tape) return 0;
  h = TAPE_HEADER_GET(tape);
  return h->cap;
}

u64
tape_clear(void *tape) {
  struct tape_header *h;
  if (!tape) return false;
  h = TAPE_HEADER_GET(tape);
  h->len = 0;
  return true;
}

u64
tape_destroy(void *tape) {
  struct tape_header *h;
  if (!tape) return 0;
  h = TAPE_HEADER_GET(tape);
  return munmap(h, h->cap) == 0;
}

/* string */
struct string {
  const char *buf;
  u64         len;
};
#define CSTRING_MAX (1ul << 20)

u64
cstring_len(const char *buf) {
  u64 n;
  if (!buf) return 0;
  for (n = 0; buf[n] && n < CSTRING_MAX; n++);
  return n;
}

struct string
string_make(const char *buf, u64 len) {
  struct string s;
  s.buf = buf;
  s.len = len ? len : cstring_len(buf);
  return s;
}

u64
string_eq(const struct string *s0, const struct string *s1) {
  u64 i;
  u64 diff = 0;
  if (!s0 || !s1) return false;
  if (s0->len != s1->len) return false;
  if (s0->len != 0 && (!s0->buf || !s1->buf)) return false;
  for (i = 0; i < s0->len; i++) diff |= s0->buf[i] ^ s1->buf[i];
  return diff == 0;
}

void
string_print(const struct string *s) {
  if (!s || !s->len || !s->buf) return;
  (void)write(STDOUT, s->buf, s->len);
}

/* string builder */
struct string_builder {
  char *buf;
  u64   len;
  u64   beg;
};

struct string_builder
string_builder_begin(char *tape_buf) {
  struct string_builder builder;
  builder.len = 0;
  if (!tape_buf) {
    builder.buf = 0;
    builder.beg = 0;
    return builder;
  }
  builder.buf = tape_buf;
  builder.beg = tape_len(tape_buf);
  return builder;
}

struct string
string_builder_end(const struct string_builder *builder) {
  struct string s;
  if (!builder || !builder->buf) {
    s.buf = 0;
    s.len = 0;
    return s;
  }
  s.buf = builder->buf + builder->beg;
  s.len = builder->len;
  return s;
}

u64
string_builder_append(struct string_builder *builder, const struct string *string) {
  u64 i;
  if (!builder || !builder->buf || !string) return false;
  if (!tape_grow(builder->buf, string->len)) return false;
  for (i = 0; i < string->len; i++) builder->buf[builder->beg + builder->len++] = string->buf[i];
  return true;
}

u64
string_builder_append_cstr(struct string_builder *builder, const char *buf) {
  u64 i, len;
  if (!builder || !builder->buf || !buf) return false;
  len = cstring_len(buf);
  if (!tape_grow(builder->buf, len)) return false;
  for (i = 0; i < len; i++) builder->buf[builder->beg + builder->len++] = buf[i];
  return true;
}

u64
string_builder_append_char(struct string_builder *builder, char c) {
  if (!builder || !builder->buf) return false;
  if (!tape_push(builder->buf)) return false;
  builder->buf[tape_len(builder->buf) - 1] =c;
  return true;
}

#define DIGIT_PAIRS "00010203040506070809" \
                    "10111213141516171819" \
                    "20212223242526272829" \
                    "30313233343536373839" \
                    "40414243444546474849" \
                    "50515253545556575859" \
                    "60616263646566676869" \
                    "70717273747576777879" \
                    "80818283848586878889" \
                    "90919293949596979899"
u64
string_builder_append_u64(struct string_builder *builder, u64 value) {
  u64 n, i, p;
  char *buf;
  if (!builder || !builder->buf) return false;
  n = value == 0;
  for (i = value; i; i /= 10) n++;
  if (!tape_grow(builder->buf, n)) return false;
  builder->len += n;
  buf = builder->buf + builder->len;
  for (i = value; i >= 100; i /= 100) {
    p = (i % 100) * 2;
    *(--buf) = DIGIT_PAIRS[p+1];
    *(--buf) = DIGIT_PAIRS[p];
  }
  if (i < 10) {
    *(--buf) = '0' + i;
  } else {
    p = i * 2;
    *(--buf) = DIGIT_PAIRS[p+1];
    *(--buf) = DIGIT_PAIRS[p];
  }
  return true;
}
#undef DIGIT_PAIRS

void
_start(void) {
  char *print_buf;
  struct string_builder b;
  struct string s;
  print_buf = tape_make(sizeof (char), 0);
  b = string_builder_begin(print_buf);
  (void)string_builder_append_cstr(&b, "the number is: ");
  (void)string_builder_append_u64(&b, 123);
  (void)string_builder_append_cstr(&b, "!\n");
  s = string_builder_end(&b);
  string_print(&s);
  exit(0);
}
