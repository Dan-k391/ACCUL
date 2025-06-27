#include "ExprParser.h"
#include "ExprLexer.h"
#include "antlr4-runtime.h"

#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>
#include <llvm/Support/raw_ostream.h>

using namespace antlr4;

int main() {
    std::string input = "1 + 2";
    ANTLRInputStream stream(input);
    ExprLexer lexer(&stream);
    CommonTokenStream tokens(&lexer);
    ExprParser parser(&tokens);
    tree::ParseTree* tree = parser.expr();

    // Setup LLVM
    llvm::LLVMContext context;
    llvm::IRBuilder<> builder(context);
    auto module = std::make_unique<llvm::Module>("expr_module", context);

    // Define: int main()
    llvm::FunctionType *funcType = llvm::FunctionType::get(builder.getInt32Ty(), false);
    llvm::Function *mainFunc = llvm::Function::Create(
        funcType,
        llvm::Function::ExternalLinkage,
        "main",
        module.get()
    );

    // Create entry block
    llvm::BasicBlock *entry = llvm::BasicBlock::Create(context, "entry", mainFunc);
    builder.SetInsertPoint(entry);

    // Create constants and add them
    llvm::Value* lhs = llvm::ConstantInt::get(context, llvm::APInt(32, 1));
    llvm::Value* rhs = llvm::ConstantInt::get(context, llvm::APInt(32, 2));
    llvm::Value* result = builder.CreateAdd(lhs, rhs, "addtmp");

    // Return the result
    builder.CreateRet(result);

    // Output the LLVM IR
    module->print(llvm::outs(), nullptr);

    return 0;
}
