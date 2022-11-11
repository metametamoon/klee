#include <cstddef>
#include <cstdint>

extern "C" {

  struct FuzzInfo {
    int (*harness)(const uint8_t *, uint64_t);
    size_t map_size; // last index is the SAT block
    uint64_t timeout;
    size_t data_size;
    unsigned char *seed;

    unsigned char *solution;
  };

  bool fuzz_internal(FuzzInfo info);
};
