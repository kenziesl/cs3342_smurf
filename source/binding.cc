#include "binding.h"

using namespace std;

Binding::Binding () {}

void Binding::set_variable(string& name, AstNode*& val) {
	map<string, AstNode*>::iterator it = bindings.find(name);
	if (it != bindings.end()) {
		it->second = val;
	} else {
		bindings.insert( pair<string, AstNode*>(name, val) );
	}
}

AstNode* Binding::get_variable_value(string& name) {
	map<string, AstNode*>::iterator it = bindings.find(name);
	if (it != bindings.end()) {
		return it->second;
	} else {
		cout << "Could not find variable" << endl;
		return new IntegerNode(-1);
	}
}

Binding& Binding::operator = (Binding& b) {
	this->bindings = b.bindings;
	return *this;
}

Binding& Binding::operator + (Binding& b) {
	this->bindings.insert(b.bindings.begin(), b.bindings.end());
	return *this;
}
