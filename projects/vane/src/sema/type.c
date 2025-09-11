#include "vane/sema/type.h"

#include <stdlib.h>
#include <stdio.h>

#include "vane/utils/hash.h"
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

    if (type1 == type2) {
        return true;
    }

    if (type1->kind != type2->kind) {
        return false;
    }

    switch (type1->kind) {
    case TYPE_UNRESOLVED:
        return string_view_eq_sv(type1->as.unresolved.name, type2->as.unresolved.name);

    case TYPE_BUILTIN:
        return type1->as.builtin.kind == type2->as.builtin.kind;

    case TYPE_POINTER:
        return type_eq_type(type1->as.pointer.base, type2->as.pointer.base);

    case TYPE_ALIAS:
        return type_eq_type(type1->as.alias.target, type2->as.alias.target);

    case TYPE_ARRAY:
        return (type1->as.array.size == type2->as.array.size) &&
                type_eq_type(type1->as.array.elem, type2->as.array.elem);

    case TYPE_SLICE:
        return type_eq_type(type1->as.slice.elem, type2->as.slice.elem);

    case TYPE_FUNCTION:
        if (!type_eq_type(type1->as.fun.ret, type2->as.fun.ret)) {
            return false;
        }

        if (type1->as.fun.param_count != type2->as.fun.param_count) {
            return false;
        }

        for (u32 i = 0; i < type1->as.fun.param_count; ++i) {
            if (!type_eq_type(type1->as.fun.params[i], type2->as.fun.params[i])) {
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