#include "value.h"

#include <library/cpp/json/json_reader.h>
#include <library/cpp/json/json_writer.h>

#include <util/charset/utf8.h>
#include <util/digest/sequence.h>
#include <util/generic/stack.h>
#include <util/stream/mem.h>
#include <util/string/escape.h>
#include <util/system/yassert.h>

namespace NAlice::NNlg {

namespace {

struct TTruthValueVisitor {
    bool operator()(TValue::TUndefined) const {
        // for compatibility with the Python backend of Jinja2
        return false;
    }

    bool operator()(TValue::TNone) const {
        return false;
    }

    bool operator()(bool value) const {
        return value;
    }

    bool operator()(i64 value) const {
        return value != 0;
    }

    bool operator()(double value) const {
        return value != 0;
    }

    bool operator()(const TText& value) const {
        return !value.IsEmpty();
    }

    bool operator()(const TValue::TListPtr& value) const {
        return !value.Get().empty();
    }

    bool operator()(const TValue::TDictPtr& value) const {
        return !value.Get().empty();
    }

    bool operator()(const TValue::TRangePtr& value) const {
        return !value.Get().IsEmpty();
    }
};

class TJsonCallbacks : public NJson::TJsonCallbacks {
public:
    TJsonCallbacks()
        : NJson::TJsonCallbacks(/* throwException = */ true)
        , Value(TValue::List())
        , Stack({&Value.GetMutableList()}) {
    }

    bool OnNull() override {
        if (CurrentKey) {
            GetMutableMap().emplace(std::move(CurrentKey), TValue::None());
        } else {
            GetMutableList().push_back(TValue::None());
        }

        return true;
    }

    bool OnBoolean(bool value) override {
        if (CurrentKey) {
            GetMutableMap().emplace(std::move(CurrentKey), TValue::Bool(value));
        } else {
            GetMutableList().push_back(TValue::Bool(value));
        }

        return true;
    }

    bool OnInteger(long long value) override {
        if (CurrentKey) {
            GetMutableMap().emplace(std::move(CurrentKey), TValue::Integer(value));
        } else {
            GetMutableList().push_back(TValue::Integer(value));
        }

        return true;
    }

    bool OnUInteger(unsigned long long value) override {
        if (value > Max<i64>()) {
            ythrow TJsonIntegerError() << "Integer value too large: " << value;
        }

        if (CurrentKey) {
            GetMutableMap().emplace(std::move(CurrentKey), TValue::Integer(value));
        } else {
            GetMutableList().push_back(TValue::Integer(value));
        }

        return true;
    }

    bool OnDouble(double value) override {
        if (CurrentKey) {
            GetMutableMap().emplace(std::move(CurrentKey), TValue::Double(value));
        } else {
            GetMutableList().push_back(TValue::Double(value));
        }

        return true;
    }

    bool OnString(const TStringBuf& value) override {
        Y_ENSURE(IsUtf(value));  // elsewhere we just assume that all strings are UTF-8
        if (CurrentKey) {
            GetMutableMap().emplace(std::move(CurrentKey), TValue::String(TString{value}));
        } else {
            GetMutableList().push_back(TValue::String(TString{value}));
        }

        return true;
    }

    bool OnOpenMap() override {
        TValue::TDict* map = nullptr;

        if (CurrentKey) {
            auto [it, ok] = GetMutableMap().emplace(std::move(CurrentKey), TValue::Dict());
            map = &it->second.GetMutableDict();
        } else {
            auto& list = GetMutableList();
            list.push_back(TValue::Dict());
            map = &list.back().GetMutableDict();
        }

        Stack.push(map);

        return true;
    }

    bool OnMapKey(const TStringBuf& key) override {
        CurrentKey = key;
        return true;
    }

    bool OnCloseMap() override {
        Stack.pop();
        return true;
    }

    bool OnOpenArray() override {
        TValue::TList* arr = nullptr;

        if (CurrentKey) {
            auto [it, ok] = GetMutableMap().emplace(std::move(CurrentKey), TValue::List());
            arr = &it->second.GetMutableList();
        } else {
            auto& list = GetMutableList();
            list.push_back(TValue::List());
            arr = &list.back().GetMutableList();
        }

        Stack.push(arr);
        return true;
    }

    bool OnCloseArray() override {
        Stack.pop();
        return true;
    }

    bool OnEnd() override {
        Stack.pop(); // initially, the stack had one value
        return true;
    }

    TValue MoveValue() {
        Y_ASSERT(Stack.empty());
        auto& list = Value.GetMutableList();
        auto value = std::move(list.front());
        list.pop_back();
        return value;
    }

private:
    TValue::TList& GetMutableList() {
        return *reinterpret_cast<TValue::TList*>(Stack.top());
    }

    TValue::TDict& GetMutableMap() {
        return *reinterpret_cast<TValue::TDict*>(Stack.top());
    }

private:
    TValue Value;
    TString CurrentKey;
    TStack<void*> Stack;
};

} // namespace

class TValuePrinter {
public:
    explicit TValuePrinter(IOutputStream& out, bool repr = false)
        : Out(out)
        , Repr(repr) {
    }

    void operator()(TValue::TUndefined) {
        if (Repr) {
            Out << TStringBuf("Undefined");
        } else {
            // nothing by design
        }
    }

    void operator()(bool value) {
        Out << (value ? TStringBuf("True") : TStringBuf("False"));
    }

    void operator()(i64 value) {
        Out << value;
    }

    void operator()(double value) {
        Out << value;
    }

    void operator()(const TText& value) {
        if (Repr) {
            Out << '\'' << EscapeC(value.GetStr()) << '\'';
        } else {
            Out << value;
        }
    }

    void operator()(const TValue::TListPtr& list) {
        Repr = true;

        Out << '[';
        bool first = true;
        for (auto& value : list.Get()) {
            if (first) {
                first = false;
            } else {
                Out << TStringBuf(", ");
            }
            std::visit(*this, value.GetData());
        }
        Out << ']';
    }

    void operator()(const TValue::TDictPtr& dict) {
        Repr = true;

        Out << '{';
        bool first = true;
        for (auto& [key, value] : dict.Get()) {
            if (first) {
                first = false;
            } else {
                Out << TStringBuf(", ");
            }

            Out << '\'' << EscapeC(key) << TStringBuf("': ");
            std::visit(*this, value.GetData());
        }
        Out << '}';
    }

    void operator()(const TValue::TRangePtr& range) {
        Out << range.Get();
    }

    void operator()(const TValue::TNone) {
        Out << TStringBuf("None");
    }

private:
    IOutputStream& Out;
    bool Repr;
};

class TJsonPrinter {
public:
    explicit TJsonPrinter(IOutputStream& out, const bool printUndefined = false)
        : Out(out)
        , Writer(&Out, /* formatOutput = */ false, /* sortKeys = */ false, /* validateUtf8 = */ false)
        , PrintUndefined(printUndefined) {
    }

    void operator()(TValue::TUndefined) {
        if (PrintUndefined) {
            Writer.Write(TString{"<undefined>"});
        } else {
            ythrow TValueError() << "to_json doesn't accept undefined values";
        }
    }

    void operator()(bool value) {
        Writer.Write(value);
    }

    void operator()(i64 value) {
        Writer.Write(value);
    }

    void operator()(double value) {
        Writer.Write(value);
    }

    void operator()(const TText& value) {
        // XXX(a-square): no way to preserve flags
        Writer.Write(value.GetBounds());
    }

    void operator()(const TValue::TListPtr& list) {
        Writer.OpenArray();
        for (const auto& value : list.Get()) {
            std::visit(*this, value.GetData());
        }
        Writer.CloseArray();
    }

    void operator()(const TValue::TDictPtr& dict) {
        Writer.OpenMap();
        for (const auto& [key, value] : dict.Get()) {
            Writer.WriteKey(key);
            std::visit(*this, value.GetData());
        }
        Writer.CloseMap();
    }

    void operator()(const TValue::TRangePtr& range) {
        Writer.OpenArray();
        for (const i64 value : range.Get()) {
            Writer.Write(value);
        }
        Writer.CloseArray();
    }

    void operator()(const TValue::TNone) {
        Writer.WriteNull();
    }

private:
    IOutputStream& Out;
    NJson::TJsonWriter Writer;
    bool PrintUndefined;
};

bool TruthValue(const TValue& value) {
    return std::visit(TTruthValueVisitor{}, value.GetData());
}

TValue TValue::ParseJson(TStringBuf str) {
    TJsonCallbacks callbacks;
    try {
        // TODO(a-square): ReadJson is safer, but it's possible to switch
        // to ReadJsonFast (but only if we control the inputs).
        // It looks like library/cpp/scheme is already using ReadJsonFast.
        // As a bonus, ReadJsonFast doesn't need a stream wrapper
        TMemoryInput in(str);
        NJson::ReadJson(&in, &callbacks);
    } catch (const NJson::TJsonException& exc) {
        std::throw_with_nested(TJsonSyntaxError() << "Couldn't parse JSON: " << exc.AsStrBuf());
    }
    return callbacks.MoveValue();
}

TValue TValue::FromJsonValue(const NJson::TJsonValue& value) {
    using namespace NJson;

    // TODO(a-square): remove this after factoring TJsonValue out of Megamind's NLG wrapper
    switch (value.GetType()) {
        case JSON_UNDEFINED:
            return TValue::Undefined();
        case JSON_NULL:
            return TValue::None();
        case JSON_BOOLEAN:
            return TValue::Bool(value.GetBoolean());
        case JSON_INTEGER:
            return TValue::Integer(value.GetInteger());
        case JSON_DOUBLE:
            return TValue::Double(value.GetDouble());
        case JSON_STRING: {
            const auto& str = value.GetString();
            Y_ENSURE(IsUtf(str));  // elsewhere we just trust that we are given UTF-8 strings
            return TValue::String(str);
        }
        case JSON_MAP: {
            const auto& map = value.GetMap();
            TValue::TDict dict(map.size());
            dict.reserve(map.size());
            for (const auto& [key, element] : map) {
                dict[key] = FromJsonValue(element);
            }
            return TValue::Dict(std::move(dict));
        }
        case JSON_ARRAY: {
            const auto& array = value.GetArray();
            TValue::TList list(Reserve(array.size()));
            for (const auto& element : array) {
                list.push_back(FromJsonValue(element));
            }
            return TValue::List(std::move(list));
        }
        case JSON_UINTEGER: {
            ui64 intVal = value.GetUInteger();
            Y_ENSURE(intVal <= static_cast<ui64>(Max<i64>()));
            return TValue::Integer(intVal);
        }
    }
}

void TValue::Freeze() {
    if (IsDict()) {
        auto& dict = std::get<TDictPtr>(Data);
        auto& dictData = dict.Get();  // must get it before freezing
        dict.Freeze();
        for (auto& [key, value] : dictData) {
            value.Freeze();
        }
        return;
    }

    if (IsList()) {
        auto& list = std::get<TListPtr>(Data);
        auto& listData = list.Get();  // must get it before freezing
        list.Freeze();
        for (auto& item : listData) {
            item.Freeze();
        }
        return;
    }

    if (IsRange()) {
        std::get<TRangePtr>(Data).Freeze();
        return;
    }
}

bool TValue::IsFrozen() const {
    if (IsDict()) {
        return std::get<TDictPtr>(Data).IsFrozen();
    }

    if (IsList()) {
        return std::get<TListPtr>(Data).IsFrozen();
    }

    if (IsRange()) {
        return std::get<TRangePtr>(Data).IsFrozen();
    }

    return false;
}

TValue GetAttrLoad(const TValue& value, TStringBuf attr) {
    if (value.IsUndefined()) {
        return TValue::Undefined();
    }

    if (!value.IsDict()) {
        return TValue::Undefined();
    }

    auto& dict = value.GetDict();
    auto it = dict.find(attr);
    if (it == dict.end()) {
        return TValue::Undefined();
    }

    return it->second;
}

TValue& GetAttrStore(TValue& value, TStringBuf attr) {
    if (!value.IsDict()) {
        ythrow TTypeError() << "Can only store attributes (items) of a dictionary";
    }

    return value.GetMutableDict()[attr];
}

TValue GetItemLoadInt(const TValue& value, i64 index) {
    if (value.IsList()) {
        const auto& list = value.GetList();
        // this is how Python supports negative indices
        if (index < 0) {
            index += list.size();
        }
        if (index >= 0 && static_cast<ui64>(index) < list.size()) {
            return list[index];
        }
    } else if (value.IsRange()) {
        const auto& range = value.GetRange();
        if (index < 0) {
            index += range.GetSize();
        }
        if (auto item = range[index]) {
            return TValue::Integer(*item);
        }
    }

    return TValue::Undefined();
}

TValue GetItemLoad(const TValue& value, const TValue& index) {
    if (index.IsInteger()) {
        return GetItemLoadInt(value, index.GetInteger());
    }

    if (value.IsDict() && index.IsString()) {
        const auto& dict = value.GetDict();
        const auto& indexStr = index.GetString().GetStr(); // ignore text flags
        if (const auto* ptr = dict.FindPtr(indexStr)) {
            return *ptr;
        }
    }

    return TValue::Undefined();
}

i64 GetLength(const TValue& value) {
    if (value.IsUndefined()) {
        return 0;
    }

    if (value.IsList()) {
        return value.GetList().size();
    }

    if (value.IsDict()) {
        return value.GetDict().size();
    }

    if (value.IsRange()) {
        return value.GetRange().GetSize();
    }

    if (value.IsString()) {
        size_t size = GetNumberOfUTF8Chars(value.GetString().GetBounds());
        Y_ENSURE(size <= static_cast<size_t>(Max<i64>()));
        return static_cast<i64>(size);
    }

    ythrow TTypeError() << "GetLength only accepts lists, dicts, ranges, strings and undefined values";
}

TMaybe<i64> ToInteger(const TValue& value) {
    if (value.IsInteger()) {
        return value.GetInteger();
    }

    if (value.IsBool()) {
        return value.GetBool();
    }

    return Nothing();
}

TMaybe<double> ToDouble(const TValue& value) {
    if (value.IsDouble()) {
        return value.GetDouble();
    }

    if (value.IsInteger()) {
        return value.GetInteger();
    }

    if (value.IsBool()) {
        return value.GetBool();
    }

    return Nothing();
}

} // namespace NAlice::NNlg

template <>
void Out<NAlice::NNlg::TValue>(IOutputStream& out, TTypeTraits<NAlice::NNlg::TValue>::TFuncParam value) {
    NAlice::NNlg::TValuePrinter printer(out);
    std::visit(printer, value.GetData());
}

template <>
void Out<NAlice::NNlg::TRepr>(IOutputStream& out, TTypeTraits<NAlice::NNlg::TRepr>::TFuncParam repr) {
    NAlice::NNlg::TValuePrinter printer(out, /* repr = */ true);
    std::visit(printer, repr.Value.GetData());
}

template <>
struct THash<NAlice::NNlg::TValue::TList> {
    size_t operator()(const NAlice::NNlg::TValue::TList& list) const {
        return TSimpleRangeHash()(list);
    }
};

template <>
struct THash<NAlice::NNlg::TValue::TDict> {
    size_t operator()(const NAlice::NNlg::TValue::TDict& dict) const {
        return TSimpleRangeHash()(dict);
    }
};

size_t THash<NAlice::NNlg::TValue>::operator()(const NAlice::NNlg::TValue& value) const {
    using NAlice::NNlg::TRange;
    using NAlice::NNlg::TValue;

    struct {
        size_t operator()(TValue::TUndefined) const {
            return 53299; // random large-ish prime
        }

        size_t operator()(bool value) const {
            return THash<bool>()(value);
        }

        size_t operator()(i64 value) const {
            return THash<i64>()(value);
        }

        size_t operator()(double value) const {
            return THash<double>()(value);
        }

        size_t operator()(const NAlice::NNlg::TText& value) const {
            return THash<NAlice::NNlg::TText>()(value);
        }

        size_t operator()(const TValue::TListPtr& value) {
            return THash<TValue::TList>()(value.Get());
        }

        size_t operator()(const TValue::TDictPtr& value) {
            return THash<TValue::TDict>()(value.Get());
        }

        size_t operator()(const TValue::TRangePtr& value) {
            return THash<TRange>()(value.Get());
        }

        size_t operator()(TValue::TNone) const {
            return 72139; // random large-ish prime
        }
    } visitor;

    return std::visit(visitor, value.GetData());
};

template <>
void Out<NAlice::NNlg::TValue::TAsJson>(IOutputStream& out, TTypeTraits<NAlice::NNlg::TValue::TAsJson>::TFuncParam wrapper) {
    NAlice::NNlg::TJsonPrinter printer(out, wrapper.PrintUndefined);
    std::visit(printer, wrapper.Value.GetData());
}
