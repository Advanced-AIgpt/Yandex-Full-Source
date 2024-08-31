#pragma once

#include <alice/nlg/library/runtime_api/text_stream.h>
#include <alice/nlg/library/runtime_api/value.h>

namespace NAlice::NNlg::NOperators {

using TValueProvider = std::function<TValue()>;

// n-ary operators
template <typename TCallback>
TValue Concat(TCallback&& callback) {
    TText result;
    TTextOutput out(result);
    callback(out);
    return TValue::String(std::move(result));
}

// comparisons
TValue Equals(const TValue& lhs, const TValue& rhs);
TValue NotEquals(const TValue& lhs, const TValue& rhs);
TValue Greater(const TValue& lhs, const TValue& rhs);
TValue GreaterEq(const TValue& lhs, const TValue& rhs);
TValue Less(const TValue& lhs, const TValue& rhs);
TValue LessEq(const TValue& lhs, const TValue& rhs);
TValue ValueIn(const TValue& item, const TValue& values);
TValue ValueNotIn(const TValue& item, const TValue& values);

// binary operators
TValue Mul(const TValue& lhs, const TValue& rhs);
TValue TrueDiv(const TValue& lhs, const TValue& rhs);
TValue FloorDiv(const TValue& lhs, const TValue& rhs);
TValue Pow(const TValue& lhs, const TValue& rhs);
TValue Mod(const TValue& lhs, const TValue& rhs);
TValue Add(const TValue& lhs, const TValue& rhs);
TValue Sub(const TValue& lhs, const TValue& rhs);

// lazy binary operators
TValue And(const TValue& lhs, const TValueProvider& rhs);
TValue Or(const TValue& lhs, const TValueProvider& rhs);

// unary operators
TValue UnaryMinus(const TValue& target);
TValue UnaryPlus(const TValue& target);
TValue UnaryNot(const TValue& target);

// misc.
TValue SliceList(const TValue& value, const TValue& start, const TValue& stop, const TValue& step);

}  // namespace NAlice::NNlg::NOperators
