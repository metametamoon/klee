#ifndef KLEE_SYMBOLICSOURCE_H
#define KLEE_SYMBOLICSOURCE_H

#include "klee/ADT/Ref.h"
#include "klee/Expr/Assignment.h"
#include "klee/Expr/ExprUtil.h"
#include "klee/Expr/ExternalCall.h"

#include "llvm/IR/Function.h"

namespace klee {

class Expr;

class SymbolicSource {
public:
  /// @brief Required by klee::ref-managed objects
  class ReferenceCounter _refCount;

  enum class Kind {
    Constant,
    MakeSymbolic,
    LazyInitialized,
    ExternalCall
  };

public:
  virtual ~SymbolicSource() {}
  virtual Kind getKind() const = 0;
  virtual std::vector<const Array *> getRelated() const {
    return {};
  }
  static bool classof(const SymbolicSource *) { return true; }
};

class ConstantSource : public SymbolicSource {
public:
  Kind getKind() const { return Kind::Constant; }
  static bool classof(const SymbolicSource *S) {
    return S->getKind() == Kind::Constant;
  }
  static bool classof(const ConstantSource *) { return true; }
};

class MakeSymbolicSource : public SymbolicSource {
public:
  Kind getKind() const { return Kind::MakeSymbolic; }
  static bool classof(const SymbolicSource *S) {
    return S->getKind() == Kind::MakeSymbolic;
  }
  static bool classof(const MakeSymbolicSource *) { return true; }
};

class LazyInitializedSource : public SymbolicSource {
public:
  ref<Expr> address;

public:
  Kind getKind() const { return Kind::LazyInitialized; }
  static bool classof(const SymbolicSource *S) {
    return S->getKind() == Kind::LazyInitialized;
  }
  static bool classof(const LazyInitializedSource *) { return true; }
};

class ExternalCallSource : public SymbolicSource {
public:
  ref<ExternalCall> call;
  // Fuzzer should ignore constraints with this array
  bool symcrete;
public:
  Kind getKind() const { return Kind::ExternalCall; }
  static bool classof(const SymbolicSource *S) {
    return S->getKind() == Kind::ExternalCall;
  }
  static bool classof(const ExternalCallSource *) { return true; }

  std::vector<const Array *> getRelated() const {
    std::vector<const Array *> result;
    ObjectFinder of(result);
    for (auto arg : call->args) {
      of.visit(arg.expr);
      result.push_back(arg.symcrete);
      if (arg.pointer) {
        result.push_back(arg.mo->SymcretePre);
        auto read = ReadExpr::alloc(arg.mo->MOState, ConstantExpr::alloc(0, 64));
        of.visit(read);

        if (!arg.mo->readOnly) {
          result.push_back(arg.mo->postArray);
          result.push_back(arg.mo->SymcretePost);
        }

      }
    }
    if (call->retval.array) {
      result.push_back(call->retval.array);
      result.push_back(call->retval.symcrete);
    }
    return result;
  }
};

bool NoExternals(std::vector<const Array *> &arrays);

bool NoExternals(std::set<const Array *> &arrays);

bool ExternalsAreAssigned(std::vector<const Array *> &arrays,
                          const Assignment &assign);

}  // End klee namespace

#endif /* KLEE_SYMBOLICSOURCE_H */
