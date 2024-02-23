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

enum class StorageIteratorKind { UMap, PersistentUMap, PersistenArray };

template <typename ValueType, typename Eq = std::equal_to<ValueType>>
struct StorageAdapter {
  using value_ty = std::pair<size_t, ValueType>;
  class inner_iterator {
  public:
    virtual StorageIteratorKind getKind() const = 0;
    virtual inner_iterator &operator++() = 0;
    virtual inner_iterator *clone() = 0;
    virtual value_ty operator*() = 0;
    virtual bool operator!=(const inner_iterator &other) const = 0;
    static bool classof(const StorageAdapter<ValueType, Eq> *SE) {
      return true;
    }
    virtual ~inner_iterator() {}
  };

  class iterator {
    inner_iterator *impl;

  public:
    iterator(inner_iterator *impl) : impl(impl) {}
    iterator(iterator const &right) : impl(right.impl->clone()) {}
    ~iterator() { delete impl; }
    iterator &operator=(iterator const &right) {
      delete impl;
      impl = right.impl->clone();
      return *this;
    }

    // forward operators to virtual calls through impl.
    iterator &operator++() {
      ++(*impl);
      return *this;
    }
    value_ty operator*() { return *(*impl); }
    bool operator!=(const iterator &other) const {
      return *impl != *other.impl;
    }
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
  using inner_iterator = typename base_ty::inner_iterator;
  struct allocator {
    UnorderedMapAdapder<ValueType, Eq> operator()(const ValueType &) const {
      return UnorderedMapAdapder<ValueType, Eq>();
    }
  };
  class umap_iterator : public inner_iterator {
    typename storage_ty::const_iterator it;

  public:
    umap_iterator(const typename storage_ty::const_iterator &it) : it(it) {}
    StorageIteratorKind getKind() const override {
      return StorageIteratorKind::UMap;
    }
    static bool classof(const StorageAdapter<ValueType, Eq> *SA) {
      return SA->getKind() == StorageIteratorKind::UMap;
    }
    static bool classof(const UnorderedMapAdapder<ValueType, Eq> *) {
      return true;
    }
    inner_iterator &operator++() override {
      ++it;
      return *this;
    }
    inner_iterator *clone() override { return new umap_iterator(it); }
    typename base_ty::value_ty operator*() override { return *it; }
    bool operator!=(const inner_iterator &other) const override {
      if (other.getKind() != getKind()) {
        return true;
      }
      const umap_iterator &el = static_cast<const umap_iterator &>(other);
      return el.it != it;
    }
  };
  storage_ty storage;
  bool contains(size_t key) const override { return storage.count(key) != 0; }
  iterator begin() const override {
    return iterator(new umap_iterator(storage.begin()));
  }
  iterator end() const override {
    return iterator(new umap_iterator(storage.end()));
  }
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
  using inner_iterator = typename base_ty::inner_iterator;
  struct allocator {
    PersistenUnorderedMapAdapder<ValueType, Eq>
    operator()(const ValueType &) const {
      return PersistenUnorderedMapAdapder<ValueType, Eq>();
    }
  };
  class persistent_umap_iterator : public inner_iterator {
    typename storage_ty::iterator it;

  public:
    persistent_umap_iterator(const typename storage_ty::iterator &it)
        : it(it) {}
    StorageIteratorKind getKind() const override {
      return StorageIteratorKind::PersistentUMap;
    }
    static bool classof(const StorageAdapter<ValueType, Eq> *SA) {
      return SA->getKind() == StorageIteratorKind::PersistentUMap;
    }
    static bool classof(const PersistenUnorderedMapAdapder<ValueType, Eq> *) {
      return true;
    }
    inner_iterator &operator++() override {
      ++it;
      return *this;
    }
    inner_iterator *clone() override {
      return new persistent_umap_iterator(it);
    }
    typename base_ty::value_ty operator*() override { return *it; }
    bool operator!=(const inner_iterator &other) const override {
      if (other.getKind() != getKind()) {
        return true;
      }
      const persistent_umap_iterator &el =
          static_cast<const persistent_umap_iterator &>(other);
      return el.it != it;
    }
  };
  storage_ty storage;
  bool contains(size_t key) const override { return storage.count(key) != 0; }
  iterator begin() const override {
    return new persistent_umap_iterator(storage.begin());
  }
  iterator end() const override {
    return new persistent_umap_iterator(storage.end());
  }
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
struct PersistentArray : public StorageAdapter<ValueType, Eq> {
  using storage_ty = ValueType *;
  using base_ty = StorageAdapter<ValueType, Eq>;
  using iterator = typename base_ty::iterator;
  using inner_iterator = typename base_ty::inner_iterator;
  struct allocator {
    size_t storageSize;
    allocator(size_t storageSize) : storageSize(storageSize) {}
    PersistentArray<ValueType, Eq>
    operator()(const ValueType &defaultValue) const {
      return PersistentArray<ValueType, Eq>(defaultValue, storageSize);
    }
  };

  Eq eq;
  class persistent_array_iterator : public inner_iterator {
    storage_ty it;
    size_t index;
    size_t size;
    Eq eq;
    ValueType defaultValue;

  public:
    persistent_array_iterator(storage_ty it, size_t index, size_t size,
                              const ValueType &defaultValue)
        : it(it), index(index), size(size), defaultValue(defaultValue) {}
    StorageIteratorKind getKind() const override {
      return StorageIteratorKind::PersistenArray;
    }
    static bool classof(const StorageAdapter<ValueType, Eq> *SA) {
      return SA->getKind() == StorageIteratorKind::PersistenArray;
    }
    static bool classof(const PersistentArray<ValueType, Eq> *) { return true; }
    inner_iterator &operator++() override {
      do {
        ++it;
        ++index;
      } while (index < size && eq(*it, defaultValue));
      return *this;
    }
    inner_iterator *clone() override {
      return new persistent_array_iterator(it, index, size, defaultValue);
    }
    typename base_ty::value_ty operator*() override { return {index, *it}; }
    bool operator!=(const inner_iterator &other) const override {
      if (other.getKind() != getKind()) {
        return true;
      }
      const persistent_array_iterator &el =
          static_cast<const persistent_array_iterator &>(other);
      return el.it != it || el.index != index;
    }
  };

public:
  storage_ty storage;
  size_t storageSize;
  ValueType defaultValue;
  size_t nonDefaultValuesCount;
  PersistentArray(const ValueType &defaultValue, size_t storageSize = 8)
      : storageSize(storageSize), defaultValue(defaultValue),
        nonDefaultValuesCount(0) {
    storage = new ValueType[storageSize];
    clear();
  }
  PersistentArray(const PersistentArray<ValueType, Eq> &pa)
      : storageSize(pa.storageSize), defaultValue(pa.defaultValue),
        nonDefaultValuesCount(pa.nonDefaultValuesCount) {
    storage = new ValueType[storageSize];
    clear();
    for (const auto &[key, val] : pa) {
      storage[key] = val;
    }
  }
  PersistentArray &operator=(const PersistentArray<ValueType, Eq> &pa) {
    storageSize = pa.storageSize;
    defaultValue = pa.defaultValue;
    nonDefaultValuesCount = pa.nonDefaultValuesCount;
    storage = new ValueType[storageSize];
    clear();
    for (const auto &[key, val] : pa) {
      storage[key] = val;
    }
    return *this;
  }
  ~PersistentArray() { delete[] storage; }
  bool contains(size_t key) const override { return storage[key] != 0; }
  iterator begin() const override {
    return new persistent_array_iterator(storage, 0, storageSize, defaultValue);
  }
  iterator end() const override {
    return new persistent_array_iterator(storage + storageSize, storageSize,
                                         storageSize, defaultValue);
  }
  const ValueType *lookup(size_t key) const override {
    auto val = storage[key];
    return Eq()(val, defaultValue) ? nullptr : &storage[key];
  }
  bool empty() const override { return nonDefaultValuesCount == 0; }
  void set(size_t key, const ValueType &value) override {
    bool wasDefault = eq(storage[key], defaultValue);
    bool newDefault = eq(value, defaultValue);
    if (wasDefault && !newDefault) {
      ++nonDefaultValuesCount;
    }
    if (!wasDefault && newDefault) {
      --nonDefaultValuesCount;
    }
    storage[key] = value;
  }
  void remove(size_t key) override {
    if (!eq(storage[key], defaultValue)) {
      --nonDefaultValuesCount;
      storage[key] = defaultValue;
    }
  }
  const ValueType &at(size_t key) const override { return storage[key]; }
  void clear() override {
    for (size_t i = 0; i < storageSize; ++i) {
      storage[i] = defaultValue;
    }
    nonDefaultValuesCount = 0;
  }
  size_t size() const override { return nonDefaultValuesCount; }
};

template <typename ValueType, typename Eq = std::equal_to<ValueType>>
class Storage {
protected:
  ValueType defaultValue;
  Eq eq;

  Storage(const ValueType &defaultValue = ValueType())
      : defaultValue(defaultValue) {}

public:
  virtual ~Storage() {}

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

  bool operator==(const Storage<ValueType, Eq> &another) const {
    return eq(defaultValue, another.defaultValue) && compare(another) == 0;
  }

  bool operator!=(const Storage<ValueType, Eq> &another) const {
    return !(*this == another);
  }

  bool operator<(const Storage &another) const {
    return compare(another) == -1;
  }

  bool operator>(const Storage &another) const { return compare(another) == 1; }

  int compare(const Storage<ValueType, Eq> &other) const {
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

  virtual Storage<ValueType, Eq> *clone() const = 0;
};

template <typename ValueType, typename Eq = std::equal_to<ValueType>,
          typename InternalStorageAdapter = UnorderedMapAdapder<ValueType, Eq>,
          typename Allocator = typename InternalStorageAdapter::allocator>
class SparseStorage : public Storage<ValueType, Eq> {
private:
  Allocator alloc;
  InternalStorageAdapter internalStorage;

  bool contains(size_t key) const { return internalStorage.containts(key); }

public:
  // SparseStorage(const ValueType &defaultValue = ValueType())
  //     : Storage<ValueType, Eq>(defaultValue) {
  //   static_assert(
  //       std::is_base_of<StorageAdapter<ValueType, Eq>,
  //                       InternalStorageAdapter>::value,
  //       "type parameter of this class must derive from StorageAdapter");
  // }

  SparseStorage(const ValueType &defaultValue = ValueType(),
                const Allocator &alloc = Allocator())
      : Storage<ValueType, Eq>(defaultValue), alloc(alloc),
        internalStorage(alloc(defaultValue)) {
    static_assert(
        std::is_base_of<StorageAdapter<ValueType, Eq>,
                        InternalStorageAdapter>::value,
        "type parameter of this class must derive from StorageAdapter");
  }

  SparseStorage(const std::unordered_map<size_t, ValueType> &internalStorage,
                const ValueType &defaultValue = ValueType(),
                const Allocator &alloc = Allocator())
      : SparseStorage(defaultValue, alloc) {
    for (auto &[index, value] : internalStorage) {
      store(index, value);
    }
  }

  SparseStorage(const std::vector<ValueType> &values,
                const ValueType &defaultValue = ValueType(),
                const Allocator &alloc = Allocator())
      : SparseStorage(defaultValue, alloc) {
    for (size_t idx = 0; idx < values.size(); ++idx) {
      store(idx, values[idx]);
    }
  }

  void store(size_t idx, const ValueType &value) override {
    if (Storage<ValueType, Eq>::eq(value,
                                   Storage<ValueType, Eq>::defaultValue)) {
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

  ValueType load(size_t idx) const override {
    auto it = internalStorage.lookup(idx);
    return it ? *it : Storage<ValueType, Eq>::defaultValue;
  }

  size_t sizeOfSetRange() const override {
    size_t sizeOfRange = 0;
    for (auto i : internalStorage) {
      sizeOfRange = std::max(i.first, sizeOfRange);
    }
    return internalStorage.empty() ? 0 : sizeOfRange + 1;
  }

  std::map<size_t, ValueType> calculateOrderedStorage() const override {
    std::map<size_t, ValueType> ordered;
    for (const auto &i : internalStorage) {
      ordered.insert(i);
    }
    return ordered;
  }

  std::vector<ValueType> getFirstNIndexes(size_t n) const override {
    std::vector<ValueType> vectorized(n);
    for (size_t i = 0; i < n; i++) {
      vectorized[i] = load(i);
    }
    return vectorized;
  }

  const InternalStorageAdapter &storage() const override {
    return internalStorage;
  };

  void reset() override {
    internalStorage = alloc(Storage<ValueType, Eq>::defaultValue);
  }

  void reset(ValueType newDefault) {
    Storage<ValueType, Eq>::defaultValue = newDefault;
    reset();
  }

  void print(llvm::raw_ostream &os, Density) const;
  Storage<ValueType, Eq> *clone() const override {
    return new SparseStorage<ValueType, Eq, InternalStorageAdapter>(*this);
  }
};

} // namespace klee

#endif
