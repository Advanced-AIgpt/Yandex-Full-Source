#pragma once

#include <util/folder/path.h>
#include <util/generic/algorithm.h>
#include <util/generic/bitmap.h>
#include <util/generic/ymath.h>
#include <util/stream/zlib.h>

namespace NGranet {

// ~~~~ Container routines ~~~~

template <class SrcContainer, class DestContainer>
inline void Extend(const SrcContainer& src, DestContainer* dest) {
    Y_ASSERT(dest);
    dest->insert(dest->end(), std::begin(src), std::end(src));
}

template <class Value, class Range>
inline TVector<Value> ToVector(const Range& range) {
    return TVector<Value>(std::begin(range), std::end(range));
}

template <class Container, class Range>
inline Container ToContainer(const Range& range) {
    return Container(std::begin(range), std::end(range));
}

template <class Container>
inline std::remove_cvref_t<Container> Sorted(Container&& container) {
    std::remove_cvref_t<Container> sorted = std::forward<Container>(container);
    Sort(sorted);
    return sorted;
}

template <class Key, class Value>
TSet<Key> OrderedSetOfKeys(const THashMap<Key, Value>& map) {
    TSet<Key> keys;
    for (const auto& [key, value] : map) {
        keys.insert(key);
    }
    return keys;
}

template <class Key, class Set>
bool TryInsert(Key&& key, Set* set) {
    const auto [it, isNew] = set->insert(key);
    return isNew;
}

// ~~~~ Sort descending ~~~~

template <class Container>
inline void SortDescending(Container& container) {
    Sort(container, std::greater<typename Container::value_type>{});
}

template <class Container>
inline void StableSortDescending(Container& container) {
    StableSort(container, std::greater<typename Container::value_type>{});
}

template <class Container>
inline void SortUniqueDescending(Container& container) {
    SortUnique(container, std::greater<typename Container::value_type>{});
}

template <class TIterator, typename TGetKey>
static inline void SortByDescending(TIterator begin, TIterator end, const TGetKey& getKey) {
    Sort(begin, end, [&](auto&& left, auto&& right) { return getKey(right) < getKey(left); });
}

template <class TContainer, typename TGetKey>
static inline void SortByDescending(TContainer& container, const TGetKey& getKey) {
    SortByDescending(container.begin(), container.end(), getKey);
}

// ~~~~ Math ~~~~

inline bool IsPowerOf10(size_t x) {
    while (x > 0 && x % 10 == 0) {
        x /= 10;
    }
    return x == 1;
}

// Optimized log(exp(x) + exp(y))
inline float LogSumExp(float x, float y) {
    return x > y
        ? x + log1pf(expf(y - x))
        : y + log1pf(expf(x - y));
}

template <class T, typename = std::enable_if_t<std::is_arithmetic<T>::value>>
inline double ToDouble(const T& x) {
    return static_cast<double>(x);
}

template <class T, typename = std::enable_if_t<std::is_arithmetic<T>::value>>
inline float ToFloat(const T& x) {
    return static_cast<float>(x);
}

template <class T, typename = std::enable_if_t<std::is_arithmetic<T>::value>>
inline int ToInt(const T& x) {
    return static_cast<int>(x);
}

template <class T, typename = std::enable_if_t<std::is_integral<T>::value>>
inline int CeilTo(T x, T y) {
    return CeilDiv(x, y) * y;
}

// ~~~~ Hash ~~~~

// FYI: MultiHash and CombineHashes from utils are very slow.

template <class T>
inline size_t CalculateHash(const T& x) {
    return THash<T>()(x);
}

template <class T, typename = std::enable_if_t<std::is_arithmetic<T>::value>>
inline size_t TrivialHash(T x) {
    return static_cast<size_t>(x);
}

inline size_t TrivialHash(const void* ptr) {
    return reinterpret_cast<size_t>(ptr);
}

// ~~~~ Bit set ~~~~

template <class Value = size_t>
inline TVector<Value> BitSetElements(const TDynBitMap& set) {
    TVector<Value> result(Reserve(set.Count()));
    for (size_t pos = set.FirstNonZeroBit(); pos != set.Size(); pos = set.NextNonZeroBit(pos)) {
        result.push_back(pos);
    }
    return result;
}

inline size_t CountBits(ui64 x) {
    size_t count = 0;
    while (x != 0) {
        count++;
        x &= x - 1;
    }
    return count;
}

// ~~~~ Other ~~~~

const int INT_NPOS = -1;

TVector<TString> LoadLines(const TFsPath& path);

} // namespace NGranet
