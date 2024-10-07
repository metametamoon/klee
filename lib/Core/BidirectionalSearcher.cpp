#include "klee/Support/OptionCategories.h"

#include "BidirectionalSearcher.h"
#include "ObjectManager.h"
#include "Searcher.h"
#include "SearcherUtil.h"

namespace klee {

llvm::cl::opt<unsigned> ForwardTicks("forward-ticks", llvm::cl::desc(""),
                                     llvm::cl::init(25),
                                     llvm::cl::cat(ExecCat));

llvm::cl::opt<unsigned> BranchTicks("branch-ticks", llvm::cl::desc(""),
                                    llvm::cl::init(25), llvm::cl::cat(ExecCat));

llvm::cl::opt<unsigned> InitTicks("init-ticks", llvm::cl::desc(""),
                                  llvm::cl::init(25), llvm::cl::cat(ExecCat));

llvm::cl::opt<unsigned> BackwardTicks("backward-ticks", llvm::cl::desc(""),
                                      llvm::cl::init(25),
                                      llvm::cl::cat(ExecCat));

llvm::cl::opt<unsigned> LemmaUpdateTicks("lemma-update-ticks",
                                         llvm::cl::desc(""), llvm::cl::init(25),
                                         llvm::cl::cat(ExecCat));

BidirectionalSearcher::StepKind BidirectionalSearcher::selectStep() {
  size_t initial_choice = ticker.getCurrent();
  size_t choice = initial_choice;

  do {
    switch (choice) {
    case 0: {
      if (!forward->empty()) {
        return StepKind::Forward;
      }
      break;
    }
    case 1: {
      if (!branch->empty()) {
        return StepKind::Branch;
      }
      break;
    }
    case 2: {
      if (!backward->empty()) {
        return StepKind::Backward;
      }
      break;
    }
    case 3: {
      if (!initializer->empty()) {
        return StepKind::Initialize;
      }
      break;
    }
    case 4: {
      return StepKind::LemmaUpdate;
    }
    }
    ticker.moveToNext();
    choice = ticker.getCurrent();
  } while (choice != initial_choice);

  assert(0 && "Empty searcher queried for an action");
}

ref<SearcherAction> BidirectionalSearcher::selectAction() {
  ref<SearcherAction> action;

  while (action.isNull()) {
    switch (selectStep()) {

    case StepKind::Forward: {
      auto &state = forward->selectState();
      state.isolated = state.isolated;
      action = new ForwardAction(&state);
      break;
    }

    case StepKind::Branch: {
      auto &state = branch->selectState();
      state.isolated = state.isolated;
      action = new ForwardAction(&state);
      break;
    }

    case StepKind::Backward: {
      auto propagation = backward->selectAction();
      action = new BackwardAction(propagation);
      break;
    }

    case StepKind::Initialize: {
      auto initAndTargets = initializer->selectAction();
      action =
          new InitializeAction(initAndTargets.first, initAndTargets.second);
      break;
    }
    case StepKind::LemmaUpdate:
      action =
          new PdrAction(pdrEngine->getPdrAction());
      break;
    }
  }
  return action;
}

void BidirectionalSearcher::update(ref<ObjectManager::Event> e) {
  if (auto states = dyn_cast<ObjectManager::States>(e)) {
    if (states->isolated) {
      branch->update(states->modified, states->added, states->removed);
    } else {
      forward->update(states->modified, states->added, states->removed);
    }
  } else if (auto props = dyn_cast<ObjectManager::Propagations>(e)) {
    backward->update(props->added, props->removed);
  } else if (auto pobs = dyn_cast<ObjectManager::ProofObligations>(e)) {
    initializer->update(pobs->added, pobs->removed);
    backward->update(pobs->added, pobs->removed);
  } else if (auto conflicts = dyn_cast<ObjectManager::Conflicts>(e)) {
    for (auto conflict : conflicts->conflicts) {
      initializer->addConflictInit(conflict->conflict, conflict->target);
    }
  } else {
    assert(0 && "Unknown event");
  }
}

bool BidirectionalSearcher::empty() {
  auto &ticks = ticker.getTicks();
  return (forward->empty() || (ticks.at(0) == 0)) &&
         (branch->empty() || (ticks.at(1) == 0)) &&
         (backward->empty() || (ticks.at(2) == 0)) &&
         (initializer->empty() || (ticks.at(3) == 0));
}

BidirectionalSearcher::BidirectionalSearcher(Searcher *_forward,
                                             Searcher *_branch,
                                             BackwardSearcher *_backward,
    Initializer *_initializer, std::unique_ptr<PdrEngine> &&_lemmaUpdater)
    : ticker({ForwardTicks, BranchTicks, BackwardTicks, InitTicks,
              LemmaUpdateTicks}),
      forward(_forward), branch(_branch), backward(_backward),
      initializer(_initializer), pdrEngine(std::move(_lemmaUpdater)) {}

BidirectionalSearcher::~BidirectionalSearcher() {
  delete forward;
  delete branch;
  delete backward;
  delete initializer;
}

ref<SearcherAction> ForwardOnlySearcher::selectAction() {
  return new ForwardAction(&searcher->selectState());
}

bool ForwardOnlySearcher::empty() { return searcher->empty(); }

void ForwardOnlySearcher::update(ref<ObjectManager::Event> e) {
  if (auto statesEvent = dyn_cast<ObjectManager::States>(e)) {
    searcher->update(statesEvent->modified, statesEvent->added,
                     statesEvent->removed);
  }
}

ForwardOnlySearcher::ForwardOnlySearcher(Searcher *_searcher) {
  searcher = _searcher;
}

ForwardOnlySearcher::~ForwardOnlySearcher() { delete searcher; }

} // namespace klee
