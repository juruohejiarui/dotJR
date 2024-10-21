#include "tokenize.hpp"
#include "cpltree.hpp"
#include "idensys/desc.hpp"
#include <fstream>
#include <iostream>

using namespace IdenSystem;

int main(int argc, char **argv) {
	fflush(stdout);
	// in test mode
	if (argc == 1) {
		freopen("test.output", "w", stdout);
		std::ifstream ifs("./test1.jrs");
		std::string str, line;
		while (!ifs.eof()) std::getline(ifs, line), str.append(line), str.push_back('\n');
		ifs.close();
		std::vector<Hcpl_Token> tokens;
		Hcpl_tokenize(str, tokens);
		CplNode *root;
		int res = Hcpl_makeCplTree(tokens, CplNodeType::SrcRoot, root);
		printf("res=%d\n", res);
		std::cout << root->toString();
		Namespace *glo = IdenSystem::buildGloNsp();
		IdenEnvironment *idenEnv = IdenSystem::build(glo, {root});
		std::cout << (idenEnv != nullptr ? "success\n" : "fail\n") << glo->toString(0);
		return 0;
	}
	return 0;
}