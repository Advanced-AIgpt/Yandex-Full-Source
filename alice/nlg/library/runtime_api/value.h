#pragma once

#include "exceptions.h"
#include "range.h"
#include "text.h"

#include <library/cpp/json/writer/json_value.h>

#include <util/generic/cast.h>
#include <util/generic/hash.h>
#include <util/generic/ptr.h>
#include <util/generic/string.h>
#include <util/generic/variant.h>
#include <util/generic/vector.h>
#include <util/generic/yexception.h>
#include <util/stream/output.h>
#include <util/system/type_name.h>

#include <cmath>
#include <type_traits>

namespace NAlice::NNlg {

class TValue {
public:
    struct TNone {
        bool operator==(TNone) const {
            return true;
        }
    };

    struct TUndefined {
        bool operator==(TUndefined) const {
            return true;
        }
    };

    using TList = TVector<TValue>;
    using TDict = THashMap<TString, TValue>; // dict keys are unbiased by design

    // TAtomicSharedPtr + operator==
    // TODO(a-square): we don't really need atomicity here, if we just don't increment/decrement
    // counts on frozen values we'll be fine. But then we'd need to ensure they're destroyed
    // and not leaked.
    // TODO(a-square): it's possible to create reference cycles in NLG,
    // we should implement arena-based cleanup to ensure no leaks after the request has been served.
    template <typename TValue>
    class TSharedPtr {
    public:
        TSharedPtr() = default;
        explicit TSharedPtr(const TValue& value)
            : Ptr(MakeAtomicShared<TValue>(value)) {
        }
        explicit TSharedPtr(TValue&& value)
            : Ptr(MakeAtomicShared<TValue>(std::move(value))) {
        }

        const TValue& Get() const {
            return *Ptr;
        }

        TValue& Get() {
            Y_ENSURE(!IsFrozen());
            return *Ptr;
        }

        void Freeze() {
            Frozen = true;
        }

        bool IsFrozen() const {
            return Frozen;
        }

        bool operator==(const TSharedPtr& other) const {
            return Ptr == other.Ptr || (Ptr && other.Ptr && *Ptr == *other.Ptr);
        }

    private:
        TAtomicSharedPtr<TValue> Ptr = nullptr;
        // NOTE(a-square): It's okay for Frozen to be a property of a pointer
        // instead of a value because by design the globals are frozen when
        // no copy of their pointers which aren't themselves globals can exist
        bool Frozen = false;
    };

    class TConstIterator {
    public:
        using iterator_category = std::forward_iterator_tag;
        using value_type = const TValue;
        using difference_type = i64;
        using pointer = const TValue*;
        using reference = const TValue&;

        struct TEndTag {};

    private:
        struct TUndefinedIter {
            bool operator==(TUndefinedIter) const {
                return true;
            }
        };

        using TIterImpl =
            std::variant<TUndefinedIter, TList::const_iterator, TDict::const_iterator, TRange::TConstIterator>;

    public:
        explicit TConstIterator(const TValue& value) {
            if (value.IsUndefined()) {
                IterImpl = TUndefinedIter{};
            } else if (value.IsList()) {
                IterImpl = value.GetList().begin();
            } else if (value.IsDict()) {
                IterImpl = value.GetDict().begin();
            } else if (value.IsRange()) {
                IterImpl = value.GetRange().begin();
            } else {
                ythrow TTypeError() << "Can only iterate over lists, dicts and undefined values";
            }
        }

        TConstIterator(const TValue& value, TEndTag) {
            if (value.IsUndefined()) {
                IterImpl = TUndefinedIter{};
            } else if (value.IsList()) {
                IterImpl = value.GetList().end();
            } else if (value.IsDict()) {
                IterImpl = value.GetDict().end();
            } else if (value.IsRange()) {
                IterImpl = value.GetRange().end();
            } else {
                ythrow TTypeError() << "Can only iterate over lists, dicts, ranges and undefined values";
            }
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

        // might return a temporary TValue so reference is out of question
        TValue operator*() const {
            struct {
                TValue operator()(TUndefinedIter) const {
                    ythrow TValueError() << "Cannot dereference iterator to undefined";
                }

                TValue operator()(TList::const_iterator it) const {
                    return *it;
                }

                TValue operator()(TDict::const_iterator it) const {
                    return TValue::String(it->first);
                }

                TValue operator()(TRange::TConstIterator it) const {
                    return TValue::Integer(*it);
                }
            } visitor;
            return std::visit(visitor, IterImpl);
        }

        bool operator==(const TConstIterator& other) const {
            return IterImpl == other.IterImpl;
        }

        bool operator!=(const TConstIterator& other) const {
            return !(*this == other);
        }

    private:
        void Next() {
            struct {
                void operator()(TUndefinedIter) const {
                }

                void operator()(TList::const_iterator& it) const {
                    ++it;
                }

                void operator()(TDict::const_iterator& it) const {
                    ++it;
                }

                void operator()(TRange::TConstIterator& it) const {
                    ++it;
                }
            } visitor;
            std::visit(visitor, IterImpl);
        }

    private:
        TIterImpl IterImpl;
    };

public:
    using TListPtr = TSharedPtr<TList>;
    using TDictPtr = TSharedPtr<TDict>;
    using TRangePtr = TSharedPtr<TRange>;
    using TData = std::variant<TUndefined, bool, i64, double, TText, TListPtr, TDictPtr, TRangePtr, TNone>;

public:
    TValue()
        : Data(TUndefined{}) {
    }

    explicit TValue(TUndefined value)
        : Data(value) {
    }

    explicit TValue(bool value)
        : Data(value) {
    }

    explicit TValue(i64 value)
        : Data(value) {
    }

    explicit TValue(double value)
        : Data(value) {
        if (!std::isfinite(value)) {
            ythrow TValueError() << "only finite double values supported";
        }
    }

    explicit TValue(const TText& value)
        : Data(value) {
    }

    explicit TValue(TText&& value)
        : Data(std::move(value)) {
    }

    explicit TValue(const TList& value) {
        // cannot really delegate to TValue(TList&& value)
        // because copying a huge list before checking its size
        // is a bad idea
        if (value.size() > static_cast<size_t>(Max<i64>())) {
            ythrow TValueError() << "The list is too large for indices to have a TValue::Integer representation";
        }
        Data = TListPtr(value);
    }

    explicit TValue(TList&& value) {
        if (value.size() > static_cast<size_t>(Max<i64>())) {
            ythrow TValueError() << "The list is too large for indices to have a TValue::Integer representation";
        }
        Data = TListPtr(std::move(value));
    }

    explicit TValue(const TDict& value)
        : Data(TDictPtr(value)) {
    }

    explicit TValue(TDict&& value)
        : Data(TDictPtr(std::move(value))) {
    }

    explicit TValue(TRange range)
        : Data(TRangePtr(range)) {
    }

    explicit TValue(TNone value)
        : Data(value) {
    }

    bool IsUndefined() const {
        return std::holds_alternative<TUndefined>(Data);
    }

    bool IsBool() const {
        return std::holds_alternative<bool>(Data);
    }

    bool IsInteger() const {
        return std::holds_alternative<i64>(Data);
    }

    bool IsDouble() const {
        return std::holds_alternative<double>(Data);
    }

    bool IsString() const {
        return std::holds_alternative<TText>(Data);
    }

    bool IsList() const {
        return std::holds_alternative<TListPtr>(Data);
    }

    bool IsDict() const {
        return std::holds_alternative<TDictPtr>(Data);
    }

    bool IsRange() const {
        return std::holds_alternative<TRangePtr>(Data);
    }

    bool IsNone() const {
        return std::holds_alternative<TNone>(Data);
    }

    bool GetBool() const {
        EnsureType<bool>();
        return std::get<bool>(Data);
    }

    i64 GetInteger() const {
        EnsureType<i64>();
        return std::get<i64>(Data);
    }

    double GetDouble() const {
        EnsureType<double>();
        return std::get<double>(Data);
    }

    const TText& GetString() const {
        EnsureType<TText>();
        return std::get<TText>(Data);
    }

    const TList& GetList() const {
        return GetShared<TList>();
    }

    TList& GetMutableList() {
        return GetMutableShared<TList>();
    }

    const TDict& GetDict() const {
        return GetShared<TDict>();
    }

    TDict& GetMutableDict() {
        return GetMutableShared<TDict>();
    }

    const TRange& GetRange() const {
        return GetShared<TRange>();
    }

    TRange& GetMutableRange() {
        return GetMutableShared<TRange>();
    }

    bool operator==(const TValue& other) const {
        return Data == other.Data;
    }

    bool operator!=(const TValue& other) const {
        return !(Data == other.Data);
    }

    TConstIterator begin() const {
        return TConstIterator(*this);
    }

    TConstIterator end() const {
        return TConstIterator(*this, TConstIterator::TEndTag{});
    }

    const TData& GetData() const {
        return Data;
    }

    TString GetTypeName() const {
        return std::visit([](auto x) { return TypeName<decltype(x)>(); }, Data);
    }

    void Freeze();
    bool IsFrozen() const;

public:
    static TValue ParseJson(TStringBuf str);
    static TValue FromJsonValue(const NJson::TJsonValue& value);

    struct TAsJson {
        const TValue& Value;
        bool PrintUndefined;
    };

    TAsJson AsJson(const bool printUndefined = false) const {
        return {*this, printUndefined};
    }

    static TValue Undefined() {
        return TValue{TUndefined{}};
    }

    static TValue Bool(bool value) {
        return TValue{value};
    }

    static TValue Integer(i64 value) {
        return TValue{value};
    }

    static TValue Double(double value) {
        return TValue{value};
    }

    static TValue String() {
        return TValue{TText{}};
    }

    // text's flagged spans behave just like xml tags so by default
    // whatever text value we create from a simple character or a string
    // is unbiased
    static TValue String(char value) {
        return TValue{TText{TString{value}}};
    }

    static TValue String(const char* value) {
        return TValue{TText{TString{value}}};
    }

    static TValue String(TStringBuf value) {
        return TValue{TText{TString{value}}};
    }

    static TValue String(const TString& value) {
        return TValue{TText{value}};
    }

    static TValue String(TString&& value) {
        return TValue{TText{std::move(value)}};
    }

    // text values created from actual text objects retain their flagged spans
    static TValue String(const TText& value) {
        return TValue{value};
    }

    static TValue String(TText&& value) {
        return TValue{std::move(value)};
    }

    static TValue List() {
        return TValue{TList{}};
    }

    static TValue List(const TList& value) {
        return TValue{value};
    }

    static TValue List(TList&& value) {
        return TValue{std::move(value)};
    }

    static TValue Dict() {
        return TValue{TDict{}};
    }

    static TValue Dict(const TDict& value) {
        return TValue{value};
    }

    static TValue Dict(TDict&& value) {
        return TValue{std::move(value)};
    }

    static TValue Range(TRange range) {
        return TValue{range};
    }

    static TValue None() {
        return TValue{TNone{}};
    }

private:
    template <typename TType>
    const TType& GetShared() const {
        using TPtr = TSharedPtr<TType>;
        EnsureType<TPtr>();
        return std::get<TPtr>(Data).Get();
    }

    template <typename TType>
    TType& GetMutableShared() {
        using TPtr = TSharedPtr<TType>;
        EnsureType<TPtr>();
        EnsureThawed();
        return std::get<TPtr>(Data).Get();
    }

    template <typename TType>
    void EnsureType() const {
        if (!std::holds_alternative<TType>(Data)) {
            ythrow TTypeError() << "Expected " << TypeName<TType>() << ", got " << GetTypeName();
        }
    }

    void EnsureThawed() const {
        if (IsFrozen()) {
            ythrow TValueFrozenError() << "Attempting to modify a frozen " << GetTypeName();
        }
    }

private:
    TData Data;
};

struct TRepr {
    const TValue& Value;
};

inline TRepr Repr(const TValue& value) {
    return TRepr{value};
}

// not a strict requirement, but it shouldn't balloon without a reason
// incorrect for TString == std::string
// static_assert(sizeof(TValue) == 32);

bool TruthValue(const TValue& value);

// derived exceptions are thrown by TValue::ParseJson when it can't successfully
// convert the given JSON string to a TValue without loss of data
class TParseJsonError : public yexception {};

// thrown by TValue::ParseJson when the given JSON string is not
// (a prefix of) a valid JSON string
class TJsonSyntaxError : public TParseJsonError {};

// thrown by TValue::ParseJson when an integer is too large to fit into an i64 value
class TJsonIntegerError : public TParseJsonError {};

TValue GetAttrLoad(const TValue& value, TStringBuf attr);
TValue& GetAttrStore(TValue& value, TStringBuf attr);
TValue GetItemLoadInt(const TValue& value, i64 index);
TValue GetItemLoad(const TValue& value, const TValue& index);

i64 GetLength(const TValue& value);

TMaybe<i64> ToInteger(const TValue& value);
TMaybe<double> ToDouble(const TValue& value);

} // namespace NAlice::NNlg

template <>
struct THash<NAlice::NNlg::TValue> {
    size_t operator()(const NAlice::NNlg::TValue& value) const;
};
