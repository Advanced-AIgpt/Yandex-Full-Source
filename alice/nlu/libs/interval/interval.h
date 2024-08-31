#pragma once

#include <util/digest/multi.h>
#include <util/generic/cast.h>
#include <util/stream/output.h>
#include <util/ysaveload.h>

namespace NNlu {

// ~~~~ TBaseInterval ~~~~

// Interval [Begin, End)
template <class T>
struct TBaseInterval {
    T Begin = T{};
    T End = T{};

    bool Valid() const {
        return Begin <= End;
    }

    T Length() const {
        Y_ASSERT(Begin <= End);
        return End - Begin;
    }

    bool Empty() const {
        return End == Begin;
    }

    void SetEmpty() {
        Begin = T{};
        End = T{};
    }

    TBaseInterval& operator&=(const TBaseInterval& other);
    TBaseInterval& operator|=(const TBaseInterval& other);

    TBaseInterval& operator+=(const T& shift);
    TBaseInterval& operator-=(const T& shift);

    // Intervals have not empty intersection.
    bool Overlaps(const TBaseInterval& other) const {
        return !Empty() && !other.Empty() && Begin < other.End && End > other.Begin;
    }

    // Does interval contain point. End point doesn't belong to interval.
    bool Contains(const T& x) const {
        return Begin <= x && x < End;
    }

    // Does interval contain other interval.
    // Empty interval belongs to any interval.
    bool Contains(const TBaseInterval& other) const {
        return other.Empty() || (Begin <= other.Begin && other.End <= End);
    }

    template <class TOther>
    TBaseInterval<TOther> ToInterval() const {
        return {SafeIntegerCast<TOther>(Begin), SafeIntegerCast<TOther>(End)};
    }

    size_t GetHash() const {
        return MultiHash(Begin, End);
    }

    Y_SAVELOAD_DEFINE(Begin, End);
};

// ~~~~ TBaseInterval methods ~~~~

template <class T>
inline TBaseInterval<T>& TBaseInterval<T>::operator&=(const TBaseInterval<T>& other) {
    Y_ASSERT(Valid());
    Y_ASSERT(other.Valid());

    Begin = Max(Begin, other.Begin);
    End = Min(End, other.End);
    if (Begin >= End) {
        SetEmpty();
    }
    return *this;
}

template <class T>
inline TBaseInterval<T>& TBaseInterval<T>::operator|=(const TBaseInterval<T>& other) {
    Y_ASSERT(Valid());
    Y_ASSERT(other.Valid());

    if (Empty()) {
        if (other.Empty()) {
            SetEmpty();
        } else {
            *this = other;
        }
    } else if (!other.Empty()) {
        Begin = Min(Begin, other.Begin);
        End = Max(End, other.End);
    }
    return *this;
}

template <class T>
inline TBaseInterval<T>& TBaseInterval<T>::operator+=(const T& shift) {
    Begin += shift;
    End += shift;
    return *this;
}

template <class T>
inline TBaseInterval<T>& TBaseInterval<T>::operator-=(const T& shift) {
    Begin -= shift;
    End -= shift;
    return *this;
}

template <class T>
inline TBaseInterval<T> operator&(const TBaseInterval<T>& interval1, const TBaseInterval<T>& interval2) {
    TBaseInterval<T> result = interval1;
    result &= interval2;
    return result;
}

template <class T>
inline TBaseInterval<T> operator|(const TBaseInterval<T>& interval1, const TBaseInterval<T>& interval2) {
    TBaseInterval<T> result = interval1;
    result |= interval2;
    return result;
}

template <class T>
inline bool operator<(const TBaseInterval<T>& interval1, const TBaseInterval<T>& interval2) {
    return std::tie(interval1.Begin, interval1.End) < std::tie(interval2.Begin, interval2.End);
}

template <class T>
inline bool operator==(const TBaseInterval<T>& interval1, const TBaseInterval<T>& interval2) {
    return interval1.Begin == interval2.Begin && interval1.End == interval2.End;
}

template <class T>
inline bool operator!=(const TBaseInterval<T>& interval1, const TBaseInterval<T>& interval2) {
    return !(interval1 == interval2);
}

template <class T>
inline IOutputStream& operator<<(IOutputStream& out, const TBaseInterval<T>& interval) {
    out << "[" << interval.Begin << ", " << interval.End << ")";
    return out;
}

template <class T>
inline double JaccardIndex(const TBaseInterval<T>& interval1, const TBaseInterval<T>& interval2) {
    const T intersection = (interval1 & interval2).Length();
    if (intersection == 0) {
        return 0;
    }
    return static_cast<double>(intersection) / (interval1.Length() + interval2.Length() - intersection);
}

// ~~~~ TBaseInterval aliases ~~~~

using TInterval = TBaseInterval<size_t>;
using TIntInterval = TBaseInterval<int>;

} // namespace NNlu

template <class T>
struct TPodTraits<NNlu::TBaseInterval<T>> {
    enum { IsPod = true };
};

template <class T>
struct THash<NNlu::TBaseInterval<T>> {
    inline size_t operator()(const NNlu::TBaseInterval<T>& value) const {
        return MultiHash(value.Begin, value.End);
    }
};
