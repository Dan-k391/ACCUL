#include "CodegenVisitor.h"

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

antlrcpp::Any CodegenVisitor::visitProg(ExprParser::ProgContext *ctx) {
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

    // Case 3: unary operations ('-' expr | '+' expr)
    if (ctx->children.size() == 2 && (ctx->children[0]->getText() == "-" || ctx->children[0]->getText() == "+")) {
        auto val = std::any_cast<llvm::Value*>(visit(ctx->expr(0)));
        if (ctx->children[0]->getText() == "-") {
            auto zero = builder.getInt32(0);
            return (llvm::Value*)builder.CreateSub(zero, val, "negtmp");
        }
        return val; // unary +
    }

    // Case 4: binary operations (expr op expr)
    if (ctx->expr().size() == 2) {
        auto lhs = std::any_cast<llvm::Value*>(visit(ctx->expr(0)));
        auto rhs = std::any_cast<llvm::Value*>(visit(ctx->expr(1)));
        std::string op = ctx->children[1]->getText();

        if (op == "+") return (llvm::Value*)builder.CreateAdd(lhs, rhs, "addtmp");
        if (op == "-") return (llvm::Value*)builder.CreateSub(lhs, rhs, "subtmp");
        if (op == "*") return (llvm::Value*)builder.CreateMul(lhs, rhs, "multmp");
        if (op == "/") return (llvm::Value*)builder.CreateSDiv(lhs, rhs, "divtmp");
        if (op == "==") {
            auto cmp = builder.CreateICmpEQ(lhs, rhs, "eqtmp");
            return (llvm::Value*)builder.CreateZExt(cmp, builder.getInt32Ty(), "booltmp");
        }
        if (op == "!=") {
            auto cmp = builder.CreateICmpNE(lhs, rhs, "netmp");
            return (llvm::Value*)builder.CreateZExt(cmp, builder.getInt32Ty(), "booltmp");
        }
        if (op == "<") {
            auto cmp = builder.CreateICmpSLT(lhs, rhs, "lttmp");
            return (llvm::Value*)builder.CreateZExt(cmp, builder.getInt32Ty(), "booltmp");
        }
        if (op == "<=") {
            auto cmp = builder.CreateICmpSLE(lhs, rhs, "letmp");
            return (llvm::Value*)builder.CreateZExt(cmp, builder.getInt32Ty(), "booltmp");
        }
        if (op == ">") {
            auto cmp = builder.CreateICmpSGT(lhs, rhs, "gttmp");
            return (llvm::Value*)builder.CreateZExt(cmp, builder.getInt32Ty(), "booltmp");
        }
        if (op == ">=") {
            auto cmp = builder.CreateICmpSGE(lhs, rhs, "getmp");
            return (llvm::Value*)builder.CreateZExt(cmp, builder.getInt32Ty(), "booltmp");
        } 
    }

    return nullptr;
}

void CodegenVisitor::emitAssembly(const std::string &filename) {
    InitializeNativeTarget();
    InitializeNativeTargetAsmPrinter();
    InitializeNativeTargetAsmParser();

    std::string TripleStr = sys::getDefaultTargetTriple();
    Triple TheTriple(TripleStr);
    module->setTargetTriple(TheTriple);

    std::string Error;
    const Target *Target = TargetRegistry::lookupTarget(TripleStr, Error);
    if (!Target) { errs() << Error << "\n"; return; }

    std::string CPU = "generic";
    std::string Features = "";
    TargetOptions opt;
    auto RM = std::optional<Reloc::Model>(Reloc::PIC_);
    auto CM = std::optional<CodeModel::Model>(CodeModel::Small);
    CodeGenOptLevel OL = CodeGenOptLevel::Default;
    // ✅ Enable position-independent code generation
    auto TM = Target->createTargetMachine(TheTriple, CPU, Features, opt, RM, CM, OL, false);

    module->setDataLayout(TM->createDataLayout());

    std::error_code EC;
    llvm::raw_fd_ostream dest(filename, EC, sys::fs::OF_None);
    if (EC) { errs() << "Could not open file: " << EC.message() << "\n"; return; }

    legacy::PassManager pm;
    if (TM->addPassesToEmitFile(pm, dest, nullptr, CodeGenFileType::AssemblyFile)) {
        errs() << "TargetMachine can't emit this file type\n";
        return;
    }
    pm.run(*module);
    dest.flush();

    outs() << "✅ Emitted assembly to " << filename << "\n";
}

void CodegenVisitor::dumpIR() {
    verifyFunction(*currentFunction);
    module->print(llvm::outs(), nullptr);
}
