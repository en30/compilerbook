#include "9cc.h"

char *argreg[] = {"rdi", "rsi", "rdx", "rcx", "r8", "r9"};

int id()
{
  static int i = 0;
  return i++;
}

void gen_lval(Node *node)
{
  if (node->kind != ND_LVAR)
    error("代入の左辺値が変数ではありません");

  printf("  mov rax, rbp\n");
  printf("  sub rax, %d\n", node->offset);
  printf("  push rax\n");
}

void gen(Node *node)
{
  int i;
  switch (node->kind)
  {
  case ND_NUM:
    printf("  push %d\n", node->val);
    return;
  case ND_LVAR:
    gen_lval(node);
    printf("  pop rax\n");
    printf("  mov rax, [rax]\n");
    printf("  push rax\n");
    return;
  case ND_ASSIGN:
    gen_lval(node->lhs);
    gen(node->rhs);

    printf("  pop rdi\n");
    printf("  pop rax\n");
    printf("  mov [rax], rdi\n");
    printf("  push rdi\n");
    return;
  case ND_RETURN:
    gen(node->lhs);
    printf("  pop rax\n");
    printf("  mov rsp, rbp\n");
    printf("  pop rbp\n");
    printf("  ret\n");
    return;
  case ND_IF:
    i = id();
    gen(node->cond);
    printf("  pop rax\n");
    printf("  cmp rax, 0\n");
    printf("  je  .Lelse%d\n", i);
    gen(node->then);
    printf("  jmp  .Lend%d\n", i);
    printf(".Lelse%d:\n", i);
    if (node->els)
      gen(node->els);
    printf(".Lend%d:\n", i);
    return;
  case ND_WHILE:
    i = id();
    printf(".Lbegin%d:\n", i);
    gen(node->cond);
    printf("  pop rax\n");
    printf("  cmp rax, 0\n");
    printf("  je  .Lend%d\n", i);
    gen(node->then);
    printf("  jmp .Lbegin%d\n", i);
    printf(".Lend%d:\n", i);
    return;
  case ND_FOR:
    i = id();
    if (node->init)
      gen(node->init);
    printf(".Lbegin%d:\n", i);
    if (node->cond)
      gen(node->cond);
    printf("  pop rax\n");
    printf("  cmp rax, 0\n");
    printf("  je  .Lend%d\n", i);
    gen(node->then);
    if (node->inc)
      gen(node->inc);
    printf("  jmp .Lbegin%d\n", i);
    printf(".Lend%d:\n", i);
    return;
  case ND_BLOCK:
    for (Node *n = node->next; n; n = n->next)
    {
      gen(n);
      if (n->next)
        printf("  pop rax\n");
    }
    return;
  case ND_FUNCALL:
    i = id();
    int nargs = 0;
    for (Node *n = node->args; n; n = n->next)
    {
      gen(n);
      nargs++;
    }
    for (int j = nargs - 1; j >= 0; j--)
    {
      printf("  pop %s\n", argreg[j]);
    }

    // align rsp
    printf("  mov rax, rsp\n");
    printf("  and rax, 15\n");
    printf("  cmp rax, 0\n");
    printf("  jne .Lalign%d\n", i);
    printf("  mov rax, %d\n", nargs);
    printf("  call %.*s\n", node->len, node->fname);
    printf("  push rax\n");
    printf("  jmp .Lend%d\n", i);
    printf(".Lalign%d:\n", i);
    printf("  sub rsp, 8\n");
    printf("  mov rax, %d\n", nargs);
    printf("  call %.*s\n", node->len, node->fname);
    printf("  add rsp, 8\n");
    printf("  push rax\n");
    printf(".Lend%d:\n", i);
    return;
  case ND_FUNC:
    printf("%.*s:\n", node->len, node->fname);

    // プロローグ
    // 変数26個分の領域を確保
    printf("  push rbp\n");
    printf("  mov rbp, rsp\n");
    printf("  sub rsp, 208\n");

    gen(node->fbody);
    printf("  pop rax\n");

    // エピローグ
    // 最後の式の結果がRAXに残っているのでそれが返り値になる
    printf("  mov rsp, rbp\n");
    printf("  pop rbp\n");
    printf("  ret\n");

    return;
  }

  gen(node->lhs);
  gen(node->rhs);

  printf("  pop rdi\n");
  printf("  pop rax\n");

  switch (node->kind)
  {
  case ND_ADD:
    printf("  add rax, rdi\n");
    break;
  case ND_SUB:
    printf("  sub rax, rdi\n");
    break;
  case ND_MUL:
    printf("  imul rax, rdi\n");
    break;
  case ND_DIV:
    printf("  cqo\n");
    printf("  idiv rdi\n");
    break;
  case ND_EQ:
    printf("  cmp rax, rdi\n");
    printf("  sete al\n");
    printf("  movzb rax, al\n");
    break;
  case ND_NE:
    printf("  cmp rax, rdi\n");
    printf("  setne al\n");
    printf("  movzb rax, al\n");
    break;
  case ND_LT:
    printf("  cmp rax, rdi\n");
    printf("  setl al\n");
    printf("  movzb rax, al\n");
    break;
  case ND_LE:
    printf("  cmp rax, rdi\n");
    printf("  setle al\n");
    printf("  movzb rax, al\n");
    break;
  }

  printf("  push rax\n");
}

void codegen()
{
  printf(".intel_syntax noprefix\n");
  printf(".global main\n");

  for (int i = 0; code[i]; i++)
  {
    gen(code[i]);
  }
}
