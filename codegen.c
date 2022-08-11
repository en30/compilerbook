#include "9cc.h"

char *argreg[] = {"rdi", "rsi", "rdx", "rcx", "r8", "r9"};

int id() {
  static int i = 0;
  return i++;
}

bool is_char(Node *node) { return node->type->ty == TY_CHAR; }
bool is_int(Node *node) { return node->type->ty == TY_INT; }

bool is_lvar_pointer(Node *node) {
  return (node->kind == ND_LVAR && node->lvar->type->ty == TY_PTR);
}

bool is_lvar_array_addr(Node *node) {
  return (node->kind == ND_ADDR && node->lhs->kind == ND_LVAR &&
          node->lhs->lvar->type->ty == TY_ARRAY);
}
bool is_gvar_pointer(Node *node) {
  return (node->kind == ND_GVAR && node->gvar->type->ty == TY_PTR);
}

bool is_gvar_array_addr(Node *node) {
  return (node->kind == ND_ADDR && node->lhs->kind == ND_GVAR &&
          node->lhs->gvar->type->ty == TY_ARRAY);
}

bool is_addr(Node *node) {
  return is_lvar_pointer(node) || is_lvar_array_addr(node) ||
         is_gvar_pointer(node) || is_gvar_array_addr(node);
}

int stride(Node *node) {
  if (is_lvar_pointer(node) && node->lvar->type->ptr_to->ty == TY_CHAR)
    return 1;
  if (is_lvar_pointer(node) && node->lvar->type->ptr_to->ty == TY_INT) return 4;
  if (is_lvar_array_addr(node) && node->lhs->lvar->type->ptr_to->ty == TY_CHAR)
    return 1;
  if (is_lvar_array_addr(node) && node->lhs->lvar->type->ptr_to->ty == TY_INT)
    return 4;
  if (is_gvar_pointer(node) && node->gvar->type->ptr_to->ty == TY_CHAR)
    return 1;
  if (is_gvar_pointer(node) && node->gvar->type->ptr_to->ty == TY_INT) return 4;
  if (is_gvar_array_addr(node) && node->lhs->gvar->type->ptr_to->ty == TY_CHAR)
    return 1;
  if (is_gvar_array_addr(node) && node->lhs->gvar->type->ptr_to->ty == TY_INT)
    return 4;
  return 8;
}

int byte_size(Type *type) {
  switch (type->ty) {
    case TY_CHAR:
      return 8;
    case TY_INT:
      return 8;
    case TY_PTR:
      return 8;
    case TY_ARRAY:
      return type->array_size * byte_size(type->ptr_to);
    default:
      break;
  }
}

void load_var(Node *node) {
  printf("  pop rax\n");
  if (type_size(node->type) == 1) {
    printf("  movsx rax, BYTE PTR [rax]\n");
  } else {
    printf("  mov rax, [rax]\n");
  }
  printf("  push rax\n");
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
      load_var(node);
      return;
    case ND_LVAR:
      gen_lval(node);
      load_var(node);
      return;
    case ND_ASSIGN:
      gen_lval(node->lhs);
      gen(node->rhs);

      printf("  pop rdi\n");
      printf("  pop rax\n");
      if (byte_size(node->lhs->type) == 1) {
        printf("  mov [rax], dil\n");
      } else {
        printf("  mov [rax], rdi\n");
      }
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
      // popできるダミーの値を詰めておく
      printf("  push 0\n");
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
      // popできるダミーの値を詰めておく
      printf("  push 0\n");
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

      printf("  mov al, 0\n");
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
      if (node->literal_data) {
        printf("%.*s:\n", node->gvar->len, node->gvar->name);
        printf("  .string \"%.*s\"\n", node->len, node->literal_data);
      } else {
        printf(".globl %.*s\n", node->gvar->len, node->gvar->name);
        printf(".bss\n");
        printf("%.*s:\n", node->gvar->len, node->gvar->name);
        printf("  .zero %d\n", byte_size(node->gvar->type));
      }
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
      printf("  sub rsp, %d\n", sz);

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
      load_var(node);
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

  printf(".section .rodata\n");
  for (Node *s = strings; s; s = s->next) {
    gen(s);
  }
  for (Node *n = funcs; n; n = n->next) {
    gen(n);
  }
}
