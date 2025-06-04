#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <errno.h>
#include <assert.h>
#include "mial.h"

#define LOG_INFO(MSG, ...)  fprintf(stderr, "\x1b[1;32m[INFO] "  MSG "\x1b[0m", ## __VA_ARGS__)
#define LOG_WARN(MSG, ...)  fprintf(stderr, "\x1b[1;33m[WARN] "  MSG "\x1b[0m", ## __VA_ARGS__)
#define LOG_ERROR(MSG, ...) fprintf(stderr, "\x1b[1;31m[ERROR] " MSG "\x1b[0m", ## __VA_ARGS__)
#define LOG_TODO(MSG, ...)  fprintf(stderr, "\x1b[1;34m[TODO] "  MSG "\x1b[0m", ## __VA_ARGS__)

struct string {
  uint64_t size;
  const char *data;
};

void
get_source(struct string *out, const char *filepath) {
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
  LEX_ON_COMMENT,
  LEX_ON_MULTILINE_COMMENT,
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
  TOK_MINUS,
  TOK_DIV,
  TOK_STAR,
  TOK_COMMA,
  TOK_MODULE,
  TOK_POWER,
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
    case TOK_MINUS: return "Minus";
    case TOK_DIV: return "Div";
    case TOK_STAR: return "Star";
    case TOK_COMMA: return "Comma";
    case TOK_MODULE: return "Module";
    case TOK_POWER: return "Power";
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
      case LEX_ON_NONE: {
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
            case ':': mial_list_push_lit(tokens, cur_pos, { 1, ":" }, TOK_ASSIGN_CONST); break;
            case '=':
              if (c_next == '>') {
                mial_list_push_lit(tokens, cur_pos, { 2, "=>" }, TOK_ASSIGN_BODY);
                PASS_CHARACTER;
              } else {
                mial_list_push_lit(tokens, cur_pos, { 1, "=" }, TOK_ASSIGN_VAR);
              }
              break;
            case '(': mial_list_push_lit(tokens, cur_pos, { 1, "(" }, TOK_PAR_OPEN); break;
            case ')': mial_list_push_lit(tokens, cur_pos, { 1, ")" }, TOK_PAR_CLOSE); break;
            case ';': mial_list_push_lit(tokens, cur_pos, { 1, ";" }, TOK_SEMICOLON); break;
            case '{': mial_list_push_lit(tokens, cur_pos, { 1, "{" }, TOK_CURLY_OPEN); break;
            case '}': mial_list_push_lit(tokens, cur_pos, { 1, "}" }, TOK_CURLY_CLOSE); break;
            case '+': mial_list_push_lit(tokens, cur_pos, { 1, "+" }, TOK_PLUS); break;
            case '-': mial_list_push_lit(tokens, cur_pos, { 1, "-" }, TOK_MINUS); break;
            case '/': mial_list_push_lit(tokens, cur_pos, { 1, "/" }, TOK_DIV); break;
            case '*': mial_list_push_lit(tokens, cur_pos, { 1, "*" }, TOK_STAR); break;
            case '^': mial_list_push_lit(tokens, cur_pos, { 1, "^" }, TOK_POWER); break;
            case ',': mial_list_push_lit(tokens, cur_pos, { 1, "," }, TOK_COMMA); break;
            case '#':
              if (c_next == '(') {
                state = LEX_ON_MULTILINE_COMMENT;
                pos = cur_pos;
              } else {
                state = LEX_ON_COMMENT;
              }
              break;
            default:
              LOG_ERROR("%s:%u:%u: '%c' isn't a valid token\n", cur_pos.file, cur_pos.line, cur_pos.character, c);
              exit(1);
          }
          PASS_CHARACTER;
        }
      } break;
      case LEX_ON_WORD: {
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
      case LEX_ON_NUMBER: {
        if (is_number(c)) {
          str.size++;
          PASS_CHARACTER;
        } else {
          mial_list_push_lit(tokens, pos, str, TOK_INT_LITERAL);
          state = LEX_ON_NONE;
        }
      } break;
      case LEX_ON_COMMENT: {
        if (c == '\n') state = LEX_ON_NONE;
        else PASS_CHARACTER;
      } break;
      case LEX_ON_MULTILINE_COMMENT: {
        if (c_next == '\0') {
          LOG_ERROR("%s:%u:%u: Unclose multiline comment\n", pos.file, pos.line, pos.character);
          exit(1);
        } else if (c == '\n') {
          cur_pos.character = 0;
          cur_pos.line++;
        } else if (c == ')' && c_next == '#') {
          state = LEX_ON_NONE;
          PASS_CHARACTER;
        }
        PASS_CHARACTER;
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
  AST_MODULE,
  AST_IDENTIFIER,
  AST_DEF_CONST,
  AST_DEF_VAR,
  AST_FN,
  AST_FN_CALL,
  AST_ASSIGN,
  AST_BIN_OPERATOR,
  AST_INT_LITERAL,
  AST_BLOCK,
  AST_EXPRESSION,
  AST_OPERATION,
};

static inline const char *
ast_node_type_string(enum ast_node_type ast_node_type) {
  switch (ast_node_type) {
    case AST_NONE: return "None";
    case AST_MODULE: return "Module";
    case AST_IDENTIFIER: return "Identifier";
    case AST_DEF_CONST: return "Def-const";
    case AST_DEF_VAR: return "Def-var";
    case AST_FN: return "Fn";
    case AST_FN_CALL: return "Fn-call";
    case AST_ASSIGN: return "Assign";
    case AST_INT_LITERAL: return "Int-literal";
    case AST_BIN_OPERATOR: return "Bin-operator";
    case AST_BLOCK: return "Block";
    case AST_EXPRESSION: return "Expression";
    case AST_OPERATION: return "Operation";
  }
  return "Unknown";
}

enum operation {
  OP_NONE = 0,
  OP_ASSIGN,
  OP_PLUS,
  OP_MINUS,
  OP_MUL,
  OP_DIV,
  OP_POWER,
  OP_COUNT,
};

static inline const char *
operation_string(enum operation operation) {
  switch (operation) {
    case OP_NONE: return "None";
    case OP_ASSIGN: return "Assign";
    case OP_POWER: return "Power";
    case OP_PLUS: return "Plus";
    case OP_MINUS: return "Minus";
    case OP_MUL: return "Mul";
    case OP_DIV: return "Div";
    case OP_COUNT: break;
  }
  return "Unknown";
}

static inline const char *
operation_symbol(enum operation operation) {
  switch (operation) {
    case OP_NONE: return "???";
    case OP_ASSIGN: return "=";
    case OP_POWER: return "^";
    case OP_PLUS: return "+";
    case OP_MINUS: return "-";
    case OP_MUL: return "*";
    case OP_DIV: return "/";
    case OP_COUNT: break;
  }
  return "Unknown";
}

enum precedence {
  PRE_NONE = 0,
  PRE_ASSIGN,
  PRE_PLUS,
  PRE_MINUS = PRE_PLUS,
  PRE_MUL,
  PRE_DIV = PRE_MUL,
  PRE_POWER,
  PRE_VALUE,
};

struct ast_node {
  int64_t root;
  int64_t self;
  int64_t *children;
  struct {
    struct operator {
      enum precedence precedence;
      enum operation type;
    } operator;
    union {
      struct string identifier;
      int64_t int_literal;
    };
  } data;
  struct position position;
  enum ast_node_type type;
  bool is_expression;
};

void
print_ast(const struct ast_node *ast, int64_t node, int self_indent, int block_indent, bool format_expressions, bool format_values) {
  switch (ast[node].type) {
    case AST_NONE: {
      assert(0 && "AST_NONE");
    } break;
    case AST_MODULE: {
      printf("%*sModule '%.*s' {\n", self_indent * 2, "", (int)ast[node].data.identifier.size, ast[node].data.identifier.data);
      for (uint32_t i = 0; i < mial_list_size(ast[node].children).val; i++) {
        print_ast(ast, ast[node].children[i], block_indent + 1, block_indent + 1, format_expressions, format_values);
      }
      printf("%*s} Module '%.*s',\n", block_indent * 2, "", (int)ast[node].data.identifier.size, ast[node].data.identifier.data);
    } break;
    case AST_DEF_CONST: {
      printf("%*sConst %.*s = ", self_indent * 2, "", (int)ast[node].data.identifier.size, ast[node].data.identifier.data);
      for (uint32_t i = 0; i < mial_list_size(ast[node].children).val; i++) {
        int new_indent = block_indent + 1;
        if (ast[ast[node].children[i]].type == AST_EXPRESSION) {
          new_indent = 0;
          printf(",\n");
        }
        print_ast(ast, ast[node].children[i], 0, new_indent, format_expressions, format_values);
      }
    } break;
    case AST_DEF_VAR: {
      printf("%*sVar %.*s = ", self_indent * 2, "", (int)ast[node].data.identifier.size, ast[node].data.identifier.data);
      for (uint32_t i = 0; i < mial_list_size(ast[node].children).val; i++) {
        int new_indent = block_indent + 1;
        if (ast[ast[node].children[i]].type == AST_EXPRESSION) {
          new_indent = 0;
          printf(",\n");
        }
        print_ast(ast, ast[node].children[i], 0, new_indent, format_expressions, format_values);
      }
    } break;
    case AST_FN: {
      printf("%*sFn {\n", self_indent * 2, "");
      for (uint32_t i = 0; i < mial_list_size(ast[node].children).val; i++) {
        print_ast(ast, ast[node].children[i], block_indent + 1, block_indent + 1, format_expressions, format_values);
        printf("\n");
      }
      printf("%*s} Fn,\n", block_indent * 2, "");
    } break;
    case AST_BLOCK: {
      printf("%*sBlock {\n", self_indent * 2, "");
      for (uint32_t i = 0; i < mial_list_size(ast[node].children).val; i++) {
        print_ast(ast, ast[node].children[i], block_indent + 1, block_indent + 1, format_expressions, format_values);
        printf(",\n");
      }
      printf("%*s} Block,", block_indent * 2, "");
    } break;
    case AST_EXPRESSION: {
      if (format_expressions) {
        printf("%*sExpr(", self_indent * 2, "");
      } else {
        printf("%*s(", self_indent * 2, "");
      }
      for (uint32_t i = 0; i < mial_list_size(ast[node].children).val; i++) {
        print_ast(ast, ast[node].children[i], 0, 0, format_expressions, format_values);
      }
      printf(")");
    } break;
    case AST_OPERATION: {
      if (format_expressions) {
        printf("%s(", operation_string(ast[node].data.operator.type));
      } else {
        printf("(");
      }
      for (uint32_t i = 0; i < mial_list_size(ast[node].children).val; i++) {
        print_ast(ast, ast[node].children[i], 0, 0, format_expressions, format_values);
        if (i < mial_list_size(ast[node].children).val - 1) {
          if (format_expressions) {
            printf(", ");
          } else {
            printf(" %s ", operation_symbol(ast[node].data.operator.type));
          }
        }
      }
      printf(")");
    } break;
    case AST_INT_LITERAL: {
      if (format_values) {
        printf("Int(%ld)", ast[node].data.int_literal);
      } else {
        printf("%ld", ast[node].data.int_literal);
      }
    } break;
    case AST_IDENTIFIER: {
      if (format_values) {
        printf("Id(%.*s)", (int)ast[node].data.identifier.size, ast[node].data.identifier.data);
      } else {
        printf("%.*s", (int)ast[node].data.identifier.size, ast[node].data.identifier.data);
      }
    } break;
    case AST_FN_CALL: {
      assert(0 && "AST_FN_CALL");
    } break;
    case AST_ASSIGN: {
      assert(0 && "AST_ASSIGN");
    } break;
    case AST_BIN_OPERATOR: {
      assert(0 && "AST_BIN_OPERATOR");
    } break;
  }
}

int64_t
ast_node_make(struct ast_node **past, int64_t root, enum ast_node_type type, bool has_children, const struct position *position) {
  struct ast_node *ast = *past;
  enum mial_error err = mial_list_grow(ast, 1, true);
  *past = ast;
  if (err != MIAL_ERROR_OK) {
    LOG_ERROR("Couldn't create ast_node: %s\n", mial_error_string(err));
    exit(1);
  }
  int64_t index = mial_list_size(ast).val - 1;
  if (root != -1 && ast[root].children) {
    enum mial_error err = mial_list_grow(ast[root].children, 1, false);
    if (err != MIAL_ERROR_OK) {
      LOG_ERROR("Couldn't push new ast_node to root: %s\n", mial_error_string(err));
      exit(1);
    }
    ast[root].children[mial_list_size(ast[root].children).val - 1] = index;
  }
  struct ast_node *out = &ast[index];
  if (has_children) {
    struct mial_ptr children_ptr = mial_list_make(int64_t, 0);
    if (children_ptr.err != MIAL_ERROR_OK) {
      LOG_ERROR("Couldn't create ast_node: %s\n", mial_error_string(children_ptr.err));
      exit(1);
    }
    out->children = children_ptr.val;
  }
  out->root = root;
  out->self = index;
  out->type = type;
  if (position) out->position = *position;
  return index;
}

void
ast_node_change_root(struct ast_node *ast, int64_t node, int64_t root) {
  if (ast[node].root == root) return;
  for (uint32_t i = 0; ast[node].root != -1 && i < mial_list_size(ast[ast[node].root].children).val; i++) {
    if (ast[ast[node].root].children[i] != node) continue;
    mial_list_remove(ast[ast[node].root].children, i);
    break;
  }
  if (root != -1 && ast[root].children) {
    enum mial_error err = mial_list_grow(ast[root].children, 1, false);
    if (err != MIAL_ERROR_OK) {
      LOG_ERROR("Couldn't push ast_node to root: %s\n", mial_error_string(err));
      exit(1);
    }
    ast[root].children[mial_list_size(ast[root].children).val - 1] = node;
  }
  ast[node].root = root;
}

#define GET_NEXT_TOKEN(EXPECTED) do { \
  if (i + 1 >= tokens_amount) { \
    LOG_ERROR("%s:%u:%u Expected %s, but got end of file \n", tokens[i].position.file, tokens[i].position.line, \
        tokens[i].position.character, EXPECTED); \
    exit(1); \
  }\
  i++; \
} while(0)
#define CHECK_TOKEN_TYPE(TOKEN_TYPE, EXPECTED) do { \
  if (tokens[i].type != TOKEN_TYPE) { \
    LOG_ERROR("%s:%u:%u Expected %s, but got '%.*s' \n", tokens[i].position.file, tokens[i].position.line, \
        tokens[i].position.character, EXPECTED, (int)tokens[i].string.size, tokens[i].string.data); \
    exit(1); \
  } \
} while(0)
#define GET_NEXT_TOKEN_AND_CHECK_TYPE(TOKEN_TYPE, EXPECTED) do { GET_NEXT_TOKEN(EXPECTED); CHECK_TOKEN_TYPE(TOKEN_TYPE, EXPECTED); } while (0)

void
token_to_operator(struct operator *op, enum token_type token_type) {
  static_assert(OP_COUNT == 7);
  switch (token_type) {
    case TOK_ASSIGN_VAR:
      op->precedence = PRE_ASSIGN;
      op->type       = OP_ASSIGN;
      break;
    case TOK_PLUS:
      op->precedence = PRE_PLUS;
      op->type       = OP_PLUS;
      break;
    case TOK_MINUS:
      op->precedence = PRE_MINUS;
      op->type       = OP_MINUS;
      break;
    case TOK_DIV:
      op->precedence = PRE_DIV;
      op->type       = OP_DIV;
      break;
    case TOK_STAR:
      op->precedence = PRE_MUL;
      op->type       = OP_MUL;
      break;
    case TOK_POWER:
      op->precedence = PRE_POWER;
      op->type       = OP_POWER;
      break;
    default:
      assert(0 && "unreachable");
      break;
  }
}

int64_t ast_expression_make(struct ast_node **past, int64_t root, const struct token *tokens, uint32_t *pi, bool has_parenthesis);

void
ast_expression_node_make(struct ast_node **past, int64_t root, const struct token *tokens, uint32_t *pi, bool has_parenthesis) {
  struct ast_node *ast = *past;
  uint32_t i = *pi;
  uint32_t tokens_amount = mial_list_size(tokens).val;
#if 0
  print_ast(ast, 0, 0, 0, true, false);
  printf("\n");
#endif
  switch (tokens[i].type) {
    case TOK_SEMICOLON: {
      if (has_parenthesis) {
        LOG_ERROR("%s:%u:%u Expected expression, but got ';'. Forgot ')'?\n", ast[root].position.file, ast[root].position.line, ast[root].position.character);
        exit(1);
      } else if (ast[root].type != AST_IDENTIFIER && ast[root].type != AST_INT_LITERAL && ast[root].type != AST_EXPRESSION) {
        LOG_ERROR("%s:%u:%u Expected expression, but got ';'\n", ast[root].position.file, ast[root].position.line, ast[root].position.character);
        exit(1);
      }
    } break;
    case TOK_PAR_CLOSE: {
      if (!has_parenthesis) {
        LOG_ERROR("%s:%u:%u Expected expression, but got ')'\n", ast[root].position.file, ast[root].position.line, ast[root].position.character);
        exit(1);
      } else if (ast[root].type != AST_IDENTIFIER && ast[root].type != AST_INT_LITERAL && ast[root].type != AST_EXPRESSION) {
        LOG_ERROR("%s:%u:%u Expected expression, but got ')'\n", ast[root].position.file, ast[root].position.line, ast[root].position.character);
        exit(1);
      }
    } break;
    case TOK_PAR_OPEN: {
      GET_NEXT_TOKEN("')'");
      int64_t index = ast_expression_make(&ast, root, tokens, &i, true);
      i++;
      ast_expression_node_make(&ast, index, tokens, &i, has_parenthesis);
    } break;
    case TOK_INT_LITERAL:
    case TOK_IDENTIFIER: {
      int64_t index;
      if (tokens[i].type == TOK_INT_LITERAL) {
        index = ast_node_make(&ast, root, AST_INT_LITERAL, false, &tokens[i].position);
        ast[index].data.int_literal = strtoll(tokens[i].string.data, 0, 10);
      } else {
        index = ast_node_make(&ast, root, AST_IDENTIFIER, false, &tokens[i].position);
        ast[index].data.identifier = tokens[i].string;
      }
      ast[index].data.operator.precedence = PRE_VALUE;
      GET_NEXT_TOKEN(has_parenthesis ? "')'" : "';'");
      ast_expression_node_make(&ast, index, tokens, &i, has_parenthesis);
    } break;
    static_assert(OP_COUNT == 7);
    case TOK_ASSIGN_VAR:
    case TOK_POWER:
    case TOK_PLUS:
    case TOK_MINUS:
    case TOK_DIV:
    case TOK_STAR: {
      if (ast[root].type != AST_IDENTIFIER && ast[root].type != AST_INT_LITERAL && ast[root].type != AST_EXPRESSION) {
        LOG_ERROR("%s:%u:%u Expected identifier, expression or literal, but got '%s'\n", ast[root].position.file, ast[root].position.line,
            ast[root].position.character, ast_node_type_string(ast[root].type));
        exit(1);
      }
      struct operator op;
      token_to_operator(&op, tokens[i].type);
      while (ast[ast[root].root].type == AST_OPERATION && ast[ast[root].root].data.operator.precedence >= op.precedence) {
        root = ast[root].root;
      }
      int64_t index = ast_node_make(&ast, root, AST_OPERATION, true, &tokens[i].position);
      ast[index].data.operator = op;
      ast_node_change_root(ast, index, ast[root].root);
      ast_node_change_root(ast, root, index);
      GET_NEXT_TOKEN("expression");
      ast_expression_node_make(&ast, index, tokens, &i, has_parenthesis);
    } break;
    default: {
      LOG_ERROR("%s:%u:%u '%.*s' is invalid on a expression. Forgot '%c'?\n", tokens[i].position.file, tokens[i].position.line,
          tokens[i].position.character, (int)tokens[i].string.size, tokens[i].string.data, has_parenthesis ? ')' : ';');
        exit(1);
    } break;
  }
  *pi = i;
  *past = ast;
}

int64_t
ast_expression_make(struct ast_node **past, int64_t root, const struct token *tokens, uint32_t *pi, bool has_parenthesis) {
  int64_t index = ast_node_make(past, root, AST_EXPRESSION, true, &tokens[*pi].position);
  (*past)[index].data.operator.precedence = PRE_VALUE;
  ast_expression_node_make(past, index, tokens, pi, has_parenthesis);
  assert(mial_list_size((*past)[index].children).val <= 1);
  return index;
}

void
ast_statement_make(struct ast_node **past, int64_t root, const struct token *tokens, uint32_t *pi) {
  (void)root;
  struct ast_node *ast = *past;
  uint32_t i = *pi;
  uint32_t tokens_amount = mial_list_size(tokens).val;
  switch (tokens[i].type) {
    case TOK_SEMICOLON: {
    } break;
    case TOK_DEF: {
      GET_NEXT_TOKEN_AND_CHECK_TYPE(TOK_IDENTIFIER, "identifier");
      uint32_t identifier_index = i;
      GET_NEXT_TOKEN("':' or '='");
      int64_t index;
      switch (tokens[i].type) {
        case TOK_ASSIGN_VAR: {
          index = ast_node_make(&ast, root, AST_DEF_VAR, true, &tokens[i].position);
        } break;
        case TOK_ASSIGN_CONST: {
          index = ast_node_make(&ast, root, AST_DEF_CONST, true, &tokens[i].position);
        } break;
        default: {
          LOG_ERROR("%s:%u:%u Expected ':' or '=', but got '%.*s'\n", tokens[i].position.file,
              tokens[i].position.line, tokens[i].position.character, (int)tokens[i].string.size, tokens[i].string.data);
          exit(1);
        } break;
      }
      GET_NEXT_TOKEN("expression");
      if (tokens[i].type != TOK_FN && tokens[i].type != TOK_IDENTIFIER && tokens[i].type != TOK_INT_LITERAL) {
        LOG_ERROR("%s:%u:%u Invalid rvalue '%.*s'\n", tokens[i].position.file,
            tokens[i].position.line, tokens[i].position.character, (int)tokens[i].string.size, tokens[i].string.data);
        exit(1);
      }
      ast[index].data.identifier = tokens[identifier_index].string;
      ast_statement_make(&ast, index, tokens, &i);
    } break;
    case TOK_FN: {
      const struct position *position = &tokens[i].position;
      GET_NEXT_TOKEN_AND_CHECK_TYPE(TOK_PAR_OPEN, "'('");
      LOG_TODO("Function parameters\n");
      GET_NEXT_TOKEN_AND_CHECK_TYPE(TOK_PAR_CLOSE, "')'");
      LOG_TODO("Function return type\n");
      GET_NEXT_TOKEN_AND_CHECK_TYPE(TOK_ASSIGN_BODY, "'=>'");
      GET_NEXT_TOKEN("statement");
      int64_t index = ast_node_make(&ast, root, AST_FN, true, position);
      ast_statement_make(&ast, index, tokens, &i);
    } break;
    case TOK_INT_LITERAL:
    case TOK_IDENTIFIER:
    case TOK_PAR_OPEN: {
      ast_expression_make(&ast, root, tokens, &i, false);
    } break;
    case TOK_CURLY_OPEN: {
      int64_t index = ast_node_make(&ast, root, AST_BLOCK, true, &tokens[i].position);
      for (;;) {
        GET_NEXT_TOKEN("'}'");
        if (tokens[i].type == TOK_CURLY_CLOSE) break;
        ast_statement_make(&ast, index, tokens, &i);
      }
    } break;
    default: {
      LOG_ERROR("%s:%u:%u '%.*s' is an invalid start for a statement \n", tokens[i].position.file, tokens[i].position.line,
          tokens[i].position.character, (int)tokens[i].string.size, tokens[i].string.data);
        exit(1);
    } break;
  }
  *pi = i;
  *past = ast;
}

struct ast_node *
parse_tokens(const struct token *tokens) {
  struct mial_ptr ast_ptr = mial_list_make(struct ast_node, 0);
  if (ast_ptr.err != MIAL_ERROR_OK) {
    LOG_ERROR("Couldn't create ast: %s\n", mial_error_string(ast_ptr.err));
    exit(1);
  }
  struct ast_node *ast = ast_ptr.val;
  int64_t current_node = ast_node_make(&ast, -1, AST_MODULE, true, 0);
  uint32_t tokens_amount = mial_list_size(tokens).val;
  for (uint32_t i = 0; i < tokens_amount; i++) {
    switch (ast[current_node].type) {
      case AST_MODULE: {
        switch (tokens[i].type) {
          case TOK_MODULE: {
            if (ast[current_node].data.identifier.data) {
              LOG_ERROR("%s:%u:%u Module name already defined\n", tokens[i].position.file, tokens[i].position.line, tokens[i].position.character);
              exit(1);
            }
            GET_NEXT_TOKEN_AND_CHECK_TYPE(TOK_IDENTIFIER, "identifier");
            ast[current_node].data.identifier = tokens[i].string;
            ast[current_node].position = tokens[i].position;
            GET_NEXT_TOKEN_AND_CHECK_TYPE(TOK_SEMICOLON, "';'");
          } break;
          case TOK_DEF: {
            ast_statement_make(&ast, current_node, tokens, &i);
          } break;
          default: {
            LOG_ERROR("%s:%u:%u '%.*s' isn't valid on module scope\n", tokens[i].position.file, tokens[i].position.line,
                tokens[i].position.character, (int)tokens[i].string.size, tokens[i].string.data);
            exit(1);
          } break;
        }
      } break;
      default: {
        LOG_ERROR("%s:%u:%u Unexpected ast_node: %s\n", tokens[i].position.file, tokens[i].position.line,
            tokens[i].position.character, ast_node_type_string(ast[current_node].type));
        exit(1);
      }
    }
  }
  print_ast(ast, 0, 0, 0, true, false);
  return ast;
}

#undef GET_NEXT_TOKEN
#undef CHECK_TOKEN_TYPE
#undef GET_NEXT_TOKEN_AND_CHECK_TYPE

enum ir_instruction_type {
  INST_NONE = 0,
  INST_DEF_FN,
  INST_DEF_VAR,
  INST_RET,
};

#define IR_INSTRUCTION_ARGS_MAX 3
struct ir_instrunction {
  enum ir_instruction_type type;
  union {
    int64_t  i[IR_INSTRUCTION_ARGS_MAX];
    uint64_t u[IR_INSTRUCTION_ARGS_MAX];
  } args;
};

struct type {
  uint64_t size;
};

struct function {
  uint64_t id;
};

struct variable {
  uint64_t id;
  uint64_t scope;
  struct string type;
};

enum constant_type {
  CONST_NONE = 0,
  CONST_VALUE,
  CONST_FN,
};

struct constant {
  enum constant_type type;
  uint64_t scope;
  union {
    uint64_t fn;
    struct {
      struct string type;
// "TODO: add the actual values to constants"
    } value;
  };
};

enum scope_type {
  SCOPE_NONE = 0,
  SCOPE_MODULE,
  SCOPE_BLOCK,
};

struct scope {
  struct variable *variables;
  struct constant *constants;
  struct scope *sub_scopes;
  enum scope_type type;
};

struct ir {
  struct function *functions;
  struct type *types;
  struct scope module;
};

void
ir_scope_make(struct scope *out, enum scope_type type, const struct ast_node *ast) {
  out->type = type;
  struct mial_ptr ptr = mial_map_make(struct );
  out->variables = 
};

void
generate_ir(struct ir *ir, const struct ast_node *ast) {
  struct mial_ptr ptr = mial_list_make(struct function, 0);
  if (ptr.err != MIAL_ERROR_OK) {
    LOG_ERROR("Couldn't create functions list: %s\n", mial_error_string(ptr.err));
    exit(1);
  }
  ir->functions = ptr.val;
  ptr = mial_map_make(struct type, 0);
  if (ptr.err != MIAL_ERROR_OK) {
    LOG_ERROR("Couldn't create types map: %s\n", mial_error_string(ptr.err));
  }
  ir->types = ptr.val;
  ir_scope_make();
}

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
#define DEFINE_KEYWORD(KEYWORD, TOKEN) do {\
  if ((err = mial_map_set_lit(keywords, KEYWORD, TOKEN)) != MIAL_ERROR_OK) {\
    LOG_ERROR("Couldn't setup keyword '%s'\n", KEYWORD);\
    return -1;\
  }\
} while(0)
    DEFINE_KEYWORD("def", TOK_DEF);
    DEFINE_KEYWORD("fn", TOK_FN);
    DEFINE_KEYWORD("module", TOK_MODULE);
#undef DEFINE_KEYWORD
  }
  const char *filepath = argv[1];
  struct string src;
  get_source(&src, filepath);
  struct token *tokens = lex_source(keywords, filepath, &src);
  struct ast_node *ast = parse_tokens(tokens);
  struct ir ir;
  generate_ir(&ir, ast);
  LOG_TODO("generate_fasm_x86_64_linux(ir)\n");
  {
    enum mial_error err;
    if ((err = mial_list_destroy(tokens)) != MIAL_ERROR_OK) LOG_ERROR("Couldn't destroy tokens list: %s\n", mial_error_string(err));
    if ((err = mial_list_destroy(ast)) != MIAL_ERROR_OK) LOG_ERROR("Couldn't destroy ast: %s\n", mial_error_string(err));
  }
  free((char *)src.data);
  return 0;
}
