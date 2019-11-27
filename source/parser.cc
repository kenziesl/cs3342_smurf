#include <peglib.h>
#include <iostream>
#include <cstdlib>
#include <vector>
#include <fstream>
#include <string>

#include "visitor.h"
#include "ast_node.h"
#include "interpreter.h"


using namespace peg;
using namespace std;

auto grammar = R"(
		program                         <-  (_ statement _)*
		statement                       <-  ( vDec 
						/   assignment 
						/   expr )+
		vDec                            <-  'let' _ assignment (',')?
		assignment                      <-  identifier _ '=' _ expr (',')?
		boolean_expression              <-  arithmetic_expression _ relop _ arithmetic_expression
		arithmetic_expression           <-  mult_term (_ add_op _ mult_term )*
		mult_term                       <-  primary (_ mul_op _ primary)* 
		expr                            <-  function_definition
						/   ifexpression
						/   boolean_expression 
						/   arithmetic_expression
		function_definition             <-  'fn' _ '(' _ (identifier ( ',' _ identifier)*)? ')'  _ block
		functioncall                    <-  variablereference _ '(' (_ expr _ (',' _ expr)*)? _ ')'
		block                           <-  '{' (_ statement _)* '}'
		
		ifexpression                    <-  'if' _ expr _ block _ ('else' _ block)? _
		variablereference               <-  identifier
		primary                         <-  functioncall
						/   variablereference
						/   number
						/   '(' _ arithmetic_expression _ ')'
		~_                              <-  [ \t\r\n]*
		identifier                      <-  < [a-z][a-zA-Z0-9'_']* >
		number                         	<-  < ('-')? [0-9]+ > 
		add_op                          <-  < '+' / '-' > 
		mul_op                          <-  < '*' / '/' > 
		relop                           <-  < '==' / '!=' / '>=' / '>' / '<=' / '<' >
	)";

class Visitor;

////////////////////
//  ParseTreeNode

class ParseTreeNode {
	AstNode *content;
public:
	ParseTreeNode(){};
	ParseTreeNode(AstNode *content_node) { content = content_node; }
	AstNode *get() const { return content; }
	string to_string() { return content->to_string(); }
};

AstNode *bin_op(const SemanticValues &sv) {
	AstNode *left = sv[0].get<ParseTreeNode>().get();
	for (auto i = 1u; i < sv.size(); i += 2) {
		AstNode *right = sv[i + 1].get<ParseTreeNode>().get();
		string op = sv[i].get<ParseTreeNode>().get()->to_string();
		left = new BinopNode(left, op, right);
	}
	return left;
};

AstNode *assign(const SemanticValues &sv) {
	AstNode *left = sv[0].get<ParseTreeNode>().get();
	for (auto i = 1u; i < sv.size(); i += 2) {
		AstNode *right = sv[i].get<ParseTreeNode>().get();
		left = new Assigner(left, "=", right);
	}
	return left;
}

AstNode *ifElse(const SemanticValues &sv) {
	AstNode *left = sv[0].get<ParseTreeNode>().get();
	AstNode *right = sv[1u].get<ParseTreeNode>().get();
	AstNode *el = nullptr;
	if (sv.size() > 2u)
		el = sv[2u].get<ParseTreeNode>().get();
	left = new IfElseNode(left, right, el);
	return left;
}

AstNode *bind(const SemanticValues &sv) {
	vector<AstNode*> vec;
	int j = 0;
	for (auto i = 0u; i < sv.size(); i+=1) {
		vec.push_back(sv[i].get<ParseTreeNode>().get());
		j++;
	}
	return new Block(vec, j);
}

AstNode *funDef(const SemanticValues &sv) {
	vector<AstNode*> vars;
	AstNode* block;
	for (auto i = 0u; i < sv.size()-1u; i+=1) {
		vars.push_back(sv[i].get<ParseTreeNode>().get());
	}
	block = sv[sv.size()-1u].get<ParseTreeNode>().get();
	return new FunDef(block, vars);

}

AstNode *funCall(const SemanticValues &sv) {
	AstNode* ref = sv[0].get<ParseTreeNode>().get();
	vector<AstNode*> vars;
	for (auto i = 1u; i < sv.size(); i+=1)
		vars.push_back(sv[i].get<ParseTreeNode>().get());

	if (ref->to_string() == "print")
		return new PrintNode(vars);
	else
		return new FunCall(vars, ref);
	
}

void setup_ast_generation(parser &parser) {

	parser["vDec"] = [](const SemanticValues &sv) {
		AstNode *n = assign(sv);
		return ParseTreeNode(n);
	};

	parser["assignment"] = [](const SemanticValues &sv) {
		AstNode *n = assign(sv);
		return ParseTreeNode(n);
	};

	parser["boolean_expression"] = [](const SemanticValues &sv) {
		AstNode *n = bin_op(sv);
		return ParseTreeNode(n);
	};

	parser["arithmetic_expression"] = [](const SemanticValues &sv) {
		AstNode *n = bin_op(sv);
		return ParseTreeNode(n);
	};

	parser["mult_term"] = [](const SemanticValues &sv) {
		AstNode *n = bin_op(sv);
		return ParseTreeNode(n);
	};

	parser["ifexpression"] = [](const SemanticValues &sv) {
		AstNode *n = ifElse(sv);
		return ParseTreeNode(n);
	};

	parser["identifier"] = [](const SemanticValues &sv) {
		return ParseTreeNode(new VariableValue(sv.str()));
	};

	parser["variablereference"] = [](const SemanticValues &sv) {
		return ParseTreeNode(new VariableValue(sv.str()));
	};

	parser["number"] = [](const SemanticValues &sv) {
	        return ParseTreeNode(new IntegerNode(atoi(sv.c_str())));
	};

	parser["add_op"] = [](const SemanticValues &sv) {
	        return ParseTreeNode(new OpNode(sv.str()));
	};

	parser["mul_op"] = [](const SemanticValues &sv) {
	        return ParseTreeNode(new OpNode(sv.str()));
	};

	parser["relop"] = [](const SemanticValues &sv) {
		return ParseTreeNode(new OpNode(sv.str()));
	};


	parser["block"] = [](const SemanticValues &sv) {
		AstNode *n = bind(sv);
		return ParseTreeNode(n);
	};

	parser["program"] = [](const SemanticValues &sv) {
		AstNode *n = bind(sv);
		ParseTreeNode val = ParseTreeNode(n);
		Interpreter* I = new Interpreter();
		val.get()->accept(I);
		return val;
	};

	parser["function_definition"] = [](const SemanticValues &sv) {
		AstNode *n = funDef(sv);
		return ParseTreeNode(n);
	
	};

	parser["functioncall"] = [](const SemanticValues &sv) {
		AstNode *n = funCall(sv);
		ParseTreeNode val = ParseTreeNode(n);
		return val;
		
	};

}

int main(int argc, const char **argv) {
	if (argc < 2 || string("--help") == argv[1]) {
		cout << "usage: parser [formula]" << endl;
		return 1;
	}

	parser parser(grammar);
	setup_ast_generation(parser);

	ParseTreeNode val = ParseTreeNode();
	
	ifstream ifs;
	//string content( (istreambuf_iterator<char>(ifs)),
	//		(istreambuf_iterator<char>() ));
	
	ifs.open(argv[1]);
	string toSend = "";
	string toParse = "";
	if (ifs.is_open()) {
		while (getline(ifs, toParse)) {
			int i = 0;
			while (toParse[i] != '#' && i < toParse.size()) {
				toSend += toParse[i];
				i++;
			}
			toSend += ' ';
		}
	}
	const char* expr = toSend.c_str();
	

	//const char* expr = content.c_str();
	if (!parser.parse(expr, val)) {
		cout << "syntax error..." << endl;
	}

}
