#ifndef KLEE_BIDIRECTIONALSEARCHER_H
#define KLEE_BIDIRECTIONALSEARCHER_H

#include "BackwardSearcher.h"
#include "Initializer.h"
#include "PdrEngine.h"
#include "ObjectManager.h"
#include "Searcher.h"
#include "SearcherUtil.h"
#include <klee/ADT/Ticker.h>

namespace klee {

class IBidirectionalSearcher : public Subscriber {
public:
  virtual ref<SearcherAction> selectAction() = 0;
  virtual bool empty() = 0;
  virtual ~IBidirectionalSearcher() {}
};

class BidirectionalSearcher : public IBidirectionalSearcher {
  enum class StepKind { Forward, Branch, Backward, Initialize, LemmaUpdate };

public:
  ref<SearcherAction> selectAction() override;
  void update(ref<ObjectManager::Event> e) override;
  bool empty() override;

  // Assumes ownership
  explicit BidirectionalSearcher(Searcher *_forward, Searcher *_branch,
                                 BackwardSearcher *_backward,
                                 Initializer *_initializer,
                                 std::unique_ptr<PdrEngine> &&pdrEngine);

  ~BidirectionalSearcher() override;

private:
  Ticker ticker;

  Searcher *forward;
  Searcher *branch;
  BackwardSearcher *backward;
  Initializer *initializer;
  std::unique_ptr<PdrEngine> pdrEngine;

private:
  StepKind selectStep();
};

class ForwardOnlySearcher : public IBidirectionalSearcher {
public:
  ref<SearcherAction> selectAction() override;
  void update(ref<ObjectManager::Event>) override;
  bool empty() override;
  explicit ForwardOnlySearcher(Searcher *searcher);
  ~ForwardOnlySearcher() override;

private:
  Searcher *searcher;
};

} // namespace klee

#endif
