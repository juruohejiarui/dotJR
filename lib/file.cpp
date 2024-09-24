#include "file.hpp"

std::string File_FullNameStr(char *fullName, u32 fullNameLen) {
	std::string str;
	for (u32 i = 0; i < fullNameLen; i++)
		str.push_back(fullName[i]);
	return str;
}