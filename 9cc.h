#include <ctype.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef enum {
  TK_PUNCT,
  TK_IDENT,
  TK_NUM,
  TK_RETURN,
  TK_IF,
  TK_ELSE,
  TK_WHILE,
  TK_FOR,
  TK_EOF,
  TK_CHAR,
  TK_INT,
  TK_SIZEOF,
  TK_STR,
  TK_STRUCT,
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
  ND_GVARDEC,
  ND_GVARDEF,
  ND_GVAR,
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
  ND_INITLIST,
} NodeKind;

typedef enum {
  TY_CHAR,
  TY_INT,
  TY_PTR,
  TY_ARRAY,
  TY_STRUCT,
} TypeKind;

typedef struct Token Token;
struct Token;
typedef struct Node Node;
struct Node;
typedef struct GVar GVar;
struct GVar;
typedef struct LVar LVar;
struct LVar;
typedef struct Type Type;
struct Type;
typedef struct Member Member;
struct Member;

struct Token {
  TokenKind kind;
  Token *next;
  int val;
  char *str;
  int len;
};
bool at_eof();
Token *peek_punct(char *op);
Token *consume_punct(char *op);
Token *expect_punct(char *op);
Token *consume(TokenKind kind);
Token *expect(TokenKind kind);
bool peek(TokenKind kind);
int token_value(Token *token);

struct Node {
  NodeKind kind;
  Node *lhs;
  Node *rhs;
  int val;     // kindがND_NUMの場合のみ使う
  LVar *lvar;  // kindがND_LVARの場合のみ使う
  GVar *gvar;  // kindがND_GVARの場合のみ使う

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
  LVar *locals;       // ND_FUNC

  Type *type;
};

struct GVar {
  GVar *next;  // 次の変数かNULL
  char *name;  // 変数の名前
  int len;     // 名前の長さ
  Type *type;
  bool initialized;
  char *init_str;
  int init_int;
};

struct LVar {
  LVar *next;  // 次の変数かNULL
  char *name;  // 変数の名前
  int len;     // 名前の長さ
  int offset;  // RBPからのオフセット
  Type *type;
};

struct Type {
  TypeKind ty;
  struct Type *ptr_to;
  size_t array_size;
  size_t size;

  // struct
  Token *name;
  Member *member;
  Type *next;
};

struct Member {
  Token *name;
  Type *type;
  int offset;
  Member *next;
};

extern Type int_type;
extern Type char_type;
extern int UNK_ARRAY_SIZE;
Type *new_pointer_to(Type *target);
Type *new_array_of(Type *target, size_t array_size);
Type *new_struct(Token *tok);
void *set_member(Type *strct, Member *member);
Type *new_anonymous_struct();
bool is_int(Type *type);
bool is_effectively_pointer(Type *type);
int type_size(Type *type);
int type_address_size(Type *type);
size_t type_array_size(Type *type);
void type_array_resize(Type *type, size_t array_size);
void type_assign(Type *lhs, Type *rhs);
Type *type_add(Type *lhs, Type *rhs);
Type *type_sub(Type *lhs, Type *rhs);

void error(char *fmt, ...);
void error_at(char *loc, char *fmt, ...);

Token *tokenize(char *p);
Node *parse();
void codegen(Node *funcs);

extern char *filename;
extern Token *token;
extern char *user_input;
extern GVar *globals;
extern LVar *locals;
extern Node *strings;
