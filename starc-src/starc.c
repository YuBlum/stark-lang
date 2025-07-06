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
  if (capacity == 0) capacity = 1ul << 32; /* 4GiB of default capacity, practically infinite */
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

#undef TAPE_HEADER_GET

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

/* default I/O buffer */
static struct string_builder io;
void
io_make(void) {
  io = string_builder_begin(0);
  if (!io.buf) exit(1);
}

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

/* source file */
struct source {
  struct string data;
  u64 pos;
};

struct source
file_to_source(const char *file_path) {
  struct source src;
  char *src_buf;
  u64 src_file;
  struct stat src_stat;
  src_file = open(file_path, O_RDONLY, 0);
  assert(!is_neg(src_file), "couldn't open source file");
  assert(!is_neg(fstat(src_file, &src_stat)), "couldn't get file info");
  src_buf = tape_make(sizeof (char), src_stat.st_size);
  assert(src_buf != 0, "couldn't make source buffer");
  assert(read(src_file, src_buf, src_stat.st_size) == src_stat.st_size, "couldn't read source file");
  assert(!is_neg(close(src_file)), "couldn't close source file");
  src.data.len = src_stat.st_size;
  src.data.buf = src_buf;
  src.pos = 0;
  return src;
}

char
source_chop(struct source *src) {
  if (!src || !src->data.buf) return '\0';
  if (src->pos >= src->data.len) return '\0';
  return src->data.buf[src->pos++];
}

char
source_peek(const struct source *src, u64 offset) {
  if (!src || !src->data.buf) return '\0';
  if (src->pos + offset >= src->data.len) return '\0';
  return src->data.buf[src->pos + offset];
}

void
source_rewind(struct source *src) {
  if (!src || !src->data.buf) return;
  if (src->pos == 0) return;
  src->pos--;
}

/* lexer */
enum token_type {
  TKN_IDEN = 0,
  TKN_INT,
  TKN_DEF,
  TKN_LPAR,
  TKN_RPAR,
  TKN_ASSIGN_BOD,
  TKN_ASSIGN_CON,
  TKN_ASSIGN_VAR,
  TKN_SEMICOLON,
  TKN_COMMA
};

struct string
token_to_string(enum token_type type) {
  struct string res;
  res.buf = "Unknown";
  res.len = 7;
  switch (type) {
    case TKN_IDEN:        res.buf = "Identifier";        res.len = 10;  break;
    case TKN_INT:         res.buf = "Integer";           res.len = 7;   break;
    case TKN_DEF:         res.buf = "Def";               res.len = 3;   break;
    case TKN_LPAR:        res.buf = "Left_Parenthesis";  res.len = 16;  break;
    case TKN_RPAR:        res.buf = "Right_Parenthesis"; res.len = 17;  break;
    case TKN_ASSIGN_BOD:  res.buf = "Assign_Body";       res.len = 11;  break;
    case TKN_ASSIGN_CON:  res.buf = "Assign_Constant";   res.len = 15;  break;
    case TKN_ASSIGN_VAR:  res.buf = "Assign_Variable";   res.len = 15;  break;
    case TKN_SEMICOLON:   res.buf = "Semicolon";         res.len = 9;   break;
    case TKN_COMMA:       res.buf = "Comma";             res.len = 5;   break;
  }
  return res;
}

struct token {
  enum token_type type;
  struct string data;
};

enum lexer_state {
  LEXER_NORMAL = 0,
  LEXER_IDEN,
  LEXER_INT,
  LEXER_COMMENT
};

static u64
is_delimiter(char c) {
  return c == ' ' || c == '\t' || c == '\n';
}

static u64
is_identifier_start(char c) {
  return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_';
}

static u64
is_number(char c) {
  return c >= '0' && c <= '9';
}

#define NEW_TOKEN(tok_type) do { \
  tok = tape_push(tokens, struct token); \
  assert(tok != 0, "exceeded maximum token capacity"); \
  tok->type = tok_type; \
  tok->data = tok_data; \
} while (0)

struct token *
source_to_tokens(struct source *src) {
  char c;
  enum lexer_state state;
  struct string tok_data;
  struct token *tok;
  struct token *tokens;
  tokens = tape_make(sizeof (struct token), 0);
  assert(tokens != 0, "couldn't make tokens buffer");
  state = LEXER_NORMAL;
  while ((c = source_chop(src))) {
    switch (state) {
      case LEXER_NORMAL: {
        if (is_delimiter(c)) continue;
        tok_data.buf = &src->data.buf[src->pos - 1];
        tok_data.len = 1;
        if (is_identifier_start(c)) {
          state = LEXER_IDEN;
          continue;
        }
        if (is_number(c)) {
          state = LEXER_INT;
          continue;
        }
        switch (c) {
          case '#':
            state = LEXER_COMMENT;
            continue;
            break;
          case '(':
            NEW_TOKEN(TKN_LPAR);
            break;
          case ')':
            NEW_TOKEN(TKN_RPAR);
            break;
          case ',':
            NEW_TOKEN(TKN_COMMA);
            break;
          case ';':
            NEW_TOKEN(TKN_SEMICOLON);
            break;
          case ':':
            NEW_TOKEN(TKN_ASSIGN_CON);
            break;
          case '=': {
            if (source_peek(src, 0) == '>') {
              (void)source_chop(src);
              tok->data.len++;
              NEW_TOKEN(TKN_ASSIGN_BOD);
            } else {
              NEW_TOKEN(TKN_ASSIGN_VAR);
            }
          } break;
          default:
            io.fd = STDERR;
            if (string_builder_clear(&io)) {
              if (string_builder_append_cstr(&io, "unknown symbol '") &&
                  string_builder_append_char(&io, c) &&
                  string_builder_append_char(&io, '\'')) {
                (void)string_builder_println(&io);
              }
            }
            exit(1);
        }
      } break;
      case LEXER_IDEN: {
        if (is_identifier_start(c) || is_number(c)) {
          tok_data.len++;
          if (source_peek(src, 0) != '\0') continue;
        }
        NEW_TOKEN(TKN_IDEN);
        state = LEXER_NORMAL;
        source_rewind(src);
      } break;
      case LEXER_INT: {
        if (is_number(c)) {
          tok_data.len++;
          if (source_peek(src, 0) != '\0') continue;
        }
        NEW_TOKEN(TKN_INT);
        state = LEXER_NORMAL;
        source_rewind(src);
      } break;
      case LEXER_COMMENT: {
        if (c == '\n') state = LEXER_NORMAL;
      }
    }
  }
  return tokens;
}
#undef NEW_TOKEN

struct token_position {
  u64 line, column;
};

struct token_position
token_get_position(const struct source *src, const struct token *tok) {
  struct token_position pos;
  u64 i;
  u64 line_idx;
  u64 tok_start;
  u64 is_line;
  pos.line = 1;
  pos.column = 1;
  if (!src || !src->data.buf || !tok || !tok->data.buf) return pos;
  line_idx = 0;
  tok_start = (u64)(tok->data.buf - src->data.buf);
  for (i = 0; i < tok_start; i++) {
    is_line = src->data.buf[i] == '\n';
    pos.line += is_line;
    line_idx = (is_line)  * (i + 1) +
               (!is_line) * line_idx;
  }
  pos.column += tok_start - line_idx;
  return pos;
}

/* entry point */
void
_start(void) {
  struct source src;
  struct token *tokens;
  u64 i;

  io_make();

  src = file_to_source("./first.sk");
  tokens = source_to_tokens(&src);

  for (i = 0; i < tape_len(tokens); i++) {
    struct token_position pos = token_get_position(&src, &tokens[i]);
    struct string str = token_to_string(tokens[i].type);
    assert(string_builder_append_cstr(&io, "token["), 0);
    assert(string_builder_append_u64(&io, i), 0);
    assert(string_builder_append_cstr(&io, "] = { "), 0);
    assert(string_builder_append(&io, &str), 0);
    assert(string_builder_append_cstr(&io, " '"), 0);
    assert(string_builder_append(&io, &tokens[i].data), 0);
    assert(string_builder_append_cstr(&io, "' "), 0);
    assert(string_builder_append_u64(&io, pos.line), 0);
    assert(string_builder_append_cstr(&io, ":"), 0);
    assert(string_builder_append_u64(&io, pos.column), 0);
    assert(string_builder_append_cstr(&io, " }\n"), 0);
  }

  assert(string_builder_print(&io), 0);

  exit(0);
}
