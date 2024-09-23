#pragma once
#include <string>

extern "C" {
	#include "bstype.h"
}

std::string Lib_getRealStr(const std::string &str);
std::string Lib_getCodeStr(const std::string &str);