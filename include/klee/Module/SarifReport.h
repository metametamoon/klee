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

#include "llvm/ADT/Optional.h"

#include <string>
#include <unordered_set>
#include <vector>

#include <nlohmann/json.hpp>

using json = nlohmann::json;
using llvm::Optional;

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
enum ReachWithError {
  DoubleFree = 0,
  UseAfterFree,
  NullPointerException,
  NullCheckAfterDerefException,
  Reachable,
  None,
};

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

struct Location {
  std::string filename;
  unsigned int startLine;
  unsigned int endLine;
  Optional<unsigned int> startColumn;
  Optional<unsigned int> endColumn;

  Location(std::string filename_, unsigned int startLine_,
           Optional<unsigned int> endLine_, Optional<unsigned int> startColumn_,
           Optional<unsigned int> endColumn_)
      : filename(filename_), startLine(startLine_),
        endLine(endLine_.hasValue() ? *endLine_ : startLine_),
        startColumn(startColumn_),
        endColumn(endColumn_.hasValue() ? endColumn_ : startColumn_) {
    computeHash();
  }

  std::size_t hash() const { return hashValue; }

  // /// @brief Required by klee::ref-managed objects
  // class ReferenceCounter _refCount;

  bool operator==(const Location &other) const {
    return filename == other.filename && startLine == other.startLine &&
           endLine == other.endLine && startColumn == other.startColumn &&
           endColumn == other.endColumn;
  }

  bool isInside(const FunctionInfo &info) const;

  using Instructions = std::unordered_map<
      unsigned int,
      std::unordered_map<unsigned int, std::unordered_set<unsigned int>>>;

  bool isInside(KBlock *block, const Instructions &origInsts) const;

  std::string toString() const;

private:
  size_t hashValue = 0;
  void computeHash() {
    hash_combine(hashValue, filename);
    hash_combine(hashValue, startLine);
    hash_combine(hashValue, endLine);
    // TODO HASH
    if (startColumn.hasValue()) {
      hash_combine(hashValue, *startColumn);
    }
    if (endColumn.hasValue()) {
      hash_combine(hashValue, *endColumn);
    }
  }

  template <class T> inline void hash_combine(std::size_t &s, const T &v) {
    std::hash<T> h;
    s ^= h(v) + 0x9e3779b9 + (s << 6) + (s >> 2);
  }
};

struct LocationHash {
  unsigned operator()(const Location &t) const { return t.hash(); }
};

struct LocationCmp {
  bool operator()(const Location &a, const Location &b) const {
    return a == b;
  }
};

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

enum Verdict { FalsePositive, TruePositive };

struct AnalysisResult { // structure representing the result of checking a trace
  unsigned id;          // unique identifier of the trace
  Verdict verdict;      // the verdict on the trace (FP or TP)
  double confidence;    // confidence in the verdict
  AnalysisResult(unsigned _id, Verdict _verdict, double _confidence)
      : id(_id), verdict(_verdict), confidence(_confidence) {}
};

struct AnalysisReport {
  std::vector<AnalysisResult> results; // list of check results
};

} // namespace klee

#endif /* KLEE_SARIF_REPORT_H */
