typedef unsigned long int u64;
#define true 1
#define false 0

#define is_neg(x) ((x) >= (1ul << 63))

/* system calls */
#define STDOUT 1
#define STDERR 2

#define PROT_READ 0x1
#define PROT_WRITE 0x2
#define MAP_PRIVATE 0x2
#define MAP_ANONYMOUS 0x20
#define MAP_FAILED ((void *) -1)

#define O_RDONLY 0x0
#define O_WRONLY 0x1
#define O_RDWR   0x2
#define O_CREAT  0x40

#define SYS_READ    0
#define SYS_WRITE   1
#define SYS_OPEN    2
#define SYS_CLOSE   3
#define SYS_FSTAT   5
#define SYS_MMAP    9
#define SYS_MUNMAP  11
#define SYS_EXIT    60

struct stat {
  u64 st_dev;
  u64 st_ino;
  u64 st_nlink;
  u64 __wrong0; /* should be 'st_mode' and 'st_uid', both u32, but for now we don't care */
  u64 __wrong1; /* should be 'st_gid' and '__pad0', both u32, but for now we don't care */
  u64 st_rdev;
  u64 st_size; /* should be i64, but for now we don't care */
  u64 st_blksize; /* should be i64, but for now we don't care */
  u64 st_blocks; /* should be i64, but for now we don't care */
  u64 st_atime;
  u64 st_atime_nsec;
  u64 st_mtime;
  u64 st_mtime_nsec;
  u64 st_ctime;
  u64 st_ctime_nsec;
  u64 __unused[3]; /* should be i64, but for now we don't care */
};

u64 __syscall__(u64 sys_code, u64 arg0, u64 arg1, u64 arg2, u64 arg3, u64 arg4, u64 arg5);

void
exit(u64 exit_code) {
  (void)__syscall__(SYS_EXIT, exit_code, 0, 0, 0, 0, 0);
}

u64
read(u64 fd, char *buf, u64 len) {
  return __syscall__(SYS_READ, fd, (u64)buf, len, 0, 0, 0);
}

u64
write(u64 fd, const char *buf, u64 len) {
  return __syscall__(SYS_WRITE, fd, (u64)buf, len, 0, 0, 0);
}

u64
open(const char *path, u64 flags, u64 mode) {
  return __syscall__(SYS_OPEN, (u64)path, flags, mode, 0, 0, 0);
}

u64
close(u64 fd) {
  return __syscall__(SYS_CLOSE, fd, 0, 0, 0, 0, 0);
}

u64
fstat(u64 fd, struct stat *out) {
  return __syscall__(SYS_FSTAT, fd, (u64)out, 0, 0, 0, 0);
}

void *
mmap(void *addr, u64 len, u64 prot, u64 flags, u64 fildes, u64 off) {
  return (void *)__syscall__(SYS_MMAP, (u64)addr, len, prot, flags, fildes, off);
}

u64
munmap(void *addr, u64 len) {
  return __syscall__(SYS_MUNMAP, (u64)addr, len, 0, 0, 0, 0);
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
tape_grow_unsafe(void *tape, u64 amount) {
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

#define tape_grow(tape, amount, T) ((T *)tape_grow_unsafe(tape, amount))
#define tape_push_unsafe(tape) tape_grow_unsafe(tape, 1)
#define tape_push(tape, T) tape_grow(tape, 1, T)
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
  return munmap(h, sizeof (struct tape_header) + h->cap) == 0;
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

u64
string_print(const struct string *s) {
  if (!s || !s->len || !s->buf) return false;
  return !is_neg(write(STDOUT, s->buf, s->len));
}

/* string builder */
struct string_builder {
  char *buf;
  u64   len;
  u64   beg;
  u64   own;
  u64   fd;
};

struct string_builder
string_builder_begin(char *tape_buf) {
  struct string_builder builder;
  builder.len = 0;
  builder.fd  = STDOUT;
  if (!tape_buf) {
    builder.own = true;
    builder.buf = tape_make(sizeof (char), 0);
    builder.beg = 0;
    return builder;
  }
  builder.own = false;
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
string_builder_destroy(struct string_builder *builder) {
  if (!builder || !builder->own) return false;
  if (!tape_destroy(builder->buf)) return false;
  builder->buf = 0;
  builder->beg = 0;
  builder->len = 0;
  builder->own = false;
  return true;
}

u64
string_builder_append(struct string_builder *builder, const struct string *string) {
  u64 i;
  if (!builder || !builder->buf || !string) return false;
  if (!tape_grow(builder->buf, string->len, char)) return false;
  for (i = 0; i < string->len; i++) builder->buf[builder->beg + builder->len++] = string->buf[i];
  return true;
}

u64
string_builder_append_cstr(struct string_builder *builder, const char *buf) {
  u64 i, len;
  if (!builder || !builder->buf || !buf) return false;
  len = cstring_len(buf);
  if (!tape_grow(builder->buf, len, char)) return false;
  for (i = 0; i < len; i++) builder->buf[builder->beg + builder->len++] = buf[i];
  return true;
}

u64
string_builder_append_char(struct string_builder *builder, char c) {
  if (!builder || !builder->buf) return false;
  if (!tape_push(builder->buf, char)) return false;
  builder->buf[builder->len++] = c;
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
  if (!tape_grow(builder->buf, n, char)) return false;
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

u64
string_builder_clear(struct string_builder *builder) {
  if (!builder || !builder->buf) return false;
  builder->len = 0;
  return true;
}

u64
string_builder_print(const struct string_builder *builder) {
  if (!builder || !builder->buf) return false;
  return !is_neg(write(builder->fd, builder->buf + builder->beg, builder->len));;
}

u64
string_builder_println(struct string_builder *builder) {
  if (!builder || !builder->buf) return false;
  if (!string_builder_append_char(builder, '\n')) return false;
  return !is_neg(write(builder->fd, builder->buf + builder->beg, builder->len));
}


static struct string_builder io;

void
assert(u64 cond, const char *msg) {
  if (cond) return;
  io.fd = STDERR;
  if (msg && string_builder_clear(&io)) {
    if (string_builder_append_cstr(&io, msg)) {
      (void)string_builder_println(&io);
    }
  }
  exit(1);
}

/* entry point */
void
_start(void) {
  struct string src;

  io = string_builder_begin(0);
  if (!io.buf) exit(1);

  {
    char *src_buf;
    u64 src_file;
    struct stat src_stat;
    src_file = open("./first.sk", O_RDONLY, 0);
    assert(!is_neg(src_file), "couldn't open source file");
    assert(!is_neg(fstat(src_file, &src_stat)), "couldn't get file info");
    src_buf = tape_make(sizeof (char), src_stat.st_size);
    assert(src_buf != 0, "couldn't make source buffer");
    assert(read(src_file, src_buf, src_stat.st_size) == src_stat.st_size, "couldn't read source file");
    assert(!is_neg(close(src_file)), "couldn't close source file");
    src.len = src_stat.st_size;
    src.buf = src_buf;
  }

  assert(string_builder_append(&io, &src), 0);
  assert(string_builder_print(&io), 0);

  exit(0);
}
