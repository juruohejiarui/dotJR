#pragma once
#include <vector>
#include <string>
#include <memory>
#include <type_traits>

extern "C" {
	#include "bstype.h"
}

std::string Lib_getRealStr(const std::string &str);
std::string Lib_getCodeStr(const std::string &str);

template<typename base, typename T>
std::shared_ptr<
	typename std::enable_if<(!std::is_same<base, T>::value) && (std::is_base_of<base, T>::value), T>::type
>
dynCastPtr(std::shared_ptr<base> ptr) {
	return std::static_pointer_cast<T>(ptr);
}

template<typename T> bool inRange(const T &val, const T &rgL, const T &rgR) { return rgL <= val && val <= rgR; }

std::vector<std::string> split(const std::string &str, const std::string &sep);

std::string getIndent(int dep);