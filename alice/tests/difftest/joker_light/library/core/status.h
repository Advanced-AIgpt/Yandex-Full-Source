#pragma once

#include "http_context.h"

#include <util/generic/maybe.h>
#include <util/generic/noncopyable.h>
#include <util/generic/variant.h>
#include <util/string/builder.h>

namespace NAlice::NJokerLight {

class TError {
public:
    enum class EType {
        Http,
        Logic
    };

public:
    TError() = default;

    TError(TError&& rhs)
        : Type{rhs.Type}
    {
        ErrorMsg_.swap(rhs.ErrorMsg_);
    }

    TError(const TError& rhs)
        : Type{rhs.Type}
    {
        ErrorMsg_ << rhs.ErrorMsg_;
    }

    TError& operator=(const TError& rhs) {
        Type = rhs.Type;
        ErrorMsg_.clear();
        ErrorMsg_ << rhs.ErrorMsg_;
        return *this;
    }

    TError& operator=(TError&& rhs) {
        Type = rhs.Type;
        ErrorMsg_.swap(rhs.ErrorMsg_);
        return *this;
    }

    template<typename T>
    TError&& operator<<(const T& t) {
        ErrorMsg_ << t;
        return std::move(*this);
    }

    TString AsString() const {
        return ErrorMsg_;
    }

public:
    EType Type = EType::Http;

private:
    TStringBuilder ErrorMsg_;
};

using TStatus = TMaybe<TError>;
inline TStatus Success() {
    return Nothing();
}

template <typename T>
using TErrorOr = std::variant<TError, T>;

inline bool IsError(const TStatus& status) {
    return status.Defined();
}

} // namespace NAlice::NJokerLight
