#ifndef CEXPRWRITER_H
#define CEXPRWRITER_H
#include "Disjunction.h"

#include <klee/ADT/Ref.h>
#include <optional>
#include <string>

namespace klee {
class Expr;
std::optional<std::string> translateToCExpr(ref<Expr> expr);
std::string disjunctionToCExpr(disjunction const &disj,
                               bool unknownsExprsAsTrue);

} // namespace klee

#endif // CEXPRWRITER_H
