#include <cstdio>
#include <cstring>
#include <fstream>
#include "tokenize.hpp"
#include "compile.hpp"

int main(int argc, char **argv) {
	if (argc == 1) {
		std::ifstream ifs("./test1.jras");
		std::string str, line;
		while (!ifs.eof()) std::getline(ifs, line), str.append(line), str.push_back('\n');
		ifs.close();
		std::vector<Hasm_Token> tokens;
		Hasm_tokenize(str, tokens);
		CompilePackage *pkg = Hasm_compile(tokens);
		Hasm_writeCplPkg("test1.jrobj", pkg);
		delete pkg;

		ifs = std::ifstream("./test2.jras");
		str.clear();
		while (!ifs.eof()) std::getline(ifs, line), str.append(line), str.push_back('\n');
		ifs.close();
		Hasm_tokenize(str, tokens);
		pkg = Hasm_compile(tokens);
		Hasm_writeCplPkg("test2.jrobj", pkg);
		delete pkg;

		int res = Hasm_link("test1.jrexe", {"test1.jrobj", "test2.jrobj"}, {});
		return 0;
	}
	
	if (argc < 4) {
		printf("Usage: hasm -o [asmPath] [objPath] or hasm -l [execPath] (-r [relyPaths])* ([objPaths])*");
		return -1;
	}
	if (!strcmp(argv[1], "-o")) {
		if (argc != 4) {
			printf("Usage: hasm -o [asmPath] [objPath]");
			return -1;
		}
		std::string asmPath = std::string(argv[2]), objPath = std::string(argv[3]);
		std::ifstream ifs(asmPath);
		std::string str, line;
		while (!ifs.eof()) std::getline(ifs, line), str.append(line), str.push_back('\n');
		ifs.close();
		std::vector<Hasm_Token> tokens;
		int res = Hasm_tokenize(str, tokens);
		if (res) return -1;
		CompilePackage *pkg = Hasm_compile(tokens);
		if (pkg == nullptr) return -1;
		Hasm_writeCplPkg(objPath, pkg);
	} else if (!strcmp(argv[1], "-l")) {
		std::vector<std::string> objPath, relyPath;
		std::string execPath = std::string(argv[2]);
		for (int i = 3; i < argc; i++) {
			if (!strcmp(argv[i], "-r")) {
				if (i + 1 == argc) {
					printf("Invalid parameter\n");
					return -1;
				}
				relyPath.push_back(std::string(argv[i + 1]));
			} else objPath.push_back(std::string(argv[i]));
		}
		int res = Hasm_link(execPath, objPath, relyPath);
		if (res) return -1;
	}
	return 0;
}