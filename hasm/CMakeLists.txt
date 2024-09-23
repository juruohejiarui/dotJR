cmake_minimum_required(VERSION 3.1)

project (hasm)

set(C_SOURCE ./main.c)
set(C_HEADER ../lib/bstype.h ../hinst/desc.h)

add_executable(hasm ${C_SOURCE} ${C_HEADER})

set(CMAKE_BUILD_TYPE "Debug")
set(CMAKE_CXX_FLAGS_DEBUG "$ENV{CXXFLAGS} -O0 -g2 -ggdb -std=c++14")
set(CMAKE_CXX_FLAGS_RELEASE "$ENV{CXXFLAGS} -DNDEBUG -O0 -std=c++14")