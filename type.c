#include "9cc.h"

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

Type *new_anonymous_struct() {
  Type *t = calloc(1, sizeof(Type));
  t->ty = TY_STRUCT;
  return t;
}

Type *new_struct(Token *tok) {
  Type *t = calloc(1, sizeof(Type));
  t->ty = TY_STRUCT;
  t->name = tok;
  return t;
}

void *set_member(Type *t, Member *member) {
  t->member = member;
  for (Member *m = member; m; m = m->next) {
    t->size += type_size(m->type);
  }
}

Member *find_member(Type *t, Token *ident) {
  for (Member *m = t->member; m; m = m->next) {
    if (token_equal(m->name, ident)) return m;
  }
  return NULL;
}

Type *type_add(Type *lhs, Type *rhs) {
  if (is_effectively_pointer(lhs) && is_int(rhs))
    return new_pointer_to(lhs->ptr_to);
  if (is_effectively_pointer(lhs) && is_effectively_pointer(rhs)) {
    error("invalid operation: ptr + ptr");
  }
  return type_size(lhs) > type_size(rhs) ? lhs : rhs;
}

Type *type_sub(Type *lhs, Type *rhs) {
  if (is_effectively_pointer(lhs) && is_int(rhs))
    return new_pointer_to(lhs->ptr_to);
  if (is_effectively_pointer(lhs) && is_effectively_pointer(rhs)) {
    return &int_type;
  }
  return type_size(lhs) > type_size(rhs) ? lhs : rhs;
}

bool is_effectively_pointer(Type *type) {
  return type->ty == TY_PTR || type->ty == TY_ARRAY;
}

bool is_int(Type *type) { return type->ty == TY_INT; }

// かなりメモリに無駄が多くなってしまっているけれど、
// align周りがよくわかっていないので余分にとっている
int type_address_size(Type *type) {
  if (type->ty == TY_ARRAY)
    return type->array_size * type_address_size(type->ptr_to);
  if (type->ty == TY_STRUCT) {
    int s = 0;
    for (Member *m = type->member; m; m = m->next) {
      s += type_address_size(m->type);
    }
    return s;
  }
  return 8;
}
