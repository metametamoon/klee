#ifndef KLEE_SPARSESTORAGE_H
#define KLEE_SPARSESTORAGE_H

#include "klee/ADT/PersistentHashMap.h"
#include "klee/Solver/SolverUtil.h"

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

template <typename ValueType, typename Eq = std::equal_to<ValueType>>
struct StorageAdapter {
  class iterator {
  public:
    using value_ty = std::pair<size_t, ValueType>;

    virtual iterator &operator+=(int right);
    friend iterator operator+(iterator const &left, int right) {
      iterator result = left;
      result += right;
      return result;
    }
    virtual value_ty operator*() = 0;
  };

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
struct UnorderedMapAdapder : public StorageAdapter<ValueType, Eq> {
public:
  using storage_ty = std::unordered_map<size_t, ValueType>;
  using base_ty = StorageAdapter<ValueType, Eq>;
  using iterator = typename base_ty::iterator;
  class umap_iterator : iterator {
    typename storage_ty::const_iterator it;

  public:
    umap_iterator(const typename storage_ty::const_iterator &it) : it(it) {}
    iterator &operator+=(int right) override {
      it += right;
      return *this;
    }
    typename base_ty::value_ty operator*() override { return *it; }
  };
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
struct PersistenUnorderedMapAdapder : public StorageAdapter<ValueType, Eq> {
  using storage_ty = PersistentHashMap<size_t, ValueType>;
  using base_ty = StorageAdapter<ValueType, Eq>;
  using iterator = typename base_ty::iterator;
  class persistent_umap_iterator : iterator {
    typename storage_ty::iterator it;

  public:
    persistent_umap_iterator(const typename storage_ty::const_iterator &it)
        : it(it) {}
    iterator &operator+=(int right) override {
      it += right;
      return *this;
    }
    typename base_ty::value_ty operator*() override { return *it; }
  };
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

template <typename ValueType, typename Eq = std::equal_to<ValueType>>
struct PeristentArray : public StorageAdapter<ValueType, Eq> {
  using storage_ty = ValueType *;
  using base_ty = StorageAdapter<ValueType, Eq>;
  using iterator = typename base_ty::iterator;

protected:
  class persistent_array_iterator : iterator {
    typename std::vector<ValueType>::const_interator it;
    size_t index;

  public:
    persistent_array_iterator(
        const typename std::vector<ValueType>::const_interator &it,
        size_t index)
        : it(it), index(index) {}
    iterator &operator+=(int right) override {
      it += right;
      index += right;
      return *this;
    }
    typename base_ty::value_ty operator*() override { return {index, *it}; }
  };

public:
  storage_ty storage;
  size_t storageSize;
  ValueType defaultValue;
  size_t nonDefaultValuesCount;
  PeristentArray(size_t storageSize, const ValueType &defaultValue)
      : storageSize(storageSize), defaultValue(defaultValue),
        nonDefaultValuesCount(0) {
    clear();
  }
  bool contains(size_t key) const override { return storage[key] != 0; }
  iterator begin() const override {
    std::vector<int>::iterator it(storage);
    return persistent_array_iterator(it, 0);
  }
  iterator end() const override {
    std::vector<int>::iterator it(storage + storageSize);
    return persistent_array_iterator(it, storageSize);
  }
  const ValueType *lookup(size_t key) const override {
    auto val = storage[key];
    return Eq()(val, defaultValue) ? nullptr : &storage[key];
  }
  bool empty() const override { return nonDefaultValuesCount == 0; }
  void set(size_t key, const ValueType &value) override {
    bool wasDefault = storage[key] == defaultValue;
    bool newDefault = value == defaultValue;
    if (wasDefault && !newDefault) {
      ++nonDefaultValuesCount;
    }
    if (!wasDefault && newDefault) {
      --nonDefaultValuesCount;
    }
    storage[key] = value;
  }
  void remove(size_t key) override {
    if (storage[key] != defaultValue) {
      --nonDefaultValuesCount;
      storage[key] = defaultValue;
    }
  }
  const ValueType &at(size_t key) const override { return storage[key]; }
  void clear() override { std::fill(begin(), end(), defaultValue); }
  size_t size() const override { return nonDefaultValuesCount; }
};

template <typename ValueType, typename Eq = std::equal_to<ValueType>>
class Storage {
private:
  ValueType defaultValue;
  Eq eq;

public:
  Storage(const ValueType &defaultValue = ValueType())
      : defaultValue(defaultValue) {}

  Storage(const std::unordered_map<size_t, ValueType> &internalStorage,
          const ValueType &defaultValue)
      : defaultValue(defaultValue) {
    for (auto &[index, value] : internalStorage) {
      store(index, value);
    }
  }

  Storage(const std::vector<ValueType> &values,
          const ValueType &defaultValue = ValueType())
      : defaultValue(defaultValue) {
    for (size_t idx = 0; idx < values.size(); ++idx) {
      store(idx, values[idx]);
    }
  }

  virtual void store(size_t idx, const ValueType &value) = 0;

  template <typename InputIterator>
  void store(size_t idx, InputIterator iteratorBegin,
             InputIterator iteratorEnd) {
    for (; iteratorBegin != iteratorEnd; ++iteratorBegin, ++idx) {
      store(idx, *iteratorBegin);
    }
  }

  virtual ValueType load(size_t idx) const = 0;

  virtual size_t sizeOfSetRange() const = 0;

  bool operator==(const Storage<ValueType> &another) const {
    return eq(defaultValue, another.defaultValue) && compare(another) == 0;
  }

  bool operator!=(const Storage<ValueType> &another) const {
    return !(*this == another);
  }

  bool operator<(const Storage &another) const {
    return compare(another) == -1;
  }

  bool operator>(const Storage &another) const { return compare(another) == 1; }

  int compare(const Storage<ValueType> &other) const {
    auto ordered = calculateOrderedStorage();
    auto otherOrdered = other.calculateOrderedStorage();

    if (ordered == otherOrdered) {
      return 0;
    } else {
      return ordered < otherOrdered ? -1 : 1;
    }
  }

  virtual std::map<size_t, ValueType> calculateOrderedStorage() const = 0;

  virtual std::vector<ValueType> getFirstNIndexes(size_t n) const = 0;

  virtual const StorageAdapter<ValueType, Eq> &storage() const = 0;

  const ValueType &defaultV() const { return defaultValue; };

  virtual void reset() = 0;

  void reset(ValueType newDefault) {
    defaultValue = newDefault;
    reset();
  }

  void print(llvm::raw_ostream &os, Density) const;
};

template <typename ValueType, typename Eq = std::equal_to<ValueType>,
          typename InternalStorageAdapter = UnorderedMapAdapder<ValueType, Eq>>
class SparseStorage : public Storage<ValueType, Eq> {
private:
  InternalStorageAdapter internalStorage;
  ValueType defaultValue;
  Eq eq;

  bool contains(size_t key) const { return internalStorage.containts(key); }

public:
  SparseStorage(const ValueType &defaultValue = ValueType())
      : Storage<ValueType, Eq>(defaultValue) {
    static_assert(
        std::is_base_of<StorageAdapter<ValueType, Eq>,
                        InternalStorageAdapter>::value,
        "type parameter of this class must derive from StorageAdapter");
  }

  SparseStorage(const std::unordered_map<size_t, ValueType> &internalStorage,
                const ValueType &defaultValue)
      : Storage<ValueType, Eq>(internalStorage, defaultValue) {
    static_assert(
        std::is_base_of<StorageAdapter<ValueType, Eq>,
                        InternalStorageAdapter>::value,
        "type parameter of this class must derive from StorageAdapter");
  }

  SparseStorage(const std::vector<ValueType> &values,
                const ValueType &defaultValue = ValueType())
      : Storage<ValueType, Eq>(values, defaultValue) {
    static_assert(
        std::is_base_of<StorageAdapter<ValueType, Eq>,
                        InternalStorageAdapter>::value,
        "type parameter of this class must derive from StorageAdapter");
  }

  void store(size_t idx, const ValueType &value) override {
    if (eq(value, defaultValue)) {
      internalStorage.remove(idx);
    } else {
      internalStorage.set(idx, value);
    }
  }

  ValueType load(size_t idx) const override {
    auto it = internalStorage.lookup(idx);
    return it ? *it : defaultValue;
  }

  size_t sizeOfSetRange() const override {
    size_t sizeOfRange = 0;
    for (auto i : internalStorage) {
      sizeOfRange = std::max(i.first, sizeOfRange);
    }
    return internalStorage.empty() ? 0 : sizeOfRange + 1;
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
Storage<unsigned char> sparseBytesFromValue(const U &value) {
  const unsigned char *valueUnsignedCharIterator =
      reinterpret_cast<const unsigned char *>(&value);
  SparseStorage<unsigned char> result;
  result.store(0, valueUnsignedCharIterator,
               valueUnsignedCharIterator + sizeof(value));
  return result;
}

} // namespace klee

#endif
