#pragma once
#include <string>

extern "C" {
	#include "file.h"
}

std::string File_FullNameStr(char *fullName, u32 fullNameLen);