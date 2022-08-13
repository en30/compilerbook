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
  return 8;
}
