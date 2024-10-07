#include "LemmaUpdater.h"
#include <fmt/format.h>

#include "Executor.h"
#include "SearcherUtil.h"

#include <klee/Support/DebugFlags.h>

namespace klee {
LemmaUpdateAction LemmaUpdater::getLemmaUpdateAction() {
  if (rootPob == nullptr) {
    return LemmaUpdateAction{LemmaUpdateAction::Noop{}};
  }
  if (auto awaitingDepth = std::get_if<AwaitingDepth>(&currentState)) {
    auto depth = awaitingDepth->n;
    auto maybeExtracted = tryExtractingSubtree(rootPob, depth);
    if (maybeExtracted.has_value()) {
      auto &extracted = maybeExtracted.value();
      std::deque<ProofObligation *> updateQueue{};
      for (auto pob : extracted) {
        updateQueue.push_back(pob);
      }
      if (debugPrints.isSet(DebugPrint::Pdr)) {
        llvm::errs() << "[lemma updater] "
                     << "Reached depth " << depth << '\n';
        llvm::errs() << "[lemma updater] pobs in queue:\n";
        for (auto pob : extracted) {
          llvm::errs() << fmt::format("\t\tloc={}\n\t\tpath={}\n",
                                      pob->location->toString(),
                                      pob->constraints.path().toString());
        }
        auto path = std::string{"dots/out_"} + std::to_string(depth) + ".dot";
        // executor->dumpBackwardsExplorationGraph(path);
      }
      currentState = MaxNodeUpdate{updateQueue, awaitingDepth->n};
    }
    return LemmaUpdateAction{LemmaUpdateAction::Noop{}};
  } else if (auto maxNodeUpdate = std::get_if<MaxNodeUpdate>(&currentState)) {
    if (!maxNodeUpdate->updateQueue.empty()) {
      auto nextPob = maxNodeUpdate->updateQueue.front();
      maxNodeUpdate->updateQueue.pop_front();
      return LemmaUpdateAction{LemmaUpdateAction::BegNodeUpdate{
          nextPob, maxNodeUpdate->queueMaximalDepth}};
    } else {
      currentState = AwaitingDepth{maxNodeUpdate->queueMaximalDepth + 1};
      if (debugPrints.isSet(DebugPrint::Pdr)) {
        llvm::errs()
            << "[lemma updater] the queue has ended! Now expecting depth= "
            << maxNodeUpdate->queueMaximalDepth + 1 << "\n";
      }
      return LemmaUpdateAction{
          LemmaUpdateAction::CheckInductive{maxNodeUpdate->queueMaximalDepth}};
    }
  } else {
    return LemmaUpdateAction{LemmaUpdateAction::Noop{}};
  }
}
void LemmaUpdater::update(ref<ObjectManager::Event> e) {
  switch (e->getKind()) {
  case ObjectManager::Event::Kind::ProofObligations: {
    auto pobsEvent = cast<ObjectManager::ProofObligations>(e);
    if (auto maxNodeUpdate = std::get_if<MaxNodeUpdate>(&currentState)) {
      for (auto it = maxNodeUpdate->updateQueue.begin();
           it != maxNodeUpdate->updateQueue.end();) {
        if (pobsEvent->removed.count(*it) > 0) {
          it = maxNodeUpdate->updateQueue.erase(it);
        } else {
          ++it;
        }
      }
    }
    break;
  }

  default:
    break;
  }
}

/**
 * returns the nodes of the tree at depth not greater than @depth (the root is
 * assumed to have a depth 1)
 */
std::optional<std::vector<ProofObligation *>>
LemmaUpdater::tryExtractingSubtree(ProofObligation *root, int depth) {
  if (!checkPobWouldNotBePropagatedInTheFuture(root)) {
    return std::nullopt;
  }
  auto result = std::vector<ProofObligation *>{};
  if (depth == 0) {
    return result;
  }
  for (auto child : root->children) {
    auto childSubtree = this->tryExtractingSubtree(child, depth - 1);
    if (childSubtree.has_value()) {
      result.insert(result.end(), childSubtree->begin(), childSubtree->end());
    } else {
      return std::nullopt;
    }
  }
  result.push_back(root);
  return result;
}

bool LemmaUpdater::checkPobWouldNotBePropagatedInTheFuture(
    ProofObligation *pob) {
  return !targetManager->hasTargetedStates(pob->location) &&
         !initializer->initsLeftForTarget(pob->location) &&
         objectManager->propagationCount[pob] == 0;
}

LemmaUpdater::LemmaUpdater(ProofObligation *rootPob,
                           TargetManager *targetManager,
                           ConflictCoreInitializer *initializer,
                           Executor *executor, ObjectManager *objectManager)
    : rootPob(rootPob), targetManager(targetManager), initializer(initializer),
      objectManager(objectManager), executor(executor) {}

} // namespace klee