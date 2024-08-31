#include "range.h"

#include <util/system/yassert.h>

namespace NAlice::NNlg {

namespace {

bool CheckSameSign(i64 a, i64 b) {
    return (a ^ b) >= 0;
}

bool IsSafeToSub(i64 a, i64 b) {
    // MIN <= a - b <= MAX
    //
    // Case analysis:
    // a = 0
    //     b = 0: okay
    //     b > 0: okay (all positive numbers can be negated)
    //     b < 0: b != MIN, or a <= MAX + b (0 <= MAX + b)
    // a > 0
    //     b = 0: okay
    //     b > 0: okay (both inequalities always satisfied)
    //     b < 0: a - b <= MAX <=> a <= MAX + b
    // a < 0
    //     b = 0: okay
    //     b > 0: okay (both inequalities always satisfied
    //     b < 0: MIN <= a - b <=> a => MIN + b
    //
    // Thus, there are really three cases (the code below)
    if (CheckSameSign(a, b)) {
        return true;
    }

    if (a >= 0) {
        return Max<i64>() + b >= a;
    }

    return Min<i64>() + b <= a;
}

}  // namespace

i64 TRange::CalcSize() const {
    if (Step == 0) {
        ythrow TValueError() << "Range " << *this << " has zero step";
    }

    // we rely on being able to compute and negate range = stop - start
    if (!IsSafeToSub(Stop, Start)) {
        ythrow TValueError() << "Range " << *this << " has unsafe bounds";
    }

    i64 span = Stop - Start;
    i64 step = Step;

    // if span and step have different signs, it means iteration would overflow i64.
    // Python thinks in this case the range should be empty
    if (!CheckSameSign(span, step)) {
        return 0;
    }

    // ceil(span / step)
    i64 div = span / step;
    i64 mod = span % step;  // fused with division so basically free
    if (mod) {
        ++div;
    }
    return div;
}

bool TRange::Contains(i64 value) const {
    Y_ASSERT(Step != 0);

    if (!IsSafeToSub(value, Start)) {
        return false;  // value obviously far from acceptable
    }

    Y_ASSERT(IsSafeToSub(Stop, Start));
    i64 span = Stop - Start;
    i64 step = Step;

    // if span and step have different signs, it means iteration would overflow i64.
    // Python thinks in this case the range should be empty
    if (!CheckSameSign(span, step)) {
        return false;
    }

    // shift the problem so that the start is effectively zero
    value -= Start;

    if (step > 0) {
        return value >= 0 && value < span && value % step == 0;
    }

    return value <= 0 && value > span && value % step == 0;
}

}  // namespace NAlice::NNlg

template <>
void Out<NAlice::NNlg::TRange>(IOutputStream& out, TTypeTraits<NAlice::NNlg::TRange>::TFuncParam range) {
    if (range.GetStep() == 1) {
        if (range.GetStart() == 0) {
            out << TStringBuf("xrange(") << range.GetStop() << ')';
        } else {
            out
                << TStringBuf("xrange(")
                << range.GetStart() << TStringBuf(", ")
                << range.GetStop() << ')';
        }
    } else {
        out
            << TStringBuf("xrange(")
            << range.GetStart() << TStringBuf(", ")
            << range.GetStop() << TStringBuf(", ")
            << range.GetStep() << ')';
    }
}
