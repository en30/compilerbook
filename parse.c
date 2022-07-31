#include "9cc.h"

Token *token;
char *user_input;
LVar *locals;

Token *new_token(TokenKind kind, Token *cur, char *str, int len)
{
  Token *tok = calloc(1, sizeof(Token));
  tok->kind = kind;
  tok->str = str;
  tok->len = len;
  cur->next = tok;
  return tok;
}

Node *new_node(NodeKind kind, Node *lhs, Node *rhs)
{
  Node *node = calloc(1, sizeof(Node));
  node->kind = kind;
  node->lhs = lhs;
  node->rhs = rhs;
  return node;
}

Node *new_node_num(int val)
{
  Node *node = calloc(1, sizeof(Node));
  node->kind = ND_NUM;
  node->val = val;
  return node;
}

void error(char *fmt, ...)
{
  va_list ap;
  va_start(ap, fmt);
  vfprintf(stderr, fmt, ap);
  fprintf(stderr, "\n");
  exit(1);
}

void error_at(char *loc, char *fmt, ...)
{
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

bool peek(char *op)
{
  if (token->kind != TK_RESERVED || strlen(op) != token->len || memcmp(token->str, op, token->len))
    return false;
  return true;
}

bool consume(char *op)
{
  if (token->kind != TK_RESERVED || strlen(op) != token->len || memcmp(token->str, op, token->len))
    return false;
  token = token->next;
  return true;
}

bool consume_token(TokenKind kind)
{
  if (token->kind != kind)
    return false;
  token = token->next;
  return true;
}

Token *consume_ident()
{
  if (token->kind != TK_IDENT)
    return NULL;
  Token *orig = token;
  token = token->next;
  return orig;
}

Token *expect_ident()
{
  if (token->kind != TK_IDENT)
    error_at(token->str, "識別子ではありません");
  Token *orig = token;
  token = token->next;
  return orig;
}

void expect(char *op)
{
  if (token->kind != TK_RESERVED || strlen(op) != token->len || memcmp(token->str, op, token->len))
    error_at(token->str, "\"%s\"ではありません", op);
  token = token->next;
}

int expect_number()
{
  if (token->kind != TK_NUM)
    error_at(token->str, "数ではありません");
  int val = token->val;
  token = token->next;
  return val;
}

bool at_eof()
{
  return token->kind == TK_EOF;
}

bool startswith(char *p, char *q)
{
  return memcmp(p, q, strlen(q)) == 0;
}

bool is_alpha(char c)
{
  return ('a' <= c && c <= 'z') ||
         ('A' <= c && c <= 'Z') ||
         (c == '_');
}

bool is_alnum(char c)
{
  return is_alpha(c) || ('0' <= c && c <= '9');
}

Token *tokenize(char *p)
{
  Token head;
  head.next = NULL;
  Token *cur = &head;

  while (*p)
  {
    if (isspace(*p))
    {
      p++;
      continue;
    }

    if (startswith(p, "<=") || startswith(p, ">=") || startswith(p, "==") || startswith(p, "!="))
    {
      cur = new_token(TK_RESERVED, cur, p, 2);
      p += 2;
      continue;
    }

    if (strchr("+-*/()<>;={},&", *p))
    {
      cur = new_token(TK_RESERVED, cur, p++, 1);
      continue;
    }

    if (strncmp(p, "return", 6) == 0 && !is_alnum(p[6]))
    {
      cur = new_token(TK_RETURN, cur, p, 6);
      p += 6;
      continue;
    }

    if (strncmp(p, "if", 2) == 0 && !is_alnum(p[2]))
    {
      cur = new_token(TK_IF, cur, p, 2);
      p += 2;
      continue;
    }

    if (strncmp(p, "else", 4) == 0 && !is_alnum(p[4]))
    {
      cur = new_token(TK_ELSE, cur, p, 4);
      p += 4;
      continue;
    }

    if (strncmp(p, "while", 5) == 0 && !is_alnum(p[5]))
    {
      cur = new_token(TK_WHILE, cur, p, 5);
      p += 5;
      continue;
    }

    if (strncmp(p, "for", 3) == 0 && !is_alnum(p[3]))
    {
      cur = new_token(TK_FOR, cur, p, 3);
      p += 3;
      continue;
    }

    if (is_alpha(*p))
    {
      cur = new_token(TK_IDENT, cur, p++, 1);
      while (is_alnum(*p))
      {
        cur->len++;
        p++;
      }
      continue;
    }

    if (isdigit(*p))
    {
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

LVar *find_lvar(Token *tok)
{
  for (LVar *var = locals; var; var = var->next)
    if (var->len == tok->len && !memcmp(tok->str, var->name, var->len))
      return var;
  return NULL;
}

/*
program    = func*
func       = ident "(" (ident ("," ident)*)? ")" block
stmt       = expr ";"
           | block
           | "if" "(" expr ")" stmt ("else" stmt)?
           | "while" "(" expr ")" stmt
           | "for" "(" expr? ";" expr? ";" expr? ")" stmt
           | "return" expr ";"
block      = "{" stmt* "}"
expr       = assign
assign     = equality ("=" assign)?
equality   = relational ("==" relational | "!=" relational)*
relational = add ("<" add | "<=" add | ">" add | ">=" add)*
add        = mul ("+" mul | "-" mul)*
mul        = unary ("*" unary | "/" unary)*
unary      = ("+" | "-" | "*" | "&")? primary
primary    = num
           | ident ( "(" (expr ("," expr)*)? ")" )?
           | "(" expr ")"
*/
Node *code[100];
void program();
Node *func();
Node *block();
Node *stmt();
Node *expr();
Node *assign();
Node *equality();
Node *relational();
Node *add();
Node *mul();
Node *unary();
Node *primary();
Node *lval();

void program()
{
  int i = 0;
  while (!at_eof())
    code[i++] = func();
  code[i] = NULL;
}

Node *func()
{
  Node *node = calloc(1, sizeof(Node));
  node->kind = ND_FUNC;

  Token *tok = expect_ident();
  node->fname = tok->str;
  node->len = tok->len;
  locals = NULL;

  expect("(");
  if (!consume(")"))
  {
    Node head = {};
    Node *current = &head;

    tok = expect_ident();
    current->next = lval(tok);
    current = current->next;

    while (!consume(")"))
    {
      expect(",");
      tok = expect_ident();
      current->next = lval(tok);
      current = current->next;
    }
    node->args = head.next;
  }

  node->fbody = block();
  return node;
}

Node *block()
{
  Node *node = calloc(1, sizeof(Node));
  node->kind = ND_BLOCK;

  expect("{");
  Node *current = node;
  while (!consume("}"))
  {
    current->next = stmt();
    current = current->next;
  }
  return node;
}

Node *stmt()
{
  Node *node;
  if (peek("{"))
  {
    return block();
  }
  else if (consume_token(TK_IF))
  {
    node = calloc(1, sizeof(Node));
    node->kind = ND_IF;
    expect("(");
    node->cond = expr();
    expect(")");
    node->then = stmt();
    if (consume_token(TK_ELSE))
    {
      node->els = stmt();
    }
  }
  else if (consume_token(TK_WHILE))
  {
    node = calloc(1, sizeof(Node));
    node->kind = ND_WHILE;
    expect("(");
    node->cond = expr();
    expect(")");
    node->then = stmt();
  }
  else if (consume_token(TK_FOR))
  {
    node = calloc(1, sizeof(Node));
    node->kind = ND_FOR;
    expect("(");
    if (!consume(";"))
    {
      node->init = expr();
      expect(";");
    }
    if (!consume(";"))
    {
      node->cond = expr();
      expect(";");
    }
    if (!consume(")"))
    {
      node->inc = expr();
      expect(")");
    }
    node->then = stmt();
  }
  else if (consume_token(TK_RETURN))
  {
    node = calloc(1, sizeof(Node));
    node->kind = ND_RETURN;
    node->lhs = expr();
    expect(";");
  }
  else
  {
    node = expr();
    expect(";");
  }

  return node;
}

Node *expr()
{
  return assign();
}

Node *assign()
{
  Node *node = equality();
  if (consume("="))
    node = new_node(ND_ASSIGN, node, assign());
  return node;
}

Node *equality()
{
  Node *node = relational();

  for (;;)
  {
    if (consume("=="))
      node = new_node(ND_EQ, node, relational());
    else if (consume("!="))
      node = new_node(ND_NE, node, relational());
    else
      return node;
  }
}

Node *relational()
{
  Node *node = add();
  for (;;)
  {
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

Node *add()
{
  Node *node = mul();

  for (;;)
  {
    if (consume("+"))
      node = new_node(ND_ADD, node, mul());
    else if (consume("-"))
      node = new_node(ND_SUB, node, mul());
    else
      return node;
  }
}

Node *mul()
{
  Node *node = unary();

  for (;;)
  {
    if (consume("*"))
      node = new_node(ND_MUL, node, unary());
    else if (consume("/"))
      node = new_node(ND_DIV, node, unary());
    else
      return node;
  }
}

Node *unary()
{
  if (consume("+"))
    return primary();
  if (consume("-"))
    return new_node(ND_SUB, new_node_num(0), primary());
  if (consume("*"))
    return new_node(ND_DEREF, primary(), NULL);
  if (consume("&"))
    return new_node(ND_ADDR, primary(), NULL);
  return primary();
}

Node *lval(Token *tok)
{
  Node *node = calloc(1, sizeof(Node));
  node->kind = ND_LVAR;

  LVar *lvar = find_lvar(tok);
  if (lvar)
  {
    node->offset = lvar->offset;
  }
  else
  {
    lvar = calloc(1, sizeof(LVar));
    lvar->next = locals;
    lvar->name = tok->str;
    lvar->len = tok->len;
    lvar->offset = (locals ? locals->offset : 0) + 8;
    node->offset = lvar->offset;
    locals = lvar;
  }
  return node;
}

Node *primary()
{
  if (consume("("))
  {
    Node *node = expr();
    expect(")");
    return node;
  }

  Token *tok = consume_ident();
  if (tok)
  {
    if (consume("("))
    {
      Node *node = calloc(1, sizeof(Node));
      node->kind = ND_FUNCALL;
      node->fname = tok->str;
      node->len = tok->len;
      Node head = {};
      Node *current = &head;
      if (!consume(")"))
      {
        current->next = expr();
        current = current->next;
        while (!consume(")"))
        {
          expect(",");
          current->next = expr();
          current = current->next;
        }
        node->args = head.next;
      }
      return node;
    }
    else
    {
      return lval(tok);
    }
  }

  return new_node_num(expect_number());
}

void parse()
{
  program();
  return;
}
