#include <iostream>
#include <peglib.h>
#include <assert.h>

using namespace peg;
using namespace std;

int main(void) {

    auto grammar = R"(
        program                         <-   code 
        code                            <-  (statement (_)? )*
        statement                       <-  ( vDec 
                                        /   assignment 
                                        /   expr 
                                        /   comment)+


        vDec                            <-  'let' _ variable_declaration
        variable_declaration            <-  decl (_)? (',' (_)? decl)*
        decl                            <-  identifier (_)? ('=' (_)? expr)?
        assignment                      <-  identifier (_)? '=' (_)? expr
        boolean_expression              <-  arithmetic_expression (_)? relop (_)? arithmetic_expression
        arithmetic_expression           <-  mult_term (_)? addop (_)? arithmetic_expression 
                                        /   mult_term
        mult_term                       <-  primary (_)? mulop (_)? mult_term 
                                        /   primary
        expr                            <-  function_definition
                                        /   ifexpression
                                        /   boolean_expression 
                                        /   arithmetic_expression
        function_definition             <-  'fn' (_)? param_list (_)? block
        param_list                      <-  '(' (_)? identifier ( ',' (_)? identifier)* ')' 
                                        /   '(' (_)? ')'
        functioncall                    <-  'print' (_)? '(' (_)? call_arguments (_)? ')'
                                        /   variablereference (_)? '(' (_)? call_arguments (_)? ')'
        call_arguments                  <-  ((_)? expr (_)? (',' (_)? expr)*)?
        block                           <-  '{' ((_)? statement (_)?)* '}'
        ifexpression                    <-  'if' (_)? expr (_)? block (_)? elseexpression (_)?
        elseexpression                  <-  ('else' (_)? block)?
        variablereference               <-  identifier
        primary                         <-  integer
                                        /   functioncall
                                        /   variablereference
                                        /   '(' (_)? arithmetic_expression (_)? ')'
        comment                         <-  '#' [''""``-+0-9|a-zA-Z=>< ]* '\n'?
        ~_                              <-  [ \t\r\n]*
        ~__                             <-  ![a-zA-Z0-9] _
        identifier                      <-  < [a-z][a-zA-Z0-9'_']* >
        integer                         <-  < ('-')? [0-9]+ > 
        addop                           <-  < '+' / '-' > 
        mulop                           <-  < '*' / '/' > 
        relop                           <-  < '==' / '!=' / '>=' / '>' / '<=' / '<' >
    )";

    parser parser;
    parser.log = [](size_t line, size_t col, const string& msg) {
        cerr << line << ":" << col << ": " << msg << "\n";
    };

    auto ok = parser.load_grammar(grammar);
    assert(ok);

    parser["integer"] = [](const SemanticValue& sv) {
        return stoi(sv.token(), nullptr, 10);
    };



    parser.enable_packrat_parsing();
    parser.parse("5", val);

    assert(val == 5);

}