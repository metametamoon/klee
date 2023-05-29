#ifndef KLEE_KLEECONFIG_H
#define KLEE_KLEECONFIG_H

#include <cstdint>
#include <string>

namespace klee {

enum class GuidanceKind {
  NoGuidance,       // Default symbolic execution
  CoverageGuidance, // Use GuidedSearcher and guidedRun to maximize full code
                    // coverage
  ErrorGuidance     // Use GuidedSearcher and guidedRun to maximize specified
                    // targets coverage
};

enum class MockMutableGlobalsPolicy {
  None,
  PrimitiveFields,
  All,
};

class Config {
public:
  GuidanceKind guidanceKind; // This should be fixed
  bool mockExternalCalls; // This should be fixed
  bool usePOSIX;
  enum class LibcType { FreestandingLibc, KleeLibc, UcLibc } libcType;
  bool skipNotLI; // This should be fixed

  unsigned maxDepth;
  std::string maxTime;
  std::string maxCoreSolverTime;
  unsigned long long maxInstructions;
  unsigned maxForks;
  unsigned maxStackFrames;
  uint64_t maxSymbolicAllocationSize;
  MockMutableGlobalsPolicy mockMutableGlobals;
};

}

#endif
