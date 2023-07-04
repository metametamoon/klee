#ifndef KLEE_FUNCTIONANNOTATION_H
#define KLEE_FUNCTIONANNOTATION_H

#include <map>
#include <string>
#include <vector>

#include <nlohmann/json.hpp>
#include <nonstd/optional.hpp>

using nonstd::nullopt;
using nonstd::optional;
using json = nlohmann::json;

namespace klee {

// Annotation format: https://github.com/UnitTestBot/klee/discussions/92
struct FunctionAnnotation {
  enum class StatementName {
    Deref,
    InitNull,
  };

  struct Statement {
    StatementName name;
    optional<std::vector<std::string>> offset = nullopt;
    optional<std::string> data = nullopt;
  };

  enum class Property {
    Determ,
    Noreturn,
  };

  using Statements = std::vector<Statement>;

  std::string functionName;
  std::vector<Statements> statements;
  std::vector<Property> properties;
};

using FunctionAnnotations = std::map<std::string, FunctionAnnotation>;

const std::map<std::string, FunctionAnnotation::StatementName> statementsAsStr{
    {"Deref", FunctionAnnotation::StatementName::Deref},
    {"InitNull", FunctionAnnotation::StatementName::InitNull},
};

const std::map<std::string, FunctionAnnotation::Property> propertiesAsStr{
    {"determ", FunctionAnnotation::Property::Determ},
    {"noreturn", FunctionAnnotation::Property::Noreturn},
};

FunctionAnnotations parseAnnotationsFile(std::string &path);

} // namespace klee

#endif // KLEE_FUNCTIONANNOTATION_H
