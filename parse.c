#include "9cc.h"

Token *token;
char *user_input;
GVar *globals;
LVar *locals;
Node program_head;

Type int_type = {TY_INT};

Type *new_pointer_to(Type *target) {
  Type *type = calloc(1, sizeof(Type));
  type->ty = TY_PTR;
  type->ptr_to = target;
  return type;
}

Type *new_array_of(Type *target, size_t size) {
  Type *type = calloc(1, sizeof(Type));
  type->ty = TY_ARRAY;
  type->ptr_to = target;
  type->array_size = size;
  return type;
}

Token *new_token(TokenKind kind, Token *cur, char *str, int len) {
  Token *tok = calloc(1, sizeof(Token));
  tok->kind = kind;
  tok->str = str;
  tok->len = len;
  cur->next = tok;
  return tok;
}

Node *new_node(NodeKind kind, Node *lhs, Node *rhs) {
  Node *node = calloc(1, sizeof(Node));
  node->kind = kind;
  node->lhs = lhs;
  node->rhs = rhs;
  return node;
}

Node *new_deref_node(Node *node) {
  Node *res = new_node(ND_DEREF, node, NULL);
  res->type = res->lhs->type->ptr_to;
  return res;
}

Type *binary_operation_result_type(Node *node) {
  switch (node->kind) {
    case ND_ADD:
    case ND_SUB:
      Type *t;
      if ((t = node_type(node->lhs))->ty == TY_PTR) {
        return t;
      } else if ((t = node_type(node->rhs))->ty == TY_PTR) {
        return t;
      } else {
        return &int_type;
      }
    default:
      error("二項演算がサポートされていません");
  }
}

Node *new_add_node(Node *lhs, Node *rhs) {
  Node *node = calloc(1, sizeof(Node));
  node->kind = ND_ADD;
  node->lhs = lhs;
  node->rhs = rhs;
  node->type = binary_operation_result_type(node);
  return node;
}

Node *new_sub_node(Node *lhs, Node *rhs) {
  Node *node = calloc(1, sizeof(Node));
  node->kind = ND_SUB;
  node->lhs = lhs;
  node->rhs = rhs;
  node->type = binary_operation_result_type(node);
  return node;
}

Node *new_node_num(int val) {
  Node *node = calloc(1, sizeof(Node));
  node->kind = ND_NUM;
  node->val = val;
  node->type = &int_type;
  return node;
}

void error(char *fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  vfprintf(stderr, fmt, ap);
  fprintf(stderr, "\n");
  exit(1);
}

void error_at(char *loc, char *fmt, ...) {
  va_list ap;
  va_start(ap, fmt);

  int pos = loc - user_input;
  fprintf(stderr, "%s\n", user_input);
  fprintf(stderr, "%*s", pos, " ");
  fprintf(stderr, "^ ");
  vfprintf(stderr, fmt, ap);
  fprintf(stderr, "\n");
  exit(1);
}

bool peek(char *op) {
  if (token->kind != TK_RESERVED || strlen(op) != token->len ||
      memcmp(token->str, op, token->len))
    return false;
  return true;
}

bool consume(char *op) {
  if (token->kind != TK_RESERVED || strlen(op) != token->len ||
      memcmp(token->str, op, token->len))
    return false;
  token = token->next;
  return true;
}

bool consume_token(TokenKind kind) {
  if (token->kind != kind) return false;
  token = token->next;
  return true;
}

Token *consume_ident() {
  if (token->kind != TK_IDENT) return NULL;
  Token *orig = token;
  token = token->next;
  return orig;
}

Token *expect_ident() {
  if (token->kind != TK_IDENT) error_at(token->str, "識別子ではありません");
  Token *orig = token;
  token = token->next;
  return orig;
}

void expect(char *op) {
  if (token->kind != TK_RESERVED || strlen(op) != token->len ||
      memcmp(token->str, op, token->len))
    error_at(token->str, "\"%s\"ではありません", op);
  token = token->next;
}

int expect_number() {
  if (token->kind != TK_NUM) error_at(token->str, "数ではありません");
  int val = token->val;
  token = token->next;
  return val;
}

bool at_eof() { return token->kind == TK_EOF; }

bool startswith(char *p, char *q) { return memcmp(p, q, strlen(q)) == 0; }

bool is_alpha(char c) {
  return ('a' <= c && c <= 'z') || ('A' <= c && c <= 'Z') || (c == '_');
}

bool is_alnum(char c) { return is_alpha(c) || ('0' <= c && c <= '9'); }

Token *tokenize(char *p) {
  Token head;
  head.next = NULL;
  Token *cur = &head;

  while (*p) {
    if (isspace(*p)) {
      p++;
      continue;
    }

    if (startswith(p, "<=") || startswith(p, ">=") || startswith(p, "==") ||
        startswith(p, "!=")) {
      cur = new_token(TK_RESERVED, cur, p, 2);
      p += 2;
      continue;
    }

    if (strchr("+-*/()<>;={},&[]", *p)) {
      cur = new_token(TK_RESERVED, cur, p++, 1);
      continue;
    }

    if (strncmp(p, "sizeof", 6) == 0 && !is_alnum(p[6])) {
      cur = new_token(TK_SIZEOF, cur, p, 6);
      p += 6;
      continue;
    }

    if (strncmp(p, "int", 3) == 0 && !is_alnum(p[3])) {
      cur = new_token(TK_INT, cur, p, 3);
      p += 3;
      continue;
    }

    if (strncmp(p, "return", 6) == 0 && !is_alnum(p[6])) {
      cur = new_token(TK_RETURN, cur, p, 6);
      p += 6;
      continue;
    }

    if (strncmp(p, "if", 2) == 0 && !is_alnum(p[2])) {
      cur = new_token(TK_IF, cur, p, 2);
      p += 2;
      continue;
    }

    if (strncmp(p, "else", 4) == 0 && !is_alnum(p[4])) {
      cur = new_token(TK_ELSE, cur, p, 4);
      p += 4;
      continue;
    }

    if (strncmp(p, "while", 5) == 0 && !is_alnum(p[5])) {
      cur = new_token(TK_WHILE, cur, p, 5);
      p += 5;
      continue;
    }

    if (strncmp(p, "for", 3) == 0 && !is_alnum(p[3])) {
      cur = new_token(TK_FOR, cur, p, 3);
      p += 3;
      continue;
    }

    if (is_alpha(*p)) {
      cur = new_token(TK_IDENT, cur, p++, 1);
      while (is_alnum(*p)) {
        cur->len++;
        p++;
      }
      continue;
    }

    if (isdigit(*p)) {
      cur = new_token(TK_NUM, cur, p, 0);
      char *q = p;
      cur->val = strtol(p, &p, 10);
      cur->len = p - q;
      continue;
    }

    error_at(p, "トークナイズできません");
  }

  new_token(TK_EOF, cur, p, 0);
  return head.next;
}

Node *find_func(Token *tok) {
  for (Node *n = program_head.next; n; n = n->next)
    if (n->len == tok->len && !memcmp(tok->str, n->fname, n->len)) return n;
  return NULL;
}

GVar *find_gvar(Token *tok) {
  for (GVar *var = globals; var; var = var->next)
    if (var->len == tok->len && !memcmp(tok->str, var->name, var->len))
      return var;
  return NULL;
}

void def_gvar(Token *tok, Type *type) {
  GVar *gvar = find_gvar(tok);
  if (gvar) error_at(tok->str, "同名のグローバル変数が既に定義済みです");

  gvar = calloc(1, sizeof(GVar));
  gvar->next = globals;
  gvar->name = tok->str;
  gvar->len = tok->len;
  gvar->type = type;
  globals = gvar;
}

LVar *find_lvar(Token *tok) {
  for (LVar *var = locals; var; var = var->next)
    if (var->len == tok->len && !memcmp(tok->str, var->name, var->len))
      return var;
  return NULL;
}

void def_lvar(Token *tok, Type *type) {
  LVar *lvar = find_lvar(tok);
  if (lvar) error_at(tok->str, "同名のローカル変数が既に定義済みです");

  lvar = calloc(1, sizeof(LVar));
  lvar->next = locals;
  lvar->name = tok->str;
  lvar->len = tok->len;
  lvar->offset = (locals ? locals->offset : 0) + 8;
  lvar->type = type;
  locals = lvar;
}

Type *node_type(Node *node) {
  if (node == NULL) return NULL;
  if (node->type) return node->type;

  Type *t = node_type(node->lhs);
  if (t) return t;
  return node_type(node->rhs);
}

/*
program    = (func | gvardec)*
func       = varspec "(" (varspec ("," varspec)*)? ")" block
gvardec    = varspec ";"
stmt       = expr ";"
           | block
           | "if" "(" expr ")" stmt ("else" stmt)?
           | "while" "(" expr ")" stmt
           | "for" "(" expr? ";" expr? ";" expr? ")" stmt
           | "return" expr ";"
block      = "{" (declaration | stmt)* "}"
declaration= varspec ";"
expr       = assign
assign     = equality ("=" assign)?
equality   = relational ("==" relational | "!=" relational)*
relational = add ("<" add | "<=" add | ">" add | ">=" add)*
add        = mul ("+" mul | "-" mul)*
mul        = unary ("*" unary | "/" unary)*
unary      = "sizeof" unary
           | ("+" | "-" | "*" | "&") unary
           | primary ("[" expr "]")?
primary    = num
           | ident ( "(" (expr ("," expr)*)? ")" )?
           | "(" expr ")"
typespec   = "int" "*"*
varspec    = typespec ident ("[" num "]")*
*/
Node *program();
Node *func();
Node *gvardec();
Node *stmt();
Node *block();
Node *declaration();
Node *expr();
Node *assign();
Node *equality();
Node *relational();
Node *add();
Node *mul();
Node *unary();
Node *primary();
Node *lvar();
void typespec();

Type *consume_typespec() {
  if (token->kind != TK_INT) return NULL;

  token = token->next;
  Type *type = &int_type;
  while (consume("*")) {
    type = new_pointer_to(type);
  }

  return type;
}

Type *expect_typespec() {
  if (token->kind != TK_INT) error_at(token->str, "intではありません");

  token = token->next;
  Type *type = &int_type;
  while (consume("*")) {
    type = new_pointer_to(type);
  }

  return type;
}

void expect_varspec(Type **t, Token **tok) {
  *t = expect_typespec();
  *tok = expect_ident();
  while (consume("[")) {
    *t = new_array_of(*t, expect_number());
    expect("]");
  }
}

bool consume_varspec(Type **t, Token **tok) {
  *t = consume_typespec();
  if (!*t) return false;
  *tok = expect_ident();
  while (consume("[")) {
    *t = new_array_of(*t, expect_number());
    expect("]");
  }
  return true;
}

bool peek_func() {
  Token *init = token;
  if (consume_typespec() && consume_ident() && consume("(")) {
    token = init;
    return true;
  } else {
    token = init;
    return false;
  }
}

Node *program() {
  Node *current = &program_head;
  while (!at_eof()) {
    current->next = peek_func() ? func() : gvardec();
    current = current->next;
  }
  return program_head.next;
}

Node *func() {
  Node *node = calloc(1, sizeof(Node));
  node->kind = ND_FUNC;

  Token *tok;
  expect_varspec(&node->return_type, &tok);
  node->fname = tok->str;
  node->len = tok->len;
  locals = NULL;

  expect("(");
  if (!consume(")")) {
    Node head = {};
    Node *current = &head;

    Type *type;
    expect_varspec(&type, &tok);
    def_lvar(tok, type);
    current->next = lvar(tok);
    current = current->next;

    while (!consume(")")) {
      expect(",");
      expect_varspec(&type, &tok);
      def_lvar(tok, type);
      current->next = lvar(tok);
      current = current->next;
    }
    node->args = head.next;
  }

  node->fbody = block();
  node->locals = locals;
  return node;
}

Node *gvardec() {
  Node *node = calloc(1, sizeof(Node));
  node->kind = ND_GVARDEC;

  Type *type;
  Token *tok;
  expect_varspec(&type, &tok);
  def_gvar(tok, type);
  node->gvar = find_gvar(tok);
  expect(";");

  return node;
}

Node *block() {
  Node *node = calloc(1, sizeof(Node));
  node->kind = ND_BLOCK;

  expect("{");
  Node *current = node;
  while (!consume("}")) {
    Type *type;
    Token *tok;
    if (consume_varspec(&type, &tok)) {
      def_lvar(tok, type);
      current->next = lvar(tok);
      expect(";");
    } else {
      current->next = stmt();
    }
    current = current->next;
  }
  return node;
}

Node *stmt() {
  Node *node;
  if (peek("{")) {
    return block();
  } else if (consume_token(TK_IF)) {
    node = calloc(1, sizeof(Node));
    node->kind = ND_IF;
    expect("(");
    node->cond = expr();
    expect(")");
    node->then = stmt();
    if (consume_token(TK_ELSE)) {
      node->els = stmt();
    }
  } else if (consume_token(TK_WHILE)) {
    node = calloc(1, sizeof(Node));
    node->kind = ND_WHILE;
    expect("(");
    node->cond = expr();
    expect(")");
    node->then = stmt();
  } else if (consume_token(TK_FOR)) {
    node = calloc(1, sizeof(Node));
    node->kind = ND_FOR;
    expect("(");
    if (!consume(";")) {
      node->init = expr();
      expect(";");
    }
    if (!consume(";")) {
      node->cond = expr();
      expect(";");
    }
    if (!consume(")")) {
      node->inc = expr();
      expect(")");
    }
    node->then = stmt();
  } else if (consume_token(TK_RETURN)) {
    node = calloc(1, sizeof(Node));
    node->kind = ND_RETURN;
    node->lhs = expr();
    expect(";");
  } else {
    node = expr();
    expect(";");
  }

  return node;
}

Node *expr() { return assign(); }

Node *assign() {
  Node *node = equality();
  if (consume("=")) node = new_node(ND_ASSIGN, node, assign());
  return node;
}

Node *equality() {
  Node *node = relational();

  for (;;) {
    if (consume("=="))
      node = new_node(ND_EQ, node, relational());
    else if (consume("!="))
      node = new_node(ND_NE, node, relational());
    else
      return node;
  }
}

Node *relational() {
  Node *node = add();
  for (;;) {
    if (consume("<="))
      node = new_node(ND_LE, node, add());
    else if (consume("<"))
      node = new_node(ND_LT, node, add());
    else if (consume(">="))
      node = new_node(ND_LE, add(), node);
    else if (consume(">"))
      node = new_node(ND_LT, add(), node);
    else
      return node;
  }
}

Node *add() {
  Node *node = mul();

  for (;;) {
    if (consume("+"))
      node = new_add_node(node, mul());
    else if (consume("-"))
      node = new_sub_node(node, mul());
    else
      return node;
  }
}

Node *mul() {
  Node *node = unary();

  for (;;) {
    if (consume("*"))
      node = new_node(ND_MUL, node, unary());
    else if (consume("/"))
      node = new_node(ND_DIV, node, unary());
    else
      return node;
  }
}

int type_size(Type *type) {
  switch (type->ty) {
    case TY_INT:
      return 4;
    case TY_PTR:
      return 8;
    case TY_ARRAY:
      return type->array_size * type_size(type->ptr_to);
  }
}

Node *unary() {
  if (consume_token(TK_SIZEOF)) {
    Node *node = unary();
    Type *type = node_type(node);
    if (node->kind == ND_ADDR && node->lhs->type->ty == TY_ARRAY)
      return new_node_num(type_size(node->lhs->type));
    return new_node_num(type_size(type));
  }
  if (consume("+")) return unary();
  if (consume("-")) return new_node(ND_SUB, new_node_num(0), unary());
  if (consume("*")) return new_deref_node(unary());
  if (consume("&")) {
    Node *node = unary();
    if (node->kind == ND_ADDR && node->lhs->type->ty == TY_ARRAY) return node;

    Node *res = new_node(ND_ADDR, node, NULL);
    res->type = new_pointer_to(res->lhs->type);
    return res;
  }

  Node *node = primary();
  if (consume("[")) {
    node = new_deref_node(new_add_node(node, expr()));
    expect("]");
  }

  return node;
}

Node *lvar(Token *tok) {
  Node *node = calloc(1, sizeof(Node));
  node->kind = ND_LVAR;

  LVar *lvar = find_lvar(tok);
  if (!lvar) return NULL;

  node->lvar = lvar;
  node->type = lvar->type;
  return node;
}

Node *gvar(Token *tok) {
  Node *node = calloc(1, sizeof(Node));
  node->kind = ND_GVAR;

  GVar *gvar = find_gvar(tok);
  if (!gvar) error_at(tok->str, "定義されていない変数が使われています");

  node->gvar = gvar;
  node->type = gvar->type;
  return node;
}

Node *primary() {
  if (consume("(")) {
    Node *node = expr();
    expect(")");
    return node;
  }

  Token *tok = consume_ident();
  if (tok) {
    if (consume("(")) {
      Node *node = calloc(1, sizeof(Node));
      node->kind = ND_FUNCALL;
      node->fname = tok->str;
      node->len = tok->len;
      Node *fn = find_func(tok);

      if (fn) {
        node->type = fn->return_type;
      } else {
        // 外部の関数はとりあえずint決め打ち
        node->type = &int_type;
      }

      Node head = {};
      Node *current = &head;
      if (!consume(")")) {
        current->next = expr();
        current = current->next;
        while (!consume(")")) {
          expect(",");
          current->next = expr();
          current = current->next;
        }
        node->args = head.next;
      }

      return node;
    } else {
      Node *node = lvar(tok);
      if (!node) node = gvar(tok);

      if (node->type->ty == TY_ARRAY) {
        node = new_node(ND_ADDR, node, NULL);
        node->type = new_pointer_to(node->lhs->type->ptr_to);
      }
      return node;
    }
  }

  return new_node_num(expect_number());
}

Node *parse() { return program(); }
