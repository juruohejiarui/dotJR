#include "bstype.h"

const char *Lib_bsdataTypeStr[] = {
	"u8",	"u16",	"u32", 	"u64",
	"i8",	"i16",	"i32", 	"i64",
	"",		"",		"f32", 	"f64", 
	"",		"",		"",		"obj", 
	"",		"",		"",		"ptr", 
	"g",	""
};

const char *Lib_readData(const char *str, u64 *data, u8 *type) {
	f64 f64Data = 0.0, fBase = 1;
	if (!isNumber(*str) && *str != '-') {
		*type = BsData_Type_void;
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
	for (; isNumber(*str); *str++) {
		int dig = ('a' <= *str && *str <= 'f' ? *str - 'a' : ('A' <= *str && *str <= 'F' ? *str - 'A' : *str - '0'));
		if (dig < 0 || dig >= base) {
			*type = BsData_Type_void;
			return str;
		}
		tmp = tmp * base + dig;
	} 
	if (*str == '.') {
		f64Data = tmp;
		fBase = 1.0 / base;
		for (*str; isNumber(*str); *str++, fBase /= base) {
			int dig = ('a' <= *str && *str <= 'f' ? *str - 'a' : ('A' <= *str && *str <= 'F' ? *str - 'A' : *str - '0'));
			if (dig < 0 || dig >= base) { *type = BsData_Type_void; return str; }
			f64Data = f64Data + fBase * dig;
		}
		// this is a float32 data
		if (*str == 'f') {
			str++;
			*data = 0;
			*type = BsData_Type_f32;
			*(f32 *)data = (f32)(sign * f64Data);
			return str;
		} else if (!isLetter(*str)) {
			// otherwise, a float64 data
			*data = BsData_Type_f64;
			*(f64 *)data = sign * f64Data;
			return str;
		} else {
			*type = BsData_Type_void;
			return str;
		}
	}
	int isUnsigned = 0, isLong = 0, isShort = 0, isByte = 0;
	for (; isLetter(*str); str++) {
		switch (*str) {
			case 'u' : isUnsigned = 1;	break;
			case 'l' : isLong = 1;		break;
			case 's' : isShort = 1;		break;
			case 'S' : isByte = 1;		break;
			default :
				*type = BsData_Type_void;
				return str;
		}
	}
	*data = 0;
	switch (isUnsigned * 8 + + isByte * 4 + isShort * 2 + isLong) {
		case 0b0001 : *type = BsData_Type_i64; *(i64 *)data = (i64)(sign * tmp); break;
		case 0b0000 : *type = BsData_Type_i32; *(i32 *)data = (i32)(sign * tmp); break;
		case 0b0010 : *type = BsData_Type_i16; *(i16 *)data = (i16)(sign * tmp); break;
		case 0b0100 : *type = BsData_Type_i8;  *(u8 *)data = (i8)(sign * tmp); break;

		case 0b1001 : *type = BsData_Type_u64; *(u64 *)data = tmp; break;
		case 0b1000 : *type = BsData_Type_u32; *(u32 *)data = tmp; break;
		case 0b1010 : *type = BsData_Type_u16; *(u16 *)data = tmp; break;
		case 0b1100 : *type = BsData_Type_u8;  *(u8  *)data = tmp; break;
		default :
			*type = BsData_Type_void;
			break;
	}
	return str;
}