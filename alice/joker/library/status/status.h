#pragma once

#include <library/cpp/http/misc/httpcodes.h>

#include <util/generic/maybe.h>
#include <util/generic/variant.h>
#include <util/string/builder.h>

class THttpOutput;

namespace NAlice::NJoker {

class TError;
class TError {
public:
    enum class EType {
        Http /* "http" */,
        Logic /* "logic" */,
    };

public:
    TError() = default;
    TError(TError&& rhs)
        : Type{rhs.Type}
        , HttpCode{rhs.HttpCode}
    {
        ErrorMsg_.swap(rhs.ErrorMsg_);
    }

    TError(const TError& rhs)
        : Type{rhs.Type}
        , HttpCode{rhs.HttpCode}
    {
        ErrorMsg_ << rhs.ErrorMsg_;
    }

    explicit TError(HttpCodes httpCode)
        : HttpCode{httpCode}
    {
    }

    explicit TError(EType type)
        : Type{type}
    {
    }

    TError& operator=(const TError& rhs) {
        Type = rhs.Type;
        HttpCode = rhs.HttpCode;

        ErrorMsg_.clear();
        ErrorMsg_ << rhs.ErrorMsg_;
        return *this;
    }

    TError& operator=(TError&& rhs) {
        Type = rhs.Type;
        HttpCode = rhs.HttpCode;
        ErrorMsg_.swap(rhs.ErrorMsg_);
        return *this;
    }

    template <typename T>
    TError&& operator<<(const T& t) {
        ErrorMsg_ << t;
        return std::move(*this);
    }

    TError& SetHttpCode(HttpCodes httpCode) {
        HttpCode = httpCode;
        return *this;
    }

    const TError& HttpResponse(THttpOutput& out) const;

    TString AsString() const;

public:
    EType Type = EType::Http;
    HttpCodes HttpCode = HTTP_I_AM_A_TEAPOT;

private:
    TStringBuilder ErrorMsg_;
};

using TStatus = TMaybe<TError>;
inline TStatus Success() {
    return Nothing();
}

// FIXME remove it since it is not used
template <typename T>
class TExpected {
public:
    using TValue = T;

public:
    TExpected() = default;
    TExpected(const TExpected& rhs) = default;

    TExpected(T obj)
        : Result_(std::move(obj))
    {
    }

    TExpected(TError&& error);
    TExpected(TStatus&& status);

    explicit TExpected(TExpected<T>&& rhs)
        : Result_(std::move(rhs.Result_))
    {
    }

    TExpected<T>& operator=(TExpected<T>&& rhs) {
        Result_ = std::move(rhs.Result_);
        return *this;
    }

    TExpected<T>& SetUnexpected(TError&& error);

    TExpected<T>& operator=(TStatus&& status);

    TExpected<T>& SetValue(T&& t);
    TExpected<T>& operator=(T&& t);


    TStatus Error() const {
        if (const TError* error = std::get_if<TError>(&Result_)) {
            return *error;
        }

        return Nothing();
    }

    operator bool() const {
        return HasValue();
    }

    bool HasValue() const {
        return Result_.Defined() && Result_->index() == 0;
    }

    bool HasError() const;

    bool IsInitialized() const;

    T* operator->() {
        return Get();
    }

    const T* operator->() const {
        return Get();
    }

    const T* Get() const {
        return Result_.Defined() ? std::get_if<T>(&Result_) : nullptr;
    }

    T* Get() {
        return Result_.Defined() ? std::get_if<T>(&Result_) : nullptr;
    }

    void Swap(TExpected<T>& rhs) {
        Result_.Swap(rhs.Result_);
    }

private:
    TMaybe<std::variant<T, TError>> Result_;
};


template <typename T>
class TStoreResult {
public:
    explicit TStoreResult(T* result)
        : Result_(*result)
    {
    }

    TStatus operator()(TError&& error) const {
        return std::move(error);
    }
    TStatus operator()(T&& r) const {
        Result_ = std::forward<T>(r);
        return Success();
    }

private:
    T& Result_;
};

template <typename T>
TStoreResult<T> StoreTo(T* result) {
    return TStoreResult<T>{result};
}

template <typename T, typename V>
TStatus StoreTo(T* result, V&& variant) {
    return std::visit(StoreTo<T>(result), variant);
}

template <typename T>
using TErrorOr = std::variant<TError, T>;

} // namespace NAlice::NJoker
