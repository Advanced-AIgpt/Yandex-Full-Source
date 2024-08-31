#pragma once

#include "variant.h"

#include <util/generic/function.h>
#include <util/generic/maybe.h>
#include <util/generic/variant.h>
#include <util/string/builder.h>

namespace NAlice {

template <typename TCode>
class TGenericError;

using TGenericErrorVoid = TGenericError<void>;

template<>
class TGenericError<void> {
public:
    TGenericError() = default;
    TGenericError(TGenericError&& error) = default;
    TGenericError(const TGenericError& error) {
        ErrorMsg_ << error.ErrorMsg_;
    }

    TGenericError& operator=(TGenericError&& error) = default;
    TGenericError& operator=(const TGenericError& error) {
        ErrorMsg_.clear();
        ErrorMsg_ << error.ErrorMsg_;
        return *this;
    }

    const TString& Message() const {
        return ErrorMsg_;
    }

private:
    TStringBuilder ErrorMsg_;

private:
    template <typename TCode, typename T>
    friend TGenericError<TCode>& operator<<(TGenericError<TCode>&, T&&);
    template <typename TCode, typename T>
    friend TGenericError<TCode>&& operator<<(TGenericError<TCode>&&, T&&);
};

template <typename TCode>
class TGenericError : public TGenericError<void> {
public:
    TGenericError() = default;
    TGenericError(TGenericError&& error) = default;
    TGenericError(const TGenericError& error) = default;

    explicit TGenericError(TCode code)
        : Code_(std::move(code))
    {
    }

    TGenericError& operator=(const TGenericError& error) = default;
    TGenericError& operator=(TGenericError&& error) = default;

    const TCode& Code() const {
        return Code_;
    }
private:
    TCode Code_;
};

template <typename TCode, typename T>
TGenericError<TCode>& operator<<(TGenericError<TCode>& genericError, T&& t) {
    genericError.ErrorMsg_ << std::forward<T>(t);
    return genericError;
}

template <typename TCode, typename T>
TGenericError<TCode>&& operator<<(TGenericError<TCode>&& genericError, T&& t) {
    genericError.ErrorMsg_ << std::forward<T>(t);
    return std::move(genericError);
}

template <typename TCode>
using TGenericStatus = TMaybe<TGenericError<TCode>>;

using TGenericStatusVoid = TGenericStatus<void>;

template <typename TCode, typename T>
class TGenericErrorOr {
public:
    using TErrorType = TGenericError<TCode>;
    using TSelfType = TGenericErrorOr<TCode, T>;
    using TType = std::variant<TErrorType, T>;

    struct TBadValue : public yexception {
        using yexception::yexception;
    };

public:
    TGenericErrorOr(const T& value)
        : Result_{value}
    {
    }

    TGenericErrorOr(T&& value)
        : Result_{std::move(value)}
    {
    }

    TGenericErrorOr(const TErrorType& error)
        : Result_{error}
    {
    }

    TGenericErrorOr(TErrorType&& error)
        : Result_{std::move(error)}
    {
    }

    TGenericErrorOr(const TSelfType& rhs) = default;
    TGenericErrorOr(TSelfType&& rhs) = default;

    TSelfType& operator=(const TSelfType& rhs) = default;
    TSelfType& operator=(TSelfType&& rhs) = default;

    bool IsSuccess() const noexcept {
        return std::get_if<TErrorType>(&Result_) == nullptr;
    }

    TGenericStatus<TCode> Status() const {
        if (const TErrorType* e = std::get_if<TErrorType>(&Result_)) {
            return *e;
        }

        return Nothing();
    }

    const TErrorType* GetError() const {
        return std::get_if<TErrorType>(&Result_);
    }

    TErrorType* GetError() {
        return std::get_if<TErrorType>(&Result_);
    }

    template <typename TFunc>
    auto AndThen(TFunc&& f) && {
        using TReturn = TFunctionResult<TFunc>;
        auto onError = [](TErrorType& error) -> TReturn {
            return std::move(error);
        };
        auto onType = [&f](T& obj) -> TReturn {
            return f(std::move(obj));
        };
        return std::visit(MakeLambdaVisitor(onError, onType), Result_);
    }

    template <typename TFunc>
    auto AndThen(TFunc&& f) const & {
        using TReturn = TFunctionResult<TFunc>;
        auto onError = [](const TErrorType& error) -> TReturn {
            return error;
        };
        auto onType = [&f](const T& obj) -> TReturn {
            return f(obj);
        };
        return std::visit(MakeLambdaVisitor(onError, onType), Result_);
    }

    template <typename TOnResult>
    TGenericStatus<TCode> OnResult(TOnResult&& onResult) {
        using TStatus = TGenericStatus<TCode>;

        auto onError = [](TErrorType& error) -> TStatus {
            return std::move(error);
        };
        auto onType = [&onResult](T& obj) -> TStatus {
            onResult(std::move(obj));
            return Nothing();
        };
        return std::visit(MakeLambdaVisitor(onError, onType), Result_);
    }

    TGenericStatus<TCode> MoveTo(T& result) {
        return OnResult([&result](T&& obj) { result = std::move(obj); });
    }

    const T* TryValue() const {
        return std::get_if<T>(&Result_);
    }

    T* TryValue() {
        return std::get_if<T>(&Result_);
    }

    const T& Value() const {
        return std::get<T>(Result_);
    }

    T& Value() {
        return std::get<T>(Result_);
    }

    const T& ValueOrThrow() const {
        const auto* value = TryValue();
        if (!value) {
            ythrow TBadValue{};
        }
        return *value;
    }

    T& ValueOrThrow() {
        auto* value = TryValue();
        if (!value) {
            ythrow TBadValue{};
        }
        return *value;
    }

    template <typename FF>
    friend decltype(auto) Visit(FF&& f, TGenericErrorOr<TCode, T>& v) {
        return ::std::visit(f, v.Result_);
    }

    template <typename FF>
    friend decltype(auto) Visit(FF&& f, const TGenericErrorOr<TCode, T>& v) {
        return ::std::visit(f, v.Result_);
    }

private:
    TType Result_;
};

template <typename T>
using TGenericErrorVoidOr = TGenericErrorOr<void, T>;

} // namespace NAlice
