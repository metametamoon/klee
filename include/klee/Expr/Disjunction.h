#ifndef DISJUNCTION_H
#define DISJUNCTION_H
#include <klee/Expr/ExprHashMap.h>
#include <set>

namespace klee {

struct disjunction {
  ExprOrderedSet elements;
  auto operator<(const disjunction& other) const {
    return elements < other.elements;
  };
  explicit disjunction(ExprOrderedSet value): elements(std::move(value)) {}
  disjunction(const disjunction& other) = default;
  disjunction(disjunction&& other) = default;
  disjunction() = default;
  [[nodiscard]] auto begin() const { return elements.begin(); }
  [[nodiscard]] auto end() const { return elements.end(); }
};

using cnf = std::set<disjunction>;


}


#endif //DISJUNCTION_H
