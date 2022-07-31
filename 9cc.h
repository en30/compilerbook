#include <ctype.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef enum {
  TK_RESERVED,
  TK_IDENT,
  TK_NUM,
  TK_RETURN,
  TK_IF,
  TK_ELSE,
  TK_WHILE,
  TK_FOR,
  TK_EOF,
  TK_INT,
} TokenKind;

typedef enum {
  ND_ADD,
  ND_SUB,
  ND_MUL,
  ND_DIV,
  ND_NUM,
  ND_EQ,
  ND_NE,
  ND_LT,
  ND_LE,
  ND_ASSIGN,
  ND_LVAR,
  ND_RETURN,
  ND_IF,
  ND_WHILE,
  ND_FOR,
  ND_BLOCK,
  ND_FUNCALL,
  ND_FUNC,
  ND_ADDR,
  ND_DEREF,
} NodeKind;

typedef struct Token Token;
struct Token;
typedef struct Node Node;
struct Node;
typedef struct LVar LVar;
struct LVar;
typedef struct Type Type;
struct Type;

struct Token {
  TokenKind kind;
  Token *next;
  int val;
  char *str;
  int len;
};

struct Node {
  NodeKind kind;
  Node *lhs;
  Node *rhs;
  int val;     // kindがND_NUMの場合のみ使う
  LVar *lvar;  // kindがND_LVARの場合のみ使う

  Node *init;  // for
  Node *cond;  // if, while, for
  Node *inc;   // for
  Node *then;  // if, while, for
  Node *els;   // if

  Node *next;  // block, ND_FUNCALL

  char *fname;  // ND_FUNCALL
  int len;      // ND_FUNCALL
  Node *args;

  Node *fbody;        // ND_FUNC
  Type *return_type;  // ND_FUNC
};

struct LVar {
  LVar *next;  // 次の変数かNULL
  char *name;  // 変数の名前
  int len;     // 名前の長さ
  int offset;  // RBPからのオフセット
  Type *type;
};

struct Type {
  enum { INT, PTR } ty;
  struct Type *ptr_to;
};

void error(char *fmt, ...);

Token *tokenize(char *p);
void parse();
void codegen();

extern Node *code[100];
extern Token *token;
extern char *user_input;
extern LVar *locals;
