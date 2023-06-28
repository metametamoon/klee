#ifndef KLEE_FUNCTIONANNOTATION_H
#define KLEE_FUNCTIONANNOTATION_H

#include <vector>
#include <map>
#include <string>

#include <nonstd/optional.hpp>
#include <nlohmann/json.hpp>

using nonstd::optional;
using nonstd::nullopt;
using json = nlohmann::json;

namespace klee {

struct FunctionAnnotation {
  enum class StatementType {
    Deref,
    InitNull,
  };

  struct Statement {
    StatementType type;
    optional<std::vector<std::string>> offset = nullopt;
    optional<std::string> data = nullopt;
  };

  enum class PropertyType {
    Determ,
    Noreturn,
  };

  using Statements = std::vector<Statement>;

  std::string name;
  std::vector<Statements> statementsOfParams;
  std::vector<PropertyType> properties;
};

using FunctionAnnotations = std::map<std::string, FunctionAnnotation>;
FunctionAnnotations parseAnnotationsFile(std::string &path);

} // namespace klee

#endif // KLEE_FUNCTIONANNOTATION_H
