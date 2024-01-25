//===---- ImmutableDAG.h ---------------------------------------*- C++ -*-===//
//
//                     The KLEE Symbolic Virtual Machine
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#ifndef KLEE_IMMUTABLEDAG_H
#define KLEE_IMMUTABLEDAG_H

#include "klee/Expr/Path.h"
#include <memory>
#include <vector>

namespace klee {

class ImmutableDAG {
  struct DAGNode {
    std::vector<std::shared_ptr<DAGNode>> prev;
    Path path;

    DAGNode() = default;
    DAGNode(std::vector<std::shared_ptr<DAGNode>> prev) : prev(prev) {}
  };

  std::shared_ptr<DAGNode> node;

public:
  Path &getValue() { return node->path; }
  const Path &getValue() const { return node->path; }

  ImmutableDAG() : node(new DAGNode()) {}

  ImmutableDAG(std::vector<ImmutableDAG> prev)
      : node(new DAGNode()) {
    std::vector<std::shared_ptr<DAGNode>> nodes;
    for (const auto &dag : prev) {
      node->prev.push_back(dag.node);
    }
  }

  ImmutableDAG(const ImmutableDAG &other) : node(new DAGNode()) {
    node->path = other.node->path;
    node->prev = other.node->prev;
  }

  ImmutableDAG &operator=(const ImmutableDAG &other) {
    if (this != &other) {
      node = std::make_shared<DAGNode>();
      node->path = other.node->path;
      node->prev = other.node->prev;
    }
    return *this;
  }

};

} // namespace klee

#endif
