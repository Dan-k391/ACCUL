#pragma once
#include "ExprBaseVisitor.h"
#include "ExprParser.h"
#include <string>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/Verifier.h>
#include <llvm/Support/raw_ostream.h>
#include <llvm/IR/LegacyPassManager.h>
#include <llvm/Support/TargetSelect.h>
#include <llvm/MC/TargetRegistry.h>
#include <llvm/Target/TargetMachine.h>
#include <llvm/Support/FileSystem.h>
#include <llvm/TargetParser/Triple.h>
#include <llvm/TargetParser/Host.h>
#include <llvm/Support/raw_ostream.h>

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
    void emitAssembly(const std::string &filename);
    void dumpIR();
};
