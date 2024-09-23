#ifndef __LIB_BSTYPE_H__
#define __LIB_BSTYPE_H__

typedef unsigned long long	u64;
typedef unsigned int 		u32;
typedef unsigned short 		u16;
typedef unsigned char 		u8;
typedef signed long long	i64;
typedef signed int			i32;
typedef signed short		i16;
typedef signed char			i8;

typedef float		f32;
typedef double		f64;
typedef long double f128;

#define container(ptr, type, mem) ((type)(((u64)ptr) - &((type *)0)->mem))

char *Lib_readData(const char *str, u64 *data, u8 *type);
#endif