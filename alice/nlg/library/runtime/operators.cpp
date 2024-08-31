#include "operators.h"

#include <alice/nlg/library/runtime_api/exceptions.h>
#include <alice/nlg/library/runtime_api/text_stream.h>

#include <util/charset/utf8.h>
#include <util/generic/algorithm.h>
#include <util/generic/ymath.h>

#include <algorithm>
#include <cmath>

namespace NAlice::NNlg::NOperators {

namespace {

[[noreturn]] void ThrowTypeError(TStringBuf op, const TValue& lhs, const TValue& rhs) {
    ythrow TTypeError() << "Invalid operands for " << op << ": " << lhs.GetTypeName() << ", " << rhs.GetTypeName();
}

i64 FloorDivInteger(i64 a, i64 b) {
    Y_ASSERT(b != 0);

    // http://www.microhowto.info/howto/round_towards_minus_infinity_when_dividing_integers_in_c_or_c++.html
    i64 div = a / b;
    i64 rem = a % b;
    if (rem != 0 && (rem ^ b) < 0) {
        --div;
    }
    return div;
}

i64 ModInteger(i64 a, i64 b) {
    return a - FloorDivInteger(a, b) * b;
}

double ModDouble(double a, double b) {
    return a - std::floor(a / b) * b;
}

// Jinja2 equality is relaxed, number upcasts are allowed
bool ValueEqualsImpl(TStringBuf op, const TValue& lhs, const TValue& rhs) {
    // we test types in the expected order of popularity,
    // right now it is based on guesswork and subject to change after profiling

    if (lhs.IsUndefined() && rhs.IsUndefined()) {
        return true;
    }

    if (lhs.IsString() && rhs.IsString()) {
        return lhs.GetString() == rhs.GetString();
    }

    if (lhs.IsDouble() || rhs.IsDouble()) {
        auto left = ToDouble(lhs);
        auto right = ToDouble(rhs);
        if (!left || !right) {
            return false;
        }
        return *left == *right;
    }

    {
        auto left = ToInteger(lhs);
        auto right = ToInteger(rhs);
        int numCorrect = left.Defined() + right.Defined();
        if (numCorrect == 2) {
            return *left == *right;
        } else if (numCorrect == 1) {
            return false;
        }
    }

    if (lhs.IsList() && rhs.IsList()) {
        const auto& left = lhs.GetList();
        const auto& right = rhs.GetList();
        return left.size() == right.size() &&
               std::equal(left.begin(), left.end(), right.begin(),
                          [op](const TValue& a, const TValue& b) { return ValueEqualsImpl(op, a, b); });
    }

    if (lhs.IsDict() && rhs.IsDict()) {
        // a copy of operator== from arcadia/util/generic/hash.h
        // with a custom value comparator
        const auto& left = lhs.GetDict();
        const auto& right = rhs.GetDict();

        if (left.size() != right.size()) {
            return false;
        }

        for (const auto& [key, value] : left) {
            const auto* rightValue = right.FindPtr(key);
            if (!rightValue || !ValueEqualsImpl(op, value, *rightValue)) {
                return false;
            }
        }
        return true;
    }

    if (lhs.IsRange() && rhs.IsRange()) {
        return lhs.GetRange() == rhs.GetRange();
    }

    if (lhs.IsNone() && rhs.IsNone()) {
        return true;
    }

    return false;
}

bool ValueInImpl(const TValue& item, const TValue& values) {
    if (values.IsList()) {
        const auto& list = values.GetList();
        return FindPtr(list, item) != nullptr;
    }

    if (values.IsDict() && item.IsString()) {
        return values.GetDict().contains(item.GetString().GetStr());  // ignore text flags
    }

    if (values.IsRange() && item.IsInteger()) {
        return values.GetRange().Contains(item.GetInteger());
    }

    if (values.IsString() && item.IsString()) {
        return values.GetString().GetStr().Contains(item.GetString().GetStr());
    }

    return false;
}

// Jinja2's comparisons semantics depends on the Python verision,
// we went for the Python 3 one because it makes more sense.
// This is subject to change if we find out that
// the existing templates depend on Python 2's behavior
//
// Normally we'd need to implement all comparisons separately,
// instead we chose to ban NaNs from the data model,
// this is subject to change if it turns out that existing templates need NaNs
bool ValueLessImpl(TStringBuf op, const TValue& lhs, const TValue& rhs) {
    if (lhs.IsDouble() || rhs.IsDouble()) {
        auto left = ToDouble(lhs);
        auto right = ToDouble(rhs);
        if (!left || !right) {
            ThrowTypeError(op, lhs, rhs);
        }
        return *left < *right;
    }

    {
        auto left = ToInteger(lhs);
        auto right = ToInteger(rhs);
        if (left && right) {
            return *left < *right;
        }
    }

    if (lhs.IsString() && rhs.IsString()) {
        // TODO(a-square): this ordering is broken, make a better one?
        return lhs.GetString().GetStr() < rhs.GetString().GetStr();
    }

    if (lhs.IsList() && rhs.IsList()) {
        const auto& left = lhs.GetList();
        const auto& right = rhs.GetList();
        return std::lexicographical_compare(
            left.begin(), left.end(), right.begin(), right.end(),
            [op](const TValue& a, const TValue& b) { return ValueLessImpl(op, a, b); });
    }

    ThrowTypeError(op, lhs, rhs);
}

} // namespace

//////////////////////////////////////////////////////////////////////
// Comparisons
//////////////////////////////////////////////////////////////////////

TValue Equals(const TValue& lhs, const TValue& rhs) {
    return TValue::Bool(ValueEqualsImpl(TStringBuf("=="), lhs, rhs));
}

TValue NotEquals(const TValue& lhs, const TValue& rhs) {
    return TValue::Bool(!ValueEqualsImpl(TStringBuf("!="), lhs, rhs));
}

TValue Greater(const TValue& lhs, const TValue& rhs) {
    return TValue::Bool(ValueLessImpl(TStringBuf(">"), rhs, lhs));
}

TValue GreaterEq(const TValue& lhs, const TValue& rhs) {
    return TValue::Bool(!ValueLessImpl(TStringBuf(">="), lhs, rhs));
}

TValue Less(const TValue& lhs, const TValue& rhs) {
    return TValue::Bool(ValueLessImpl(TStringBuf("<"), lhs, rhs));
}

TValue LessEq(const TValue& lhs, const TValue& rhs) {
    return TValue::Bool(!ValueLessImpl(TStringBuf("<="), rhs, lhs));
}

TValue ValueIn(const TValue& item, const TValue& values) {
    return TValue::Bool(ValueInImpl(item, values));
}

TValue ValueNotIn(const TValue& item, const TValue& values) {
    return TValue::Bool(!ValueInImpl(item, values));
}

//////////////////////////////////////////////////////////////////////
// Binary operators
//////////////////////////////////////////////////////////////////////

TValue Mul(const TValue& lhs, const TValue& rhs) {
    // double multiplication is strictly numeric
    if (lhs.IsDouble() && rhs.IsDouble()) {
        return TValue::Double(lhs.GetDouble() * rhs.GetDouble());
    }

    // in this function, we're using an undefined value as a failure marker,
    // it's okay because the result of a binary operator is always defined
    struct TMultiplyVisitor {
        i64 Factor;

        TValue operator()(TValue::TUndefined) const {
            return TValue::Undefined();
        }

        TValue operator()(bool value) const {
            return TValue::Integer(Factor * value);
        }

        TValue operator()(i64 value) const {
            return TValue::Integer(Factor * value);
        }

        TValue operator()(double value) const {
            return TValue::Double(Factor * value);
        }

        TValue operator()(const TText& value) const {
            if (Factor <= 0) {
                return TValue::String();
            }

            TText result;
            TTextOutput out(result);
            for (i64 i = 0; i < Factor; ++i) {
                out << value;
            }
            return TValue::String(std::move(result));
        }

        TValue operator()(const TValue::TListPtr& value) const {
            if (Factor <= 0) {
                return TValue::List();
            }

            const auto& list = value.Get();

            TValue::TList result;
            result.reserve(Factor * list.size());

            for (i64 i = 0; i < Factor; ++i) {
                Copy(list.begin(), list.end(), std::back_inserter(result));
            }

            return TValue::List(std::move(result));
        }

        TValue operator()(const TValue::TDictPtr&) const {
            return TValue::Undefined();
        }

        TValue operator()(const TValue::TRangePtr&) const {
            return TValue::Undefined();
        }

        TValue operator()(TValue::TNone) const {
            return TValue::Undefined();
        }
    };

    // integer multiplication may be numeric or it may be string/list replication
    auto integerMul = [&lhs, &rhs](i64 factor, const TValue& value) {
        auto result = std::visit(TMultiplyVisitor{factor}, value.GetData());
        if (result.IsUndefined()) {
            ThrowTypeError(TStringBuf("*"), lhs, rhs);
        }
        return result;
    };

    if (auto leftInt = ToInteger(lhs)) {
        return integerMul(*leftInt, rhs);
    }

    if (auto rightInt = ToInteger(rhs)) {
        return integerMul(*rightInt, lhs);
    }

    ThrowTypeError(TStringBuf("*"), lhs, rhs);
}

TValue TrueDiv(const TValue& lhs, const TValue& rhs) {
    // true division always casts to double
    auto left = ToDouble(lhs);
    auto right = ToDouble(rhs);
    if (!left || !right) {
        ThrowTypeError(TStringBuf("/"), lhs, rhs);
    }

    if (*right == 0) {
        ythrow TZeroDivisionError() << "True division by zero";
    }

    return TValue::Double(left.GetRef() / right.GetRef());
}

TValue FloorDiv(const TValue& lhs, const TValue& rhs) {
    // floor division casts to double if at least one of its operands is a double
    auto doubleFloorDiv = [](double left, double right) {
        if (right == 0) {
            ythrow TZeroDivisionError() << "Floor divison by zero";
        }

        return TValue::Double(std::floor(left / right));
    };

    if (lhs.IsDouble()) {
        auto right = ToDouble(rhs);
        if (!right) {
            ThrowTypeError(TStringBuf("//"), lhs, rhs);
        }

        return doubleFloorDiv(lhs.GetDouble(), *right);
    }

    if (rhs.IsDouble()) {
        auto left = ToDouble(lhs);
        if (!left) {
            ThrowTypeError(TStringBuf("//"), lhs, rhs);
        }

        return doubleFloorDiv(*left, rhs.GetDouble());
    }

    auto left = ToInteger(lhs);
    auto right = ToInteger(rhs);
    if (left && right) {
        if (*right == 0) {
            ythrow TZeroDivisionError() << "Floor division by zero";
        }
        return TValue::Integer(FloorDivInteger(*left, *right));
    }

    ThrowTypeError(TStringBuf("//"), lhs, rhs);
}

TValue Pow(const TValue& lhs, const TValue& rhs) {
    if (lhs.IsDouble() || rhs.IsDouble()) {
        auto left = ToDouble(lhs);
        auto right = ToDouble(rhs);

        if (!left || !right) {
            ThrowTypeError(TStringBuf("**"), lhs, rhs);
        }

        return TValue::Double(std::pow(*left, *right));
    }

    auto left = ToInteger(lhs);
    auto right = ToInteger(rhs);
    if (left && right) {
        if (right < 0) {
            return TValue::Double(std::pow(*left, *right));
        }

        return TValue::Integer(Power(*left, *right));
    }

    ThrowTypeError(TStringBuf("**"), lhs, rhs);
}

TValue Mod(const TValue& lhs, const TValue& rhs) {
    // modulo casts to double if at least one of its operands is a double
    auto doubleMod = [](double left, double right) {
        if (right == 0) {
            ythrow TZeroDivisionError() << "Modulo by zero";
        }

        return TValue::Double(ModDouble(left, right));
    };

    if (lhs.IsDouble()) {
        auto right = ToDouble(rhs);
        if (!right) {
            ThrowTypeError(TStringBuf("%"), lhs, rhs);
        }

        return doubleMod(lhs.GetDouble(), *right);
    }

    if (rhs.IsDouble()) {
        auto left = ToDouble(lhs);
        if (!left) {
            ThrowTypeError(TStringBuf("%"), lhs, rhs);
        }

        return doubleMod(*left, rhs.GetDouble());
    }

    auto left = ToInteger(lhs);
    auto right = ToInteger(rhs);
    if (left && right) {
        if (*right == 0) {
            ythrow TZeroDivisionError() << "Modulo by zero";
        }
        return TValue::Integer(ModInteger(*left, *right));
    }

    ThrowTypeError(TStringBuf("%"), lhs, rhs);
}

TValue Add(const TValue& lhs, const TValue& rhs) {
    if (lhs.IsString() && rhs.IsString()) {
        TText result = lhs.GetString();
        result.Append(rhs.GetString());
        return TValue::String(std::move(result));
    }

    if (lhs.IsList() && rhs.IsList()) {
        const auto& left = lhs.GetList();
        const auto& right = rhs.GetList();

        TValue::TList result;
        result.reserve(left.size() + right.size());
        Copy(left.begin(), left.end(), std::back_inserter(result));
        Copy(right.begin(), right.end(), std::back_inserter(result));
        return TValue::List(std::move(result));
    }

    if (lhs.IsDouble() || rhs.IsDouble()) {
        auto left = ToDouble(lhs);
        auto right = ToDouble(rhs);

        if (!left || !right) {
            ThrowTypeError(TStringBuf("+"), lhs, rhs);
        }

        return TValue::Double(left.GetRef() + right.GetRef());
    }

    auto left = ToInteger(lhs);
    auto right = ToInteger(rhs);
    if (left && right) {
        return TValue::Integer(left.GetRef() + right.GetRef());
    }

    ThrowTypeError(TStringBuf("+"), lhs, rhs);
}

TValue Sub(const TValue& lhs, const TValue& rhs) {
    if (lhs.IsDouble() || rhs.IsDouble()) {
        auto left = ToDouble(lhs);
        auto right = ToDouble(rhs);

        if (!left || !right) {
            ThrowTypeError(TStringBuf("-"), lhs, rhs);
        }

        return TValue::Double(left.GetRef() - right.GetRef());
    }

    auto left = ToInteger(lhs);
    auto right = ToInteger(rhs);
    if (left && right) {
        return TValue::Integer(left.GetRef() - right.GetRef());
    }

    ThrowTypeError(TStringBuf("-"), lhs, rhs);
}

TValue And(const TValue& lhs, const TValueProvider& rhs) {
    // lhs => need to compute rhs, return rhs
    // !lhs => no need to compute rhs, return lhs (= false)
    return TruthValue(lhs) ? rhs() : lhs;
}

TValue Or(const TValue& lhs, const TValueProvider& rhs) {
    // lhs => no need to compute rhs, return lhs (= true)
    // !lhs => need to compute rhs, return rhs
    return TruthValue(lhs) ? lhs : rhs();
}

//////////////////////////////////////////////////////////////////////
// Unary operators
//////////////////////////////////////////////////////////////////////

TValue UnaryMinus(const TValue& target) {
    if (target.IsInteger()) {
        return TValue::Integer(-target.GetInteger());
    }

    if (target.IsDouble()) {
        return TValue::Double(-target.GetDouble());
    }

    ythrow TTypeError() << "Invalid unary minus operand: " << target;
}

TValue UnaryPlus(const TValue& target) {
    if (target.IsInteger() || target.IsDouble()) {
        return target;
    }

    ythrow TTypeError() << "Invalid unary plus operand: " << target;
}

TValue UnaryNot(const TValue& target) {
    return TValue::Bool(!TruthValue(target));
}

// TODO(a-square): replace data copy with view creation
// TODO(a-square): support (UTF-8) strings?
TValue SliceList(const TValue& value, const TValue& start, const TValue& stop, const TValue& step) {
    if (!(value.IsList() || value.IsString())) {
        ythrow TValueError() << "slice only accepts lists and strings, got " << value.GetTypeName();
    }

    const size_t length = value.IsList() ? value.GetList().size()
                                         : GetNumberOfUTF8Chars(value.GetString().GetStr());

    // normalize start, stop, and step (requires knowledge of the list's size)
    i64 startInt = start.IsNone() ? 0 : start.GetInteger();
    i64 stopInt = stop.IsNone() ? length : stop.GetInteger();
    i64 stepInt = step.IsNone() ? 1 : step.GetInteger();

    if (value.IsString() && stepInt != 1) {
        ythrow TValueError() << "step values other than 1 are unsupported with strings";
    }

    // .. Python's indexing scheme applies to start and stop indices
    if (startInt < 0) {
        startInt += length;
    }

    if (stopInt < 0) {
        stopInt += length;
    }

    // after indices have been normalized, the difference between
    // a Python slice and a Python xrange disappears
    TRange range{startInt, stopInt, stepInt};

    if (value.IsString()) {
        return TValue::String(SubstrUTF8(value.GetString().GetStr(), startInt, range.GetSize()));
    }

    const auto& list = value.GetList();
    TValue::TList result;
    result.reserve(range.GetSize());

    for (i64 index : range) {
        // we check list size at creation so no UB can happen here
        if (index < 0 || static_cast<size_t>(index) >= length) {
            break;  // this is Python's behavior: stop at the first invalid index
        }

        result.push_back(list[index]);
    }

    return TValue::List(std::move(result));
}

} // namespace NAlice::NNlg::NOperators
