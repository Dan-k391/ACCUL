#include "ExprLexer.h"
#include "ExprParser.h"
#include "CodegenVisitor.h"
#include <antlr4-runtime.h>
#include <iostream>

int main() {
    std::string input = "(1 + 2) * 3";
    antlr4::ANTLRInputStream stream(input);
    ExprLexer lexer(&stream);
    antlr4::CommonTokenStream tokens(&lexer);
    ExprParser parser(&tokens);

    auto *tree = parser.prog();
    std::cout << tree->toStringTree(&parser) << std::endl;

    CodegenVisitor visitor;
    visitor.visit(tree);
    visitor.dumpIR();

    return 0;
}
