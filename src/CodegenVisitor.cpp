#include "CodegenVisitor.h"
#include "ExprParser.h"
#include <llvm/IR/Verifier.h>
#include <llvm/Support/raw_ostream.h>
#include <any>

using namespace llvm;

CodegenVisitor::CodegenVisitor() : builder(context) {
    module = std::make_unique<Module>("expr_module", context);

    // int main()
    FunctionType *funcType = FunctionType::get(builder.getInt32Ty(), false);
    Function *mainFunc = Function::Create(funcType, Function::ExternalLinkage, "main", module.get());
    BasicBlock *entry = BasicBlock::Create(context, "entry", mainFunc);
    builder.SetInsertPoint(entry);
    currentFunction = mainFunc;
    lastValue = nullptr;
}

// NEW: start rule handler: prog : expr EOF ;
antlrcpp::Any CodegenVisitor::visitProg(ExprParser::ProgContext *ctx) {
    // Visit the single expr child; store result for return
    lastValue = std::any_cast<Value*>(visit(ctx->expr()));
    return lastValue;
}

antlrcpp::Any CodegenVisitor::visitExpr(ExprParser::ExprContext *ctx) {
    // Case 1: parentheses — '(' expr ')'
    if (ctx->children.size() == 3 &&
        ctx->children.front()->getText() == "(" &&
        ctx->children.back()->getText() == ")") {
        return visit(ctx->expr(0)); // just evaluate inner expression
    }

    // Case 2: integer literal
    if (ctx->INT()) {
        int val = std::stoi(ctx->INT()->getText());
        return (llvm::Value*)builder.getInt32(val);
    }

    // Case 3: binary operations (expr op expr)
    if (ctx->expr().size() == 2) {
        auto lhs = std::any_cast<llvm::Value*>(visit(ctx->expr(0)));
        auto rhs = std::any_cast<llvm::Value*>(visit(ctx->expr(1)));
        std::string op = ctx->children[1]->getText();

        if (op == "+") return (llvm::Value*)builder.CreateAdd(lhs, rhs, "addtmp");
        if (op == "-") return (llvm::Value*)builder.CreateSub(lhs, rhs, "subtmp");
        if (op == "*") return (llvm::Value*)builder.CreateMul(lhs, rhs, "multmp");
        if (op == "/") return (llvm::Value*)builder.CreateSDiv(lhs, rhs, "divtmp");
    }

    return nullptr;
}

void CodegenVisitor::dumpIR() {
    // Always return something from main
    if (!lastValue) lastValue = builder.getInt32(0);
    // Only create a ret once
    if (!builder.GetInsertBlock()->getTerminator())
        builder.CreateRet(lastValue);

    verifyFunction(*currentFunction);
    module->print(llvm::outs(), nullptr);
}
