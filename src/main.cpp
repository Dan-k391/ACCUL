#include "CLexer.h"
#include "CParser.h"
#include "CodegenVisitor.h"
#include <antlr4-runtime.h>
#include <iostream>

int main() {
    std::string input = "a = 5; a;";
    antlr4::ANTLRInputStream stream(input);
    CLexer lexer(&stream);
    antlr4::CommonTokenStream tokens(&lexer);
    CParser parser(&tokens);

    auto *tree = parser.prog();
    std::cout << tree->toStringTree(&parser) << std::endl;

    CodegenVisitor visitor;
    visitor.visit(tree);
    visitor.dumpIR();
    visitor.emitAssembly("../build/output.s");

    return 0;
}
