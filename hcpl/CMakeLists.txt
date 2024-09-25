cmake_minimum_required(VERSION 3.1)

project (hasm)

set(C_SOURCE 
	../lib/bstype.c
	../hinst/api.c)
set(C_HEADER 
	../lib/bstype.h ../lib/file.h 
	../hinst/desc.h ../hinst/api.h)
set(CXX_SOURCE 
	../lib/bstype.cpp ../lib/file.cpp
	main.cpp tokenize.cpp)
set(CXX_HEADER 
	../lib/bstype.hpp ../lib/file.hpp
	../hinst/api.hpp 
	tokenize.hpp)

add_executable(hcpl ${C_SOURCE} ${C_HEADER} ${CXX_SOURCE} ${CXX_HEADER})

set(CMAKE_BUILD_TYPE "Debug")
set(CMAKE_CXX_FLAGS_DEBUG "$ENV{CXXFLAGS} -O0 -g2 -ggdb -std=c++14")
set(CMAKE_CXX_FLAGS_RELEASE "$ENV{CXXFLAGS} -DNDEBUG -O0 -std=c++14")