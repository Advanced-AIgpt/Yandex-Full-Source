#pragma once

#include <util/str_stl.h>
#include <util/ysaveload.h>

#define DECLARE_TUPLE_LIKE_TYPE(Self, ...)                  \
    inline auto TieMembers() const {                        \
        return std::tie(__VA_ARGS__);                       \
    }                                                       \
    inline auto TieMembers() {                              \
        return std::tie(__VA_ARGS__);                       \
    }                                                       \
    inline bool operator<(const Self& other) const {        \
        return TieMembers() < other.TieMembers();           \
    }                                                       \
    inline bool operator<=(const Self& other) const {       \
        return TieMembers() <= other.TieMembers();          \
    }                                                       \
    inline bool operator>(const Self& other) const {        \
        return TieMembers() > other.TieMembers();           \
    }                                                       \
    inline bool operator>=(const Self& other) const {       \
        return TieMembers() >= other.TieMembers();          \
    }                                                       \
    inline bool operator==(const Self& other) const {       \
        return TieMembers() == other.TieMembers();          \
    }                                                       \
    inline bool operator!=(const Self& other) const {       \
        return TieMembers() != other.TieMembers();          \
    }                                                       \
    inline void Save(IOutputStream* s) const {              \
        ::SaveMany(s, __VA_ARGS__);                         \
    }                                                       \
    inline void Load(IInputStream* s) {                     \
        ::LoadMany(s, __VA_ARGS__);                         \
    }

namespace NPrivateTupleLikeTypeHash {
    template <class T>
    inline size_t CalculateHash(const T& x) {
        return THash<T>()(x);
    }
}

struct TTupleLikeTypeHash {
    template <class T>
    size_t operator()(const T& value) const {
        return NPrivateTupleLikeTypeHash::CalculateHash(value.TieMembers());
    }
};
