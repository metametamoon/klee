#ifndef KLEE_TRACEVERIFIER_H
#define KLEE_TRACEVERIFIER_H

#include "klee/KLEEConfig.h"
#include "klee/Module/InstructionInfoTable.h"
#include "klee/Module/SarifReport.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"
#include <memory>

namespace klee {

struct ModuleOptions;

class TraceVerifier {
public:
  TraceVerifier(const llvm::Module *m, Config cfg);

  AnalysisReport verifyTraces(SarifReport report);

private:
  llvm::Module *mod;
  llvm::LLVMContext ctx;
  Config cfg;

  std::vector<std::unique_ptr<llvm::Module>> loadedUserModules;
  std::vector<std::unique_ptr<llvm::Module>> loadedLibsModules;
  ModuleOptions *Opts;
  std::vector<std::string> mainModuleFunctions;
  std::unique_ptr<InstructionInfoTable> origInfos;
  std::string ep;

  int pArgc;
  char **pArgv;
  char **pEnvp;
};

}

#endif
