#pragma once

#include <ydb/public/sdk/cpp/client/ydb_value/value.h>

#include <util/generic/maybe.h>
#include <util/generic/strbuf.h>
#include <util/generic/string.h>
#include <util/stream/output.h>
#include <util/stream/str.h>

#include <type_traits>

#include <google/protobuf/descriptor.h>

namespace google {
namespace protobuf {
class FieldDescriptor;
}
}

namespace NYdbHelpers {

TString EscapeYQL(TStringBuf s);

inline void EmitValue(IOutputStream& out, bool value) {
    out << (value ? "true" : "false");
}

inline void EmitValue(IOutputStream& out, i8 value) {
    out << static_cast<int>(value);
}

inline void EmitValue(IOutputStream& out, ui8 value) {
    out << static_cast<unsigned>(value);
}

inline void EmitValue(IOutputStream& out, TStringBuf value) {
    out << '"' << EscapeYQL(value) << '"';
}

inline void EmitValue(IOutputStream& out, const TString& value) {
    return EmitValue(out, TStringBuf{value});
}

template <typename T>
void EmitValue(IOutputStream& out, const T& value) {
    out << value;
}

template <typename T>
TString EmitValue(const T& value) {
    TString s;
    TStringOutput out(s);
    EmitValue(out, value);
    return s;
}

class TSepVisitor {
public:
    TSepVisitor(IOutputStream& out, TStringBuf sep)
        : Out(out)
        , Sep(sep)
    {
    }

    template <typename T>
    void operator()(const TMaybe<T>& value, const google::protobuf::FieldDescriptor& /* field */) {
        return (*this)(value);
    }

    template <typename T>
    void operator()(const T& value, const google::protobuf::FieldDescriptor& /* field */) {
        return (*this)(value);
    }

    template <typename T>
    void operator()(const T& value) {
        EmitSep();
        EmitValue(Out, value);
    }

    template <typename T>
    void operator()(const TMaybe<T>& value) {
        EmitSep();

        if (!value)
            Out << "null";
        else
            EmitValue(Out, *value);
    }

private:
    void EmitSep() {
        if (!First)
            Out << Sep;
        First = false;
    }

private:
    IOutputStream& Out;
    const TString Sep;
    bool First = true;
};

// This struct must be used only to estimate encoded protobuf size,
// not to get the exact size.
struct TProtobufSizeVisitor {
    template <typename T>
    void operator()(const TMaybe<T>& value, const google::protobuf::FieldDescriptor& field) {
        if (value)
            return (*this)(*value, field);
    }

    void operator()(const TString& value, const google::protobuf::FieldDescriptor& /* field */) {
        Size += value.size() + 2;
    }

    template <typename T>
    typename std::enable_if<std::is_integral<T>::value || std::is_floating_point<T>::value>::type
    operator()(T /* value */, const google::protobuf::FieldDescriptor& /* field */) {
        Size += sizeof(T) + 1;
    }

    size_t Size = 0;
};

struct TProtobufToYDBValueVisitor {
    explicit TProtobufToYDBValueVisitor(NYdb::TValueBuilder& builder)
        : Builder(builder) {
    }

#if defined(DECLARE_OPERATOR)
#error DECLARE_OPERATOR macro is already defined!
#endif

#define DECLARE_OPERATOR(CPP_TYPE, YDB_TYPE)                                                                          \
    void operator()(const TMaybe<CPP_TYPE>& value, const google::protobuf::FieldDescriptor& field) {                  \
        Builder.AddMember(field.name()).Optional##YDB_TYPE(value);                                                    \
    }                                                                                                                 \
    void operator()(const CPP_TYPE& value, const google::protobuf::FieldDescriptor& field) {                          \
        Builder.AddMember(field.name()).YDB_TYPE(value);                                                              \
    }

    DECLARE_OPERATOR(TString, String);
    DECLARE_OPERATOR(bool, Bool);
    DECLARE_OPERATOR(double, Double);
    DECLARE_OPERATOR(float, Float);
    DECLARE_OPERATOR(i32, Int32);
    DECLARE_OPERATOR(i64, Int64);
    DECLARE_OPERATOR(ui32, Uint32);
    DECLARE_OPERATOR(ui64, Uint64);

#undef DECLARE_OPERATOR

    NYdb::TValueBuilder& Builder;
};

} // namespace NYdbHelpers
