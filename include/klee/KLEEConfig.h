#ifndef KLEE_KLEECONFIG_H
#define KLEE_KLEECONFIG_H

namespace klee {

enum class GuidanceKind {
  NoGuidance,       // Default symbolic execution
  CoverageGuidance, // Use GuidedSearcher and guidedRun to maximize full code
                    // coverage
  ErrorGuidance     // Use GuidedSearcher and guidedRun to maximize specified
                    // targets coverage
};

class Config {
public:
  GuidanceKind guidanceKind;
  bool mockExternalCalls;
  bool usePOSIX;
  enum class LibcType { FreestandingLibc, KleeLibc, UcLibc } libcType;
  bool skipNotLI;
};

}

#endif
