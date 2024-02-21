#ifndef KLEE_SPARSESTORAGE_H
#define KLEE_SPARSESTORAGE_H

#include "klee/ADT/PersistentHashMap.h"

#include <cassert>
#include <cstddef>
#include <functional>
#include <map>
#include <unordered_map>
#include <vector>

namespace llvm {
class raw_ostream;
};

namespace klee {

enum class Density {
  Sparse,
  Dense,
};

template <typename Iter, typename ValueType,
          typename Eq = std::equal_to<ValueType>>
struct StorageAdapter {
  using iterator = Iter;
  virtual bool contains(size_t key) const = 0;
  virtual iterator begin() const = 0;
  virtual iterator end() const = 0;
  virtual const ValueType *lookup(size_t key) const = 0;
  virtual bool empty() const = 0;
  virtual void set(size_t key, const ValueType &value) = 0;
  virtual void remove(size_t key) = 0;
  virtual const ValueType &at(size_t key) const = 0;
  virtual void clear() = 0;
  virtual size_t size() const = 0;
};

template <typename ValueType, typename Eq = std::equal_to<ValueType>>
struct UnorderedMapAdapder
    : public StorageAdapter<
          typename std::unordered_map<size_t, ValueType>::const_iterator,
          ValueType, Eq> {
  using storage_ty = std::unordered_map<size_t, ValueType>;
  using iterator = typename storage_ty::const_iterator;
  storage_ty storage;
  bool contains(size_t key) const override { return storage.count(key) != 0; }
  iterator begin() const override { return storage.begin(); }
  iterator end() const override { return storage.end(); }
  const ValueType *lookup(size_t key) const override {
    auto it = storage.find(key);
    if (it != storage.end()) {
      return &it->second;
    }
    return nullptr;
  }
  bool empty() const override { return storage.empty(); }
  void set(size_t key, const ValueType &value) override {
    storage[key] = value;
  }
  void remove(size_t key) override { storage.erase(key); }
  const ValueType &at(size_t key) const override { return storage.at(key); }
  void clear() override { storage.clear(); }
  size_t size() const override { return storage.size(); }
};

template <typename ValueType, typename Eq = std::equal_to<ValueType>>
struct PersistenUnorderedMapAdapder
    : public StorageAdapter<
          typename PersistentHashMap<size_t, ValueType>::iterator, ValueType,
          Eq> {
  using storage_ty = PersistentHashMap<size_t, ValueType>;
  using iterator = typename storage_ty::iterator;
  storage_ty storage;
  bool contains(size_t key) const override { return storage.count(key) != 0; }
  iterator begin() const override { return storage.begin(); }
  iterator end() const override { return storage.end(); }
  const ValueType *lookup(size_t key) const override {
    return storage.lookup(key);
  }
  bool empty() const override { return storage.empty(); }
  void set(size_t key, const ValueType &value) override {
    storage.replace({key, value});
  }
  void remove(size_t key) override { storage.remove(key); }
  const ValueType &at(size_t key) const override { return storage.at(key); }
  void clear() override { storage.clear(); }
  size_t size() const override { return storage.size(); }
};

template <typename ValueType, typename Eq = std::equal_to<ValueType>,
          typename InternalStorageAdapter = UnorderedMapAdapder<ValueType, Eq>>
class SparseStorage {
private:
  InternalStorageAdapter internalStorage;
  ValueType defaultValue;
  Eq eq;

  bool contains(size_t key) const { return internalStorage.containts(key); }

public:
  SparseStorage(const ValueType &defaultValue = ValueType())
      : defaultValue(defaultValue) {
    static_assert(
        std::is_base_of<
            StorageAdapter<typename InternalStorageAdapter::iterator, ValueType,
                           Eq>,
            InternalStorageAdapter>::value,
        "type parameter of this class must derive from StorageAdapter");
  }

  SparseStorage(const std::unordered_map<size_t, ValueType> &internalStorage,
                const ValueType &defaultValue)
      : defaultValue(defaultValue) {
    for (auto &[index, value] : internalStorage) {
      store(index, value);
    }
  }

  SparseStorage(const std::vector<ValueType> &values,
                const ValueType &defaultValue = ValueType())
      : defaultValue(defaultValue) {
    for (size_t idx = 0; idx < values.size(); ++idx) {
      store(idx, values[idx]);
    }
  }

  void store(size_t idx, const ValueType &value) {
    if (eq(value, defaultValue)) {
      internalStorage.remove(idx);
    } else {
      internalStorage.set(idx, value);
    }
  }

  template <typename InputIterator>
  void store(size_t idx, InputIterator iteratorBegin,
             InputIterator iteratorEnd) {
    for (; iteratorBegin != iteratorEnd; ++iteratorBegin, ++idx) {
      store(idx, *iteratorBegin);
    }
  }

  ValueType load(size_t idx) const {
    auto it = internalStorage.lookup(idx);
    return it ? *it : defaultValue;
  }

  size_t sizeOfSetRange() const {
    size_t sizeOfRange = 0;
    for (auto i : internalStorage) {
      sizeOfRange = std::max(i.first, sizeOfRange);
    }
    return internalStorage.empty() ? 0 : sizeOfRange + 1;
  }

  bool operator==(const SparseStorage<ValueType> &another) const {
    return eq(defaultValue, another.defaultValue) && compare(another) == 0;
  }

  bool operator!=(const SparseStorage<ValueType> &another) const {
    return !(*this == another);
  }

  bool operator<(const SparseStorage &another) const {
    return compare(another) == -1;
  }

  bool operator>(const SparseStorage &another) const {
    return compare(another) == 1;
  }

  int compare(const SparseStorage<ValueType> &other) const {
    auto ordered = calculateOrderedStorage();
    auto otherOrdered = other.calculateOrderedStorage();

    if (ordered == otherOrdered) {
      return 0;
    } else {
      return ordered < otherOrdered ? -1 : 1;
    }
  }

  std::map<size_t, ValueType> calculateOrderedStorage() const {
    std::map<size_t, ValueType> ordered;
    for (const auto &i : internalStorage) {
      ordered.insert(i);
    }
    return ordered;
  }

  std::vector<ValueType> getFirstNIndexes(size_t n) const {
    std::vector<ValueType> vectorized(n);
    for (size_t i = 0; i < n; i++) {
      vectorized[i] = load(i);
    }
    return vectorized;
  }

  const InternalStorageAdapter &storage() const { return internalStorage; };

  const ValueType &defaultV() const { return defaultValue; };

  void reset() { internalStorage.clear(); }

  void reset(ValueType newDefault) {
    defaultValue = newDefault;
    reset();
  }

  void print(llvm::raw_ostream &os, Density) const;
};

template <typename U>
SparseStorage<unsigned char> sparseBytesFromValue(const U &value) {
  const unsigned char *valueUnsignedCharIterator =
      reinterpret_cast<const unsigned char *>(&value);
  SparseStorage<unsigned char> result;
  result.store(0, valueUnsignedCharIterator,
               valueUnsignedCharIterator + sizeof(value));
  return result;
}

} // namespace klee

#endif
