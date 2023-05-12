// -*- c++ -*-
#ifndef KLEE_RUN_KLEE_H
#define KLEE_RUN_KLEE_H

#include "klee/Module/SarifReport.h"
#include "llvm/Support/CommandLine.h"

klee::SarifReport parseInputPathTree(const std::string &inputPathTreePath);

namespace klee {
extern llvm::cl::OptionCategory ExtCallsCat;
extern llvm::cl::OptionCategory PointerResolvingCat;
}

int run_klee(int argc, char **argv, char **envp);

#endif // KLEE_RUN_KLEE_H
