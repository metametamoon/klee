#include "klee/Solver/Fuzzer.h"

#include "klee/Core/Interpreter.h"
#include "klee/Expr/Assignment.h"
#include "klee/Expr/Constraints.h"
#include "klee/Expr/Expr.h"
#include "klee/Expr/ExprUtil.h"
#include "klee/Expr/ExternalCall.h"
#include "klee/Expr/SymbolicSource.h"
#include "klee/Solver/IREmitter.h"
#include "llvm/ADT/StringRef.h"
#include "llvm/ExecutionEngine/MCJIT.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/LegacyPassManager.h"
#include "llvm/IR/Verifier.h"
#include "llvm/Support/Casting.h"
#include "llvm/Support/DynamicLibrary.h"
#include "llvm/Support/Path.h"
#include "llvm/Support/TargetSelect.h"
#include "llvm/Support/raw_ostream.h"

#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <memory>
#include <string>
#include <sys/stat.h>
#include <sys/wait.h>
#include <system_error>
#include <unistd.h>
#include <unordered_set>
#include <utility>

using namespace klee;

Fuzzer::Fuzzer(LLVMContext &ctx, Solver *s) : ctx(ctx), s(s), timeout(2000000) {
  llvm::InitializeNativeTarget();
  llvm::InitializeNativeTargetAsmParser();
  llvm::InitializeNativeTargetAsmPrinter();
  executionEngine = nullptr;
}

void Fuzzer::initializeEngine() {
  InitializeNativeTarget();
  InitializeNativeTargetAsmPrinter();
  InitializeNativeTargetAsmParser();
  std::string error;
  singleDispatchModule = new Module("", ctx);
  // The MCJIT JITs whole modules at a time rather than individual functions
  // so we will let it manage the modules.
  // Note that we don't do anything with `singleDispatchModule`. This is just
  // so we can use the EngineBuilder API.
  auto dispatchModuleUniq = std::unique_ptr<Module>(singleDispatchModule);
  executionEngine = EngineBuilder(std::move(dispatchModuleUniq))
                        .setErrorStr(&error)
                        .setEngineKind(EngineKind::JIT)
                        .create();

  if (!executionEngine) {
    llvm::errs() << "unable to make jit: " << error << "\n";
    abort();
  }

  // from ExecutionEngine::create
  if (executionEngine) {
    // Make sure we can resolve symbols in the program as well. The zero arg
    // to the function tells DynamicLibrary to load the program, not a library.
    sys::DynamicLibrary::LoadLibraryPermanently(0);
    // InitializeNativeTarget();
    // InitializeNativeTargetAsmPrinter();
    // InitializeNativeTargetAsmParser();
  }
}

bool Fuzzer::fuzz(const Query &q, const std::vector<const Array *> &symcretes,
                  const std::vector<const Array *> &freeSymbolics,
                  Assignment &assign) {
  auto automatonState = automaton[current];
  llvm::errs() << "Attepmpt to fuzz, automaton state: " << automatonState << "\n";
  if (automatonState == -2) {
    return false;
  }
  auto toEmit = readyQuery(q);
  std::set<ref<Expr>> set;
  for (auto i : toEmit.first) {
    set.insert(i);
  }
  if (auto look = resultCache.lookup(set)) {
    if (look) {
      llvm::errs() << "Cache hit\n";
      if (*look) {
        llvm::errs() << "cache: success\n";
        auto cacheEntry = cache.lookup(set);
        assert(cacheEntry);
        for (auto i : cacheEntry->bindings) {
          assign.bindings.insert(i);
          return true;
        }
      } else {
        llvm::errs() << "cache: failure\n";
        return false;
      }
    }
  }
  llvm::errs() << "Cache miss\n";
  IREmitter emitter(ctx, toEmit.first, toEmit.second, freeSymbolics);
  auto module = emitter.emit();
  auto instrSize = toEmit.first.size();
  llvm::legacy::PassManager pm;
  std::string dumpedM;
  llvm::raw_string_ostream l(dumpedM);
  l << *module;
  pm.add(createVerifierPass());
  pm.run(*module);

  delete executionEngine;
  executionEngine = nullptr;
  initializeEngine();
  executionEngine->addModule(std::move(module));
  uint64_t fnAddr =
      executionEngine->getFunctionAddress("LLVMFuzzerTestOneInput");
  executionEngine->finalizeObject();
  auto harness = (int (*)(const uint8_t *, uint64_t))fnAddr;

  uint64_t solutionSize = 0;
  for (auto i : freeSymbolics) {
    solutionSize += i->size;
  }

  unsigned char *solution = (unsigned char *) malloc(solutionSize);

  FuzzInfo fi;
  fi.harness = harness;
  fi.timeout = timeout;
  fi.map_size = instrSize + 1;
  fi.data_size = solutionSize;
  fi.solution = solution;

  unsigned char *seed = (unsigned char *) malloc(solutionSize);
  unsigned seedIndex = 0;
  std::vector<std::vector<unsigned char>> sols;
  s->getInitialValues(q.withFalse(), freeSymbolics, sols);
  for (unsigned i = 0; i < freeSymbolics.size(); i++) {
    for (unsigned j = 0; j < freeSymbolics[i]->size; j++) {
      seed[seedIndex] = sols[i][j];
      seedIndex++;
    }
  }
  fi.seed = seed;

  bool success = fuzz_internal(fi);

  if (!success) {
    // llvm::errs() << "Fuzzing failed for the following:\n";
    // llvm::errs() << "Expressions:\n";
    // for (auto i : toEmit.first) {
    //   i->dump();
    // }
    // llvm::errs() << "Calls:\n";
    // for (auto i : toEmit.second) {
    //   llvm::errs() << i->f->getName() << " with args:\n";
    //   for (auto j : i->args) {
    //     j.expr->dump();
    //   }
    // }
    // llvm::errs() << "Emitted program:\n";
    // llvm::errs() << dumpedM;
    automaton[current]--;
    free(solution);
    resultCache.insert(set, false);
    delete executionEngine;
    executionEngine = nullptr;
    return false;
  }

  IREmitter emitterCheck(ctx, toEmit.first, toEmit.second, freeSymbolics, &symcretes);
  auto moduleCheck = emitterCheck.emit();
  pm.run(*moduleCheck);

  delete executionEngine;
  executionEngine = nullptr;
  initializeEngine();
  executionEngine->addModule(std::move(moduleCheck));
  fnAddr = executionEngine->getFunctionAddress("LLVMFuzzerTestOneInput");
  executionEngine->finalizeObject();

  auto symcreteHarness = (unsigned char * (*)(uint8_t *, uint64_t))fnAddr;
  int pipefd[2];
  pipe(pipefd);

  int status = 0;
  if (auto pid = fork()) {
    waitpid(pid, &status, 0);
    close(pipefd[1]);
    unsigned char temp;
    std::vector<std::vector<unsigned char>> values;
    for (auto array : symcretes) {
      std::vector<unsigned char> value;
      for (unsigned i = 0; i < array->size; i++) {
        bool n_read = read(pipefd[0], &temp, 1);
        assert(n_read == 1 && "symcrete retrieval failed");
        value.push_back(temp);
      }
      values.push_back(std::move(value));
    }

    // IREmitter should translate the addresses back

    assign = Assignment(symcretes, values, true);
    resultCache.insert(set, true);
    cache.insert(set, assign);
    close(pipefd[0]);
    free(solution);
    delete executionEngine;
    executionEngine = nullptr;
    if (automaton[current] != 2) {
      automaton[current]++;
    }
    return true;
  } else {
    close(pipefd[0]);
    auto ans = symcreteHarness(solution, solutionSize);
    assert(ans && "symcrete retrieval failed");
    uint64_t symcretesSize = 0;
    for (auto i : symcretes) {
      symcretesSize += i->size;
    }
    write(pipefd[1], ans, symcretesSize);
    free(ans);
    close(pipefd[1]);
    exit(0);
  }
}

void Fuzzer::setTimeout(time::Span _timeout) {
  timeout = _timeout.toMicroseconds();
  if (!timeout) {
    timeout  = 2000000;
  }
}

std::pair<std::vector<ref<Expr>>, std::vector<ref<ExternalCall>>>
Fuzzer::readyQuery(const Query &q) {
  std::vector<ref<ExternalCall>> calls;
  std::set<ref<ExternalCall>> callsSet;
  for (auto s : q.constraints.symcretes()) {
    if (auto source = dyn_cast<ExternalCallSource>(s.marker->source)) {
      if (!callsSet.count(source->call)) {
        callsSet.insert(source->call);
        calls.push_back(source->call);
      }
    }
  }

  std::sort(calls.begin(), calls.end());
  std::vector<ref<Expr>> constraints;

  for (auto e : q.constraints.constraints()) {
    constraints.push_back(e);
  }
  constraints.push_back(q.expr);
  return std::make_pair(constraints, calls);
}

void Fuzzer::dumpProgram(Module *m) {
  llvm::errs() << *m;
}
