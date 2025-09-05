#pragma once

typedef signed char        i8;
typedef short              i16;
typedef int                i32;
typedef long long          i64;

typedef unsigned char      u8;
typedef unsigned short     u16;
typedef unsigned int       u32;
typedef unsigned long long u64;

typedef float              f32;
typedef double             f64;

typedef u8                 byte;

#define U8_MAX  0xFFu
#define U16_MAX 0xFFFFu
#define U32_MAX 0xFFFFFFFFu
#define U64_MAX 0xFFFFFFFFFFFFFFFFull


#define I8_MAX  0x7F
#define I16_MAX 0x7FFF
#define I32_MAX 0x7FFFFFFF
#define I64_MAX 0x7FFFFFFFFFFFFFFFll

#ifndef NULL
#define NULL (void*)0
#endif

#ifndef NPOS
#define NPOS (-1)
#endif

#ifndef bool
#define bool u8
#endif

#ifndef true
#define true 1
#endif

#ifndef false
#define false 0
#endif

#ifndef assert
#include <assert.h>
#endif

#ifndef unreachable
#define unreachable() assert(false && "reached unreachable")
#endif

#ifndef ARR_SIZE
#define ARR_SIZE(arr) (sizeof(arr) / sizeof((arr)[0]))
#endif

#ifndef SET_FLAG
#define SET_FLAG(FLAGS, FLAG) ((void)((FLAGS) |= (FLAG)))
#endif

#ifndef CLEAR_FLAG
#define CLEAR_FLAG(FLAGS, FLAG) ((void)((FLAGS) &= ~(FLAG)))
#endif

#ifndef TOGGLE_FLAG
#define TOGGLE_FLAG(FLAGS, FLAG) ((void)((FLAGS) ^= (FLAG)))
#endif

#ifndef IS_FLAG_SET
#define IS_FLAG_SET(FLAGS, FLAG) (((FLAGS) & (FLAG)) != 0)
#endif
