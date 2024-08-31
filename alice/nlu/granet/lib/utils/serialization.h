#pragma once

#include <library/cpp/containers/comptrie/comptrie_trie.h>
#include <util/memory/blob.h>
#include <util/ysaveload.h>

namespace NGranet {

inline void SaveSmallSize(IOutputStream* output, size_t value) {
    if (value < 0xFFU) {
        Save(output, static_cast<ui8>(value));
    } else {
        Save(output, static_cast<ui8>(0xFFU));
        SaveSize(output, value);
    }
}

inline size_t LoadSmallSize(IInputStream* input) {
    ui8 small;
    ::Load(input, small);
    if (small < 0xFFU) {
        return small;
    }
    return LoadSize(input);
}

inline void SaveVersion(IOutputStream* output, size_t version) {
    SaveSmallSize(output, version);
}

inline void LoadVersion(IInputStream* input, size_t expected) {
    const size_t actual = LoadSmallSize(input);
    Y_ENSURE_EX(actual == expected,
        (TSerializeException() << "Invalid archive version: " << actual << ", expected: " << expected << "."));
}

} // namespace NGranet

template <>
inline void Save<TBlob>(IOutputStream* output, const TBlob& blob) {
    ::SaveSize(output, blob.Size());
    ::SavePodArray(output, blob.AsCharPtr(), blob.Size());
}

template <>
inline void Load<TBlob>(IInputStream* input, TBlob& blob) {
    TBuffer buffer;
    buffer.Resize(::LoadSize(input));
    ::LoadPodArray(input, buffer.Data(), buffer.Size());
    blob = TBlob::FromBuffer(buffer);
}

template <class TSymbol, class TData, class TPacker>
class TSerializer<TCompactTrie<TSymbol, TData, TPacker>> {
public:
    using TTrie = TCompactTrie<TSymbol, TData, TPacker>;

    static inline void Save(IOutputStream* output, const TTrie& trie) {
        ::Save(output, trie.Data());
    }

    static inline void Load(IInputStream* input, TTrie& trie) {
        TBlob blob;
        ::Load(input, blob);
        trie.Init(std::move(blob));
    }
};
