#pragma once

#include "exceptions.h"

#include <util/digest/multi.h>
#include <util/generic/hash.h>
#include <util/generic/maybe.h>
#include <util/stream/output.h>

namespace NAlice::NNlg {

// Represents all values of the form x = start + i * step such that
// start <= x < stop if step > 0 or stop < x <= start if step < 0.
// If (stop - start) and step have different signs, the range is declared empty.
class TRange {
public:
    TRange(i64 start, i64 stop, i64 step = 1)
        : Start(start)
        , Stop(stop)
        , Step(step)
        , Size(CalcSize()) {
    }

    i64 GetStart() const {
        return Start;
    }

    i64 GetStop() const {
        return Stop;
    }

    i64 GetStep() const {
        return Step;
    }

    i64 GetSize() const {
        return Size;
    }

    bool IsEmpty() const {
        return Size == 0;
    }

    bool operator==(const TRange& other) const {
        return Start == other.Start && Size == other.Size && Step == other.Step;
    }

    bool Contains(i64 value) const;

    TMaybe<i64> operator[](i64 index) const {
        if (index >= 0 && index < Size) {
            // no overflow can happen:
            // Start + (Size - 1) * Step < Start + (Stop - Start) = Stop
            // (assuming positive Step, likewise for the negative Step)
            return Start + index * Step;
        }

        return Nothing();
    }

public:
    class TConstIterator {
    public:
        struct TEndTag {};

        using iterator_category = std::forward_iterator_tag;
        using value_type = i64;
        using difference_type = i64;
        using pointer = const i64*;
        using reference = i64;

        explicit TConstIterator(const TRange& range)
            : Pos(range.Start)
            , Step(range.Step)
            , Size(range.Size) {
        }

        TConstIterator(const TRange& range, TEndTag)
            : Pos(range.Stop)
            , Step(range.Step)
            , Size(0) {
        }

        bool operator==(TConstIterator other) const {
            return Size == other.Size;
        }

        bool operator!=(TConstIterator other) const {
            return Size != other.Size;
        }

        value_type operator*() const {
            return Pos;
        }

        TConstIterator& operator++() {
            Next();
            return *this;
        }

        TConstIterator operator++(int) {
            auto result = *this;
            Next();
            return result;
        }

    private:
        void Next() {
            Y_ASSERT(Size > 0);
            Pos += Step;
            --Size;
        }

    private:
        i64 Pos;
        i64 Step;
        i64 Size;
    };

    TConstIterator begin() const {
        return TConstIterator(*this);
    }

    TConstIterator end() const {
        return TConstIterator(*this, TConstIterator::TEndTag{});
    }

private:
    i64 CalcSize() const;

private:
    i64 Start;
    i64 Stop;
    i64 Step;
    i64 Size;
};

}  // namespace NAlice::NNlg

template <>
struct THash<NAlice::NNlg::TRange> {
    size_t operator()(NAlice::NNlg::TRange range) const {
        // not hashing Size because it is determined by the other three values
        return MultiHash(range.GetStart(), range.GetStop(), range.GetStep());
    }
};
