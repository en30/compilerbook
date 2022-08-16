#include "9cc.h"

Token *token;
Token *new_token(TokenKind kind, Token *cur, char *str, int len) {
  Token *tok = calloc(1, sizeof(Token));
  tok->kind = kind;
  tok->str = str;
  tok->len = len;
  cur->next = tok;
  return tok;
}

char *kind_name(TokenKind kind) {
  switch (kind) {
    case TK_PUNCT:
      return "punctuator";
    case TK_IDENT:
      return "identifier";
    case TK_NUM:
      return "number";
    case TK_RETURN:
      return "return";
    case TK_IF:
      return "if";
    case TK_ELSE:
      return "else";
    case TK_WHILE:
      return "while";
    case TK_FOR:
      return "for";
    case TK_EOF:
      return "eof";
    case TK_CHAR:
      return "char";
    case TK_INT:
      return "int";
    case TK_SIZEOF:
      return "sizeof";
    case TK_STR:
      return "string";
    default:
      error("no token kind name");
  }
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
  static char *kw[] = {
      "sizeof", "void", "char",  "int", "return",
      "if",     "else", "while", "for", "struct",
  };
  static int kind[] = {TK_SIZEOF, TK_VOID, TK_CHAR,  TK_INT, TK_RETURN,
                       TK_IF,     TK_ELSE, TK_WHILE, TK_FOR, TK_STRUCT};

  static char *puncts[] = {"<=", ">=", "==", "!=", "->", "+", "-", "*",
                           "/",  "(",  ")",  "<",  ">",  ";", "=", "{",
                           "}",  ",",  "&",  "[",  "]",  "."};

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

    for (int i = 0; i < sizeof(puncts) / sizeof(puncts[0]); i++) {
      int l = strlen(puncts[i]);
      if (strncmp(p, puncts[i], l) == 0) {
        cur = new_token(TK_PUNCT, cur, p, l);
        p += l;
        goto next;
      }
    }

    for (int i = 0; i < sizeof(kind) / sizeof(kind[0]); i++) {
      int l = strlen(kw[i]);
      if (strncmp(p, kw[i], l) == 0 && !is_alnum(p[l])) {
        cur = new_token(kind[i], cur, p, l);
        p += l;
        goto next;
      }
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
  next:
  }

  new_token(TK_EOF, cur, p, 0);
  return head.next;
}

Token *peek_punct(char *op) {
  if (token->kind != TK_PUNCT || strlen(op) != token->len ||
      memcmp(token->str, op, token->len))
    return NULL;
  return token;
}

Token *consume_punct(char *op) {
  if (token->kind != TK_PUNCT || strlen(op) != token->len ||
      memcmp(token->str, op, token->len))
    return NULL;
  Token *tok = token;
  token = token->next;
  return tok;
}

Token *expect_punct(char *op) {
  if (token->kind != TK_PUNCT || strlen(op) != token->len ||
      memcmp(token->str, op, token->len))
    error_at(token->str, "\"%s\"ではありません", op);
  Token *tok = token;
  token = token->next;
  return tok;
}

Token *consume(TokenKind kind) {
  if (token->kind != kind) return NULL;
  Token *tok = token;
  token = token->next;
  return tok;
}

Token *expect(TokenKind kind) {
  if (token->kind != kind)
    error_at(token->str, "%sではありません。", kind_name(kind));
  Token *tok = token;
  token = token->next;
  return tok;
}

bool peek(TokenKind kind) { return token->kind == kind; }

int token_value(Token *token) { return token->val; }

bool token_equal(Token *lhs, Token *rhs) {
  return lhs->kind == rhs->kind && lhs->len == rhs->len &&
         !memcmp(lhs->str, rhs->str, lhs->len);
}
