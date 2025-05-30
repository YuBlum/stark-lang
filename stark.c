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
  TOK_COMMA,
  TOK_MODULE,
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
    case TOK_MODULE: return "Module";
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
operator_string(enum operator operation) {
  switch (operation) {
    case OP_NONE: return "None";
    case OP_ADD: return "Add";
    case OP_SUB: return "Sub";
    case OP_MUL: return "Mul";
    case OP_DIV: return "Div";
  }
  return "Unknown";
}

struct ast_node {
  int64_t root;
  int64_t self;
  int64_t *children;
  enum ast_node_type type;
  union {
    struct string identifier;
    int64_t int_literal;
    struct {
      enum operator type;
      uint32_t precedence;
    } operator;
  } data;
};

int64_t
ast_node_make(struct ast_node **past, int64_t root, enum ast_node_type type, bool has_children) {
  struct ast_node *ast = *past;
  enum mial_error err = mial_list_grow(ast, 1, true);
  *past = ast;
  if (err != MIAL_ERROR_OK) {
    LOG_ERROR("Couldn't create ast_node: %s\n", mial_error_string(err));
    exit(1);
  }
  int64_t index = mial_list_size(ast).val - 1;
  if (root != -1) {
    enum mial_error err = mial_list_grow(ast[root].children, 1, false);
    if (err != MIAL_ERROR_OK) {
      LOG_ERROR("Couldn't push new ast_node to root: %s\n", mial_error_string(err));
      exit(1);
    }
    ast[root].children[mial_list_size(ast[root].children).val - 1] = index;
  }
  struct ast_node *out = &ast[index];
  out->root = root;
  out->self = index;
  out->type = type;
  if (has_children) {
    struct mial_ptr children_ptr = mial_list_make(int64_t, 0);
    if (children_ptr.err != MIAL_ERROR_OK) {
      LOG_ERROR("Couldn't create ast_node: %s\n", mial_error_string(children_ptr.err));
      exit(1);
    }
    out->children = children_ptr.val;
  }
  return index;
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
          index = ast_node_make(&ast, root, AST_DEF_VAR, true);
        } break;
        case TOK_ASSIGN_CONST: {
          index = ast_node_make(&ast, root, AST_DEF_CONST, true);
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
      GET_NEXT_TOKEN_AND_CHECK_TYPE(TOK_PAR_OPEN, "'('");
      LOG_TODO("Function parameters\n");
      GET_NEXT_TOKEN_AND_CHECK_TYPE(TOK_PAR_CLOSE, "')'");
      LOG_TODO("Function return type\n");
      GET_NEXT_TOKEN_AND_CHECK_TYPE(TOK_ASSIGN_BODY, "'=>'");
      GET_NEXT_TOKEN("statement");
      int64_t index = ast_node_make(&ast, root, AST_FN, true);
      ast_statement_make(&ast, index, tokens, &i);
    } break;
    case TOK_INT_LITERAL: {
      int64_t index = ast_node_make(&ast, root, AST_INT_LITERAL, false);
      ast[index].data.int_literal = strtoll(tokens[i].string.data, 0, 10);
      GET_NEXT_TOKEN("';'");
      switch (tokens[i].type) {
        case TOK_SEMICOLON: {
        } break;
        default: {
          LOG_ERROR("%s:%u:%u Expected ';', but got '%.*s'\n", tokens[i].position.file,
              tokens[i].position.line, tokens[i].position.character, (int)tokens[i].string.size, tokens[i].string.data);
          exit(1);
        } break;
      }
    } break;
    case TOK_IDENTIFIER: {
      uint32_t identifier = i;
      GET_NEXT_TOKEN("';'");
      bool func_call = false;
      switch (tokens[i].type) {
        case TOK_SEMICOLON: {
        } break;
        case TOK_PAR_OPEN: {
          func_call = true;
        } break;
        default: {
          LOG_ERROR("%s:%u:%u Expected ';', but got '%.*s'\n", tokens[i].position.file,
              tokens[i].position.line, tokens[i].position.character, (int)tokens[i].string.size, tokens[i].string.data);
          exit(1);
        } break;
      }
      if (func_call) {
        int64_t index = ast_node_make(&ast, root, AST_FN_CALL, true);
        ast[index].data.identifier = tokens[identifier].string;
        ast_statement_make(&ast, index, tokens, &i);
      }
      int64_t index = ast_node_make(&ast, root, AST_IDENTIFIER, false);
      ast[index].data.identifier = tokens[identifier].string;
    } break;
    case TOK_CURLY_OPEN: {
      int64_t index = ast_node_make(&ast, root, AST_BLOCK, true);
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

void
print_ast(const struct ast_node *ast, int64_t node, int indent) {
  (void)indent;
  switch (ast[node].type) {
    case AST_NONE: {
      assert(0 && "AST_NONE");
    } break;
    case AST_MODULE: {
      printf("%*sModule '%.*s' {\n", indent * 2, "", (int)ast[node].data.identifier.size, ast[node].data.identifier.data);
      for (uint32_t i = 0; i < mial_list_size(ast[node].children).val; i++) {
        print_ast(ast, ast[node].children[i], indent + 1);
      }
      printf("%*s},\n", indent * 2, "");
    } break;
    case AST_DEF_CONST: {
      printf("%*sConst '%.*s' {\n", indent * 2, "", (int)ast[node].data.identifier.size, ast[node].data.identifier.data);
      for (uint32_t i = 0; i < mial_list_size(ast[node].children).val; i++) {
        print_ast(ast, ast[node].children[i], indent + 1);
      }
      printf("%*s},\n", indent * 2, "");
    } break;
    case AST_DEF_VAR: {
      printf("%*sVar '%.*s' {\n", indent * 2, "", (int)ast[node].data.identifier.size, ast[node].data.identifier.data);
      for (uint32_t i = 0; i < mial_list_size(ast[node].children).val; i++) {
        print_ast(ast, ast[node].children[i], indent + 1);
      }
      printf("%*s},\n", indent * 2, "");
    } break;
    case AST_FN: {
      printf("%*sFn {\n", indent * 2, "");
      for (uint32_t i = 0; i < mial_list_size(ast[node].children).val; i++) {
        print_ast(ast, ast[node].children[i], indent + 1);
      }
      printf("%*s},\n", indent * 2, "");
    } break;
    case AST_BLOCK: {
      printf("%*sBlock {\n", indent * 2, "");
      for (uint32_t i = 0; i < mial_list_size(ast[node].children).val; i++) {
        print_ast(ast, ast[node].children[i], indent + 1);
      }
      printf("%*s},\n", indent * 2, "");
    } break;
    case AST_INT_LITERAL: {
      printf("%*sInt-literal = %ld,\n", indent * 2, "", ast[node].data.int_literal);
    } break;
    case AST_IDENTIFIER: {
      printf("%*sIdentifier = %.*s,\n", indent * 2, "", (int)ast[node].data.identifier.size, ast[node].data.identifier.data);
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

struct ast_node *
parse_tokens(const struct token *tokens) {
  struct mial_ptr ast_ptr = mial_list_make(struct ast_node, 0);
  if (ast_ptr.err != MIAL_ERROR_OK) {
    LOG_ERROR("Couldn't create ast: %s\n", mial_error_string(ast_ptr.err));
    exit(1);
  }
  struct ast_node *ast = ast_ptr.val;
  int64_t current_node = ast_node_make(&ast, -1, AST_MODULE, true);
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
  print_ast(ast, 0, 0);
  return ast;
}

#undef GET_NEXT_TOKEN
#undef CHECK_TOKEN_TYPE
#undef GET_NEXT_TOKEN_AND_CHECK_TYPE

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
  LOG_TODO("generate_ir(ast)\n");
  LOG_TODO("generate_fasm_x86_64_linux(ir)\n");
  {
    enum mial_error err;
    if ((err = mial_list_destroy(tokens)) != MIAL_ERROR_OK) LOG_ERROR("Couldn't destroy tokens list: %s\n", mial_error_string(err));
    if ((err = mial_list_destroy(ast)) != MIAL_ERROR_OK) LOG_ERROR("Couldn't destroy ast: %s\n", mial_error_string(err));
  }
  free((char *)src.data);
  return 0;
}
