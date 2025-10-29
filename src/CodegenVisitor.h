#pragma once
#include "ExprBaseVisitor.h"
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>

class CodegenVisitor : public ExprBaseVisitor {
public:
    llvm::LLVMContext context;
    llvm::IRBuilder<> builder;
    std::unique_ptr<llvm::Module> module;
    llvm::Function *currentFunction = nullptr;
    llvm::Value *lastValue = nullptr;

    CodegenVisitor();
    antlrcpp::Any visitProg(ExprParser::ProgContext *ctx) override;
    antlrcpp::Any visitExpr(ExprParser::ExprContext *ctx) override;
    void dumpIR();
};
