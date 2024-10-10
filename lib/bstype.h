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

enum BsData_Type {
	BsData_Type_u8, BsData_Type_u16,	BsData_Type_u32, 		BsData_Type_u64,
	BsData_Type_i8, BsData_Type_i16,	BsData_Type_i32, 		BsData_Type_i64,
										BsData_Type_f32 = 10, 	BsData_Type_f64, 
																BsData_Type_obj = 15, 
																BsData_Type_ptr	= 19,
	BsData_Type_generic,				BsData_Type_void,
};

#define Lib_bsdataTypeNum 20
extern const char *Lib_bsdataTypeStr[];

static inline int Lib_BsData_isInt(u8 type) { return type < BsData_Type_f32; }
static inline int Lib_BsData_isFloat(u8 type) { return type >= BsData_Type_f32 && type <= BsData_Type_f64; }
static inline int Lib_BsData_isSign(u8 type) { return type <= BsData_Type_i64; }
static inline int Lib_BsData_isUnsigned(u8 type) { return Lib_BsData_isInt(type) && !Lib_BsData_isSign(type); }

static inline int Lib_getBsDataSize(int type) { return type >= BsData_Type_generic ? 0 : (1 << (type & 0x3)); }

const char *Lib_readData(const char *str, u64 *data, u8 *type);

static inline int isNumber(char ch) { return '0' <= ch && ch <= '9'; }
static inline int isLetter(char ch) { return ('a' <= ch && ch <= 'z') || ('A' <= ch && ch <= 'Z') || ch == '_' || ch == '@'; }
static inline int isSeparator(char ch) { return ch == '\t' || ch == ' ' || ch == '\n'; }

#define Res_Error			(1ul << 0)
#define Res_SeriousError	(1ul << 1)

#ifdef _MSC_VER
#define __inline__ inline
#elif __APPLE__
#define __inline__ __attribute__((always_inline)) inline
#elif __linux__
#define __inline__ __attribute__((always_inline)) inline
#endif

#ifdef _MSC_VER
#  define PACK(...) __pragma(pack(push, 1)) __VA_ARGS__ __pragma(pack(pop))
#elif defined(__GNUC__) || defined(__linux__) || defined(__APPLE__)
#  define PACK(...) __VA_ARGS__ __attribute__((packed))
#endif

/* Cross-compiler alignment */
#if defined(_MSC_VER)
	#define ALIGNED_(x) __declspec(align(x))
#elif defined(__GNUC__) || defined(__linux__) || defined(__APPLE__)
	#define ALIGNED_(x) __attribute__ ((aligned(x)))
#endif


#endif