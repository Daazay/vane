#include "vane/sema/type_system.h"

#include <stdlib.h>

static inline void type_system_set_builtin(TypeSystem* ts, TypeBuiltinKind kind, u32 size, u32 align) {
    ts->builtin_types[kind] = type_builtin_create(kind, size, align);
}

static inline void type_system_init_builtins_for_target(TypeSystem* ts) {
#define SET_BUILTIN(ID, KIND, SIZE, ALIGN) do { \
    ts->builtin_types[KIND] = type_builtin_create(KIND, SIZE, ALIGN); \
    Type* __type = &ts->builtin_types[KIND]; \
    StringView __id = STR_LIT(ID); \
    hashmap_insert(&ts->builtin_lookup, &__id, &__type); \
} while(false)


    switch (ts->target.arch) {
    case TARGET_ARCH_X86_64: {
        SET_BUILTIN("void", TYPE_BUILTIN_VOID, 0, 1);
        SET_BUILTIN("bool", TYPE_BUILTIN_BOOL, 1, 1);
        SET_BUILTIN("unsized int", TYPE_BUILTIN_UNSIZED_INT, 0, 0);
        SET_BUILTIN("u8",   TYPE_BUILTIN_U8,   1, 1);
        SET_BUILTIN("i8",   TYPE_BUILTIN_I8,   1, 1);
        SET_BUILTIN("u16",  TYPE_BUILTIN_U16,  2, 2);
        SET_BUILTIN("i16",  TYPE_BUILTIN_I16,  2, 2);
        SET_BUILTIN("u32",  TYPE_BUILTIN_U32,  4, 4);
        SET_BUILTIN("i32",  TYPE_BUILTIN_I32,  4, 4);
        SET_BUILTIN("u64",  TYPE_BUILTIN_U64,  8, 8);
        SET_BUILTIN("i64",  TYPE_BUILTIN_I64,  8, 8);

        // type + ptr
        const u32 any_size = ts->target.pointer_size * 2;
        SET_BUILTIN("any", TYPE_BUILTIN_ANY, any_size, ts->target.pointer_size);
    } break;

    default:
        unreachable();
        break;
    }
}

void type_system_init(TypeSystem* ts, TargetInfo target) {
    assert(ts != NULL);

    *ts = (TypeSystem) { 0 };

    ts->target = target;

    ts->pointer_types   = (Hashset) { 0 };
    ts->array_types     = (Hashset) { 0 };
    ts->slice_types     = (Hashset) { 0 };
    ts->alias_types     = (Hashset) { 0 };
    ts->function_types  = (Hashset) { 0 };

    ts->unresolved_types = (Hashmap) { 0 };

    ts->builtin_lookup = hashmap_create(TYPE_BUILTIN_COUNT,
        HASHMAP_KEY_SPECS(StringView, &string_view_item_hash, &string_view_item_eq, NULL),
        HASHMAP_VALUE_SPECS(Type*, &type_destroy)
    );

    type_system_init_builtins_for_target(ts);
}

void type_system_destroy(TypeSystem* ts) {
    if (ts == NULL) {
        return;
    }

    hashset_destroy(&ts->pointer_types);
    hashset_destroy(&ts->array_types);
    hashset_destroy(&ts->slice_types);
    hashset_destroy(&ts->alias_types);
    hashset_destroy(&ts->function_types);

    hashmap_destroy(&ts->builtin_lookup);
    hashmap_destroy(&ts->unresolved_types);
}

Type* type_system_get_builtin(TypeSystem* ts, StringView name) {
    assert(ts != NULL);
    return hashmap_get(&ts->builtin_lookup, &name);
}

Type* type_system_get_pointer_or_create(TypeSystem* ts, const Type* base) {
    assert(ts != NULL && base != NULL);

    Type t = { .kind = TYPE_POINTER, .as.pointer.base = base, };
    Type* target_type = &t;

    Type* existing = NULL;
    if (ts->pointer_types.size > 0) {
        existing = (Type*)hashset_get(&ts->pointer_types, &target_type);
    }

    if (existing != NULL) {
        return existing;
    }

    Type* type = type_pointer_create(ts, base);

    if (ts->pointer_types.buckets == NULL) {
        ts->pointer_types = hashset_create(TYPE_SYSTEM_DEFAULT_POUNTER_TYPE_COUNT,
            HASHSET_ITEM_SPECS(Type*, &type_get_hash, &type_eq_type, &type_destroy)
        );
    }

    hashset_insert(&ts->pointer_types, &type);
    return type;
}

Type* type_system_get_array_or_create(TypeSystem* ts, const Type* elem, u32 size) {
    assert(ts != NULL && elem != NULL);

    Type t = { .kind = TYPE_ARRAY, .as.array.elem = elem, .as.array.size = size, };
    Type* target_type = &t;

    Type* existing = NULL;
    if (ts->array_types.size > 0) {
        existing = (Type*)hashset_get(&ts->array_types, &target_type);
    }

    if (existing != NULL) {
        return existing;
    }

    Type* type = type_array_create(ts, elem, size);

    if (ts->array_types.buckets == NULL) {
        ts->array_types = hashset_create(TYPE_SYSTEM_DEFAULT_ARRAY_TYPE_COUNT,
            HASHSET_ITEM_SPECS(Type*, &type_get_hash, &type_eq_type, &type_destroy)
        );
    }

    hashset_insert(&ts->array_types, &type);
    return type;
}

Type* type_system_get_slice_or_create(TypeSystem* ts, const Type* elem) {
    assert(ts != NULL && elem != NULL);

    Type t = { .kind = TYPE_SLICE, .as.slice.elem = elem, };
    Type* target_type = &t;

    Type* existing = NULL;
    if (ts->slice_types.size > 0) {
        existing = (Type*)hashset_get(&ts->slice_types, &target_type);
    }

    if (existing != NULL) {
        return existing;
    }

    Type* type = type_slice_create(ts, elem);

    if (ts->slice_types.buckets == NULL) {
        ts->slice_types = hashset_create(TYPE_SYSTEM_DEFAULT_SLICE_TYPE_COUNT,
            HASHSET_ITEM_SPECS(Type*, &type_get_hash, &type_eq_type, &type_destroy)
        );
    }

    hashset_insert(&ts->slice_types, &type);
    return type;
}

Type* type_system_get_alias_or_create(TypeSystem* ts, StringView name, const Type* target) {
    assert(ts != NULL && target != NULL);

    Type t = { .kind = TYPE_ALIAS, .as.alias.target = target, };
    Type* target_type = &t;

    Type* existing = NULL;
    if (ts->alias_types.size > 0) {
        existing = (Type*)hashset_get(&ts->alias_types, &target_type);
    }

    if (existing != NULL) {
        return existing;
    }

    Type* type = type_alias_create(ts, name, target);

    if (ts->alias_types.buckets == NULL) {
        ts->alias_types = hashset_create(TYPE_SYSTEM_DEFAULT_ALIAS_TYPE_COUNT,
            HASHSET_ITEM_SPECS(Type*, &type_get_hash, &type_eq_type, &type_destroy)
        );
    }

    hashset_insert(&ts->alias_types, &type);
    return type;
}

Type* type_system_get_fun_or_create(TypeSystem* ts, Type** params, u32 param_count, const Type* ret) {
    assert(ts != NULL && ((params != NULL && param_count > 0) || params == NULL) && ret != NULL);

    Type t = { .kind = TYPE_FUNCTION, .as.fun.params = (const Type**)params, .as.fun.param_count = param_count, .as.fun.ret = ret, };
    Type* target_type = &t;

    Type* existing = NULL;
    if (ts->function_types.size > 0) {
        existing = (Type*)hashset_get(&ts->function_types, &target_type);
    }

    if (existing != NULL) {
        // we pass here heap allocated params,
        // we dont know whether type which will be returned as newbord or existed
        // so need to free heap here
        free(params);
        return existing;
    }

    Type* type = type_fun_create(ts, (const Type**)params, param_count, ret);

    if (ts->function_types.buckets == NULL) {
        ts->function_types = hashset_create(TYPE_SYSTEM_DEFAULT_FUNCTION_TYPE_COUNT,
            HASHSET_ITEM_SPECS(Type*, &type_get_hash, &type_eq_type, &type_destroy)
        );
    }

    hashset_insert(&ts->function_types, &type);
    return type;
}

Type* type_system_get_unresolved_or_create(TypeSystem* ts, StringView name) {
    assert(ts != NULL);

    Type* existing = NULL;
    if (ts->unresolved_types.size > 0) {
        existing = (Type*)hashmap_get(&ts->unresolved_types, &name);
    }

    if (existing != NULL) {
        return existing;
    }

    Type* type = type_unresolved_create(name);

    if (ts->unresolved_types.buckets == NULL) {
        ts->unresolved_types = hashmap_create(TYPE_SYSTEM_DEFAULT_UNRESOLVED_TYPE_COUNT,
            HASHMAP_KEY_SPECS(StringView, &string_view_item_hash, &string_view_item_eq, NULL),
            HASHMAP_VALUE_SPECS(Type*, &type_destroy)
        );
    }

    hashmap_insert(&ts->unresolved_types, &name , &type);
    return type;
}