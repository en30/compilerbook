#include "9cc.h"

char *argreg[] = {"rdi", "rsi", "rdx", "rcx", "r8", "r9"};

int id() {
  static int i = 0;
  return i++;
}

bool is_int(Node *node) { return node_type(node)->ty == TY_INT; }

bool is_pointer(Node *node) {
  return (node->kind == ND_LVAR && node->lvar->type->ty == TY_PTR);
}

bool is_array_addr(Node *node) {
  return (node->kind == ND_ADDR && node->lhs->kind == ND_LVAR &&
          node->lhs->lvar->type->ty == TY_ARRAY);
}

bool is_addr(Node *node) { return is_pointer(node) || is_array_addr(node); }

int stride(Node *node) {
  if (is_pointer(node) && node->lvar->type->ptr_to->ty == TY_INT) return 4;
  if (is_array_addr(node) && node->lhs->lvar->type->ptr_to->ty == TY_INT)
    return 4;
  return 8;
}

int byte_size(Type *type) {
  switch (type->ty) {
    case TY_INT:
      return 1;
      break;
    case TY_PTR:
      return 1;
    case TY_ARRAY:
      return type->array_size * byte_size(type->ptr_to);
    default:
      break;
  }
}

void gen(Node *node);

void gen_lval(Node *node) {
  switch (node->kind) {
    case ND_DEREF:
      gen(node->lhs);
      return;
    case ND_GVAR:
      printf("  lea rax, [rip+%.*s]\n", node->gvar->len, node->gvar->name);
      printf("  push rax\n");
      return;
    case ND_LVAR:
      printf("  mov rax, rbp\n");
      printf("  sub rax, %d\n", node->lvar->offset);
      printf("  push rax\n");
      return;
    default:
      error("代入の左辺値が変数ではありません");
      return;
  }
}

void gen(Node *node) {
  int i;
  switch (node->kind) {
    case ND_NUM:
      printf("  push %d\n", node->val);
      return;
    case ND_GVAR:
      gen_lval(node);
      printf("  pop rax\n");
      printf("  mov rax, [rax]\n");
      printf("  push rax\n");
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
      if (node->els) gen(node->els);
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
      if (node->init) gen(node->init);
      printf(".Lbegin%d:\n", i);
      if (node->cond) gen(node->cond);
      printf("  pop rax\n");
      printf("  cmp rax, 0\n");
      printf("  je  .Lend%d\n", i);
      gen(node->then);
      if (node->inc) gen(node->inc);
      printf("  jmp .Lbegin%d\n", i);
      printf(".Lend%d:\n", i);
      return;
    case ND_BLOCK:
      for (Node *n = node->next; n; n = n->next) {
        gen(n);
        if (n->next) printf("  pop rax\n");
      }
      return;
    case ND_FUNCALL:
      i = id();
      int nargs = 0;
      for (Node *n = node->args; n; n = n->next) {
        gen(n);
        nargs++;
      }
      for (int j = nargs - 1; j >= 0; j--) {
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
    case ND_GVARDEC:
      printf(".globl %.*s\n", node->gvar->len, node->gvar->name);
      printf(".bss\n");
      printf("%.*s:\n", node->gvar->len, node->gvar->name);
      printf("  .zero %d\n", byte_size(node->gvar->type) * 8);
      return;
    case ND_FUNC:
      printf(".text\n");
      printf(".globl %.*s\n", node->len, node->fname);
      printf("%.*s:\n", node->len, node->fname);

      // プロローグ
      int sz = 0;
      for (LVar *l = node->locals; l; l = l->next) sz += byte_size(l->type);
      for (Node *n = node->args; n; n = n->next) sz += byte_size(n->lvar->type);
      printf("  push rbp\n");
      printf("  mov rbp, rsp\n");
      printf("  sub rsp, %d\n", sz * 8);

      // 引数
      int j = 0;
      for (Node *n = node->args; n; n = n->next) {
        gen_lval(n);
        printf("  pop rax\n");
        printf("  mov [rax], %s\n", argreg[j++]);
      }

      gen(node->fbody);
      printf("  pop rax\n");

      // エピローグ
      // 最後の式の結果がRAXに残っているのでそれが返り値になる
      printf("  mov rsp, rbp\n");
      printf("  pop rbp\n");
      printf("  ret\n");

      return;
    case ND_ADDR:
      gen_lval(node->lhs);
      return;
    case ND_DEREF:
      gen(node->lhs);
      printf("  pop rax\n");
      printf("  mov rax, [rax]\n");
      printf("  push rax\n");
      return;
  }

  gen(node->lhs);
  gen(node->rhs);

  printf("  pop rdi\n");
  printf("  pop rax\n");

  switch (node->kind) {
    case ND_ADD:
      if (is_addr(node->lhs) && is_int(node->rhs)) {
        printf("  imul rdi, %d\n", stride(node->lhs));
      } else if (is_int(node->lhs) && is_addr(node->rhs)) {
        printf("  imul rax, %d\n", stride(node->rhs));
      }
      printf("  add rax, rdi\n");
      break;
    case ND_SUB:
      if (is_addr(node->lhs) && is_int(node->rhs)) {
        printf("  imul rdi, %d\n", stride(node->lhs));
      } else if (is_int(node->lhs) && is_addr(node->rhs)) {
        printf("  imul rax, %d\n", stride(node->rhs));
      }
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

void codegen(Node *funcs) {
  printf(".intel_syntax noprefix\n");

  for (Node *n = funcs; n; n = n->next) {
    gen(n);
  }
}
