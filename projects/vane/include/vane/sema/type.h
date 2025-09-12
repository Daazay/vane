#pragma once

#include "vane/utils/defines.h"
#include "vane/utils/string.h"
#include "vane/utils/string_view.h"
#include "vane/utils/vector.h"

struct TypeSystem;

typedef enum TypeResolveState TypeResolveState;
typedef enum TypeKind TypeKind;
typedef enum TypeBuiltinKind TypeBuiltinKind;

typedef struct Type Type;

#define TYPE_UNWRAP_GUARD   256

enum TypeResolveState {
    TYPE_STATE_UNRESOLVED = 0,
    TYPE_STATE_RESOLVING  = 1,
    TYPE_STATE_RESOLVED   = 2,
};

enum TypeKind {
    TYPE_UNKNOWN = 0,

    TYPE_UNRESOLVED,
    TYPE_BUILTIN,
    TYPE_POINTER,
    TYPE_ALIAS,
    TYPE_ARRAY,    // stack array with fixed size. e.g. [16]u8
    TYPE_SLICE,    // pointer + length
    TYPE_FUNCTION,

    TYPE_COUNT,
};

enum TypeBuiltinKind {
    TYPE_BUILTIN_UNKNOWN = 0,

    TYPE_BUILTIN_VOID,
    TYPE_BUILTIN_BOOL,
    TYPE_BUILTIN_UNSIZED_INT, // used for literals
    TYPE_BUILTIN_U8,
    TYPE_BUILTIN_I8,
    TYPE_BUILTIN_U16,
    TYPE_BUILTIN_I16,
    TYPE_BUILTIN_U32,
    TYPE_BUILTIN_I32,
    TYPE_BUILTIN_U64,
    TYPE_BUILTIN_I64,
    TYPE_BUILTIN_ANY,

    TYPE_BUILTIN_COUNT,
};

struct Type {
    TypeKind kind;

    u32 size;      // in bytes, target specific
    u32 alignment; // target-specific

    union {
        struct { TypeBuiltinKind kind; } builtin;
        struct { const Type* base; } pointer;
        struct {
            StringView name;
            const Type* target;
        } alias;
        struct {
            const Type* elem;
            u32 size;
        } array;
        struct { const Type* elem; } slice;
        struct {
            const Type** params;
            u32 param_count;
            const Type* ret;
        } fun;
        struct { StringView name; } unresolved;
    } as;
};

const char* type_kind_get_name(TypeKind kind);

const char* type_builtin_kind_get_name(TypeBuiltinKind kind);

bool is_type_builtin_kind_signed(TypeBuiltinKind kind);

bool is_type_builtin_kind_unsigned(TypeBuiltinKind kind);

u32 type_builtin_kind_get_size(TypeBuiltinKind kind);

Type* type_create(TypeKind kind, u32 size, u32 alignment);

void type_destroy(Type* type);

Type type_builtin_create(TypeBuiltinKind kind, u32 size, u32 alignment);

Type* type_pointer_create(const struct TypeSystem* ts, const Type* base);

Type* type_alias_create(const struct TypeSystem* ts, StringView name, const Type* target);

Type* type_array_create(const struct TypeSystem* ts, const Type* elem, u32 size);

Type* type_slice_create(const struct TypeSystem* ts, const Type* elem);

Type* type_fun_create(const struct TypeSystem* ts, const Type** params, u32 param_count, const Type* ret);

Type* type_unresolved_create(StringView name);

bool type_eq_type(const Type* type1, const Type* type2);

u32 type_get_hash(const Type* type);

//

String type_to_str(const Type* type);

const Type* type_unwrap(const Type* type);

bool is_type_builtin(const Type* type, TypeBuiltinKind kind);

bool is_type_any(const Type* type);

bool is_type_bool(const Type* type);

bool is_type_unsized_integer(const Type* type);

bool is_type_sized_integer(const Type* type, TypeBuiltinKind* out);

bool is_type_integer(const Type* type);

bool is_type_compatible(const Type* to, const Type* from);
