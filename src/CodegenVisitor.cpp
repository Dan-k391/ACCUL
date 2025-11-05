#include "CodegenVisitor.h"

CodegenVisitor::CodegenVisitor() : builder(context) {
    module = std::make_unique<llvm::Module>("expr_module", context);

    // int main()
    llvm::FunctionType *funcType = llvm::FunctionType::get(builder.getInt32Ty(), false);
    llvm::Function *mainFunc = llvm::Function::Create(funcType, llvm::Function::ExternalLinkage, "main", module.get());
    llvm::BasicBlock *entry = llvm::BasicBlock::Create(context, "entry", mainFunc);
    builder.SetInsertPoint(entry);
    currentFunction = mainFunc;
    lastValue = nullptr;
}

antlrcpp::Any CodegenVisitor::CodegenVisitor::visitProg(CParser::ProgContext *ctx) {
    // Evaluate the top-level expression
    lastValue = std::any_cast<llvm::Value*>(visit(ctx->expr()));

    // Ensure we’re inserting into main’s entry block
    if (!builder.GetInsertBlock() || builder.GetInsertBlock()->getParent() != currentFunction) {
        auto &entry = currentFunction->getEntryBlock();
        builder.SetInsertPoint(&entry, entry.end());
    }

    // ------------------------------------------------------------------------
    // Add printf("%d\n", result)
    // ------------------------------------------------------------------------
    llvm::FunctionCallee printfFunc = module->getOrInsertFunction(
        "printf",
        llvm::FunctionType::get(
            builder.getInt32Ty(),
            llvm::Type::getInt8Ty(context),
            true));

    builder.CreateCall(
        printfFunc,
        {
            builder.CreateGlobalString("%d\n"),
            lastValue ? lastValue : builder.getInt32(0)
        });

    // ------------------------------------------------------------------------
    // Return 0 from main
    // ------------------------------------------------------------------------
    if (!builder.GetInsertBlock()->getTerminator())
        builder.CreateRet(builder.getInt32(0));

    return lastValue;
}

antlrcpp::Any CodegenVisitor::visitEqExpr(CParser::EqExprContext *ctx) {
    auto L = std::any_cast<llvm::Value*>(visit(ctx->expr(0)));
    auto R = std::any_cast<llvm::Value*>(visit(ctx->expr(1)));
    std::string op = ctx->op->getText();
    llvm::Value* cmp = nullptr;
    if (op == "==") cmp = builder.CreateICmpEQ(L, R, "eqtmp");
    if (op == "!=") cmp = builder.CreateICmpNE(L, R, "netmp");
    return (llvm::Value*)builder.CreateZExt(cmp, builder.getInt64Ty(), "booltmp");
}

// <, >, <=, >=
antlrcpp::Any CodegenVisitor::visitRelExpr(CParser::RelExprContext *ctx) {
    auto L = std::any_cast<llvm::Value*>(visit(ctx->expr(0)));
    auto R = std::any_cast<llvm::Value*>(visit(ctx->expr(1)));
    std::string op = ctx->op->getText();
    llvm::Value* cmp = nullptr;
    if (op == "<")  cmp = builder.CreateICmpSLT(L, R, "lttmp");
    if (op == "<=") cmp = builder.CreateICmpSLE(L, R, "letmp");
    if (op == ">")  cmp = builder.CreateICmpSGT(L, R, "gttmp");
    if (op == ">=") cmp = builder.CreateICmpSGE(L, R, "getmp");
    return (llvm::Value*)builder.CreateZExt(cmp, builder.getInt64Ty(), "booltmp");
}

// + and -
antlrcpp::Any CodegenVisitor::visitAddSubExpr(CParser::AddSubExprContext *ctx) {
    auto L = std::any_cast<llvm::Value*>(visit(ctx->expr(0)));
    auto R = std::any_cast<llvm::Value*>(visit(ctx->expr(1)));
    if (ctx->op->getText() == "+")
        return (llvm::Value*)builder.CreateAdd(L, R, "addtmp");
    else
        return (llvm::Value*)builder.CreateSub(L, R, "subtmp");
}

// * and /
antlrcpp::Any CodegenVisitor::visitMulDivExpr(CParser::MulDivExprContext *ctx) {
    auto L = std::any_cast<llvm::Value*>(visit(ctx->expr(0)));
    auto R = std::any_cast<llvm::Value*>(visit(ctx->expr(1)));
    if (ctx->op->getText() == "*")
        return (llvm::Value*)builder.CreateMul(L, R, "multmp");
    else
        return (llvm::Value*)builder.CreateSDiv(L, R, "divtmp");
}

// unary + / -
antlrcpp::Any CodegenVisitor::visitUnaryExpr(CParser::UnaryExprContext *ctx) {
    auto val = std::any_cast<llvm::Value*>(visit(ctx->expr()));
    if (ctx->op->getText() == "-")
        return (llvm::Value*)builder.CreateNeg(val, "negtmp");
    return val;
}

// integer literal
antlrcpp::Any CodegenVisitor::visitIntLiteral(CParser::IntLiteralContext *ctx) {
    long long v = std::stoll(ctx->INT()->getText());
    return (llvm::Value*)builder.getInt64(v);
}

antlrcpp::Any CodegenVisitor::visitVarRef(CParser::VarRefContext *ctx) {
}

// ( expr )
antlrcpp::Any CodegenVisitor::visitParenExpr(CParser::ParenExprContext *ctx) {
    return visit(ctx->expr());
}

void CodegenVisitor::emitAssembly(const std::string &filename) {
    llvm::InitializeNativeTarget();
    llvm::InitializeNativeTargetAsmPrinter();
    llvm::InitializeNativeTargetAsmParser();

    std::string TripleStr = llvm::sys::getDefaultTargetTriple();
    llvm::Triple TheTriple(TripleStr);
    module->setTargetTriple(TheTriple);

    std::string Error;
    const llvm::Target *Target = llvm::TargetRegistry::lookupTarget(TripleStr, Error);
    if (!Target) { llvm::errs() << Error << "\n"; return; }

    std::string CPU = "generic";
    std::string Features = "";
    llvm::TargetOptions opt;
    auto RM = std::optional<llvm::Reloc::Model>(llvm::Reloc::PIC_);
    auto CM = std::optional<llvm::CodeModel::Model>(llvm::CodeModel::Small);
    llvm::CodeGenOptLevel OL = llvm::CodeGenOptLevel::Default;
    // ✅ Enable position-independent code generation
    auto TM = Target->createTargetMachine(TheTriple, CPU, Features, opt, RM, CM, OL, false);

    module->setDataLayout(TM->createDataLayout());

    std::error_code EC;
    llvm::raw_fd_ostream dest(filename, EC, llvm::sys::fs::OF_None);
    if (EC) { llvm::errs() << "Could not open file: " << EC.message() << "\n"; return; }

    llvm::legacy::PassManager pm;
    if (TM->addPassesToEmitFile(pm, dest, nullptr, llvm::CodeGenFileType::AssemblyFile)) {
        llvm::errs() << "TargetMachine can't emit this file type\n";
        return;
    }
    pm.run(*module);
    dest.flush();

    llvm::outs() << "✅ Emitted assembly to " << filename << "\n";
}

void CodegenVisitor::dumpIR() {
    verifyFunction(*currentFunction);
    module->print(llvm::outs(), nullptr);
}
