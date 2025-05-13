#ifndef PERFTRACKER_H
#define PERFTRACKER_H

#include <algorithm>
#include <chrono>
#include <string>
#include <unordered_map>
#include <vector>

#include <fmt/chrono.h>
#include <fmt/core.h>

#include "llvm/Support/ErrorHandling.h"
#include "llvm/Support/raw_ostream.h"

namespace klee {
struct EventStats {
  std::chrono::duration<double> total_duration{};
  long long count{};
};

namespace PerfEventKind {
static std::string BACKWARD = "backward";
static std::string CHECK_INDUCTIVE = "check-inductive";
};

class SimplePerfTracker {
public:
  SimplePerfTracker() = default;
  SimplePerfTracker(const SimplePerfTracker &) = delete;
  SimplePerfTracker &operator=(const SimplePerfTracker &) = delete;
  SimplePerfTracker(SimplePerfTracker &&) = delete;
  SimplePerfTracker &operator=(SimplePerfTracker &&) = delete;

  void event_begin(const std::string &event_name) {
    start_times[event_name] = std::chrono::high_resolution_clock::now();
  }

  void event_end(const std::string &event_name) {
    auto it = start_times.find(event_name);
    if (it != start_times.end()) {
      auto end_time = std::chrono::high_resolution_clock::now();
      std::chrono::duration<double> duration = end_time - it->second;
      event_stats[event_name].total_duration += duration;
      event_stats[event_name].count++;
      start_times.erase(it);
    } else {
      llvm::errs() << fmt::format(
          "SimplePerfTracker Warning: event_end called for '{}' without "
          "corresponding event_begin.\n",
          event_name);
    }
  }

  void print_results() const {
    llvm::errs() << "--- Performance Results ---\n";

    if (event_stats.empty()) {
      llvm::errs() << "No completed events recorded.\n";
    } else {
      std::vector<std::string> sorted_event_names;
      for (const auto &pair : event_stats) {
        sorted_event_names.push_back(pair.first);
      }
      std::sort(sorted_event_names.begin(), sorted_event_names.end());

      for (const auto &name : sorted_event_names) {
        const auto &stats = event_stats.at(name);

        if (stats.count > 0) {
          std::chrono::duration<double> average_duration =
              stats.total_duration / stats.count;

          llvm::errs() << fmt::format("{}: Total: {}, Avg: {}, Count: {}\n",
                                      name, stats.total_duration,
                                      average_duration, stats.count);
        }
      }
    }

    if (!start_times.empty()) {
      llvm::errs() << "\n--- Unfinished Events (Started but not Ended) ---\n";
      std::vector<std::string> unfinished_names;
      for (const auto &pair : start_times) {
        unfinished_names.push_back(pair.first);
      }
      std::sort(unfinished_names.begin(), unfinished_names.end());

      for (const auto &name : unfinished_names) {
        llvm::errs() << fmt::format("{}\n", name);
      }
    }

    llvm::errs() << "---------------------------\n";
  }

  void reset() {
    start_times.clear();
    event_stats.clear();
  }


private:
  std::unordered_map<std::string,
                     std::chrono::high_resolution_clock::time_point>
      start_times;
  std::unordered_map<std::string, EventStats> event_stats;
};

} // namespace klee

#endif // PERFTRACKER_H
