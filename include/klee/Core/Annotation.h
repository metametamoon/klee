#ifndef KLEE_ANNOTATION_H
#define KLEE_ANNOTATION_H

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
struct Annotation {
  enum class StatementName {
    Unknown,
    Deref,
    InitNull,
  };

  enum class Property {
    Unknown,
    Determ,
    Noreturn,
  };

  struct Statement {
    StatementName name;
    optional<std::vector<std::string>> offset = nullopt;
    optional<std::string> data = nullopt;
  };

  using Statements = std::vector<Statement>;

  std::string functionName;
  std::vector<Statements> statements;
  std::vector<Property> properties;
};

using Annotations = std::map<std::string, Annotation>;

const std::map<std::string, Annotation::StatementName> statementsAsStr{
    {"Deref", Annotation::StatementName::Deref},
    {"InitNull", Annotation::StatementName::InitNull},
};

const std::map<std::string, Annotation::Property> propertiesAsStr{
    {"determ", Annotation::Property::Determ},
    {"noreturn", Annotation::Property::Noreturn},
};

Annotations parseAnnotationsFile(std::string &path);

} // namespace klee

#endif // KLEE_ANNOTATION_H
