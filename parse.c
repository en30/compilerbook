#include "9cc.h"

Token *token;
char *user_input;
GVar *globals;
LVar *locals;
Node program_head;
Node *strings;

Type int_type = {TY_INT};
Type char_type = {TY_CHAR};

int type_size(Type *type) {
  switch (type->ty) {
    case TY_CHAR:
      return 1;
    case TY_INT:
      return 4;
    case TY_PTR:
      return 8;
    case TY_ARRAY:
      return type->array_size * type_size(type->ptr_to);
  }
}

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
  node->type = &int_type;
  return node;
}

Node *new_deref_node(Node *node) {
  Node *res = new_node(ND_DEREF, node, NULL);
  res->type = node->type->ptr_to;
  if (!res->type) res->type = node->type;
  return res;
}

Type *binary_operation_result_type(Node *node) {
  switch (node->kind) {
    case ND_ADD:
    case ND_SUB:
      Type *lt = node->lhs->type;
      Type *rt = node->rhs->type;
      if (lt->ty == TY_PTR) {
        return lt;
      } else if (rt->ty == TY_PTR) {
        return rt;
      } else if (type_size(lt) > type_size(rt)) {
        return lt;
      } else {
        return rt;
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

// エラーの起きた場所を報告するための関数
// 下のようなフォーマットでエラーメッセージを表示する
//
// foo.c:10: x = y + + 5;
//                   ^ 式ではありません
void error_at(char *loc, char *fmt, ...) {
  va_list ap;
  va_start(ap, fmt);

  // locが含まれている行の開始地点と終了地点を取得
  char *line = loc;
  while (user_input < line && line[-1] != '\n') line--;

  char *end = loc;
  while (*end != '\n') end++;

  // 見つかった行が全体の何行目なのかを調べる
  int line_num = 1;
  for (char *p = user_input; p < line; p++)
    if (*p == '\n') line_num++;

  // 見つかった行を、ファイル名と行番号と一緒に表示
  int indent = fprintf(stderr, "%s:%d ", filename, line_num);
  fprintf(stderr, "%.*s\n", (int)(end - line), line);

  // エラー箇所を"^"で差し示して、エラーメッセージを表示
  int pos = loc - line + indent;
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

Token *consume_str() {
  if (token->kind != TK_STR) return NULL;
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

    if (strncmp(p, "//", 2) == 0) {
      p += 2;
      while (*p != '\n') p++;
      continue;
    }

    if (strncmp(p, "/*", 2) == 0) {
      char *q = strstr(p + 2, "*/");
      if (!q) error_at(p, "コメントが閉じられていません");
      p = q + 2;
      continue;
    }

    if (*p == '"') {
      p++;
      cur = new_token(TK_STR, cur, p, 0);
      while (*p != '"') {
        if (*p == '\n' || *p == '\0')
          error_at(p, "文字列リテラルが閉じられていません");
        if (strncmp(p, "\\\"", 2) == 0) {
          p += 2;
          cur->len += 2;
          continue;
        }

        p++;
        cur->len++;
      }
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

    if (strncmp(p, "char", 4) == 0 && !is_alnum(p[4])) {
      cur = new_token(TK_CHAR, cur, p, 4);
      p += 4;
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

char *new_literal_string_name() {
  static int i = 0;
  char *buf = calloc(1, 20);
  sprintf(buf, ".LC%d", i++);
  return buf;
}

GVar *new_string_literal(Token *tok) {
  GVar *gvar = calloc(1, sizeof(GVar));
  gvar->name = new_literal_string_name();
  gvar->len = strlen(gvar->name);
  gvar->type = new_array_of(&char_type, tok->len + 1);
  return gvar;
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
           | str
           | ident ( "(" (expr ("," expr)*)? ")" )?
           | "(" expr ")"
typespec   = ("int" | "char") "*"*
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
  Type *type;
  switch (token->kind) {
    case TK_CHAR:
      type = &char_type;
      break;
    case TK_INT:
      type = &int_type;
      break;
    default:
      return NULL;
  }
  token = token->next;
  while (consume("*")) {
    type = new_pointer_to(type);
  }

  return type;
}

Type *expect_typespec() {
  Type *type;
  switch (token->kind) {
    case TK_CHAR:
      type = &char_type;
      break;
    case TK_INT:
      type = &int_type;
      break;
    default:
      error_at(token->str, "サポートされていない型です");
      return NULL;
  }

  token = token->next;
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
  if (consume("=")) {
    node = new_node(ND_ASSIGN, node, assign());
    node->type = node->lhs->type->ptr_to;
  }
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

Node *unary() {
  if (consume_token(TK_SIZEOF)) {
    Node *node = unary();
    Type *type = node->type;
    if (node->kind == ND_ADDR && node->lhs->type->ty == TY_ARRAY)
      return new_node_num(type_size(node->lhs->type));
    return new_node_num(type_size(type));
  }
  if (consume("+")) return unary();
  if (consume("-")) return new_sub_node(new_node_num(0), unary());
  if (consume("*")) return new_deref_node(unary());
  if (consume("&")) {
    Node *node = unary();
    if (node->kind == ND_ADDR && node->lhs->type->ty == TY_ARRAY) {
      node->type = new_pointer_to(node->lhs->type->ptr_to);
      return node;
    }
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

Node *str(Token *tok) {
  GVar *gvar = new_string_literal(tok);

  Node *dec = calloc(1, sizeof(Node));
  dec->kind = ND_GVARDEC;
  dec->gvar = gvar;
  dec->next = strings;
  dec->literal_data = tok->str;
  dec->len = tok->len;
  strings = dec;

  Node *node = calloc(1, sizeof(Node));
  node->kind = ND_GVAR;
  node->gvar = gvar;
  node->type = gvar->type;

  node = new_node(ND_ADDR, node, NULL);
  node->type = new_pointer_to(node->lhs->type->ptr_to);
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

  if (tok = consume_str()) {
    return str(tok);
  }

  return new_node_num(expect_number());
}

Node *parse() { return program(); }
