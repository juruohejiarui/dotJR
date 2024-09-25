#include <cstdio>
#include <fstream>
#include "tokenize.hpp"
#include "compile.hpp"

int main(int argc, char **argv) {
	if (argc == 1) {
		std::ifstream ifs("./test1.hasm");
		std::string str, line;
		while (!ifs.eof()) std::getline(ifs, line), str.append(line), str.push_back('\n');
		ifs.close();
		std::vector<Hasm_Token> tokens;
		Hasm_tokenize(str, tokens);
		CompilePackage *pkg = Hasm_compile(tokens);
		Hasm_writeCplPkg("test1.hobj", pkg);
		delete pkg;

		ifs = std::ifstream("./test2.hasm");
		str.clear();
		while (!ifs.eof()) std::getline(ifs, line), str.append(line), str.push_back('\n');
		ifs.close();
		Hasm_tokenize(str, tokens);
		pkg = Hasm_compile(tokens);
		Hasm_writeCplPkg("test2.hobj", pkg);
		delete pkg;

		int res = Hasm_link("test1.hexe", {Hasm_readCplPkg("test1.hobj"), Hasm_readCplPkg("test2.hobj")}, {});
	}
	return 0;
}