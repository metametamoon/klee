#ifndef KLEE_SARIF_REPORT_API_H
#define KLEE_SARIF_REPORT_API_H

#include "llvm/ADT/Optional.h"

#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

using llvm::Optional;

namespace klee {

enum ReachWithError {
  DoubleFree = 0,
  UseAfterFree,
  NullPointerException,
  NullCheckAfterDerefException,
  Reachable,
  None,
};

struct KBlock;
struct FunctionInfo;

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
  bool operator()(const Location &a, const Location &b) const { return a == b; }
};

struct ResultAPI {
  std::vector<Location> locations;
  std::vector<Optional<std::string>> metadatas;
  unsigned id;
  std::unordered_set<ReachWithError> errors;
};

struct SarifReportAPI {
  std::vector<ResultAPI> results;

  bool empty() const { return results.empty(); }
};

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
};

#endif 
