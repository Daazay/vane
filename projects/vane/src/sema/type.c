#include "vane/sema/type.h"

#include <stdlib.h>
#include <stdio.h>

#include "vane/utils/hash.h"
#include "vane/utils/string_builder.h"
#include "vane/sema/type_system.h"

const char* type_kind_get_name(TypeKind kind) {
    switch (kind) {
    case TYPE_UNRESOLVED: return "unresolved";
    case TYPE_BUILTIN:    return "builtin";
    case TYPE_POINTER:    return "pointer";
    case TYPE_ALIAS:      return "alias";
    case TYPE_ARRAY:      return "array";
    case TYPE_SLICE:      return "slice";
    case TYPE_FUNCTION:   return "function";
    default:
        unreachable();
        return NULL;
    }
}

const char* type_builtin_kind_get_name(TypeBuiltinKind kind) {
    switch (kind) {
    case TYPE_BUILTIN_VOID:        return "void";
    case TYPE_BUILTIN_BOOL:        return "bool";
    case TYPE_BUILTIN_UNSIZED_INT: return "unsized int";
    case TYPE_BUILTIN_U8:          return "u8";
    case TYPE_BUILTIN_I8:          return "i8";
    case TYPE_BUILTIN_U16:         return "u16";
    case TYPE_BUILTIN_I16:         return "i16";
    case TYPE_BUILTIN_U32:         return "u32";
    case TYPE_BUILTIN_I32:         return "i32";
    case TYPE_BUILTIN_U64:         return "u64";
    case TYPE_BUILTIN_I64:         return "i64";
    case TYPE_BUILTIN_ANY:         return "any";
    default:
        unreachable();
        return NULL;
    }
}

bool is_type_builtin_kind_signed(TypeBuiltinKind kind) {
    return kind == TYPE_BUILTIN_I8  ||
           kind == TYPE_BUILTIN_I16 ||
           kind == TYPE_BUILTIN_I32 ||
           kind == TYPE_BUILTIN_I64;
}

bool is_type_builtin_kind_unsigned(TypeBuiltinKind kind) {
    return kind == TYPE_BUILTIN_U8  ||
           kind == TYPE_BUILTIN_U16 ||
           kind == TYPE_BUILTIN_U32 ||
           kind == TYPE_BUILTIN_U64;
}

u32 type_builtin_kind_get_size(TypeBuiltinKind kind) {
    switch (kind) {
    case TYPE_BUILTIN_U8:
    case TYPE_BUILTIN_I8:  return 1;
    case TYPE_BUILTIN_U16:
    case TYPE_BUILTIN_I16: return 2;
    case TYPE_BUILTIN_U32:
    case TYPE_BUILTIN_I32: return 4;
    case TYPE_BUILTIN_U64:
    case TYPE_BUILTIN_I64: return 8;
    default:
        unreachable();
        return 0;
    }
}

Type* type_create(TypeKind kind, u32 size, u32 alignment) {
    Type* type = malloc(sizeof(Type));
    assert(type != NULL);

    type->kind = kind;
    type->size = size;
    type->alignment = alignment;

    return type;
}

void type_destroy(Type* type) {
    if (type == NULL) {
        return;
    }

    if (type->kind == TYPE_BUILTIN) {
        return;
    }

    if (type->kind == TYPE_FUNCTION) {
        free(type->as.fun.params);
    }

    free(type);
}

Type type_builtin_create(TypeBuiltinKind kind, u32 size, u32 alignment) {
    return (Type) {
        .kind = TYPE_BUILTIN,
        .as.builtin.kind = kind,
        .size = size,
        .alignment = alignment,
    };
}

Type* type_pointer_create(const TypeSystem* ts, const Type* base) {
    assert(base != NULL);

    Type* type = type_create(TYPE_POINTER, ts->target.pointer_size, ts->target.pointer_size);
    type->as.pointer.base = base;

    return type;
}

Type* type_alias_create(const TypeSystem* ts, StringView name, const Type* target) {
    assert(target != NULL);

    (void)ts;

    Type* type = type_create(TYPE_ALIAS, target->size, target->alignment);
    type->as.alias.name = name;
    type->as.alias.target = target;

    return type;
}

Type* type_array_create(const TypeSystem* ts, const Type* elem, u32 size) {
    assert(elem != NULL);

    const u32 array_size = elem->size * size;

    Type* type = type_create(TYPE_ARRAY, array_size, ts->target.pointer_size);
    type->as.array.elem = elem;
    type->as.array.size = size;

    return type;
}

Type* type_slice_create(const TypeSystem* ts, const Type* elem) {
    assert(elem != NULL);

    // pointer + length
    u32 slice_size = ts->target.pointer_size * 2;

    Type* type = type_create(TYPE_SLICE, slice_size, ts->target.pointer_size);
    type->as.slice.elem = elem;

    return type;
}

Type* type_fun_create(const TypeSystem* ts, const Type** params, u32 param_count, const Type* ret) {
    assert(ret != NULL && ((params != NULL && param_count > 0) || params == NULL));

    Type* type = type_create(TYPE_FUNCTION, ts->target.pointer_size, ts->target.pointer_size);
    type->as.fun.params = params;
    type->as.fun.param_count = param_count;
    type->as.fun.ret = ret;

    return type;
}

Type* type_unresolved_create(StringView name) {
    Type* type = malloc(sizeof(Type));
    assert(type != NULL);

    type->kind = TYPE_UNRESOLVED;
    type->as.unresolved.name = name;

    return type;
}

bool type_eq_type(const Type* type1, const Type* type2) {
    if (type1 == NULL || type2 == NULL) {
        return false;
    }

    const Type* t1 = type_unwrap(type1);
    const Type* t2 = type_unwrap(type2);

    if (t1 == t2) {
        return true;
    }

    if (t1->kind != t2->kind) {
        return false;
    }

    switch (t1->kind) {
    case TYPE_UNRESOLVED:
        return string_view_eq_sv(t1->as.unresolved.name, t2->as.unresolved.name);

    case TYPE_BUILTIN:
        return t1->as.builtin.kind == t2->as.builtin.kind;

    case TYPE_POINTER:
        return type_eq_type(t1->as.pointer.base, t2->as.pointer.base);

    case TYPE_ARRAY:
        return (t1->as.array.size == t2->as.array.size) &&
                type_eq_type(t1->as.array.elem, t2->as.array.elem);

    case TYPE_SLICE:
        return type_eq_type(t1->as.slice.elem, t2->as.slice.elem);

    case TYPE_FUNCTION:
        if (!type_eq_type(t1->as.fun.ret, t2->as.fun.ret)) {
            return false;
        }

        if (t1->as.fun.param_count != t2->as.fun.param_count) {
            return false;
        }

        for (u32 i = 0; i < t1->as.fun.param_count; ++i) {
            if (!type_eq_type(t1->as.fun.params[i], t2->as.fun.params[i])) {
                return false;
            }
        }
        return true;

    default:
        unreachable();
        return false;
    }
}

u32 type_get_hash(const Type* type) {
    if (type == NULL) {
        return 0;
    }

    u32 hash = (u32)type->kind;

    switch (type->kind) {
    case TYPE_UNRESOLVED:
        return hash_combine_u32(hash, string_view_get_hash(type->as.unresolved.name));

    case TYPE_BUILTIN:
        return hash_combine_u32(hash, type->as.builtin.kind);

    case TYPE_POINTER:
        return hash_combine_u32(hash, type_get_hash(type->as.pointer.base));

    case TYPE_ALIAS:
        hash = hash_combine_u32(hash, type_get_hash(type->as.alias.target));
        return hash_combine_u32(hash, string_view_get_hash(type->as.alias.name));

    case TYPE_ARRAY:
        hash = hash_combine_u32(hash, type_get_hash(type->as.array.elem));
        return hash_combine_u32(hash, type->as.array.size);

    case TYPE_SLICE:
        return hash_combine_u32(hash, type_get_hash(type->as.slice.elem));

    case TYPE_FUNCTION:
        hash = hash_combine_u32(hash, type_get_hash(type->as.fun.ret));
        for (u32 i = 0; i < type->as.fun.param_count; ++i) {
            hash = hash_combine_u32(hash, type_get_hash(type->as.fun.params[i]));
        }
        return hash;

    default:
        unreachable();
        return 0;
    }
}

static inline void construct_type_str(StringBuilder* sb, const Type* type) {
    if (type == NULL) {
        string_builder_append_cstr(sb, "<null>");
        return;
    }

    switch (type->kind) {
    case TYPE_UNRESOLVED: {
        string_builder_append_cstr(sb, "<unresolved ");
        string_builder_append_sv(sb, type->as.unresolved.name);
        string_builder_append_c(sb, '>');
    } break;

    case TYPE_BUILTIN: {
        const char* name = type_builtin_kind_get_name(type->as.builtin.kind);
        string_builder_append_cstr(sb, name);
    } break;

    case TYPE_POINTER: {
        string_builder_append_c(sb, '^');
        construct_type_str(sb, type->as.pointer.base);
    } break;

    case TYPE_ALIAS: {
        /* For diagnostics we prefer the alias name as written by the user. */
        string_builder_append_sv(sb, type->as.alias.name);
    } break;

    case TYPE_ARRAY: {
        string_builder_append_c(sb, '[');
        string_builder_append_fmt(sb, "%d", type->as.array.size);
        string_builder_append_c(sb, ']');
        construct_type_str(sb, type->as.array.elem);
    } break;

    case TYPE_SLICE: {
        string_builder_append_cstr(sb, "[]");
        construct_type_str(sb, type->as.slice.elem);
    } break;

    case TYPE_FUNCTION: {
        string_builder_append_c(sb, '(');
        for (u32 i = 0; i < type->as.fun.param_count; ++i) {
            if (i > 0) {
                string_builder_append_cstr(sb, ", ");
            }
            const Type* p = type->as.fun.params[i];
            if (p != NULL) {
                construct_type_str(sb, p);
            }
            else {
                string_builder_append_cstr(sb, "<null>");
            }
        }
        string_builder_append_cstr(sb, ") -> ");
        construct_type_str(sb, type->as.fun.ret);
    } break;

    default:
        string_builder_append_cstr(sb, "<type>");
        break;
    }
}

String type_to_str(const Type* type) {
    StringBuilder sb = string_builder_create(8);
    construct_type_str(&sb, type);
    return string_builder_release(&sb);
}

const Type* type_unwrap(const Type* type) {
    assert(type != NULL);

    const Type* t = type;
    u32 guard = 0;
    while (t != NULL && t->kind == TYPE_ALIAS && guard++ < TYPE_UNWRAP_GUARD) {
        t = t->as.alias.target;
    }

    return t;
}

bool is_type_builtin(const Type* type, TypeBuiltinKind kind) {
    type = type_unwrap(type);
    return type != NULL && type->kind == TYPE_BUILTIN && type->as.builtin.kind == kind;
}

bool is_type_any(const Type* type) {
    return is_type_builtin(type, TYPE_BUILTIN_ANY);
}

bool is_type_bool(const Type* type) {
    return is_type_builtin(type, TYPE_BUILTIN_BOOL);
}

bool is_type_unsized_integer(const Type* type) {
    return is_type_builtin(type, TYPE_BUILTIN_UNSIZED_INT);
}

static inline bool is_type_sized_integer_kind(TypeBuiltinKind kind) {
    switch (kind) {
    case TYPE_BUILTIN_U8:
    case TYPE_BUILTIN_I8:
    case TYPE_BUILTIN_U16:
    case TYPE_BUILTIN_I16:
    case TYPE_BUILTIN_U32:
    case TYPE_BUILTIN_I32:
    case TYPE_BUILTIN_U64:
    case TYPE_BUILTIN_I64: return true;

    default: return false;
    }
}

bool is_type_sized_integer(const Type* type, TypeBuiltinKind* out) {
    type = type_unwrap(type);
    if (type == NULL || type->kind != TYPE_BUILTIN) {
        return false;
    }
    if (!is_type_sized_integer_kind(type->as.builtin.kind)) {
        return false;
    }
    if (out != NULL) {
        *out = type->as.builtin.kind;
    }
    return true;
}

bool is_type_integer(const Type* type) {
    return is_type_unsized_integer(type) || is_type_sized_integer(type, NULL);
}

bool is_type_compatible(const Type* to, const Type* from) {
    if (to == NULL || from == NULL) {
        return true;
    }

    to   = type_unwrap(to);
    from = type_unwrap(from);

    if (to == from) {
        return true;
    }

    if (is_type_any(to) || is_type_any(from)) {
        return true;
    }

    // unsized int can flow intor any sized integer (and vice versa)
    if (is_type_unsized_integer(from) && is_type_sized_integer(to, NULL)) {
        return true;
    }

    // allow array -> slice of same element
    if (to->kind == TYPE_SLICE && from->kind == TYPE_ARRAY) {
        return type_eq_type(to->as.slice.elem, from->as.array.elem);
    }

    // exact match otherwise
    return type_eq_type(to, from);
}

bool is_type_array_compatible(const Type* lhs, const Type* rhs) {
    if (lhs == NULL || rhs == NULL) {
        return false;
    }

    lhs = type_unwrap(lhs);
    rhs = type_unwrap(rhs);

    if (lhs->kind != TYPE_ARRAY || rhs->kind != TYPE_ARRAY) {
        return false;
    }

    return is_type_compatible(lhs->as.array.elem, rhs->as.array.elem) && (lhs->as.array.size == rhs->as.array.size);
}

bool is_type_slice_compatible(const Type* lhs, const Type* rhs) {
    if (lhs == NULL || rhs == NULL) {
        return false;
    }

    lhs = type_unwrap(lhs);
    rhs = type_unwrap(rhs);

    if (lhs->kind != TYPE_SLICE || rhs->kind != TYPE_SLICE) {
        return false;
    }

    return is_type_compatible(lhs->as.slice.elem, rhs->as.slice.elem);
}

bool is_type_array_to_slice_ok(const Type* lhs, const Type* rhs) {
    if (lhs == NULL || rhs == NULL) {
        return false;
    }

    lhs = type_unwrap(lhs);
    rhs = type_unwrap(rhs);

    if (lhs->kind != TYPE_SLICE || rhs->kind != TYPE_ARRAY) {
        return false;
    }

    return is_type_compatible(lhs->as.slice.elem, rhs->as.array.elem);
}

bool is_type_basic_assignable(const Type* lhs, const Type* rhs) {
    if (lhs == NULL || rhs == NULL) {
        return false;
    }

    lhs = type_unwrap(lhs);
    rhs = type_unwrap(rhs);

    if (lhs == rhs) {
        return true;
    }

    if (type_eq_type(lhs, rhs)) {
        return true;
    }

    if (is_type_any(lhs) || is_type_any(rhs)) {
        return true;
    }

    if (is_type_integer(lhs) && is_type_integer(rhs)) {
        return true;
    }
    if (is_type_bool(lhs) && is_type_bool(rhs)) {
        return true;
    }

    if (lhs->kind == TYPE_POINTER && rhs->kind == TYPE_POINTER) {
        return type_eq_type(lhs->as.pointer.base, rhs->as.pointer.base);
    }

    return false;
}

bool is_type_assignable(const Type* lhs, const Type* rhs) {
    if (lhs == NULL || rhs == NULL) {
        return true;
    }

    if (lhs == rhs) {
        return true;
    }
    if (type_eq_type(lhs, rhs)) {
        return true;
    }

    /* General/basic rules */
    if (is_type_basic_assignable(lhs, rhs)) {
        return true;
    }

    /* Containers */
    if (is_type_array_compatible(lhs, rhs)) {
        return true;
    }
    if (is_type_slice_compatible(lhs, rhs)) {
        return true;
    }

    /* Optional implicit array -> slice decay */
    if (is_type_array_to_slice_ok(lhs, rhs)) {
        return true;
    }

    return false;
}