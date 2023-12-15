#ifndef KLEE_TAINT_ANNOTATION_H
#define KLEE_TAINT_ANNOTATION_H

#include "nlohmann/json.hpp"
#include "nonstd/optional.hpp"

#include "map"
#include "set"
#include "string"
#include "vector"

using json = nlohmann::json;

namespace klee {

using TaintSinksSourcesMap = std::map<size_t , std::set<size_t>>;

class TaintAnnotation final {
public:
  TaintSinksSourcesMap sinksToSources;
  std::map<std::string, size_t> sinks;
  std::map<std::string, size_t> sources;

  explicit TaintAnnotation(const std::string &taintAnnotationsFile);
  virtual ~TaintAnnotation();
};

} // namespace klee

#endif // KLEE_TAINT_ANNOTATION_H
