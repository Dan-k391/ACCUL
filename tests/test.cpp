#include "ExprLexer.h"
#include "ExprParser.h"
#include "CodegenVisitor.h"
#include <antlr4-runtime.h>

#include <cassert>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

#if !defined(CLANG_PATH)
#  define CLANG_PATH "clang"
#endif

namespace fs = std::filesystem;

static int run_command(const std::string &cmd, std::string *out = nullptr) {
    std::string tmpFile = (fs::temp_directory_path() / "accul_stdout.txt").string();
    std::string fullCmd = cmd + " >\"" + tmpFile + "\" 2>/dev/null";
    int rc = std::system(fullCmd.c_str());
    if (out) {
        std::ifstream in(tmpFile);
        std::ostringstream ss;
        ss << in.rdbuf();
        *out = ss.str();
    }
    fs::remove(tmpFile);
    return rc;
}

int main(int argc, char *argv[]) {
    if (argc < 3) {
        std::cerr << "usage: tests \"<expr>\" <expected_int>\n";
        return 2;
    }

    const std::string expr = argv[1];
    const int expected = std::stoi(argv[2]);

    antlr4::ANTLRInputStream stream(expr);
    ExprLexer lexer(&stream);
    antlr4::CommonTokenStream tokens(&lexer);
    ExprParser parser(&tokens);

    CodegenVisitor visitor;
    visitor.visit(parser.prog());

    fs::path tmpDir = fs::temp_directory_path();
    fs::path asmPath = tmpDir / "accul_test_output.s";
    fs::path exePath = tmpDir / "accul_test_bin";
#ifdef _WIN32
    exePath += ".exe";
#endif

    visitor.emitAssembly(asmPath.string());
    std::cout << "[ASM] " << asmPath << "\n";

    // Build executable
    std::string ccCmd = std::string(CLANG_PATH) + " \"" + asmPath.string() + "\" -o \"" + exePath.string() + "\"";
    if (run_command(ccCmd) != 0) {
        std::cerr << "[FAIL] clang failed\n";
        return 3;
    }

    // Run and capture stdout
    std::string output;
    run_command("\"" + exePath.string() + "\"", &output);

    // Parse printed integer
    int result = 0;
    std::stringstream(output) >> result;

    if (result != expected) {
        std::cerr << "[FAIL] expr=" << expr << " expected=" << expected << " got=" << result << "\n";
        return 4;
    }

    std::cout << "[OK] expr=" << expr << " -> " << result << "\n";
    return 0;
}
