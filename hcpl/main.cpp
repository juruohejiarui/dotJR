#include "tokenize.hpp"
#include "cpltree.hpp"
#include <fstream>
#include <iostream>
int main(int argc, char **argv) {
	fflush(stdout);
	if (argc == 1) {
		std::ifstream ifs("./test1.hs");
		std::string str, line;
		while (!ifs.eof()) std::getline(ifs, line), str.append(line), str.push_back('\n');
		ifs.close();
		std::vector<Hcpl_Token> tokens;
		Hcpl_tokenize(str, tokens);
		CplNode *root;
		int res = Hcpl_makeCplTree(tokens, CplNodeType::SrcRoot, root);
		printf("res=%d\n", res);
		std::cout << root->toString();
		return 0;
	}
	return 0;
}