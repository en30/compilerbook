#include "9cc.h"

char *user_input;
GVar *globals;
LVar *locals;
Node program_head;
Node *strings;

Type int_type = {.ty = TY_INT, .size = 4};
Type char_type = {.ty = TY_CHAR, .size = 1};

int UNK_ARRAY_SIZE = -1;

int type_size(Type *type) { return type->size; }
size_t type_array_size(Type *type) { return type->array_size; }
void type_array_resize(Type *type, size_t array_size) {
  type->array_size = array_size;
  type->size = array_size * type->ptr_to->size;
}

void type_assign(Type *lhs, Type *rhs) {
  if (lhs->array_size == UNK_ARRAY_SIZE)
    type_array_resize(lhs, type_array_size(rhs));
}

Type *new_pointer_to(Type *target) {
  Type *type = calloc(1, sizeof(Type));
  type->ty = TY_PTR;
  type->ptr_to = target;
  type->size = 8;
  return type;
}

Type *new_array_of(Type *target, size_t array_size) {
  Type *type = calloc(1, sizeof(Type));
  type->ty = TY_ARRAY;
  type->ptr_to = target;
  type_array_resize(type, array_size);
  return type;
}

Type *binary_operation_result_type(Node *node) {
  switch (node->kind) {
    case ND_ADD:
    case ND_SUB:
      Type *lt = node->lhs->type;
      Type *rt = node->rhs->type;
      if (lt->ty == TY_PTR) {
        return lt;
      } else if (lt->ty == TY_ARRAY) {
        return new_pointer_to(lt->ptr_to);
      } else if (rt->ty == TY_PTR) {
        return rt;
      } else if (rt->ty == TY_ARRAY) {
        return new_pointer_to(rt->ptr_to);
      } else if (type_size(lt) > type_size(rt)) {
        return lt;
      } else {
        return rt;
      }
    default:
      error("二項演算がサポートされていません");
  }
}

Node *new_node(NodeKind kind, Node *lhs, Node *rhs) {
  Node *node = calloc(1, sizeof(Node));
  node->kind = kind;
  node->lhs = lhs;
  node->rhs = rhs;
  if (node->kind == ND_DEREF) {
    node->type = node->lhs->type->ptr_to;
    if (!node->type) node->type = node->lhs->type;
  } else if (node->kind == ND_ADD || node->kind == ND_SUB) {
    node->type = binary_operation_result_type(node);
  } else if (node->kind == ND_INITLIST) {
    node->type = new_array_of(&int_type, 0);
  } else if (node->kind == ND_ASSIGN) {
    node->type = node->lhs->type->ptr_to;
  } else if (node->kind == ND_ADDR) {
    if (node->lhs->type->ty == TY_ARRAY) {
      node->type = new_pointer_to(node->lhs->type->ptr_to);
    } else {
      node->type = new_pointer_to(node->lhs->type);
    }
  } else {
    node->type = &int_type;
  }
  return node;
}

Node *new_node_num(int val) {
  Node *node = calloc(1, sizeof(Node));
  node->kind = ND_NUM;
  node->val = val;
  node->type = &int_type;
  return node;
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

GVar *def_gvar(Token *tok, Type *type) {
  GVar *gvar = find_gvar(tok);
  if (gvar) return NULL;

  gvar = calloc(1, sizeof(GVar));
  gvar->next = globals;
  gvar->name = tok->str;
  gvar->len = tok->len;
  gvar->type = type;
  globals = gvar;
  return gvar;
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
  gvar->init_str = calloc(1, tok->len + 1);
  strncpy(gvar->init_str, tok->str, tok->len);
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
gvardec    = varspec ("=" initializer)? ";"
initializer= "{" (initlist ","?)? "}"
           | equality
initlist   = equality ("," equality)*
stmt       = expr ";"
           | block
           | "if" "(" expr ")" stmt ("else" stmt)?
           | "while" "(" expr ")" stmt
           | "for" "(" expr? ";" expr? ";" expr? ")" stmt
           | "return" expr ";"
block      = "{" (declaration | stmt)* "}"
declaration= varspec ("=" initializer)? ";"
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
varspec    = typespec ident ("[" num? "]")*
*/
Node *program();
Node *func();
Node *gvardec();
Node *initializer();
Node *initlist();
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
  while (consume_punct("*")) {
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
  while (consume_punct("*")) {
    type = new_pointer_to(type);
  }

  return type;
}

void expect_varspec(Type **t, Token **tok) {
  *t = expect_typespec();
  *tok = expect(TK_IDENT);
  while (consume_punct("[")) {
    if (consume_punct("]")) {
      *t = new_array_of(*t, UNK_ARRAY_SIZE);
    } else {
      *t = new_array_of(*t, token_value(consume(TK_NUM)));
      expect_punct("]");
    }
  }
}

bool consume_varspec(Type **t, Token **tok) {
  *t = consume_typespec();
  if (!*t) return false;
  *tok = expect(TK_IDENT);
  while (consume_punct("[")) {
    if (consume_punct("]")) {
      *t = new_array_of(*t, UNK_ARRAY_SIZE);
    } else {
      *t = new_array_of(*t, token_value(consume(TK_NUM)));
      expect_punct("]");
    }
  }
  return true;
}

bool peek_func() {
  Token *init = token;
  if (consume_typespec() && consume(TK_IDENT) && consume_punct("(")) {
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
    Node *next = peek_func() ? func() : gvardec();
    if (!next) continue;
    current->next = next;
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

  expect_punct("(");
  if (!consume_punct(")")) {
    Node head = {};
    Node *current = &head;

    Type *type;
    expect_varspec(&type, &tok);
    def_lvar(tok, type);
    current->next = lvar(tok);
    current = current->next;

    while (!consume_punct(")")) {
      expect_punct(",");
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

int eval_constexp(Node *node) {
  switch (node->kind) {
    case ND_NUM:
      return node->val;
    case ND_ADD:
      return eval_constexp(node->lhs) + eval_constexp(node->rhs);
    case ND_SUB:
      return eval_constexp(node->lhs) - eval_constexp(node->rhs);
    case ND_MUL:
      return eval_constexp(node->lhs) * eval_constexp(node->rhs);
    case ND_DIV:
      return eval_constexp(node->lhs) / eval_constexp(node->rhs);
    case ND_EQ:
      return eval_constexp(node->lhs) == eval_constexp(node->rhs);
    case ND_NE:
      return eval_constexp(node->lhs) != eval_constexp(node->rhs);
    case ND_LT:
      return eval_constexp(node->lhs) < eval_constexp(node->rhs);
    case ND_LE:
      return eval_constexp(node->lhs) <= eval_constexp(node->rhs);
    default:
      error("not a constexpr");
  }
}

int comptimeops[] = {ND_ADD, ND_SUB, ND_MUL, ND_DIV,
                     ND_EQ,  ND_NE,  ND_LT,  ND_LE};
Node *comptime_eval(Node *node) {
  int l = sizeof(comptimeops) / sizeof(ND_ADD);
  for (int i = 0; i < l; i++) {
    if (node->kind != comptimeops[i]) continue;
    if (node->lhs->kind == ND_NUM && node->rhs->kind == ND_NUM)
      return new_node_num(eval_constexp(node));

    return new_node(node->kind, comptime_eval(node->lhs),
                    comptime_eval(node->rhs));
  }
  return node;
}

bool node_is_string_literal(Node *node) {
  return node->kind == ND_GVAR && node->gvar->init_str;
}

GVar *string_literal_gvar(Node *node) { return node->gvar; }

Node *gvardec() {
  Node *node = calloc(1, sizeof(Node));

  Type *type;
  Token *tok;
  expect_varspec(&type, &tok);
  GVar *new_gvar = def_gvar(tok, type);
  node->gvar = find_gvar(tok);

  if (consume_punct("=")) {
    node->kind = ND_GVARDEF;

    if (!new_gvar && node->gvar->initialized) {
      error_at(token->str, "既にグローバル変数が初期化済みです");
    }
    node->gvar->initialized = true;
    node->lhs = initializer();
    if (node_is_string_literal(node->lhs)) {
      type_assign(node->gvar->type, string_literal_gvar(node->lhs)->type);
    } else if (node->lhs->kind == ND_INITLIST) {
      type_assign(node->gvar->type, node->lhs->type);
    }
  } else {
    node->kind = ND_GVARDEC;

    // 再宣言は無視
    if (!new_gvar) node = NULL;
  }

  expect_punct(";");
  return node;
}

Node *initializer() {
  if (consume_punct("{")) {
    if (consume_punct("}")) return new_node(ND_INITLIST, NULL, NULL);
    Node *node = initlist();
    expect_punct("}");
    return node;
  } else {
    return comptime_eval(equality());
  }
}

Node *initlist() {
  Node *node = new_node(ND_INITLIST, NULL, NULL);
  node->next = comptime_eval(equality());
  int l = 1;
  Node *current = node->next;
  while (consume_punct(",") && !peek_punct("}")) {
    current->next = comptime_eval(equality());
    current = current->next;
    l++;
  }
  type_array_resize(node->type, l);
  return node;
}

Node *block() {
  Node *node = calloc(1, sizeof(Node));
  node->kind = ND_BLOCK;

  expect_punct("{");
  Node *current = node;
  while (!consume_punct("}")) {
    Type *type;
    Token *tok;
    if (consume_varspec(&type, &tok)) {
      def_lvar(tok, type);
      current->next = lvar(tok);
      if (consume_punct("=")) {
        Node *arr_node = current->next;
        if (consume_punct("{")) {
          int i = 0;
          if (!consume_punct("}")) {
            current = current->next;
            current->next = new_node(
                ND_ASSIGN,
                new_node(ND_DEREF, new_node(ND_ADD, arr_node, new_node_num(i)),
                         NULL),
                expr());
            i++;
            while (consume_punct(",") && !peek_punct("}")) {
              current = current->next;
              current->next = new_node(
                  ND_ASSIGN,
                  new_node(ND_DEREF,
                           new_node(ND_ADD, arr_node, new_node_num(i)), NULL),
                  expr());
              i++;
            }
            type_array_resize(arr_node->lvar->type, i);
            expect_punct("}");
          }
        } else {
          Node *lvar_node = current->next;
          Node *e = equality();
          if (node_is_string_literal(e)) {
            type_assign(lvar_node->lvar->type, string_literal_gvar(e)->type);
            for (int i = 0; i < string_literal_gvar(e)->type->array_size; i++) {
              current = current->next;
              current->next = new_node(
                  ND_ASSIGN,
                  new_node(ND_DEREF,
                           new_node(ND_ADD, lvar_node, new_node_num(i)), NULL),
                  new_node(ND_DEREF, new_node(ND_ADD, e, new_node_num(i)),
                           NULL));
            }
          } else {
            current->next = new_node(ND_ASSIGN, lvar(tok), e);
          }
        }
      }
      expect_punct(";");
    } else {
      current->next = stmt();
    }
    current = current->next;
  }
  return node;
}

Node *stmt() {
  Node *node;
  if (peek_punct("{")) {
    return block();
  } else if (consume(TK_IF)) {
    node = calloc(1, sizeof(Node));
    node->kind = ND_IF;
    expect_punct("(");
    node->cond = expr();
    expect_punct(")");
    node->then = stmt();
    if (consume(TK_ELSE)) {
      node->els = stmt();
    }
  } else if (consume(TK_WHILE)) {
    node = calloc(1, sizeof(Node));
    node->kind = ND_WHILE;
    expect_punct("(");
    node->cond = expr();
    expect_punct(")");
    node->then = stmt();
  } else if (consume(TK_FOR)) {
    node = calloc(1, sizeof(Node));
    node->kind = ND_FOR;
    expect_punct("(");
    if (!consume_punct(";")) {
      node->init = expr();
      expect_punct(";");
    }
    if (!consume_punct(";")) {
      node->cond = expr();
      expect_punct(";");
    }
    if (!consume_punct(")")) {
      node->inc = expr();
      expect_punct(")");
    }
    node->then = stmt();
  } else if (consume(TK_RETURN)) {
    node = calloc(1, sizeof(Node));
    node->kind = ND_RETURN;
    node->lhs = expr();
    expect_punct(";");
  } else {
    node = expr();
    expect_punct(";");
  }

  return node;
}

Node *expr() { return assign(); }

Node *assign() {
  Node *node = equality();
  if (consume_punct("=")) {
    node = new_node(ND_ASSIGN, node, assign());
  }
  return node;
}

Node *equality() {
  Node *node = relational();

  for (;;) {
    if (consume_punct("=="))
      node = new_node(ND_EQ, node, relational());
    else if (consume_punct("!="))
      node = new_node(ND_NE, node, relational());
    else
      return node;
  }
}

Node *relational() {
  Node *node = add();

  for (;;) {
    if (consume_punct("<="))
      node = new_node(ND_LE, node, add());
    else if (consume_punct("<"))
      node = new_node(ND_LT, node, add());
    else if (consume_punct(">="))
      node = new_node(ND_LE, add(), node);
    else if (consume_punct(">"))
      node = new_node(ND_LT, add(), node);
    else
      return node;
  }
}

Node *add() {
  Node *node = mul();

  for (;;) {
    if (consume_punct("+"))
      node = new_node(ND_ADD, node, mul());
    else if (consume_punct("-"))
      node = new_node(ND_SUB, node, mul());
    else
      return node;
  }
}

Node *mul() {
  Node *node = unary();

  for (;;) {
    if (consume_punct("*"))
      node = new_node(ND_MUL, node, unary());
    else if (consume_punct("/"))
      node = new_node(ND_DIV, node, unary());
    else
      return node;
  }
}

Node *unary() {
  if (consume(TK_SIZEOF)) {
    Node *node = unary();
    return new_node_num(type_size(node->type));
  }
  if (consume_punct("+")) return unary();
  if (consume_punct("-")) return new_node(ND_SUB, new_node_num(0), unary());
  if (consume_punct("*")) return new_node(ND_DEREF, unary(), NULL);
  if (consume_punct("&")) {
    Node *node = new_node(ND_ADDR, unary(), NULL);
    return node;
  }

  Node *node = primary();
  if (consume_punct("[")) {
    node = new_node(ND_DEREF, new_node(ND_ADD, node, expr()), NULL);
    expect_punct("]");
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
  dec->kind = ND_GVARDEF;
  dec->gvar = gvar;
  dec->next = strings;
  strings = dec;

  Node *node = calloc(1, sizeof(Node));
  node->kind = ND_GVAR;
  node->gvar = gvar;
  node->type = gvar->type;

  return node;
}

Node *primary() {
  if (consume_punct("(")) {
    Node *node = expr();
    expect_punct(")");
    return node;
  }

  Token *tok = consume(TK_IDENT);
  if (tok) {
    if (consume_punct("(")) {
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
      if (!consume_punct(")")) {
        current->next = expr();
        current = current->next;
        while (!consume_punct(")")) {
          expect_punct(",");
          current->next = expr();
          current = current->next;
        }
        node->args = head.next;
      }

      return node;
    } else {
      Node *node = lvar(tok);
      if (!node) node = gvar(tok);
      return node;
    }
  }

  if (tok = consume(TK_STR)) {
    return str(tok);
  }

  return new_node_num(token_value(expect(TK_NUM)));
}

Node *parse() { return program(); }
