#include "bstype.h"

const char *Lib_bsdataTypeStr[] = {
	"u8",	"u16",	"u32", 	"u64",
	"i8",	"i16",	"i32", 	"i64",
	"",		"",		"f32", 	"f64", 
	"",		"",		"",		"obj", 
	"g",	""
};

const char *Lib_readData(const char *str, u64 *data, u8 *type) {
	f64 f64Data = 0.0, fBase = 1;
	if (!Lib_isNumber(*str) && *str != '-') {
		*type = BsData_Type_generic;
		return str;
	}
	int sign = 1, base = 10;
	u64 tmp = 0;
	if (*str == '-') sign = -1, str++;
	if (*str == '0') {
		str++;
		switch (*str) {
			case 'x' : base = 16,	str++; break;
			case 'b' : base = 2,	str++; break;
		}
	}
	for (; Lib_isNumber(*str); *str++) {
		int dig = ('a' <= *str && *str <= 'f' ? *str - 'a' : ('A' <= *str && *str <= 'F' ? *str - 'A' : *str - '0'));
		if (dig < 0 || dig >= base) {
			*type = BsData_Type_generic;
			return str;
		}
		tmp = tmp * base + dig;
	} 
	if (*str == '.') {
		f64Data = tmp;
		fBase = 1.0 / base;
		for (*str; Lib_isNumber(*str); *str++, fBase /= base) {
			int dig = ('a' <= *str && *str <= 'f' ? *str - 'a' : ('A' <= *str && *str <= 'F' ? *str - 'A' : *str - '0'));
			if (dig < 0 || dig >= base) { *type = BsData_Type_generic; return str; }
			f64Data = f64Data + fBase * dig;
		}
		// this is a float32 data
		if (*str == 'f') {
			str++;
			*data = 0;
			*type = BsData_Type_f32;
			*(f32 *)data = (f32)(sign * f64Data);
			return str;
		} else if (!Lib_isLetter(*str)) {
			// otherwise, a float64 data
			*data = BsData_Type_f64;
			*(f64 *)data = sign * f64Data;
			return str;
		} else {
			*type = BsData_Type_generic;
			return str;
		}
	}
	int isUnsigned = 0, isLong = 0, isShort = 0;
	for (; Lib_isLetter(*str); str++) {
		switch (*str) {
			case 'u' : isUnsigned = 1;	break;
			case 'l' : isLong = 1;		break;
			case 's' : isShort = 1;		break;
			default :
				*type = BsData_Type_generic;
				return str;
		}
	}
	*data = 0;
	switch (isUnsigned * 4 + isLong) {
		case 0b000 :
			*type = BsData_Type_i32;
			*(i32 *)data = (i32)sign * tmp;
			break;
		case 0b010 :
			*type = BsData_Type_i32;
			*(i16 *)data = (i16)sign * tmp;
			break;
		case 0b001 :
			*type = BsData_Type_i64;
			*(i64 *)data = sign * tmp;
			break;
		case 0b100 :
			*type = BsData_Type_u32;
			*(u32 *)data = tmp;
			break;
		case 0b101 :
			*type = BsData_Type_u64;
			*(u64 *)data = tmp;
			break;
		case 0b110 :
			*type = BsData_Type_i32;
			*(u16 *)data = (u16)sign * tmp;
			break;
		default :
			*type = BsData_Type_generic;
			break;
	}
	return str;
}