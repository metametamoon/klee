#include "klee/Expr/SourceBuilder.h"
#include "klee/Expr/ExternalCall.h"
#include "klee/Expr/SymbolicSource.h"
#include "klee/Expr/Expr.h"

using namespace klee;

SourceBuilder::SourceBuilder() {
  constantSource = ref<SymbolicSource>(new ConstantSource());
  makeSymbolicSource = ref<SymbolicSource>(new MakeSymbolicSource());
}

SymbolicSource *SourceBuilder::constant() {
  return constantSource.get();
}

SymbolicSource *SourceBuilder::makeSymbolic() {
  return makeSymbolicSource.get();
}

SymbolicSource *SourceBuilder::lazyInitialized(ref<Expr> _source) {
  if (!lazyInitializations.count(_source)) {
    auto source = new LazyInitializedSource();
    source->address = _source;
    lazyInitializations[_source] = source;
  }
  return lazyInitializations[_source].get();
}


SymbolicSource *SourceBuilder::externalCall(ref<ExternalCall> _source, bool symcrete) {
  auto pair = std::make_pair(_source, symcrete);
  if (!externalCalls.count(pair)) {
    auto source = new ExternalCallSource();
    source->call = _source;
    source->symcrete = symcrete;
    externalCalls[pair] = source;
  }
  return externalCalls[pair].get();
}
