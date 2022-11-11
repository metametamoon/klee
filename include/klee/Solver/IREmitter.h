#ifndef KLEE_IREMITTER_H
#define KLEE_IREMITTER_H

#include "klee/Expr/Constraints.h"
#include "klee/Expr/Expr.h"
#include "klee/Expr/ExprHashMap.h"
#include "klee/Expr/ExternalCall.h"
#include "klee/Solver/Solver.h"
#include "llvm/ADT/APInt.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/DataLayout.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Value.h"
#include <cstdint>
#include <vector>

using namespace llvm;

namespace klee {

class IREmitter {
private:
  // Convenience
  struct TypeMaker {
    inline Type *Array(uint64_t bytes);
    inline Type *IN(uint64_t n);
    inline Type *I1();
    inline Type *I8();
    inline Type *I32();
    inline Type *I64();
    inline Type *INPtr(uint64_t n);
    inline Type *I1Ptr();
    inline Type *I8Ptr();
    inline Type *I32Ptr();
    inline Type *I64Ptr();

    inline std::pair<Type *, uint64_t> VT(Type *subtype, Expr::Width total);

    TypeMaker(LLVMContext &context) : context(context) {}

  private:
    LLVMContext &context;
  };

  struct IntMaker {
    inline ConstantInt *I8(uint64_t i);
    inline ConstantInt *I32(uint64_t i);
    inline ConstantInt *I64(uint64_t i);

    inline ConstantInt *AP(APInt i);

    IntMaker(LLVMContext &context) : context(context) {}

  private:
    LLVMContext &context;
  };

  struct Emittee {
    enum class Type { Expr, Call };

    Type type;
    ref<Expr> expr;
    ref<ExternalCall> call;

    explicit Emittee(ref<Expr> expr)
        : type(Type::Expr), expr(expr), call(nullptr) {}

    explicit Emittee(ref<ExternalCall> call)
        : type(Type::Call), expr(nullptr), call(call) {}
  };

  struct Emitted {
    BasicBlock *bb;
    Value *val;
    std::pair<BasicBlock *, BasicBlock *> callBBs;
    Emittee::Type type;

    Emitted(BasicBlock *bb, Value *val)
        : bb(bb), val(val), type(Emittee::Type::Expr) {}
    Emitted(std::pair<BasicBlock *, BasicBlock *> callBBs)
        : callBBs(callBBs), type(Emittee::Type::Call) {}

    bool isCall() {
      return type == Emittee::Type::Call;
    }
  };

  struct Description {
    /// Constant arrays
    std::vector<const Array *> constant;

    /// Arrays whose values are arbitrary and therefore must be picked by the
    /// fuzzer
    std::vector<const Array *> free;

    /// List of things to emit. It is sorted wrt the external calls.
    std::vector<Emittee> emittees;
  };

  /// LLVM objects needed for IR emission
  LLVMContext &context;
  std::unique_ptr<Module> module;
  DataLayout DL;
  IRBuilder<> B;

  TypeMaker T;
  IntMaker I;

  const std::vector<ref<Expr>> &constraints;
  const std::vector<ref<ExternalCall>> &calls;

  const std::vector<const Array *> &free;

  Description D;

  /// Allocas where the initial values of arrays are stored. This is used
  /// in order to:
  /// 1. Get concretizations of symcretes after fuzzing
  /// 2. Build array versions
  std::map<const Array *, AllocaInst *> valueStore;

  /// This map tracks array allocation addresses
  std::map<const Array *, AllocaInst *> arraymap;

  /// Allocas used to synchronize MO addresses in external calls
  std::map<const Array *, AllocaInst *> MOAllocas;
  std::vector<std::pair<const Array *, uint64_t>> addresses;

  /// This allows us to reuse previous calculations
  ExprHashMap<Value *> exprs;


  /// These are used by almost all emitter routines
  Function *f;
  AllocaInst *dataAlloca;
  AllocaInst *sizeAlloca;


  /// Emit a program that, upon reaching sat, will output values for
  /// the arrays in order of their appearance in the vector
  bool outputSymcretes;
  const std::vector<const Array *> *symcretes;

  /// Instrument the module to capture the coverage info via calls
  /// to the external __record__coverage function
  bool instrument;
  llvm::Function *instrumentation;

public:
  IREmitter(LLVMContext &context, const std::vector<ref<Expr>> &constraints,
            const std::vector<ref<ExternalCall>> &calls,
            const std::vector<const Array *> &free);

  IREmitter(LLVMContext &context, const std::vector<ref<Expr>> &constraints,
            const std::vector<ref<ExternalCall>> &calls,
            const std::vector<const Array *> &free,
            const std::vector<const Array *> *symcretes);

  std::unique_ptr<Module> emit();

private:
  Description sortAndGetSymbolics();

  BasicBlock *initFunction();
  BasicBlock *makeWrongSizeBB();
  BasicBlock *makeIndependent();
  std::pair<BasicBlock *, BasicBlock *> makeDestinations(unsigned exprCount);

  llvm:: Function* addInstrumentationDeclaration();

  /// Helpers
  AllocaInst *getVersionedArray(UpdateList);
  AllocaInst *getMOAlloca(const Array *, uint64_t address);

  void recordSymcrete(const Array *, AllocaInst *);
  void recordSymcrete(const Array *, Value *);

  std::vector<ref<Expr>> unwindConcat(ref<ConcatExpr>);
  void unwindConcat(ref<ConcatExpr>, std::vector<ref<Expr>> &);

  /// Emission routines

  std::pair<BasicBlock *, BasicBlock *> visitCall(ref<ExternalCall>);
  Value *visitExpr(ref<Expr>);

  Value *visitNotOptimized(ref<NotOptimizedExpr>);
  Value *visitRead(ref<ReadExpr>);
  Value *visitSelect(ref<SelectExpr>);
  Value *visitConcat(ref<ConcatExpr>);
  Value *visitExtract(ref<ExtractExpr>);
  Value *visitZExt(ref<ZExtExpr>);
  Value *visitSExt(ref<SExtExpr>);
  Value *visitAdd(ref<AddExpr>);
  Value *visitSub(ref<SubExpr>);
  Value *visitMul(ref<MulExpr>);
  Value *visitUDiv(ref<UDivExpr>);
  Value *visitSDiv(ref<SDivExpr>);
  Value *visitURem(ref<URemExpr>);
  Value *visitSRem(ref<SRemExpr>);
  Value *visitNot(ref<NotExpr>);
  Value *visitAnd(ref<AndExpr>);
  Value *visitOr(ref<OrExpr>);
  Value *visitXor(ref<XorExpr>);
  Value *visitShl(ref<ShlExpr>);
  Value *visitLShr(ref<LShrExpr>);
  Value *visitAShr(ref<AShrExpr>);
  Value *visitEq(ref<EqExpr>);
  Value *visitNe(ref<NeExpr>);
  Value *visitUlt(ref<UltExpr>);
  Value *visitUle(ref<UleExpr>);
  Value *visitUgt(ref<UgtExpr>);
  Value *visitUge(ref<UgeExpr>);
  Value *visitSlt(ref<SltExpr>);
  Value *visitSle(ref<SleExpr>);
  Value *visitSgt(ref<SgtExpr>);
  Value *visitSge(ref<SgeExpr>);
  Value *visitConstant(ref<ConstantExpr>);
};

} // namespace klee

#endif /* KLEE_IREMITTER_H */
