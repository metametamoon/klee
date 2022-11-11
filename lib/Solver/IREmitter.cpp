#include "klee/Solver/IREmitter.h"

#include "klee/ADT/Ref.h"
#include "klee/Expr/Constraints.h"
#include "klee/Expr/Expr.h"
#include "klee/Expr/ExprHashMap.h"
#include "klee/Expr/ExprUtil.h"
#include "klee/Expr/SymbolicSource.h"

#include "llvm/ADT/APInt.h"
#include "llvm/ADT/StringExtras.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/Constant.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/DataLayout.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Type.h"
#include "llvm/IR/Value.h"
#include "llvm/Support/Casting.h"

#include <algorithm>
#include <cassert>
#include <cstdint>
#include <memory>
#include <unordered_set>
#include <vector>

using namespace llvm;

namespace klee {

Type *IREmitter::TypeMaker::Array(uint64_t bytes) {
  return ArrayType::get(Type::getInt8Ty(context), bytes);
}

Type *IREmitter::TypeMaker::IN(uint64_t n) {
  return Type::getIntNTy(context, n);
}

Type *IREmitter::TypeMaker::I1() { return Type::getInt1Ty(context); }

Type *IREmitter::TypeMaker::I8() { return Type::getInt8Ty(context); }

Type *IREmitter::TypeMaker::I32() { return Type::getInt32Ty(context); }

Type *IREmitter::TypeMaker::I64() { return Type::getInt64Ty(context); }

Type *IREmitter::TypeMaker::INPtr(uint64_t n) {
  return Type::getIntNPtrTy(context, n);
}

Type *IREmitter::TypeMaker::I1Ptr() { return Type::getInt1PtrTy(context); }

Type *IREmitter::TypeMaker::I8Ptr() { return Type::getInt8PtrTy(context); }

Type *IREmitter::TypeMaker::I32Ptr() { return Type::getInt32PtrTy(context); }

Type *IREmitter::TypeMaker::I64Ptr() { return Type::getInt64PtrTy(context); }

std::pair<Type *, uint64_t> IREmitter::TypeMaker::VT(Type *subtype,
                                                     Expr::Width total) {
  auto width = subtype->getIntegerBitWidth();
  assert(total % width == 0);
  auto ec = total / width;
  return std::make_pair(FixedVectorType::get(subtype, ec), ec);
}

ConstantInt *IREmitter::IntMaker::I8(uint64_t i) {
  return ConstantInt::get(context, APInt(8, i, false));
}
ConstantInt *IREmitter::IntMaker::I32(uint64_t i) {
  return ConstantInt::get(context, APInt(32, i, false));
}
ConstantInt *IREmitter::IntMaker::I64(uint64_t i) {
  return ConstantInt::get(context, APInt(64, i, false));
}

ConstantInt *IREmitter::IntMaker::AP(APInt i) {
  return ConstantInt::get(context, i);
}

IREmitter::IREmitter(LLVMContext &context, const std::vector<ref<Expr>> &constraints,
                     const std::vector<ref<ExternalCall>> &calls,
                     const std::vector<const Array *> &free)
    : context(context), module(std::make_unique<Module>("IR", context)),
      DL(module.get()), B(context), T(context), I(context),
      constraints(constraints), calls(calls), free(free), outputSymcretes(false),
      symcretes(nullptr), instrument(true) {}

IREmitter::IREmitter(LLVMContext &context, const std::vector<ref<Expr>> &constraints,
                     const std::vector<ref<ExternalCall>> &calls,
                     const std::vector<const Array *> &free,
                     const std::vector<const Array *> *symcretes)
    : context(context), module(std::make_unique<Module>("IR", context)),
      DL(module.get()), B(context), T(context), I(context),
      constraints(constraints), calls(calls), free(free), outputSymcretes(true),
      symcretes(symcretes), instrument(false) {}

BasicBlock *IREmitter::initFunction() {
  std::vector<Type *> args(2);

  args[0] = T.I8Ptr();
  args[1] = T.I64();

  FunctionType *ft;
  if (outputSymcretes) {
    ft = FunctionType::get(T.I8Ptr(), args, false);
  } else {
    ft = FunctionType::get(T.I32(), args, false);
  }

  f = Function::Create(ft, Function::ExternalLinkage, "LLVMFuzzerTestOneInput",
                       module.get());

  unsigned argIndex = 0;
  for (auto &arg : f->args()) {
    if (argIndex == 0) {
      arg.setName("data");
      argIndex++;
    } else {
      arg.setName("size");
    }
  }

  auto bb = BasicBlock::Create(context, "entry", f);
  B.SetInsertPoint(bb);
  dataAlloca = B.CreateAlloca(T.I8Ptr());
  sizeAlloca = B.CreateAlloca(T.I64());

  argIndex = 0;
  for (auto &arg : f->args()) {
    if (argIndex == 0) {
      B.CreateStore(&arg, dataAlloca);
      argIndex++;
    } else {
      B.CreateStore(&arg, sizeAlloca);
    }
  }

  return bb;
}

BasicBlock *IREmitter::makeWrongSizeBB() {
  auto bb = BasicBlock::Create(context, "wrong_size", f);
  B.SetInsertPoint(bb);
  if (outputSymcretes) {
    B.CreateRet(Constant::getNullValue(T.I8Ptr()));
  } else {
    B.CreateRet(I.I32(0));
  }
  return bb;
}

BasicBlock *IREmitter::makeIndependent() {
  auto bb = BasicBlock::Create(context, "free_and_const", f);
  B.SetInsertPoint(bb);
  auto data = B.CreateLoad(T.I8Ptr(), dataAlloca);
  uint64_t iter = 0;

  for (auto array : D.constant) {
    auto alloca = B.CreateAlloca(T.Array(array->size));
    for (unsigned i = 0; i < array->constantValues.size(); i++) {
      auto val = array->constantValues[i];
      auto gep = B.CreateGEP(alloca, {I.I64(0), I.I64(i)});
      B.CreateStore(I.AP(val->getAPValue()), gep);
    }
    valueStore[array] = alloca;
  }

  for (auto array : D.free) {
    auto alloca = B.CreateAlloca(T.Array(array->size));
    for (unsigned i = 0; i < array->size; i++) {
      auto gep = B.CreateGEP(alloca, {I.I64(0), I.I64(i)});
      auto gepData = B.CreateGEP(data, I.I64(iter));
      auto load = B.CreateLoad(T.I8(), gepData);
      B.CreateStore(load, gep);
      iter++;
    }
    valueStore[array] = alloca;
  }

  return bb;
}

std::pair<BasicBlock *, BasicBlock *>
IREmitter::makeDestinations(unsigned exprCount) {
  auto sat = BasicBlock::Create(context, "sat", f);
  auto unsat = BasicBlock::Create(context, "unsat", f);

  B.SetInsertPoint(sat);

  if (instrument) {
    B.CreateCall(instrumentation, {I.I64(exprCount)});
  }

  if (outputSymcretes) {
    uint64_t size = 0;
    for (auto array : *symcretes) {
      size += array->size;
    }

    auto mallocType = FunctionType::get(T.I8Ptr(), {T.I64()}, false);
    auto malloc = Function::Create(mallocType, Function::ExternalLinkage,
                                   "malloc", *module);
    // Heap allocation
    auto retArray = B.CreateCall(malloc, {I.I64(size)});

    uint64_t index = 0;
    for (auto array : *symcretes) {
      assert(valueStore.count(array));
      auto alloca = valueStore[array];
      for (unsigned i = 0; i < array->size; i++) {
        auto allocaGEP = B.CreateGEP(alloca, {I.I64(0), I.I64(i)});
        auto load = B.CreateLoad(allocaGEP);
        auto gep = B.CreateGEP(retArray, I.I64(index));
        B.CreateStore(load, gep);
        index++;
      }
    }
    B.CreateRet(retArray);
  } else {
    B.CreateRet(I.I32(1));
  }

  B.SetInsertPoint(unsat);
  if (outputSymcretes) {
    B.CreateRet(Constant::getNullValue(T.I8Ptr()));
  } else {
    B.CreateRet(I.I32(0));
  }

  return {sat, unsat};
}

AllocaInst *IREmitter::getVersionedArray(UpdateList ul) {
  auto arrayValue = valueStore[ul.root];
  auto alloca = B.CreateAlloca(T.Array(ul.root->size));
  auto load = B.CreateLoad(T.Array(ul.root->size), arrayValue);
  B.CreateStore(load, alloca);
  std::vector<ref<UpdateNode>> versions;
  auto current = ul.head;
  while (current) {
    versions.push_back(current);
    current = current->next;
  }

  for (auto i = versions.rbegin(); i != versions.rend(); i++) {
    Value *index = visitExpr((*i)->index);
    Value *value = visitExpr((*i)->value);
    auto gep = B.CreateGEP(alloca, {I.I64(0), index});
    B.CreateStore(value, gep);
  }
  return alloca;
}

AllocaInst *IREmitter::getMOAlloca(const Array *arr, uint64_t address) {
  if (MOAllocas.count(arr)) {
    return MOAllocas.at(arr);
  } else {
    auto alloca = B.CreateAlloca(T.Array(arr->size));
    MOAllocas[arr] = alloca;
    addresses.push_back({arr, address});
    return alloca;
  }
}

void IREmitter::recordSymcrete(const Array *symcrete, AllocaInst *alloca) {
  auto recordAlloca = B.CreateAlloca(T.Array(symcrete->size));
  valueStore[symcrete] = recordAlloca;
  auto load = B.CreateLoad(T.Array(symcrete->size), alloca);
  B.CreateStore(load, recordAlloca);
}

void IREmitter::recordSymcrete(const Array *symcrete, Value *value) {
  auto recordAlloca = B.CreateAlloca(T.Array(symcrete->size));
  valueStore[symcrete] = recordAlloca;
  auto bitcast =
      B.CreateBitCast(recordAlloca, PointerType::get(value->getType(), 0));
  B.CreateStore(value, bitcast);
}

std::vector<ref<Expr>> IREmitter::unwindConcat(ref<ConcatExpr> e) {
  std::vector<ref<Expr>> unwound;
  unwindConcat(e, unwound);
  return unwound;
}

void IREmitter::unwindConcat(ref<ConcatExpr> e,
                             std::vector<ref<Expr>> &unwound) {
  if (auto l = dyn_cast<ConcatExpr>(e->getLeft())) {
    unwindConcat(l, unwound);
  } else {
    unwound.push_back(e->getLeft());
  }
  if (auto r = dyn_cast<ConcatExpr>(e->getRight())) {
    unwindConcat(r, unwound);
  } else {
    unwound.push_back(e->getRight());
  }
}

llvm::Function *IREmitter::addInstrumentationDeclaration() {
  return Function::Create(
      FunctionType::get(Type::getVoidTy(context), {T.I64()}, false),
      Function::ExternalLinkage, "__record_coverage", module.get());
}

IREmitter::Description IREmitter::sortAndGetSymbolics() {
  Description D;
  std::map<const Array *, unsigned> sortMap;
  for (unsigned i = 0; i < calls.size(); i++) {
    for (auto arg : calls[i]->args) {
      sortMap[arg.symcrete] = i + 1;
      if (arg.pointer) {
        sortMap[arg.mo->postArray] = i + 1;
      }
    }
    if (calls[i]->retval.array) {
      sortMap[calls[i]->retval.array] = i + 1;
    }
  }

  std::vector<std::vector<ref<Expr>>> sorted(calls.size() + 1);
  std::vector<const Array *> arrays;
  ObjectFinder of(arrays);

  for (auto i : constraints) {
    unsigned maxArray = 0;
    std::vector<const Array *> arrays;
    findObjects(i, arrays);
    for (auto j : arrays) {
      if (sortMap.count(j)) {
        maxArray = std::max(maxArray, sortMap[j]);
      }
    }
    sorted[maxArray].push_back(i);
  }

  for (auto i : constraints) {
    of.visit(i);
  }

  for (auto call : calls) {
    for (auto arg : call->args) {
      of.visit(arg.expr);
      if (arg.pointer) {
        auto read = ReadExpr::alloc(arg.mo->MOState, ConstantExpr::alloc(0, 64)); // Hack
        of.visit(read);
      }
    }
  }

  for (auto array : arrays) {
    if (!sortMap.count(array)) {
      if (array->isConstantArray()) {
        D.constant.push_back(array);
      } else {
        assert(isa<MakeSymbolicSource>(array->source));
        assert(std::find(free.begin(), free.end(), array) != free.end());
      }
    }
  }

  for (unsigned i = 0; i < sorted.size(); i++) {
    if (i != 0) {
      D.emittees.push_back(Emittee(calls[i - 1]));
    }
    for (auto j : sorted[i]) {
      D.emittees.push_back(Emittee(j));
    }
  }

  D.free = free;
  return D;
}

std::pair<BasicBlock *, BasicBlock *>
IREmitter::visitCall(ref<ExternalCall> call) {
  std::string callNum = llvm::utostr(call->num);
  auto callBB = BasicBlock::Create(context, "call_" + callNum, f);
  B.SetInsertPoint(callBB);

  std::set<ref<ExternalCall::MO>> MOs;

  std::vector<Value *> arguments;
  for (unsigned i = 0; i < call->args.size(); i++) {
    auto arg = call->args[i];
    auto argValue = visitExpr(arg.expr);
    recordSymcrete(arg.symcrete, argValue);
    if (arg.pointer) {
      auto alloca = getMOAlloca(arg.mo->MOState.root, arg.mo->address);
      if (!MOs.count(arg.mo)) {
        auto version = getVersionedArray(arg.mo->MOState);
        auto load = B.CreateLoad(version);
        B.CreateStore(load, alloca);
        recordSymcrete(arg.mo->SymcretePre, alloca);
        MOs.insert(arg.mo);
      }
      auto offset = B.CreateSub(argValue, I.I64(arg.mo->address));
      auto address = B.CreateGEP(alloca, {I.I64(0), offset});
      Value *bitcast;
      if (i < call->f->arg_size()) {
        bitcast = B.CreateBitCast(address, call->f->getArg(i)->getType());
      } else {
        // what to do with vararg functions?
        bitcast = B.CreateBitCast(address, T.INPtr(64));
      }
      argValue = bitcast;
    }
    arguments.push_back(argValue);
  }

  auto ft = call->f->getFunctionType();
  auto name = call->f->getName();
  auto function = module->getOrInsertFunction(name, ft);
  auto callInst = B.CreateCall(function, arguments);

  std::set<ref<ExternalCall::MO>> MOsPost;
  for (unsigned i = 0; i < call->args.size(); i++) {
    auto arg = call->args[i];
    if (arg.pointer && !arg.mo->readOnly && !MOsPost.count(arg.mo)) {
      auto alloca = getMOAlloca(arg.mo->MOState.root, arg.mo->address);
      recordSymcrete(arg.mo->SymcretePost, alloca);
      recordSymcrete(arg.mo->postArray, alloca); // ?
      MOAllocas.insert({arg.mo->postArray, alloca});
      MOsPost.insert(arg.mo);
    }
  }

  if (call->retval.array) {
    auto rawRetvalBB = BasicBlock::Create(context, "raw_retval_" + callNum, f);
    auto retvalBB = BasicBlock::Create(context, "retval_" + callNum, f);

    B.CreateBr(rawRetvalBB);
    B.SetInsertPoint(rawRetvalBB);
    auto bitWidth = DL.getTypeSizeInBits(call->f->getReturnType());
    auto allocationSize = (bitWidth % 8 == 0 ? bitWidth / 8 : bitWidth / 8 + 1);
    assert(allocationSize == call->retval.array->size);
    auto alloca = B.CreateAlloca(T.Array(allocationSize));
    auto sizedPtr =
        B.CreateBitCast(alloca, PointerType::get(call->f->getReturnType(), 0));
    B.CreateStore(callInst, sizedPtr);

    if (callInst->getType()->isPointerTy()) {
      auto translationAllocBB =
          BasicBlock::Create(context, "translation_alloc_" + callNum, f);
      B.CreateBr(translationAllocBB);
      B.SetInsertPoint(translationAllocBB);
      auto translationAlloc = B.CreateAlloca(T.I64());
      auto address = B.CreatePtrToInt(callInst, T.I64());

      std::vector<std::pair<BasicBlock *, BasicBlock *>> checkBB;
      for (auto MO : MOs) {
        checkBB.push_back({BasicBlock::Create(context, "", f),
                           BasicBlock::Create(context, "", f)});
      }

      assert(!MOs.empty());

      auto translationBB = BasicBlock::Create(context, "translation_write_" + callNum, f);

      unsigned MOIndex = 0;
      for (auto MO : MOs) {
        auto bb = checkBB[MOIndex];
        if (MOIndex == 0) {
          B.CreateBr(bb.first);
        }
        auto falseBB =
            (MOIndex < MOs.size() - 1 ? checkBB[MOIndex + 1].first : retvalBB);
        B.SetInsertPoint(bb.first);
        auto MOAlloca = getMOAlloca(MO->MOState.root, MO->address);
        auto MOAddress = B.CreatePtrToInt(MOAlloca, T.I64());
        auto offset = B.CreateSub(address, MOAddress);
        B.CreateStore(B.CreateAdd(I.I64(MO->address), offset),
                      translationAlloc);
        auto lower = B.CreateICmpULE(MOAddress, address);
        B.CreateCondBr(lower, bb.second, falseBB);
        B.SetInsertPoint(bb.second);
        auto MOEnd = B.CreateAdd(MOAddress, I.I64(MO->MOState.root->size));
        auto upper = B.CreateICmpULT(address, MOEnd);
        B.CreateCondBr(upper, translationBB, falseBB);
        MOIndex++;
      }

      B.SetInsertPoint(translationBB);
      auto load = B.CreateLoad(translationAlloc);
      auto bitcast = B.CreateBitCast(alloca, T.I64Ptr());
      B.CreateStore(load, bitcast);
    }

    B.CreateBr(retvalBB);
    B.SetInsertPoint(retvalBB);

    valueStore[call->retval.array] = alloca;
    recordSymcrete(call->retval.symcrete, alloca);

    return {callBB, retvalBB};
  }

  return {callBB, callBB};
}

Value *IREmitter::visitExpr(ref<Expr> e) {
  if (!exprs.count(e)) {
    Value *v = nullptr;
    switch (e->getKind()) {
    case Expr::NotOptimized:
      v = visitNotOptimized(dyn_cast<NotOptimizedExpr>(e));
      break;
    case Expr::Read:
      v = visitRead(dyn_cast<ReadExpr>(e));
      break;
    case Expr::Select:
      v = visitSelect(dyn_cast<SelectExpr>(e));
      break;
    case Expr::Concat:
      v = visitConcat(dyn_cast<ConcatExpr>(e));
      break;
    case Expr::Extract:
      v = visitExtract(dyn_cast<ExtractExpr>(e));
      break;
    case Expr::ZExt:
      v = visitZExt(dyn_cast<ZExtExpr>(e));
      break;
    case Expr::SExt:
      v = visitSExt(dyn_cast<SExtExpr>(e));
      break;
    case Expr::Add:
      v = visitAdd(dyn_cast<AddExpr>(e));
      break;
    case Expr::Sub:
      v = visitSub(dyn_cast<SubExpr>(e));
      break;
    case Expr::Mul:
      v = visitMul(dyn_cast<MulExpr>(e));
      break;
    case Expr::UDiv:
      v = visitUDiv(dyn_cast<UDivExpr>(e));
      break;
    case Expr::SDiv:
      v = visitSDiv(dyn_cast<SDivExpr>(e));
      break;
    case Expr::URem:
      v = visitURem(dyn_cast<URemExpr>(e));
      break;
    case Expr::SRem:
      v = visitSRem(dyn_cast<SRemExpr>(e));
      break;
    case Expr::Not:
      v = visitNot(dyn_cast<NotExpr>(e));
      break;
    case Expr::And:
      v = visitAnd(dyn_cast<AndExpr>(e));
      break;
    case Expr::Or:
      v = visitOr(dyn_cast<OrExpr>(e));
      break;
    case Expr::Xor:
      v = visitXor(dyn_cast<XorExpr>(e));
      break;
    case Expr::Shl:
      v = visitShl(dyn_cast<ShlExpr>(e));
      break;
    case Expr::LShr:
      v = visitLShr(dyn_cast<LShrExpr>(e));
      break;
    case Expr::AShr:
      v = visitAShr(dyn_cast<AShrExpr>(e));
      break;
    case Expr::Eq:
      v = visitEq(dyn_cast<EqExpr>(e));
      break;
    case Expr::Ne:
      v = visitNe(dyn_cast<NeExpr>(e));
      break;
    case Expr::Ult:
      v = visitUlt(dyn_cast<UltExpr>(e));
      break;
    case Expr::Ule:
      v = visitUle(dyn_cast<UleExpr>(e));
      break;
    case Expr::Ugt:
      v = visitUgt(dyn_cast<UgtExpr>(e));
      break;
    case Expr::Uge:
      v = visitUge(dyn_cast<UgeExpr>(e));
      break;
    case Expr::Slt:
      v = visitSlt(dyn_cast<SltExpr>(e));
      break;
    case Expr::Sle:
      v = visitSle(dyn_cast<SleExpr>(e));
      break;
    case Expr::Sgt:
      v = visitSgt(dyn_cast<SgtExpr>(e));
      break;
    case Expr::Sge:
      v = visitSge(dyn_cast<SgeExpr>(e));
      break;
    case Expr::Constant:
      v = visitConstant(dyn_cast<ConstantExpr>(e));
      break;
    default:
      assert(0 && "invalid expression kind");
    }
    exprs[e] = v;
  }
  return exprs[e];
}

Value *IREmitter::visitNotOptimized(ref<NotOptimizedExpr> e) {
  return visitExpr(e->src);
}

Value *IREmitter::visitRead(ref<ReadExpr> e) {
  auto ul = e->updates;
  auto alloca = getVersionedArray(ul);

  Value *readIndex = visitExpr(e->index);
  auto gep = B.CreateGEP(alloca, {I.I64(0), readIndex});
  return B.CreateLoad(gep);
}

Value *IREmitter::visitSelect(ref<SelectExpr> e) {
  Value *cond = visitExpr(e->cond);
  Value *trueExpr = visitExpr(e->trueExpr);
  Value *falseExpr = visitExpr(e->falseExpr);
  return B.CreateSelect(cond, trueExpr, falseExpr);
}

Value *IREmitter::visitConcat(ref<ConcatExpr> e) {
  auto unwound = unwindConcat(e);
  std::vector<std::pair<Value *, Expr::Width>> values;
  values.reserve(unwound.size());
  bool allWidthAreByteSized = true;
  uint64_t width = 0;
  for (auto i = unwound.rbegin(); i != unwound.rend(); i++) {
    auto expr = *i;
    auto value = visitExpr(expr);
    auto exprWidth = expr->getWidth();
    assert(value->getType()->getIntegerBitWidth() == exprWidth);
    values.push_back(std::make_pair(value, exprWidth));
    width += exprWidth;
    if (exprWidth % 8 != 0) {
      allWidthAreByteSized = false;
    }
  }

  auto subtype = (allWidthAreByteSized ? T.I8() : T.I1());

  auto concatAlloca = B.CreateAlloca(T.IN(width));
  auto concat = B.CreateLoad(concatAlloca);
  auto vectorConcat = B.CreateBitCast(concat, T.VT(subtype, width).first);

  uint64_t shift = 0;
  for (auto i : values) {
    auto vectorType = T.VT(subtype, i.second);
    auto vectorVal = B.CreateBitCast(i.first, vectorType.first);
    for (uint64_t valShift = 0; valShift < vectorType.second; valShift++) {
      auto extract = B.CreateExtractElement(vectorVal, valShift);
      vectorConcat = B.CreateInsertElement(vectorConcat, extract, shift);
      shift++;
    }
  }
  vectorConcat = B.CreateBitCast(vectorConcat, T.IN(width));
  return vectorConcat;
}

Value *IREmitter::visitExtract(ref<ExtractExpr> e) {
  auto expr = visitExpr(e->expr);
  expr = B.CreateLShr(expr, e->offset);
  auto alloca = B.CreateAlloca(expr->getType());
  B.CreateStore(expr, alloca);
  auto bitcast = B.CreateBitCast(alloca, T.INPtr(e->width));
  return B.CreateLoad(T.IN(e->width), bitcast);
}

Value *IREmitter::visitZExt(ref<ZExtExpr> e) {
  Value *src = visitExpr(e->src);
  return B.CreateZExt(src, Type::getIntNTy(context, e->width));
}

Value *IREmitter::visitSExt(ref<SExtExpr> e) {
  Value *src = visitExpr(e->src);
  return B.CreateSExt(src, Type::getIntNTy(context, e->width));
}

Value *IREmitter::visitAdd(ref<AddExpr> e) {
  Value *left = visitExpr(e->left);
  Value *right = visitExpr(e->right);
  return B.CreateAdd(left, right);
}

Value *IREmitter::visitSub(ref<SubExpr> e) {
  Value *left = visitExpr(e->left);
  Value *right = visitExpr(e->right);
  return B.CreateSub(left, right);
}

Value *IREmitter::visitMul(ref<MulExpr> e) {
  Value *left = visitExpr(e->left);
  Value *right = visitExpr(e->right);
  return B.CreateMul(left, right);
}

Value *IREmitter::visitUDiv(ref<UDivExpr> e) {
  Value *left = visitExpr(e->left);
  Value *right = visitExpr(e->right);
  return B.CreateUDiv(left, right);
}

Value *IREmitter::visitSDiv(ref<SDivExpr> e) {
  Value *left = visitExpr(e->left);
  Value *right = visitExpr(e->right);
  return B.CreateSDiv(left, right);
}

Value *IREmitter::visitURem(ref<URemExpr> e) {
  Value *left = visitExpr(e->left);
  Value *right = visitExpr(e->right);
  return B.CreateURem(left, right);
}

Value *IREmitter::visitSRem(ref<SRemExpr> e) {
  Value *left = visitExpr(e->left);
  Value *right = visitExpr(e->right);
  return B.CreateSRem(left, right);
}

Value *IREmitter::visitNot(ref<NotExpr> e) {
  Value *expr = visitExpr(e->expr);
  return B.CreateNot(expr);
}

Value *IREmitter::visitAnd(ref<AndExpr> e) {
  Value *left = visitExpr(e->left);
  Value *right = visitExpr(e->right);
  return B.CreateAnd(left, right);
}

Value *IREmitter::visitOr(ref<OrExpr> e) {
  Value *left = visitExpr(e->left);
  Value *right = visitExpr(e->right);
  return B.CreateOr(left, right);
}

Value *IREmitter::visitXor(ref<XorExpr> e) {
  Value *left = visitExpr(e->left);
  Value *right = visitExpr(e->right);
  return B.CreateXor(left, right);
}

Value *IREmitter::visitShl(ref<ShlExpr> e) {
  Value *left = visitExpr(e->left);
  Value *right = visitExpr(e->right);
  return B.CreateShl(left, right);
}

Value *IREmitter::visitLShr(ref<LShrExpr> e) {
  Value *left = visitExpr(e->left);
  Value *right = visitExpr(e->right);
  return B.CreateLShr(left, right);
}

Value *IREmitter::visitAShr(ref<AShrExpr> e) {
  Value *left = visitExpr(e->left);
  Value *right = visitExpr(e->right);
  return B.CreateAShr(left, right);
}

Value *IREmitter::visitEq(ref<EqExpr> e) {
  Value *left = visitExpr(e->left);
  Value *right = visitExpr(e->right);
  return B.CreateICmpEQ(left, right);
}

Value *IREmitter::visitNe(ref<NeExpr> e) {
  Value *left = visitExpr(e->left);
  Value *right = visitExpr(e->right);
  return B.CreateICmpNE(left, right);
}

Value *IREmitter::visitUlt(ref<UltExpr> e) {
  Value *left = visitExpr(e->left);
  Value *right = visitExpr(e->right);
  return B.CreateICmpULT(left, right);
}

Value *IREmitter::visitUle(ref<UleExpr> e) {
  Value *left = visitExpr(e->left);
  Value *right = visitExpr(e->right);
  return B.CreateICmpULE(left, right);
}

Value *IREmitter::visitUgt(ref<UgtExpr> e) {
  Value *left = visitExpr(e->left);
  Value *right = visitExpr(e->right);
  return B.CreateICmpUGT(left, right);
}

Value *IREmitter::visitUge(ref<UgeExpr> e) {
  Value *left = visitExpr(e->left);
  Value *right = visitExpr(e->right);
  return B.CreateICmpUGE(left, right);
}

Value *IREmitter::visitSlt(ref<SltExpr> e) {
  Value *left = visitExpr(e->left);
  Value *right = visitExpr(e->right);
  return B.CreateICmpSLT(left, right);
}

Value *IREmitter::visitSle(ref<SleExpr> e) {
  Value *left = visitExpr(e->left);
  Value *right = visitExpr(e->right);
  return B.CreateICmpSLE(left, right);
}

Value *IREmitter::visitSgt(ref<SgtExpr> e) {
  Value *left = visitExpr(e->left);
  Value *right = visitExpr(e->right);
  return B.CreateICmpSGT(left, right);
}

Value *IREmitter::visitSge(ref<SgeExpr> e) {
  Value *left = visitExpr(e->left);
  Value *right = visitExpr(e->right);
  return B.CreateICmpSGE(left, right);
}

Value *IREmitter::visitConstant(ref<ConstantExpr> e) {
  return I.AP(e->getAPValue());
}

std::unique_ptr<Module> IREmitter::emit() {
  D = sortAndGetSymbolics();

  if (instrument) {
    instrumentation = addInstrumentationDeclaration();
  }

  auto initBB = initFunction();
  auto wrongSizeBB = makeWrongSizeBB();
  auto makeIndependentBB = makeIndependent();

  B.SetInsertPoint(initBB);
  unsigned neededSize = 0;
  for (auto array : D.free) {
    neededSize += array->size;
  }
  auto size = B.CreateLoad(T.I64(), sizeAlloca);
  auto cond = B.CreateICmpULE(I.I64(neededSize), size);
  B.CreateCondBr(cond, makeIndependentBB, wrongSizeBB);

  std::vector<Emitted> emitted;
  emitted.reserve((D.emittees.size()));
  auto expr_index = 0;
  for (auto i : D.emittees) {
    if (i.type == Emittee::Type::Call) {
      auto callBBs = visitCall(i.call);
      emitted.push_back(Emitted(callBBs));
    } else {
      auto bb = BasicBlock::Create(context, "", f);
      B.SetInsertPoint(bb);
      if (instrument) {
        B.CreateCall(instrumentation, {I.I64(expr_index)});
      }
      auto val = visitExpr(i.expr);
      emitted.push_back(Emitted(bb, val));
      expr_index++;
    }
  }

  auto dests = makeDestinations(expr_index);
  auto sat = dests.first;
  auto unsat = dests.second;

  for (unsigned i = 0; i < emitted.size(); i++) {
    if (emitted[i].isCall()) {
      B.SetInsertPoint(emitted[i].callBBs.second);
    } else {
      B.SetInsertPoint(emitted[i].bb);
    }
    if (i == emitted.size() - 1) {
      if (emitted[i].isCall()) {
        B.CreateBr(sat);
      } else {
        B.CreateCondBr(emitted[i].val, sat, unsat);
      }
    } else {
      auto next = (emitted[i + 1].isCall() ? emitted[i + 1].callBBs.first
                                           : emitted[i + 1].bb);
      if (emitted[i].isCall()) {
        B.CreateBr(next);
      } else {
        B.CreateCondBr(emitted[i].val, next, unsat);
      }
    }
  }

  B.SetInsertPoint(makeIndependentBB);
  if (emitted[0].isCall()) {
    B.CreateBr(emitted[0].callBBs.first);
  } else {
    B.CreateBr(emitted[0].bb);
  }

  return std::move(module);
}

} // namespace klee
