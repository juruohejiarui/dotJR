#include <cstdio>
#include <fstream>
#include "tokenize.hpp"

int main(int argc, char **argv) {
	if (argc == 1) {
		std::ifstream ifs("./test1.hasm");
		std::string str, line;
		while (!ifs.eof()) std::getline(ifs, line), str.append(line), str.push_back('\n');
		std::vector<Hasm_Token> tokens;
		Hasm_tokenize(str, tokens);

		for (int i = 0; i < tokens.size(); i++) {
			printf("%02d: type:%d data:%d dataType:%d strData:%s\n", i, tokens[i].type, tokens[i].data, tokens[i].dataType, tokens[i].strData.c_str());
		}
	}
	return 0;
}