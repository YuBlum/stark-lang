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
  capacity = capacity ? capacity * type_size : 1ul << 32; /* 4GiB of default capacity, practically infinite */
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
  u64 i, diff = 0;
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

struct stu64_result { u64 val, err; }
string_to_u64(const struct string *s) {
  u64 prv, i, invalid;
  struct stu64_result res;
  res.err = true;
  if (!s || !s->buf || !s->len) return res;
  res.val = 0;
  prv = 0;
  invalid = 0; 
  for (i = 0; i < s->len; i++) {
    prv = res.val;
    res.val = res.val * 10 + (s->buf[i] - '0');
    invalid |= s->buf[i] < '0' || s->buf[i] > '9' || res.val < prv;
  }
  res.err = invalid > 0;
  return res;
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

void
io_append(const struct string *string) {
  assert(string_builder_append(&io, string), 0);
}

void
io_append_cstr(const char *buf) {
  assert(string_builder_append_cstr(&io, buf), 0);
}

void
io_append_char(char c) {
  assert(string_builder_append_char(&io, c), 0);
}

void
io_append_u64(u64 value) {
  assert(string_builder_append_u64(&io, value), 0);
}

void
io_clear(void) {
  assert(string_builder_clear(&io), 0);
}

void
io_print(void) {
  assert(string_builder_print(&io), 0);
}

void
io_println(void) {
  assert(string_builder_println(&io), 0);
}

void io_set_black(void)        { io_append_cstr("\x1b[30m");   }
void io_set_red(void)          { io_append_cstr("\x1b[31m");   }
void io_set_green(void)        { io_append_cstr("\x1b[32m");   }
void io_set_yellow(void)       { io_append_cstr("\x1b[33m");   }
void io_set_blue(void)         { io_append_cstr("\x1b[34m");   }
void io_set_magenta(void)      { io_append_cstr("\x1b[35m");   }
void io_set_cyan(void)         { io_append_cstr("\x1b[36m");   }
void io_set_white(void)        { io_append_cstr("\x1b[37m");   }
void io_set_default(void)      { io_append_cstr("\x1b[39m");   }
void io_set_bold_black(void)   { io_append_cstr("\x1b[1;30m"); }
void io_set_bold_red(void)     { io_append_cstr("\x1b[1;31m"); }
void io_set_bold_green(void)   { io_append_cstr("\x1b[1;32m"); }
void io_set_bold_yellow(void)  { io_append_cstr("\x1b[1;33m"); }
void io_set_bold_blue(void)    { io_append_cstr("\x1b[1;34m"); }
void io_set_bold_magenta(void) { io_append_cstr("\x1b[1;35m"); }
void io_set_bold_cyan(void)    { io_append_cstr("\x1b[1;36m"); }
void io_set_bold_white(void)   { io_append_cstr("\x1b[1;37m"); }
void io_set_bold_default(void) { io_append_cstr("\x1b[1;39m"); }
void io_reset(void)            { io_append_cstr("\x1b[0m");    }

/* source file */
struct source {
  struct string file_path;
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
  src.file_path = string_make(file_path, 0);
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

struct source_line { u64 index, number; }
source_get_line(const struct source *src, u64 index) {
  struct source_line line;
  u64 i;
  u64 is_line;
  line.index = 0;
  line.number = 1;
  if (!src || !src->data.buf || index >= src->data.len) return line;
  for (i = 0; i < index; i++) {
    is_line = src->data.buf[i] == '\n';
    line.number += is_line;
    line.index   = (is_line)  * (i + 1)   +
                   (!is_line) * line.index;
  }
  return line;
}

struct source_position { u64 line, column; }
source_get_position(const struct source *src, u64 index) {
  struct source_position pos;
  struct source_line line;
  pos.column = 1;
  if (!src || !src->data.buf || index >= src->data.len) {
    pos.line = 1;
    return pos;
  }
  line = source_get_line(src, index);
  pos.column += index - line.index;
  pos.line    = line.number;
  return pos;
}

void
source_error_location_to_io(struct source *src, struct source_position *pos) {
  if (!src || !src->file_path.buf || !pos) return;
  io_set_bold_white();
  io_append(&src->file_path);
  io_append_char(':');
  io_append_u64(pos->line);
  io_append_char(':');
  io_append_u64(pos->column);
  io_append_char(':');
  io_set_bold_red();
  io_append_cstr(" error: ");
  io_reset();
}

void
source_error_code_snippet_to_io(struct source *src, u64 index, u64 len) {
  struct source_line line; 
  struct string line_str;
  u64 idx_on_line;
  u64 i, line_number_digits;
  if (!src || !src->data.buf || index + len > src->data.len) return;
  line = source_get_line(src, index);
  idx_on_line = index - line.index;
  io_append_cstr("  ");
  io_append_u64(line.number);
  io_append_cstr(" | ");
  line_str.buf = &src->data.buf[line.index];
  line_str.len = idx_on_line;
  io_append(&line_str);
  io_set_bold_red();
  line_str.buf = &src->data.buf[index];
  line_str.len = len;
  io_append(&line_str);
  io_reset();
  line_str.buf = &src->data.buf[index + len];
  line_str.len = 0;
  for (i = index + len; i < src->data.len; i++) {
    if (src->data.buf[i] == '\n') break;
    line_str.len++;
  }
  io_append(&line_str);
  io_append_cstr("\n  ");
  for (line_number_digits = line.number == 0; line.number; line.number /= 10) line_number_digits++;
  for (i = 0; i < line_number_digits; i++) io_append_char(' ');
  io_append_cstr(" | ");
  for (i = 0; i < idx_on_line; i++) io_append_char(' ');
  io_set_bold_red();
  io_append_char('^');
  if (len > 1) {
    for (i = 0; i < len - 1; i++) io_append_char('~');
  }
  io_reset();
  io_append_char('\n');
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
  TKN_COMMA,
  TKN_SYSCALL
};

#define TOKEN_STRING(str) do { res.buf = str; res.len = sizeof(str) - 1; } while (0)
struct string
token_to_string(enum token_type type) {
  struct string res;
  res.buf = "Unknown";
  res.len = 7;
  switch (type) {
    case TKN_IDEN:        TOKEN_STRING("Identifier");         break;
    case TKN_INT:         TOKEN_STRING("Integer");            break;
    case TKN_DEF:         TOKEN_STRING("Def");                break;
    case TKN_LPAR:        TOKEN_STRING("Left_Parenthesis");   break;
    case TKN_RPAR:        TOKEN_STRING("Right_Parenthesis");  break;
    case TKN_ASSIGN_BOD:  TOKEN_STRING("Assign_Body");        break;
    case TKN_ASSIGN_CON:  TOKEN_STRING("Assign_Constant");    break;
    case TKN_ASSIGN_VAR:  TOKEN_STRING("Assign_Variable");    break;
    case TKN_SEMICOLON:   TOKEN_STRING("Semicolon");          break;
    case TKN_COMMA:       TOKEN_STRING("Comma");              break;
    case TKN_SYSCALL:     TOKEN_STRING("Syscall");            break;
  }
  return res;
}
#undef TOKEN_STRING

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

struct lexer {
  struct token *tokens;
  struct source *src;
  u64 pos;
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

#define RETURN_KEYWORD(keyword_string, keyword_type) do { \
  keyword = string_make(keyword_string, 0); \
  if (string_eq(tok_data, &keyword)) return keyword_type; \
} while (0)
static enum token_type
token_type_from_identifier(const struct string *tok_data) {
  struct string keyword;
  RETURN_KEYWORD("def", TKN_DEF);
  RETURN_KEYWORD("__syscall__", TKN_SYSCALL);
  return TKN_IDEN;
}
#undef RETURN_KEYWORD

#define NEW_TOKEN(tok_type) do { \
  tok = tape_push(lexer.tokens, struct token); \
  assert(tok != 0, "exceeded maximum token capacity"); \
  tok->type = tok_type; \
  tok->data = tok_data; \
} while (0)

struct lexer
source_to_lexer(struct source *src) {
  char c;
  enum lexer_state state;
  struct string tok_data;
  struct lexer lexer;
  struct token *tok;
  lexer.tokens = tape_make(sizeof (struct token), 0);
  assert(lexer.tokens != 0, "couldn't make tokens buffer");
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
              tok_data.len++;
              NEW_TOKEN(TKN_ASSIGN_BOD);
            } else {
              NEW_TOKEN(TKN_ASSIGN_VAR);
            }
          } break;
          default: {
            u64 symbol_index = src->pos ? src->pos - 1 : 0;
            struct source_position pos = source_get_position(src, symbol_index);
            io.fd = STDERR;
            io_clear();
            source_error_location_to_io(src, &pos);
            io_append_cstr("unknown symbol '");
            io_set_bold_white();
            io_append_char(c);
            io_reset();
            io_append_cstr("'\n");
            source_error_code_snippet_to_io(src, symbol_index, 1);
            io_print();
            exit(1);
          } break;
        }
      } break;
      case LEXER_IDEN: {
        if (is_identifier_start(c) || is_number(c)) {
          tok_data.len++;
          if (source_peek(src, 0) != '\0') continue;
        }
        NEW_TOKEN(token_type_from_identifier(&tok_data));
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
  lexer.pos = 0;
  lexer.src = src;
  return lexer;
}
#undef NEW_TOKEN

struct token *
lexer_chop(struct lexer *lexer) {
  if (!lexer || !lexer->tokens) return 0;
  if (lexer->pos >= tape_len(lexer->tokens)) return 0;
  return &lexer->tokens[lexer->pos++];
}

struct token *
lexer_peek(const struct lexer *lexer, u64 offset) {
  if (!lexer || !lexer->tokens) return 0;
  if (lexer->pos + offset >= tape_len(lexer->tokens)) return 0;
  return &lexer->tokens[lexer->pos + offset];
}

void
lexer_rewind(struct lexer *lexer) {
  if (!lexer || !lexer->tokens) return;
  if (lexer->pos == 0) return;
  lexer->pos--;
}

struct source_position
token_get_position(const struct source *src, const struct token *tok) {
  if (!src || !src->data.buf || !tok || !tok->data.buf || tok->data.buf < src->data.buf || tok->data.buf >= src->data.buf + src->data.len) {
    struct source_position pos;
    pos.line = 1;
    pos.column = 1;
    return pos;
  }
  return source_get_position(src, (u64)(tok->data.buf - src->data.buf));
}

void
token_error_code_snippet_to_io(struct source *src, const struct token *tok) {
  if (!src || !src->data.buf || !tok || !tok->data.buf || tok->data.buf < src->data.buf || tok->data.buf >= src->data.buf + src->data.len) return;
  source_error_code_snippet_to_io(src, (u64)(tok->data.buf - src->data.buf), tok->data.len);
}

void
token_error_begin(struct lexer *lexer, struct token *tok) {
  struct source_position pos;
  if (!lexer || lexer->pos >= tape_len(lexer->tokens)) return;
  pos = token_get_position(lexer->src, tok);
  io.fd = STDERR;
  io_clear();
  source_error_location_to_io(lexer->src, &pos);
}

void
token_error_end(struct lexer *lexer, struct token *tok) {
  if (!lexer || lexer->pos >= tape_len(lexer->tokens)) return;
  io_append_char('\n');
  token_error_code_snippet_to_io(lexer->src, tok);
  io_print();
  exit(1);
}

/* parser */
enum ast_type {
  AST_NONE = 0,
  AST_ROOT,
  AST_IDEN,
  AST_GROUP,
  AST_INT,
  AST_DEF_CON,
  AST_DEF_VAR,
  AST_FN,
  AST_PARAM,
  AST_CALL,
  AST_STRUCT
};

struct ast_node;
struct ast_node_slice {
  struct ast_node **nodes;
  u64 len;
};

struct ast_node {
  enum ast_type type;
  union {
    struct { struct ast_node **children;                                                  } root;
    struct { struct string value;                                                         } iden;
    struct { u64 value;                                                                   } int_lit;
    struct { struct string name; struct ast_node *value;                                  } def;
    struct { struct ast_node *value;                                                      } group;
    struct { struct ast_node *body; struct ast_node_slice params; struct string ret_type; } fn;
    struct { struct string name, type;                                                    } param;
    struct { struct string name; struct ast_node_slice arg_list;                          } call;
  } data;
};

struct parser {
  struct ast_node *ast;
  struct ast_node **node_refs;
  struct lexer *lexer;
};

struct ast_node *
parser_node_make(struct parser *parser, enum ast_type type) {
  struct ast_node *node;
  if (!parser || !parser->ast) return 0;
  node = tape_push(parser->ast, struct ast_node);
  assert(node != 0, "exceeded maximum AST capacity");
  node->type = type;
  return node;
}

struct ast_node_slice
parser_node_slice_make(struct parser *parser, u64 amount) {
  struct ast_node_slice slice;
  if (!parser || !parser->ast) {
    slice.nodes = 0;
    slice.len = 0;
    return slice;
  }
  slice.nodes = tape_grow(parser->node_refs, amount, struct ast_node *);
  assert(slice.nodes != 0, "exceeded maximum AST node references capacity");
  slice.len = amount;
  return slice;
}

u64 parse_expression(struct parser *parser, struct ast_node **output, u64 is_part_of_expression);

struct ast_node *
parse_identifier(struct parser *parser, struct token *tok) {
  struct ast_node *node;
  node = parser_node_make(parser, AST_IDEN);
  node->data.iden.value = tok->data;
  return node;
}

struct ast_node *
parse_integer_literal(struct parser *parser, struct token *tok) {
  struct ast_node *node;
  struct stu64_result int_val;
  node = parser_node_make(parser, AST_INT);
  int_val = string_to_u64(&tok->data);
  if (int_val.err) {
    token_error_begin(parser->lexer, tok);
    io_append_cstr("integer literal is too large");
    token_error_end(parser->lexer, tok);
  }
  node->data.int_lit.value = int_val.val;
  return node;
}

struct ast_node *
parse_symbol_definition(struct parser *parser, u64 *res) {
  struct ast_node *node;
  struct token *iden, *assign;
  iden = lexer_chop(parser->lexer);
  if (!iden) {
    token_error_begin(parser->lexer, iden);
    io_append_cstr("expected identifier, but found end of file");
    token_error_end(parser->lexer, iden);
  }
  if (iden->type != TKN_IDEN) {
    token_error_begin(parser->lexer, iden);
    io_append_cstr("expected identifier, but found '");
    io_set_bold_white();
    io_append(&iden->data);
    io_reset();
    io_append_char('\'');
    token_error_end(parser->lexer, iden);
  }
  assign = lexer_chop(parser->lexer);
  if (!assign) {
    token_error_begin(parser->lexer, assign);
    io_append_cstr("expected '");
    io_set_bold_white();
    io_append_char(':');
    io_reset();
    io_append_cstr("' or '");
    io_set_bold_white();
    io_append_char('=');
    io_reset();
    io_append_cstr("', but found end of file");
    token_error_end(parser->lexer, assign);
  }
  if (assign->type == TKN_ASSIGN_CON) {
    node = parser_node_make(parser, AST_DEF_CON);
  } else if (assign->type == TKN_ASSIGN_VAR) {
    node = parser_node_make(parser, AST_DEF_VAR);
  } else {
    token_error_begin(parser->lexer, assign);
    io_append_cstr("expected '");
    io_set_bold_white();
    io_append_char(':');
    io_reset();
    io_append_cstr("' or '");
    io_set_bold_white();
    io_append_char('=');
    io_reset();
    io_append_cstr("', but found '");
    io_set_bold_white();
    io_append(&assign->data);
    io_reset();
    io_append_char('\'');
    token_error_end(parser->lexer, assign);
  }
  node->data.def.name = iden->data;
  *res = parse_expression(parser, &node->data.def.value, true);
  return node;
}

struct ast_node *
parse_expression_group(struct parser *parser, struct token *rpar, u64 *res) {
  struct ast_node *node;
  struct token *next;
  node = parser_node_make(parser, AST_GROUP);
  *res = parse_expression(parser, &node->data.group.value, true);
  next = lexer_chop(parser->lexer);
  assert(next != 0, "'parse_expression' has some wrong logic somewhere");
  if (next != rpar) {
    token_error_begin(parser->lexer, next);
    io_append_cstr("expected '");
    io_append_char('\'');
    io_set_bold_white();
    io_append_char(')');
    io_reset();
    io_append_cstr("', but found '");
    io_set_bold_white();
    io_append(&next->data);
    io_reset();
    io_append_char('\'');
    token_error_end(parser->lexer, next);
  }
  return node;
}

struct ast_node *
parse_function(struct parser *parser, u64 *res, u64 member_amount) {
  struct ast_node *node;
  struct token *next;
  node = parser_node_make(parser, AST_FN);
  if (member_amount) {
    struct token *param_tkn;
    struct ast_node *param;
    u64 param_idx;
    node->data.fn.params = parser_node_slice_make(parser, member_amount);
    for (param_idx = 0; param_idx < member_amount; param_idx++) {
      param_tkn = lexer_chop(parser->lexer);
      if (param_tkn->type == TKN_RPAR) break;
      if (param_tkn->type != TKN_IDEN) {
        token_error_begin(parser->lexer, param_tkn);
        io_append_cstr("expected parameter name, but found '");
        io_set_bold_white();
        io_append(&next->data);
        io_reset();
        io_append_char('\'');
        token_error_end(parser->lexer, param_tkn);
      }
      param = parser_node_make(parser, AST_PARAM);
      param->data.param.name = param_tkn->data;
      next = lexer_chop(parser->lexer);
      if (next->type == TKN_ASSIGN_VAR) {
        next = lexer_chop(parser->lexer);
        if (next->type != TKN_IDEN) {
          token_error_begin(parser->lexer, next);
          io_append_cstr("expected parameter type, but found '");
          io_set_bold_white();
          io_append(&next->data);
          io_reset();
          io_append_char('\'');
          token_error_end(parser->lexer, next);
        }
        param->data.param.type = next->data;
        next = lexer_chop(parser->lexer);
        if (next->type != TKN_COMMA) {
          if (next->type != TKN_RPAR) {
            token_error_begin(parser->lexer, next);
            io_append_cstr("expected '");
            io_set_bold_white();
            io_append_char(')');
            io_reset();
            io_append_cstr("', but found '");
            io_set_bold_white();
            io_append(&next->data);
            io_reset();
            io_append_char('\'');
            token_error_end(parser->lexer, next);
          }
        }
      } else if (next->type == TKN_COMMA) {
        u64 i;
        for (i = 0; ; i++) {
          next = lexer_peek(parser->lexer, i);
          if (next->type == TKN_ASSIGN_VAR) {
            next = lexer_peek(parser->lexer, i);
            /* don't need to check if 'next' is an identifier
               * it will cause an error on a future parameter parsing iteration if it isn't */
            param->data.param.type = next->data; 
            break;
          }
          if (next->type == TKN_RPAR) {
            token_error_begin(parser->lexer, param_tkn);
            io_append_cstr("parameter without a type");
            token_error_end(parser->lexer, param_tkn);
          }
        }
      } else {
        token_error_begin(parser->lexer, param_tkn);
        io_append_cstr("parameter without a type");
        token_error_end(parser->lexer, param_tkn);
      }
      node->data.fn.params.nodes[param_idx] = param;
    }
  } else {
    node->data.fn.params.len = 0;
    next = lexer_chop(parser->lexer);
    if (next->type != TKN_RPAR) { 
      token_error_begin(parser->lexer, next);
      io_append_cstr("expected '");
      io_set_bold_white();
      io_append_char(')');
      io_reset();
      io_append_cstr("', but found '");
      io_set_bold_white();
      io_append(&next->data);
      io_reset();
      io_append_char('\'');
      token_error_end(parser->lexer, next);
    }
  }
  next = lexer_chop(parser->lexer);
  if (next) {
    if (next->type == TKN_IDEN) {
      node->data.fn.ret_type = next->data;
      next = lexer_chop(parser->lexer);
    } else {
      node->data.fn.ret_type.len = 0;
    }
    if (next->type != TKN_ASSIGN_BOD) {
      token_error_begin(parser->lexer, next);
      io_append_cstr("expected '");
      io_set_bold_white();
      io_append_cstr("=>");
      io_reset();
      io_append_cstr("' but found '");
      io_set_bold_white();
      io_append(&next->data);
      io_reset();
      io_append_char('\'');
      token_error_end(parser->lexer, next);
    }
    *res = parse_expression(parser, &node->data.fn.body, true);
  }
  return node;
}

struct ast_node *
parse_function_call(struct parser *parser, u64 *res, u64 member_amount) {
  struct ast_node *node;
  struct string name;
  name = parser->prv_node->data.iden.value;
  node = parser->prv_node;
  node->type = AST_CALL;
  node->data.call.name = name;
  if (member_amount) {
    struct token *next;
    u64 arg_idx;
    node->data.call.arg_list = parser_node_slice_make(parser, member_amount);
    for (arg_idx = 0; arg_idx < member_amount; arg_idx++) {
      if (!parse_expression(parser, &node->data.call.arg_list.nodes[arg_idx], true)) {
        *res = false;
        return 0;
      }
      next = lexer_chop(parser->lexer);
      if (next->type != TKN_COMMA) {
        token_error_begin(parser->lexer, next);
        io_append_cstr("expected '");
        io_set_bold_white();
        io_append_char(',');
        io_reset();
        io_append_cstr("' but found '");
        io_set_bold_white();
        io_append(&next->data);
        io_reset();
        io_append_char('\'');
        token_error_end(parser->lexer, next);
      }
    }
  } else {
    node->data.call.arg_list.len = 0;
  }
  return node;
}

struct ast_node *
parse_parenthesis(struct parser *parser, struct token *tok, u64 *res, u64 *is_part_of_expression) {
  struct ast_node *node;
  struct token *rpar;
  struct token *next;
  enum ast_type type;
  u64 rpar_offset, skip_rpar, member_amount;
  skip_rpar = false;
  member_amount = 0;
  for (rpar_offset = 0; ; rpar_offset++) {
    rpar = lexer_peek(parser->lexer, rpar_offset);
    if (!rpar) {
      token_error_begin(parser->lexer, tok);
      io_append_char('\'');
      io_set_bold_white();
      io_append_char('(');
      io_reset();
      io_append_cstr("' without closing '");
      io_set_bold_white();
      io_append_char(')');
      io_reset();
      io_append_char('\'');
      token_error_end(parser->lexer, tok);
    }
    if (rpar->type == TKN_LPAR) {
      skip_rpar = true;
    } else if (rpar->type == TKN_RPAR) {
      if (skip_rpar) skip_rpar = false;
      else break;
    } else if (rpar->type == TKN_COMMA && !skip_rpar) {
      member_amount++;
    }
  }
  type = AST_GROUP;
  next = lexer_peek(parser->lexer, 0);
  if (parser->prv_node) {
    io_clear();
    io_append_u64(parser->prv_node->type);
    io_println();
  }
  if (parser->prv_node && parser->prv_node->type == AST_IDEN) {
    type = AST_CALL;
    member_amount += next != rpar;
  } else if (next->type == TKN_IDEN) {
    member_amount++;
    next = lexer_peek(parser->lexer, 1);
    if (next->type == TKN_ASSIGN_VAR || next->type == TKN_COMMA) {
      type = AST_STRUCT;
      next = lexer_peek(parser->lexer, rpar_offset + 1);
      if (next) {
        if (next->type == TKN_ASSIGN_BOD) {
          type = AST_FN;
        } else if (next->type == TKN_IDEN) {
          next = lexer_peek(parser->lexer, rpar_offset + 2);
          if (next && next->type == TKN_ASSIGN_BOD) type = AST_FN;
        }
      }
    }
  } else if (next == rpar) {
    next = lexer_peek(parser->lexer, 1);
    if (next && next->type == TKN_ASSIGN_BOD) type = AST_FN;
  }
  if (type == AST_GROUP) {
    node = parse_expression_group(parser, rpar, res);
  } else if (type == AST_FN) {
    node = parse_function(parser, res, member_amount);
  } else if (type == AST_CALL) {
    *is_part_of_expression = true;
    node = parse_function_call(parser, res, member_amount);
  } else if (type == AST_STRUCT) {
    assert(0, "structs aren't handled yet");
  } else {
    assert(0, "parse_parenthesis: unreachable");
  }
  return node;
}

u64
parse_expression(struct parser *parser, struct ast_node **output, u64 is_part_of_expression) {
  u64 res;
  struct ast_node *node;
  struct token *tok;
  tok = lexer_chop(parser->lexer);
  if (!parser->lexer || !parser || !output || !tok) return false;
  res = true;
  switch (tok->type) {
    case TKN_IDEN: {
      node = parse_identifier(parser, tok);
    } break;
    case TKN_INT: {
      node = parse_integer_literal(parser, tok);
    } break;
    case TKN_DEF: {
      node = parse_symbol_definition(parser, &res);
    } break;
    case TKN_LPAR: {
      node = parse_parenthesis(parser, tok, &res, &is_part_of_expression);
    } break;
    default: {
      token_error_begin(parser->lexer, tok);
      io_append_char('\'');
      io_set_bold_white();
      io_append(&tok->data);
      io_reset();
      io_append_cstr("' isn't a valid expression start");
      token_error_end(parser->lexer, tok);
    } break;
  }
  if (!is_part_of_expression) {
    struct token *semicolon = lexer_chop(parser->lexer);
    if (!semicolon) {
      token_error_begin(parser->lexer, semicolon);
      io_append_cstr("expected '");
      io_set_bold_white();
      io_append_char(';');
      io_reset();
      io_append_cstr("', but found end of file");
      token_error_end(parser->lexer, semicolon);
    }
    if (semicolon->type != TKN_SEMICOLON) {
      token_error_begin(parser->lexer, semicolon);
      io_append_cstr("expected '");
      io_set_bold_white();
      io_append_char(';');
      io_reset();
      io_append_cstr("', but found '");
      io_set_bold_white();
      io_append(&semicolon->data);
      io_reset();
      io_append_char('\'');
      token_error_end(parser->lexer, semicolon);
    }
  }
  *output = node;
  return res;
}

struct parser
lexer_to_parser(struct lexer *lexer) {
  struct parser parser;
  struct ast_node *root, **next;
  parser.lexer = lexer;
  parser.ast = tape_make(sizeof (struct ast_node), 0);
  assert(parser.ast != 0, "couldn't allocate enough memory for the AST");
  parser.node_refs = tape_make(sizeof (struct ast_node *), 0);
  assert(parser.ast != 0, "couldn't allocate enough memory for the AST");
  root = parser_node_make(&parser, AST_ROOT);
  root->data.root.children = tape_make(sizeof (struct ast_node *), 0);
  assert(root->data.root.children != 0, "couldn't allocate enough memory for the AST root node");
  do {
    next = tape_push(root->data.root.children, struct ast_node *);
    assert(next != 0, "exceeded maximum AST root node capacity");
  } while (parse_expression(&parser, next, false));
  return parser;
}

/* entry point */
void
_start(void) {
  struct source src;
  struct lexer lexer;
  struct parser parser;

  io_make();

  src    = file_to_source("./first.sk");
  lexer  = source_to_lexer(&src);
  parser = lexer_to_parser(&lexer);
  (void)parser;

#if 0
  {
    u64 i;
    io_clear();
    for (i = 0; i < tape_len(tokens); i++) {
      struct source_position pos = token_get_position(&src, &tokens[i]);
      struct string type = token_to_string(tokens[i].type);
      (void)source_error_location_to_io(&src, &pos);
      io_append(&type);
      io_append_cstr(" '");
      io_set_bold_white();
      io_append(&tokens[i].data);
      io_reset();
      io_append_cstr("'\n");
      (void)token_error_code_snippet_to_io(&src, &tokens[i]);
    }
    io_print();
  }
#endif

  exit(0);
}
