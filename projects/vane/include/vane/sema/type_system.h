#pragma once

#include "vane/utils/defines.h"
#include "vane/utils/hashset.h"
#include "vane/utils/hashmap.h"

#include "vane/sema/type.h"
#include "vane/sema/target.h"

typedef struct TypeSystem TypeSystem;

#define TYPE_SYSTEM_DEFAULT_POUNTER_TYPE_COUNT    4
#define TYPE_SYSTEM_DEFAULT_ARRAY_TYPE_COUNT      4
#define TYPE_SYSTEM_DEFAULT_SLICE_TYPE_COUNT      4
#define TYPE_SYSTEM_DEFAULT_ALIAS_TYPE_COUNT      4
#define TYPE_SYSTEM_DEFAULT_FUNCTION_TYPE_COUNT   4
#define TYPE_SYSTEM_DEFAULT_UNRESOLVED_TYPE_COUNT 4

struct TypeSystem {
    TargetInfo target;

    Type builtin_types[TYPE_BUILTIN_COUNT];

    Hashset pointer_types;
    Hashset array_types;
    Hashset slice_types;
    Hashset alias_types;
    Hashset function_types;

    Hashmap builtin_lookup;
    Hashmap unresolved_types;
};

void type_system_init(TypeSystem* ts, TargetInfo target);

void type_system_destroy(TypeSystem* ts);

Type* type_system_get_builtin(TypeSystem* ts, StringView name);

Type* type_system_get_pointer_or_create(TypeSystem* ts, const Type* base);

Type* type_system_get_array_or_create(TypeSystem* ts, const Type* elem, u32 size);

Type* type_system_get_slice_or_create(TypeSystem* ts, const Type* elem);

Type* type_system_get_alias_or_create(TypeSystem* ts, StringView name, const Type* target);

Type* type_system_get_fun_or_create(TypeSystem* ts, Type** params, u32 param_count, const Type* ret);

Type* type_system_get_unresolved_or_create(TypeSystem* ts, StringView name);