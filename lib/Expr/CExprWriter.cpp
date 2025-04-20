#include "klee/Expr/CExprWriter.h"
#include <klee/ADT/Ref.h>
#include <klee/Expr/Expr.h>
#include <klee/Module/KInstruction.h>
#include <klee/Module/KModule.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/IntrinsicInst.h>
#include <optional>

namespace klee {

std::string printConstAsC(const ref<ConstantExpr> &e) {
  if (e->getWidth() == Expr::Bool)
    return (e->isTrue() ? "true" : "false");
  else
    return std::to_string(e->getZExtValue());
}

std::optional<std::string> retrieveName(const llvm::Instruction *inst) {
  llvm::Function const *caller = inst->getParent()->getParent();
  // Search for llvm.dbg.declare
  for (auto i = caller->begin(); i != caller->end(); ++i) {
    llvm::BasicBlock const &BB = *i;
    for (llvm::Instruction const &I : BB) {
      if (auto const *dbg = dyn_cast<llvm::DbgDeclareInst>(&I)) {
        if (auto *dbgInst = dyn_cast<llvm::Instruction>(dbg->getAddress())) {
          if (dbgInst == inst) {
            if (llvm::DILocalVariable *varMD = dbg->getVariable()) {
              return varMD->getName().str();
            }
          }
        }
      }
    }
  }
  return std::nullopt;
}

// extracts source from reading from zero
std::optional<ref<SymbolicSource>>
tryRetrieveSourceFromFullArrayRead(ref<Expr> e) {
  ref<ReadExpr> base = e->hasOrderedReads(false);
  const bool isLSB = (!base.isNull());
  if (!isLSB)
    return std::nullopt;
  if (base.isNull())
    return std::nullopt;
  bool isPureRead = base->updates.head.isNull();
  if (base->index->isZero() && isPureRead) {
    return base->updates.root->source;
  } else {
    return std::nullopt;
  }
}

std::optional<std::string>
instSourceToString(ref<InstructionSource> instSource) {
  auto &instruction = instSource->allocSite;
  auto kf = instSource->km->functionMap.at(instruction.getFunction());
  auto ki = kf->instructionMap.at(&instruction);
  if (true) {
    if (true) {
      auto name = retrieveName(&instruction);
      if (name.has_value()) {
        return name.value();
      }
    }
    auto name = ki->inst()->getName().str();
    if (!name.empty()) {
      return "LlvmReg(" + name + ")";
    }
  }
  return std::string{"<var>"};
}

// sample translation -- from
// (ReadLSB w32 0 (array (w64 4) (lazyInitializationContent N0:(ReadLSB w64 0
// (array (w64 8) (lazyInitializationAddress N1:(ReadLSB w64 0 (array (w64 8)
// (instruction 2 %entry loop 0)))))))))
std::optional<std::string> printConcatAsC(const ref<Expr> &e) {
  auto readExpr = dyn_cast<ConcatExpr>(e);
  auto content = tryRetrieveSourceFromFullArrayRead(readExpr);
  if (!content.has_value())
    return std::nullopt;
  if (auto liContent =
          dyn_cast<LazyInitializationContentSource>(content.value())) {
    auto liContentSource =
        tryRetrieveSourceFromFullArrayRead(liContent->pointer);
    if (liContentSource.has_value()) {
      if (auto instSource =
              dyn_cast<InstructionSource>(liContentSource.value())) {
        return instSourceToString(instSource);
      }
      if (auto liAdress = dyn_cast<LazyInitializationAddressSource>(
              liContentSource.value())) {
        auto liSource = tryRetrieveSourceFromFullArrayRead(liAdress->pointer);
        if (liSource.has_value()) {
          if (auto instSource = dyn_cast<InstructionSource>(liSource.value())) {
            return instSourceToString(instSource);
          }
        }
      }
    }
  } else {
    auto source = tryRetrieveSourceFromFullArrayRead(readExpr);
    if (source.has_value()) {
      if (auto const instSource = dyn_cast<InstructionSource>(source.value())) {
        return instSourceToString(instSource);
      } else if (auto const argSource = dyn_cast<ArgumentSource>(source.value())) {
        return argSource->allocSite.getName().str();
      }
    }
  }
  return std::nullopt;
}

std::string braced(const std::string &s) { return std::string{'('} + s + ")"; }

std::string biexprKindToString(Expr::Kind kind) {
  std::string empty{};
  llvm::raw_string_ostream ss(empty);
  ss << '`';
  Expr::printKind(ss, kind);
  ss << '`';
  return ss.str();
}

std::optional<std::string> printBinopAsC(BinaryExpr *BE) {
  auto left = translateToCExpr(BE->left);
  if (!left.has_value()) {
    return std::nullopt;
  }
  auto right = translateToCExpr(BE->right);
  if (!right.has_value()) {
    return std::nullopt;
  }
  std::map<Expr::Kind, std::string> ops{
      {Expr::Add, "+"},  {Expr::Sub, "-"}, {Expr::Mul, "*"},  {Expr::URem, "%"},
      {Expr::And, "&"},  {Expr::Slt, "<"}, {Expr::Sle, "<="}, {Expr::Sgt, ">"},
      {Expr::Sge, ">="}, {Expr::Eq, "=="}};
  auto const opString = [&]() {
    if (auto it = ops.find(BE->getKind()); it != ops.end()) {
      return it->second;
    } else {
      return biexprKindToString(BE->getKind());
    }
  }();
  return braced(left.value() + " " + opString + " " + right.value());
}

std::optional<std::string> translateToCExpr(ref<Expr> expr) {
  if (!expr) {
    return std::nullopt;
  }
  if (auto *CE = dyn_cast<ConstantExpr>(expr)) {
    return printConstAsC(CE);
  }
  if (expr->getKind() == Expr::Concat) {
    return printConcatAsC(expr);
  }
  if (auto *BE = dyn_cast<BinaryExpr>(expr)) {
    return printBinopAsC(BE);
  }
  if (auto *NE = dyn_cast<NotExpr>(expr)) {
    auto subExpr = translateToCExpr(NE->expr);
    if (subExpr.has_value()) {
      if (NE->expr->getWidth() == Expr::Bool)
        return "!(" + subExpr.value() + ")";
      else
        return "~(" + subExpr.value() + ")";
    }
  }
  return std::nullopt;
}

std::string disjunctionToCExpr(disjunction const &disj,
                               bool unknownsExprsAsTrue) {
  if (disj.elements.empty()) {
    return "0";
  } else {
    std::string result;
    bool first_atom = true;
    for (auto &literal : disj.elements) {
      if (!first_atom) {
        result += " || ";
      }
      auto maybeCExpr = translateToCExpr(literal);
      if (maybeCExpr.has_value()) {
        result += maybeCExpr.value();
      } else {
        if (unknownsExprsAsTrue) {
          result += "1";
        } else {
          result += "unknown";
        }
      }
      first_atom = false;
    }
    return result;
  }
}

} // namespace klee