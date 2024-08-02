#include "KTestBuilder.h"
#include "ConstantAddressSpace.h"

using namespace klee;

static std::unordered_map<const MemoryObject *, std::size_t>
enumerateObjects(const ExecutionState &state,
                 const ConstantPointerGraph &pointerGraph) {
  auto constantNumCounter = state.symbolics.size();

  std::unordered_map<const MemoryObject *, std::size_t> enumeration;
  enumeration.reserve(pointerGraph.size());

  for (const auto &[objectPair, resolution] : pointerGraph) {
    if (state.inSymbolics(*objectPair.first)) {
      auto &wrappingSymbolic = state.symbolics.at(objectPair.first);
      enumeration[objectPair.first] = wrappingSymbolic.num;
    } else {
      enumeration[objectPair.first] = constantNumCounter;
      ++constantNumCounter;
    }
  }

  return enumeration;
}

////////////////////////////////////////////////////////////////////////

KTestBuilder::KTestBuilder(const ExecutionState &state, const Assignment &model)
    : state_(state), model_(model),
      constantAddressSpace_(state.addressSpace, model),
      constantPointerGraph_(constantAddressSpace_.createPointerGraph()) {
  initialize();
}

void KTestBuilder::initialize() {
  for (auto &[object, symbolic] : state_.symbolics) {
    constantPointerGraph_.addSource(
        ObjectPair{symbolic.memoryObject.get(), symbolic.objectState.get()});
  }

  order_ = enumerateObjects(state_, constantPointerGraph_);

  ktest_.numObjects = order_.size();
  ktest_.objects = new KTestObject[ktest_.numObjects]();
}

KTestBuilder &KTestBuilder::constructPointerGraph() {
  for (const auto &[objectPair, resolution] : constantPointerGraph_) {
    // Select KTestObject for the object
    const auto objectNum = order_.at(objectPair.first);
    auto &ktestObject = ktest_.objects[objectNum];

    ktestObject.address = constantAddressSpace_.addressOf(*objectPair.first);
    ktestObject.name = const_cast<char *>(objectPair.first->name.c_str());

    // Allocate memory for pointers
    if (resolution.empty()) {
      continue;
    }
    ktestObject.content.numPointers = resolution.size();
    ktestObject.content.pointers = new Pointer[resolution.size()];

    std::size_t currentPointerIdx = 0;
    // Populate pointers
    for (const auto &[offset, singleResolution] : resolution) {
      auto [writtenAddress, resolvedObjectPair] = singleResolution;

      auto referencesToObjectNum = order_.at(resolvedObjectPair.first);
      auto addressOfReferencedObject =
          constantAddressSpace_.addressOf(*resolvedObjectPair.first);

      auto &pointerObject = ktestObject.content.pointers[currentPointerIdx];
      pointerObject.indexOfObject = referencesToObjectNum;
      pointerObject.indexOffset = offset;
      pointerObject.offset = writtenAddress - addressOfReferencedObject;

      ++currentPointerIdx;
    }
  }
  return *this;
}

KTestBuilder &KTestBuilder::fillContent() {
  // Required only for symbolics
  for (const auto &[object, symbolic] : state_.symbolics) {
    auto &ktestObject = ktest_.objects[order_.at(object)];

    auto array = symbolic.array();
    const auto &modelForArray = model_.bindings.at(array);

    auto objectSize = constantAddressSpace_.sizeOf(*object);
    ktestObject.content.bytes = new unsigned char[objectSize];

    auto buffer = ktestObject.content.bytes;
    for (std::uint64_t offset = 0; offset < objectSize; ++offset) {
      buffer[offset] = modelForArray.load(offset);
    }
  }

  return *this;
}

KTestBuilder &KTestBuilder::fillFinalContent() {
  // Requied for all object in graph
  for (auto &[objectPair, resolutionList] : constantPointerGraph_) {
    auto [object, state] = objectPair;

    auto &ktestObject = ktest_.objects[order_.at(object)];

    auto objectSize = constantAddressSpace_.sizeOf(*object);
    ktestObject.content.finalBytes = new unsigned char[objectSize];

    auto buffer = ktestObject.content.bytes;
    for (std::uint64_t offset = 0; offset < objectSize; ++offset) {
      auto byteExpr = state->read8(offset);
      if (auto pointerByteExpr = dyn_cast<PointerExpr>(byteExpr)) {
        byteExpr = pointerByteExpr->getValue();
      }

      ref<ConstantExpr> constantByteExpr =
          dyn_cast<ConstantExpr>(model_.evaluate(byteExpr));
      assert(!constantByteExpr.isNull());

      buffer[offset] = constantByteExpr->getZExtValue();
    }
  }

  return *this;
}

KTest KTestBuilder::build() { return std::move(ktest_); }