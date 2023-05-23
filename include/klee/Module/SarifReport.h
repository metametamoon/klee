//===-- SarifReport.h --------------------------------------------*- C++-*-===//
//
//                     The KLEE Symbolic Virtual Machine
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#ifndef KLEE_SARIF_REPORT_H
#define KLEE_SARIF_REPORT_H

#include <string>
#include <unordered_set>
#include <vector>

#include <nlohmann/json.hpp>

#include "SarifReportAPI.h"

using json = nlohmann::json;

namespace nlohmann {
template <typename T> struct adl_serializer<Optional<T>> {
  static void to_json(json &j, const Optional<T> &opt) {
    if (opt.hasValue()) {
      j = nullptr;
    } else {
      j = *opt;
    }
  }

  static void from_json(const json &j, Optional<T> &opt) {
    if (j.is_null()) {
      opt = Optional<T>();
    } else {
      opt = j.get<T>();
    }
  }
};
} // namespace nlohmann

namespace klee {

static const char *ReachWithErrorNames[] = {
    "DoubleFree",
    "UseAfterFree",
    "NullPointerException",
    "NullCheckAfterDerefException",
    "Reachable",
    "None",
};

const char *getErrorString(ReachWithError error);
std::string getErrorsString(const std::unordered_set<ReachWithError> &errors);

struct FunctionInfo;
struct KBlock;

struct ArtifactLocationJson {
  Optional<std::string> uri;
};

struct RegionJson {
  Optional<unsigned int> startLine;
  Optional<unsigned int> endLine;
  Optional<unsigned int> startColumn;
  Optional<unsigned int> endColumn;
};

struct PhysicalLocationJson {
  Optional<ArtifactLocationJson> artifactLocation;
  Optional<RegionJson> region;
};

struct LocationJson {
  Optional<PhysicalLocationJson> physicalLocation;
};

struct ThreadFlowLocationJson {
  Optional<LocationJson> location;
  Optional<json> metadata;
};

struct ThreadFlowJson {
  std::vector<ThreadFlowLocationJson> locations;
};

struct CodeFlowJson {
  std::vector<ThreadFlowJson> threadFlows;
};

struct Message {
  std::string text;
};

struct ResultJson {
  Optional<std::string> ruleId;
  Optional<Message> message;
  std::vector<LocationJson> locations;
  std::vector<CodeFlowJson> codeFlows;
};

struct DriverJson {
  std::string name;
};

struct ToolJson {
  DriverJson driver;
};

struct RunJson {
  std::vector<ResultJson> results;
  ToolJson tool;
};

struct SarifReportJson {
  std::vector<RunJson> runs;
};

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(ArtifactLocationJson, uri)

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(RegionJson, startLine, endLine,
                                                startColumn, endColumn)

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(PhysicalLocationJson,
                                                artifactLocation, region)

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(LocationJson, physicalLocation)

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(ThreadFlowLocationJson,
                                                location, metadata)

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(ThreadFlowJson, locations)

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(CodeFlowJson, threadFlows)

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(Message, text)

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(ResultJson, ruleId, message,
                                                codeFlows, locations)

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(DriverJson, name)

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(ToolJson, driver)

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(RunJson, results, tool)

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(SarifReportJson, runs)

struct Result {
  std::vector<Location> locations;
  std::vector<Optional<json>> metadatas;
  unsigned id;
  std::unordered_set<ReachWithError> errors;
};

struct SarifReport {
  std::vector<Result> results;

  bool empty() const { return results.empty(); }
};

SarifReport convertAndFilterSarifJson(const SarifReportJson &reportJson);

} // namespace klee

#endif /* KLEE_SARIF_REPORT_H */
