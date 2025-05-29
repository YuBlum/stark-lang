#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <errno.h>
#include <assert.h>
#include "mial.h"

#define LOG_INFO(MSG, ...)  fprintf(stderr, "\x1b[1;32m[MIAL INFO] "  MSG "\x1b[0m", ## __VA_ARGS__)
#define LOG_WARN(MSG, ...)  fprintf(stderr, "\x1b[1;33m[MIAL WARN] "  MSG "\x1b[0m", ## __VA_ARGS__)
#define LOG_ERROR(MSG, ...) fprintf(stderr, "\x1b[1;31m[MIAL ERROR] " MSG "\x1b[0m", ## __VA_ARGS__)

struct string {
  uint64_t size;
  const char *data;
};

void
get_source_from_file(struct string *out, const char *filepath) {
  FILE *f = fopen(filepath, "r");
  if (!f) {
    LOG_ERROR("Couldn't open source file '%s': %s\n", filepath, strerror(errno));
    exit(1);
  }
  if (fseek(f, 0, SEEK_END) < 0) {
    LOG_ERROR("Couldn't get source file '%s' size: %s\n", filepath, strerror(errno));
    exit(1);
  }
  long src_size = ftell(f);
  if (src_size < 0) {
    LOG_ERROR("Couldn't get source file '%s' size: %s\n", filepath, strerror(errno));
    exit(1);
  }
  rewind(f);
  char *src = malloc(src_size);
  if (!src) {
    LOG_ERROR("Couldn't allocate space for source '%s': %s\n", filepath, strerror(errno));
    exit(1);
  }
  long read_res = fread(src, 1, src_size, f);
  if (read_res != src_size) {
    if (feof(f)) {
      LOG_ERROR("Couldn't read source '%s': Reached end of file\n", filepath);
    } else if (ferror(f)) {
      LOG_ERROR("Couldn't read source '%s'\n", filepath);
    } else {
      LOG_ERROR("Couldn't read source '%s': Unknown error\n", filepath);
    }
    exit(1);
  }
  fclose(f);
  out->size = src_size;
  out->data = src;
}

bool
is_delimiter(char c) {
  return c == ' ' || c == '\t' || c == '\n' || c == '\0';
}

bool
is_word_character(char c) {
  return c == '_' || c == 'a' || c == 'b' || c == 'c' || c == 'd' || c == 'e' || c == 'f' || c == 'g' || c == 'h' || c == 'i' || c == 'j' ||
         c == 'k' || c == 'l' || c == 'm' || c == 'n' || c == 'o' || c == 'p' || c == 'q' || c == 'r' || c == 's' || c == 't' || c == 'u' ||
         c == 'v' || c == 'w' || c == 'x' || c == 'y' || c == 'z' || c == 'A' || c == 'B' || c == 'C' || c == 'D' || c == 'E' || c == 'F' ||
         c == 'G' || c == 'H' || c == 'I' || c == 'J' || c == 'K' || c == 'L' || c == 'M' || c == 'N' || c == 'O' || c == 'P' || c == 'Q' ||
         c == 'R' || c == 'S' || c == 'T' || c == 'U' || c == 'V' || c == 'W' || c == 'X' || c == 'Y' || c == 'Z';
}


bool
is_number(char c) {
  return c == '0' || c == '1' || c == '2' || c == '3' || c == '4' || c == '5' || c == '6' || c == '7' || c == '8' || c == '9';
}

enum lex_state {
  LEX_ON_NONE = 0,
  LEX_ON_WORD,
  LEX_ON_NUMBER,
};

enum token_type {
  TOK_NONE = 0,
  TOK_IDENTIFIER,
  TOK_DEF,
  TOK_ASSIGN_CONST,
  TOK_ASSIGN_VAR,
  TOK_ASSIGN_BODY,
  TOK_FN,
  TOK_PAR_OPEN,
  TOK_PAR_CLOSE,
  TOK_SEMICOLON,
  TOK_CURLY_OPEN,
  TOK_CURLY_CLOSE,
  TOK_INT_LITERAL,
  TOK_PLUS,
  TOK_COMMA,
};

static inline const char *
token_type_string(enum token_type type) {
  switch (type) {
    case TOK_NONE: return "None";
    case TOK_IDENTIFIER: return "Identifier";
    case TOK_DEF: return "Def";
    case TOK_ASSIGN_CONST: return "Assign-const";
    case TOK_ASSIGN_VAR: return "Assign-var";
    case TOK_ASSIGN_BODY: return "Assign-body";
    case TOK_FN: return "Fn";
    case TOK_PAR_OPEN: return "Par-open";
    case TOK_PAR_CLOSE: return "Par-close";
    case TOK_SEMICOLON: return "Semicolon";
    case TOK_CURLY_OPEN: return "Curly-open";
    case TOK_CURLY_CLOSE: return "Curly-close";
    case TOK_INT_LITERAL: return "Int-literal";
    case TOK_PLUS: return "Plus";
    case TOK_COMMA: return "Comma";
  }
  return "Unknown";
}

struct position {
  uint32_t line;
  uint32_t character;
  const char *file;
};

struct token {
  struct position position;
  struct string string;
  enum token_type type;
};

struct token *
lex_source(const enum token_type *keywords, const char *file, const struct string *src) {
  struct mial_ptr tokens_ptr = mial_list_make(struct token, 0);
  if (tokens_ptr.err != MIAL_ERROR_OK) {
    LOG_ERROR("Couldn't create tokens list: %s\n", mial_error_string(tokens_ptr.err));
    exit(1);
  }
  struct token *tokens = tokens_ptr.val;
  struct string str;
  struct position cur_pos = { 1, 1, file };
  struct position pos = { 0 };
  enum lex_state state = LEX_ON_NONE;
  uint64_t i = 0;
  bool exit_from_loop = false;
  while (i < src->size && !exit_from_loop) {
    char c = src->data[i];
    char c_next = i + 1 < src->size ? src->data[i + 1] : '\0';
#define PASS_CHARACTER do { i++; cur_pos.character++; } while(0)
    switch (state) {
      case LEX_ON_NONE:
      {
        if (is_delimiter(c)) {
          if (c == '\n') {
            cur_pos.character = 0;
            cur_pos.line++;
          }
          if (c == '\0') exit_from_loop = true;
          PASS_CHARACTER;
        } else if (is_word_character(c)) {
          str.data = &src->data[i];
          str.size = 1;
          pos = cur_pos;
          state = LEX_ON_WORD;
          PASS_CHARACTER;
        } else if (is_number(c)) {
          str.data = &src->data[i];
          str.size = 1;
          pos = cur_pos;
          state = LEX_ON_NUMBER;
          PASS_CHARACTER;
        } else {
          switch (c) {
          case ':': mial_list_push_lit(tokens, cur_pos, { 0 }, TOK_ASSIGN_CONST); break;
          case '=':
            if (c_next == '>') {
              mial_list_push_lit(tokens, cur_pos, { 0 }, TOK_ASSIGN_BODY);
              PASS_CHARACTER;
            } else {
              mial_list_push_lit(tokens, cur_pos, { 0 }, TOK_ASSIGN_VAR);
            }
            break;
          case '(': mial_list_push_lit(tokens, cur_pos, { 0 }, TOK_PAR_OPEN); break;
          case ')': mial_list_push_lit(tokens, cur_pos, { 0 }, TOK_PAR_CLOSE); break;
          case ';': mial_list_push_lit(tokens, cur_pos, { 0 }, TOK_SEMICOLON); break;
          case '{': mial_list_push_lit(tokens, cur_pos, { 0 }, TOK_CURLY_OPEN); break;
          case '}': mial_list_push_lit(tokens, cur_pos, { 0 }, TOK_CURLY_CLOSE); break;
          case '+': mial_list_push_lit(tokens, cur_pos, { 0 }, TOK_PLUS); break;
          case ',': mial_list_push_lit(tokens, cur_pos, { 0 }, TOK_COMMA); break;
          default:
            LOG_ERROR("%s:%u:%u: '%c' isn't a valid token\n", cur_pos.file, cur_pos.line, cur_pos.character, c);
            exit(1);
          }
          PASS_CHARACTER;
        }
      } break;
      case LEX_ON_WORD:
      {
        if (is_word_character(c) || is_number(c)) {
          str.size++;
          PASS_CHARACTER;
        } else {
          struct mial_u32 keyword_index = mial_map_get_index_n(keywords, str.data, str.size);
          struct token token = {
            .position = pos,
            .string = str,
            .type = TOK_IDENTIFIER
          };
          if (keyword_index.err == MIAL_ERROR_OK) {
            token.type = keywords[keyword_index.val];
          } else if (keyword_index.err != MIAL_ERROR_DONT_EXISTS) {
            LOG_ERROR("Couldn't get keyword: %s\n", mial_error_string(keyword_index.err));
            exit(1);
          }
          mial_list_push_var(tokens, token);
          state = LEX_ON_NONE;
        }
      } break;
      case LEX_ON_NUMBER:
      {
        if (is_number(c)) {
          str.size++;
          PASS_CHARACTER;
        } else {
          mial_list_push_lit(tokens, pos, str, TOK_INT_LITERAL);
          state = LEX_ON_NONE;
        }
      } break;
    }
  }
#undef PASS_CHARACTER
  for (uint32_t i = 0; i < mial_list_size(tokens).val; i++) {
    printf("tokens[%u] = { %s, \"%.*s\" }\n", i, token_type_string(tokens[i].type), (int)tokens[i].string.size, tokens[i].string.data);
  }
  return tokens;
}

enum ast_node_type {
  AST_NONE = 0,
  AST_IDENTIFIER,
  AST_DEF_CONST,
  AST_DEF_VAR,
  AST_ASSIGN,
  AST_FN,
  AST_BIN_OPERATOR,
  AST_INT_LITERAL,
};

static inline const char *
ast_node_type_string(enum ast_node_type ast_node_type) {
  switch (ast_node_type) {
    case AST_NONE: return "None";
    case AST_IDENTIFIER: return "Identifier";
    case AST_DEF_CONST: return "Def-const";
    case AST_DEF_VAR: return "Def-var";
    case AST_ASSIGN: return "Assign";
    case AST_FN: return "Fn";
    case AST_INT_LITERAL: return "Int-literal";
    case AST_BIN_OPERATOR: return "Bin-operator";
  }
  return "Unknown";
}

enum operator {
  OP_NONE = 0,
  OP_ADD,
  OP_SUB,
  OP_MUL,
  OP_DIV,
};

static inline const char *
operation_string(enum operation operation) {
  switch (operation) {
    case OP_NONE: return "None";
    case OP_ADD: return "Add";
    case OP_SUB: return "Sub";
    case OP_MUL: return "Mul";
    case OP_DIV: return "Div";
  }
  return "Unknown";
}

// def <identifier> <=|:> <expression|type>
// fn(<args>) => <body>
// <identifier|address> = <expression>
// <expression> + <expression>

struct ast_node {
  struct ast_node *root;
  struct ast_node **children;
  uint32_t children_amount;
  enum ast_node_type type;
  union {
    struct string identifier;
    int64_t int_literal;
    struct {
      enum binary_operator type;
      uint32_t precedence;
    } operator;
  };
};

struct parser {
  struct ast_node program;
};

int
main(int argc, const char *argv[]) {
  if (argc < 2) {
    LOG_INFO("Usage:\n");
    LOG_INFO("  stark <file>\n");
    return 1;
  }
  enum token_type *keywords;
  {
    struct mial_ptr keywords_ptr = mial_map_make(enum token_type, 0);
    if (keywords_ptr.err != MIAL_ERROR_OK) {
      LOG_ERROR("Couldn't create keywords map: %s\n", mial_error_string(keywords_ptr.err));
      return -1;
    }
    keywords = keywords_ptr.val;
    enum mial_error err;
    if ((err = mial_map_set_lit(keywords, "def", TOK_DEF)) != MIAL_ERROR_OK) {
      LOG_ERROR("Couldn't setup keyword 'def'\n");
      return -1;
    }
    if ((err = mial_map_set_lit(keywords, "fn", TOK_FN)) != MIAL_ERROR_OK) {
      LOG_ERROR("Couldn't setup keyword 'fn'\n");
      return -1;
    }
  }
  const char *filepath = argv[1];
  struct string src;
  get_source_from_file(&src, filepath);
  struct token *tokens = lex_source(keywords, filepath, &src);
  enum mial_error err = mial_list_destroy(tokens);
  if (err != MIAL_ERROR_OK) {
    LOG_ERROR("Couldn't destroy tokens list: %s\n", mial_error_string(err));
    return -1;
  }
  free((char *)src.data);
  return 0;
}
