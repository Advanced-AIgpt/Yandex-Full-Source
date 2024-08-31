#pragma once

#include <library/cpp/scheme/scheme.h>

#include <util/generic/strbuf.h>
#include <util/string/builder.h>

namespace NBASS {
template <typename TType>
struct TGenericError {
    using EType = TType;

    explicit TGenericError(EType type);
    TGenericError(const TGenericError& error);
    TGenericError(TGenericError&& rhs);
    explicit TGenericError(const NSc::TValue& value);
    TGenericError(EType type, TStringBuf msg);

    TGenericError& operator=(const TGenericError& rhs);

    bool operator==(EType type) const {
        return Type == type;
    }

    bool operator==(const TGenericError& rhs) const {
        return Type == rhs.Type;
    }

    void ToJson(NSc::TValue& out) const;
    NSc::TValue ToJson() const;

    EType Type;
    TStringBuilder Msg;
};

template <typename TType>
TGenericError<TType>::TGenericError(EType type)
    : Type(type) {
}

template <typename TType>
TGenericError<TType>::TGenericError(EType type, TStringBuf msg)
    : TGenericError(type) {
    Msg << msg;
}

template <typename TType>
TGenericError<TType>::TGenericError(const TGenericError& error)
    : TGenericError(error.Type) {
    Msg << error.Msg;
}

template <typename TType>
TGenericError<TType>::TGenericError(TGenericError&& rhs)
    : TGenericError(rhs.Type) {
    Msg.swap(rhs.Msg);
}

template <typename TType>
TGenericError<TType>::TGenericError(const NSc::TValue& value) {
    const NSc::TValue error(value["error"]);
    Type = FromString<EType>(error["type"]);
    Msg << error["msg"];
}

template <typename TType>
TGenericError<TType>& TGenericError<TType>::operator=(const TGenericError& rhs) {
    Type = rhs.Type;
    Msg.clear();
    Msg << rhs.Msg;
    return *this;
}

template <typename TType>
void TGenericError<TType>::ToJson(NSc::TValue& out) const {
    out["error"]["type"].SetString(ToString(Type));
    out["error"]["msg"].SetString(Msg);
}

template <typename TType>
NSc::TValue TGenericError<TType>::ToJson() const {
    NSc::TValue out;
    ToJson(out);
    return out;
}
} // namespace NBASS
