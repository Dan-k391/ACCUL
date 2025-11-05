#pragma once
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
#include "CBaseVisitor.h"
#include "CParser.h"


class CodegenVisitor : public CBaseVisitor {
public:
    llvm::LLVMContext context;
    llvm::IRBuilder<> builder;
    std::unique_ptr<llvm::Module> module;
    llvm::Function *currentFunction = nullptr;
    llvm::Value *lastValue = nullptr;

    CodegenVisitor();
    antlrcpp::Any visitProg(CParser::ProgContext *ctx) override;
    antlrcpp::Any visitEqExpr(CParser::EqExprContext *ctx) override;
    antlrcpp::Any visitRelExpr(CParser::RelExprContext *ctx) override;
    antlrcpp::Any visitAddSubExpr(CParser::AddSubExprContext *ctx) override;
    antlrcpp::Any visitMulDivExpr(CParser::MulDivExprContext *ctx) override;
    antlrcpp::Any visitUnaryExpr(CParser::UnaryExprContext *ctx) override;
    antlrcpp::Any visitIntLiteral(CParser::IntLiteralContext *ctx) override;
    antlrcpp::Any visitVarRef(CParser::VarRefContext *ctx) override;
    antlrcpp::Any visitParenExpr(CParser::ParenExprContext *ctx) override;
    void emitAssembly(const std::string &filename);
    void dumpIR();
};
