#include "FunctionInfo.h"
#include "GenICFG.h"
#include "ICFG.h"
#include "PathFinder.h"
#include "VarFinder.h"
#include "utils.h"
#include <fstream>
#include <iostream>
#include <unistd.h>

#include "llvm/Support/InitLLVM.h"
#include "llvm/Support/PrettyStackTrace.h"
#include "llvm/Support/Program.h"

class FunctionAccumulator : public RecursiveASTVisitor<FunctionAccumulator> {
  private:
    fif &functionsInFile;

  public:
    explicit FunctionAccumulator(fif &functionsInFile)
        : functionsInFile(functionsInFile) {}

    bool VisitFunctionDecl(FunctionDecl *D) {
        FunctionInfo *fi = FunctionInfo::fromDecl(D);
        if (fi == nullptr)
            return true;

        functionsInFile[fi->file].insert(fi);

        return true;
    }
};

void deduplicateCompilationDatabase(fs::path path) {
    std::ifstream ifs(path);
    ordered_json input = ordered_json::parse(ifs);
    ifs.close();

    bool hasDuplicate = false;
    std::set<std::string> visitedFiles;
    ordered_json output;
    for (auto &cmd : input) {
        std::string file = cmd["file"];
        if (visitedFiles.find(file) != visitedFiles.end()) {
            logger.warn("Duplicate entry for file: {}", file);
            hasDuplicate = true;
            continue;
        }
        visitedFiles.insert(file);
        output.push_back(cmd);
    }

    if (!hasDuplicate) {
        logger.info("No duplicate entries found in compilation database");
        return;
    }

    logger.warn("Found duplicate entries in compilation database!");
    {
        fs::path backup = path.string() + ".bk";
        logger.warn("Original database is backed up to: {}", backup);
        std::ofstream o(backup);
        o << input.dump(4, ' ', false, json::error_handler_t::replace)
          << std::endl;
        o.close();
    }
    {
        std::ofstream o(path);
        o << output.dump(4, ' ', false, json::error_handler_t::replace)
          << std::endl;
        o.close();
        logger.warn("Deduplicated database is in: {}", path);
    }
}

std::unique_ptr<CompilationDatabase>
getCompilationDatabase(fs::path buildPath) {
    llvm::errs() << "Getting compilation database from: " << buildPath << "\n";
    std::string errorMsg;
    std::unique_ptr<CompilationDatabase> cb =
        CompilationDatabase::autoDetectFromDirectory(buildPath.string(),
                                                     errorMsg);
    // FIXME: 在 npe/input.json 下会报错
    // #include "clang/Tooling/JSONCompilationDatabase.h"
    // JSONCompilationDatabase::loadFromFile(
    //     buildPath.string(), errorMsg, JSONCommandLineSyntax::AutoDetect);
    if (!cb) {
        llvm::errs() << "Error while trying to load a compilation database:\n"
                     << errorMsg << "Running without flags.\n";
        exit(1);
    }
    return cb;
}

struct VarLocResult {
    int fid, bid;

    VarLocResult() : fid(-1), bid(-1) {}
    VarLocResult(const FunctionInfo *fi, const CFGBlock *block)
        : fid(Global.getIdOfFunction(fi->signature)), bid(block->getBlockID()) {
    }

    bool isValid() const { return fid != -1; }
};

VarLocResult locateVariable(const fif &functionsInFile, const std::string &file,
                            int line, int column, bool isStmt) {
    FindVarVisitor visitor;

    for (const FunctionInfo *fi : functionsInFile.at(file)) {
        // function is defined later than targetLoc
        if (fi->line > line)
            continue;

        // search all CFG stmts in function for matching variable
        ASTContext *Context = &fi->D->getASTContext();
        for (const auto &[stmt, block] : fi->stmtBlockPairs) {
            if (isStmt) {
                // search for stmt
                auto bLoc =
                    Location::fromSourceLocation(*Context, stmt->getBeginLoc());
                if (bLoc && bLoc->file == file && bLoc->line == line &&
                    bLoc->column == column) {
                    int id = block->getBlockID();
                    llvm::errs()
                        << "Found stmt in " << fi->signature << " at " << line
                        << ":" << column << " in block " << id << "\n";
                    return VarLocResult(fi, block);
                }
            } else {
                // search for var within stmt
                const std::string var =
                    visitor.findVarInStmt(Context, stmt, file, line, column);
                if (!var.empty()) {
                    int id = block->getBlockID();
                    llvm::errs() << "Found var '" << var << "' in "
                                 << fi->signature << " at " << line << ":"
                                 << column << " in block " << id << "\n";
                    return VarLocResult(fi, block);
                }
            }
        }
    }
    return VarLocResult();
}

struct FunctionLocator {
    // file -> (fid, start line)
    std::map<std::string, std::vector<std::pair<int, int>>> functionLocations;

    FunctionLocator() {
        for (int i = 0; i < Global.functionLocations.size(); i++) {
            const Location &loc = Global.functionLocations[i];
            functionLocations[loc.file].emplace_back(i, loc.line);
        }
        // 根据 line 降序排列
        for (auto &[file, locs] : functionLocations) {
            std::sort(locs.begin(), locs.end(),
                      [](const auto &a, const auto &b) {
                          return a.second > b.second;
                      });
        }
    }

    int getFid(const Location &loc) const {
        auto it = functionLocations.find(loc.file);
        if (it == functionLocations.end())
            return -1;

        for (const auto &[fid, startLine] : it->second) {
            if (loc.line >= startLine) {
                return fid;
            }
        }
        return -1;
    }
};

VarLocResult locateVariable(const FunctionLocator &locator, const Location &loc,
                            bool isStmt) {
    int fid = locator.getFid(loc);
    if (fid == -1) {
        return VarLocResult();
    }

    ClangTool Tool(*Global.cb, {loc.file});
    DiagnosticConsumer DC = IgnoringDiagConsumer();
    Tool.setDiagnosticConsumer(&DC);

    std::vector<std::unique_ptr<ASTUnit>> ASTs;
    Tool.buildASTs(ASTs);

    fif functionsInFile;
    for (auto &AST : ASTs) {
        ASTContext &Context = AST->getASTContext();
        auto *TUD = Context.getTranslationUnitDecl();
        if (TUD->isUnavailable())
            continue;
        FunctionAccumulator(functionsInFile).TraverseDecl(TUD);
    }

    return locateVariable(functionsInFile, loc.file, loc.line, loc.column,
                          isStmt);
}

std::string getSourceCode(SourceManager &SM, const SourceRange &range) {
    const auto &_b = range.getBegin();
    const auto &_e = range.getEnd();
    const char *b = SM.getCharacterData(_b);
    const char *e = SM.getCharacterData(_e);

    return std::string(b, e - b);
}

void saveLocationInfo(ASTContext &Context, const SourceRange &range,
                      ordered_json &j) {
    SourceManager &SM = Context.getSourceManager();

    SourceLocation b = range.getBegin();
    if (b.isMacroID()) {
        b = SM.getExpansionLoc(b);
    }
    auto bLoc = Location::fromSourceLocation(Context, b);
    if (bLoc) {
        j["file"] = bLoc->file;
        j["beginLine"] = bLoc->line;
        j["beginColumn"] = bLoc->column;
    } else {
        j["file"] = "!!! begin loc invalid !!!";
        j["beginLine"] = -1;
        j["beginColumn"] = -1;
    }

    /**
     * SourceRange中，endLoc是最后一个token的起始位置，所以需要找到这个token的结束位置
     * See:
     * https://stackoverflow.com/a/11154162/11938767
     * https://discourse.llvm.org/t/problem-with-retrieving-the-binaryoperator-rhs-end-location/51897
     * https://clang.llvm.org/docs/InternalsManual.html#sourcerange-and-charsourcerange
     */
    SourceLocation _e = range.getEnd();
    if (_e.isMacroID()) {
        // _e = SM.getExpansionLoc(_e);
        _e = getEndOfMacroExpansion(_e, Context);
    }
    SourceLocation e =
        Lexer::getLocForEndOfToken(_e, 0, SM, Context.getLangOpts());
    if (e.isInvalid())
        e = _e;
    auto eLoc = Location::fromSourceLocation(Context, e);

    if (eLoc) {
        j["endLine"] = eLoc->line;
        j["endColumn"] = eLoc->column;
    } else {
        j["endLine"] = -1;
        j["endColumn"] = -1;
    }

    std::string content;
    if (b.isValid() && e.isValid()) {
        const char *cb = SM.getCharacterData(b);
        const char *ce = SM.getCharacterData(e);
        auto length = ce - cb;
        if (length < 0) {
            content = "!!! length < 0 !!!";
        } else {
            if (length > 80) {
                content = std::string(cb, 80);
                content += " ...";
            } else {
                content = std::string(cb, length);
            }
        }
    }
    j["content"] = content;
}

void dumpICFGNode(int u, ordered_json &jPath) {
    auto [fid, bid] = Global.icfg.functionBlockOfNodeId[u];
    requireTrue(fid != -1);

    const NamedLocation &loc = Global.functionLocations[fid];

    llvm::errs() << ">> Node " << u << " is in " << loc.name << " at block "
                 << bid << "\n";

    ClangTool Tool(*Global.cb, {loc.file});
    DiagnosticConsumer DC = IgnoringDiagConsumer();
    Tool.setDiagnosticConsumer(&DC);

    std::vector<std::unique_ptr<ASTUnit>> ASTs;
    Tool.buildASTs(ASTs);

    // FIXME: compile_commands
    // 中一个文件可能对应多条编译命令，所以这里可能有多个AST
    // TODO: 想办法记录 function 用的是哪一条命令，然后只用这一条生成的
    requireTrue(ASTs.size() != 0, "No AST for file");
    if (ASTs.size() > 1) {
        llvm::errs() << "Warning: multiple ASTs for file " << loc.file << "\n";
    }
    std::unique_ptr<ASTUnit> AST = std::move(ASTs[0]);

    fif functionsInFile;
    ASTContext &Context = AST->getASTContext();
    auto *TUD = Context.getTranslationUnitDecl();
    requireTrue(!TUD->isUnavailable());
    FunctionAccumulator(functionsInFile).TraverseDecl(TUD);

    for (const FunctionInfo *fi : functionsInFile.at(loc.file)) {
        if (fi->signature != Global.functionLocations[fid].name)
            continue;
        for (auto BI = fi->cfg->begin(); BI != fi->cfg->end(); ++BI) {
            const CFGBlock &B = **BI;
            if (B.getBlockID() != bid)
                continue;

            bool isEntry = (&B == &fi->cfg->getEntry());
            bool isExit = (&B == &fi->cfg->getExit());
            if (isEntry || isExit) {
                ordered_json j;
                j["type"] = isEntry ? "entry" : "exit";
                // TODO: content只要declaration就行，不然太大了
                saveLocationInfo(Context, fi->D->getSourceRange(), j);
                jPath.push_back(j);
                return;
            }

            // B.dump(fi->cfg, Context.getLangOpts(), true);

            std::vector<const Stmt *> allStmts;
            std::set<const Stmt *> isChild;

            // iterate over all elements to find stmts & record children
            for (auto EI = B.begin(); EI != B.end(); ++EI) {
                const CFGElement &E = *EI;
                if (std::optional<CFGStmt> CS = E.getAs<CFGStmt>()) {
                    const Stmt *S = CS->getStmt();
                    allStmts.push_back(S);

                    // iterate over childern
                    for (const Stmt *child : S->children()) {
                        if (child != nullptr)
                            isChild.insert(child);
                    }
                }
            }

            // print all non-child stmts
            for (const Stmt *S : allStmts) {
                if (isChild.find(S) != isChild.end())
                    continue;
                // S is not child of any stmt in this CFGBlock

                // auto bLoc =
                //     Location::fromSourceLocation(Context, S->getBeginLoc());
                // auto eLoc =
                //     Location::fromSourceLocation(Context, S->getEndLoc());
                // llvm::errs()
                //     << "  Stmt " << bLoc->line << ":" << bLoc->column << " "
                //     << eLoc->line << ":" << eLoc->column << "\n";
                // S->dumpColor();

                ordered_json j;
                j["type"] = "stmt";
                saveLocationInfo(Context, S->getSourceRange(), j);
                j["stmtKind"] = std::string(S->getStmtClassName());
                jPath.push_back(j);
            }

            return;
        }
    }
}

void saveAsJson(const std::set<std::vector<int>> &results,
                const std::string &type, ordered_json &jResults) {
    std::vector<std::vector<int>> sortedResults(results.begin(), results.end());
    // sort based on length
    std::sort(sortedResults.begin(), sortedResults.end(),
              [](const std::vector<int> &a, const std::vector<int> &b) {
                  return a.size() < b.size();
              });
    int cnt = 0;
    for (const auto &path : sortedResults) {
        if (cnt++ > 10)
            break;
        ordered_json jPath;
        jPath["type"] = type;
        jPath["nodes"] = path;
        for (int x : path) {
            dumpICFGNode(x, jPath["locations"]);
        }
        jResults.push_back(jPath);
    }
}

static void findPathBetween(const VarLocResult &from, const VarLocResult &to,
                            const std::vector<VarLocResult> &path,
                            const std::string &type, ordered_json &jResults) {
    requireTrue(from.isValid());
    requireTrue(to.isValid());

    ICFG &icfg = Global.icfg;
    int u = icfg.getNodeId(from.fid, from.bid);
    int v = icfg.getNodeId(to.fid, to.bid);

    std::vector<int> pathFilter;
    for (const auto &loc : path) {
        requireTrue(loc.isValid());
        pathFilter.push_back(icfg.getNodeId(loc.fid, loc.bid));
    }

    ICFGPathFinder pFinder(icfg);
    pFinder.search(u, v, pathFilter, 3);

    saveAsJson(pFinder.results, type, jResults);
}

/**
 * 生成全程序调用图
 */
void generateICFG(const std::vector<std::string> &allFiles) {
    llvm::errs() << "\n--- Generating whole program call graph ---\n";
    ClangTool Tool(*Global.cb, allFiles);
    DiagnosticConsumer DC = IgnoringDiagConsumer();
    Tool.setDiagnosticConsumer(&DC);

    Tool.run(newFrontendActionFactory<GenICFGAction>().get());
}

/**
 * FIXME: all files 应该只有
 * .c/.cpp，不包括头文件，所以统计会不准。以及，buildPath
 * 现在是一个 JSON 文件了，不是目录。
 */
void printCloc(const std::vector<std::string> &allFiles) {
    // save all files to "compile_files.txt" under build path
    fs::path resultFiles =
        fs::path(Global.projectDirectory) / "compile_files.txt";
    std::ofstream ofs(resultFiles);
    if (!ofs.is_open()) {
        llvm::errs() << "Error: cannot open file " << resultFiles << "\n";
        exit(1);
    }
    for (auto &file : allFiles)
        ofs << file << "\n";
    ofs.close();

    // run cloc on all files
    if (ErrorOr<std::string> P = sys::findProgramByName("cloc")) {
        std::string programPath = *P;
        std::vector<StringRef> args;
        args.push_back("cloc");
        args.push_back("--list-file");
        args.push_back(resultFiles.c_str()); // don't use .string() here
        std::string errorMsg;
        if (sys::ExecuteAndWait(programPath, args, std::nullopt, {}, 0, 0,
                                &errorMsg)) {
            llvm::errs() << "Error: " << errorMsg << "\n";
        }
    }
}

int main(int argc, const char **argv) {
    spdlog::set_level(spdlog::level::debug);

    if (argc != 2) {
        logger.error("Usage: {} IR.json", argv[0]);
        return 1;
    }

    llvm::InitLLVM X(argc, argv);

    fs::path jsonPath = fs::absolute(argv[1]);
    std::ifstream ifs(jsonPath);
    if (!ifs.is_open()) {
        logger.error("Cannot open file {}", jsonPath);
        return 1;
    }
    logger.info("Reading from input json: {}", jsonPath);
    ordered_json input = ordered_json::parse(ifs);

    Global.projectDirectory =
        fs::canonical(input["root"].template get<std::string>()).string();

    fs::path compile_commands =
        fs::canonical(input["compile_commands"].template get<std::string>());
    logger.info("Compilation database: {}", compile_commands);
    deduplicateCompilationDatabase(compile_commands);
    Global.cb = getCompilationDatabase(compile_commands);

    // print all files in compilation database
    const auto &allFiles = Global.cb->getAllFiles();
    llvm::errs() << "All files (" << allFiles.size() << "):\n";
    for (auto &file : allFiles)
        llvm::errs() << "  " << file << "\n";

    generateICFG(allFiles);

    {
        llvm::errs() << "--- ICFG ---\n";
        llvm::errs() << "  n: " << Global.icfg.n << "\n";
        int m = 0;
        for (const auto &edges : Global.icfg.G) {
            m += edges.size();
        }
        llvm::errs() << "  m: " << m << "\n";
    }

    fs::path outputDir = jsonPath.parent_path();
    {
        llvm::errs() << "--- Path-finding ---\n";

        FunctionLocator locator;
        fs::path jsonResult = outputDir / "output.json";
        llvm::errs() << "Output: " << jsonResult << "\n";

        int total = input["results"].size();
        llvm::errs() << "There are " << total << " results to search.\n";

        ordered_json output(input);
        output["results"].clear();

        int cnt = 0;
        for (ordered_json &result : input["results"]) {
            cnt++;
            std::string type = result["type"].template get<std::string>();
            llvm::errs() << "[" << cnt << "/" << total << "] " << type << "\n";

            ordered_json &locations = result["locations"];
            VarLocResult from, to;
            std::vector<VarLocResult> path;
            for (const ordered_json &loc : locations) {
                std::string type = loc["type"].template get<std::string>();

                bool isStmt = type == "stmt";
                VarLocResult varLoc =
                    locateVariable(locator, Location(loc), isStmt);
                if (!varLoc.isValid()) {
                    llvm::errs() << "Error: cannot locate " << type << " at "
                                 << loc.dump(4, ' ', false,
                                             json::error_handler_t::replace)
                                 << "\n";
                    exit(1);
                }

                if (type == "source") {
                    from = varLoc;
                } else if (type == "sink") {
                    to = varLoc;
                } else {
                    path.emplace_back(varLoc);
                }
            }

            findPathBetween(from, to, path, type, output["results"]);
        }

        std::ofstream o(jsonResult);
        o << output.dump(4, ' ', false, json::error_handler_t::replace)
          << std::endl;
        o.close();
    }

    return 0;

    while (true) {
        std::string methodName;
        llvm::errs() << "> ";
        std::getline(std::cin, methodName);
        if (!std::cin)
            break;
        if (methodName.find('(') == std::string::npos)
            methodName += "(";

        for (const auto &[caller, callees] : Global.callGraph) {
            if (caller.find(methodName) != 0)
                continue;
            llvm::errs() << caller << "\n";
            for (const auto &callee : callees) {
                llvm::errs() << "  " << callee << "\n";
            }
        }
    }
}