#pragma once

#include <util/generic/noncopyable.h>
#include <util/generic/string.h>
#include <util/generic/maybe.h>
#include <util/string/builder.h>

namespace NAlice::NShooter {

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

    template<typename T>
    TError&& operator<<(const T& t) {
        ErrorMsg_ << t;
        return std::move(*this);
    }

    TString Msg() const {
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
inline bool IsError(const TStatus& status) {
    return status.Defined();
}

} // namespace NAlice::NShooter
