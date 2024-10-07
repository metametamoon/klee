#ifndef KLEE_LEMMAUPDATER_H
#define KLEE_LEMMAUPDATER_H

#include "Initializer.h"
#include "SearcherUtil.h"
#include "TargetManager.h"
#include <queue>
#include <variant>


namespace klee {
class PdrEngine: public Subscriber {
public:
  PdrEngine(ProofObligation *rootPob, TargetManager *targetManager,
               ConflictCoreInitializer *initializer,
               Executor* executor,
               ObjectManager *objectManager);

  struct AwaitingDepth {
    int n = 1;
    AwaitingDepth() : n(1) {}
    explicit AwaitingDepth(int n) : n(n) {}
  };
  struct MaxNodeUpdate {
    std::deque<ProofObligation *> updateQueue;
    int queueMaximalDepth;
  };
  using state_t = std::variant<AwaitingDepth, MaxNodeUpdate>;
  state_t currentState;
  PdrAction getPdrAction();
  void update(ref<ObjectManager::Event> e) override;

private:
  // return the subtree in an order where all the children have lesser indices
  // than their parents
  std::optional<std::vector<ProofObligation *>>
  tryExtractingSubtree(ProofObligation *root, int depth);
  ProofObligation *rootPob;
  // non-owning pointer; executor surely lives longer then we do
  TargetManager *targetManager;
  ConflictCoreInitializer *initializer;
  ObjectManager *objectManager;
  Executor* executor;

  bool checkPobWouldNotBePropagatedInTheFuture(ProofObligation *pob);
};
} // namespace klee

#endif // KLEE_LEMMAUPDATER_H
