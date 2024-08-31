#pragma once

#include <alice/bass/libs/fetcher/request.h>

#include <library/cpp/http/misc/httpcodes.h>
#include <library/cpp/json/json_value.h>

#include <util/generic/maybe.h>
#include <util/generic/variant.h>
#include <util/string/builder.h>

namespace NAlice {

class IHttpResponse;

class TError {
public:
    enum class EType {
        BadRequest /* "bad_request" */,
        Begemot /* "begemot" */,
        Biometry /* "biometry" */,
        DataError /* "data_error" */,
        Empty /* "empty" */,
        Exception /* "exception" */,
        Http /* "http" */,
        Logic /* "logic" */,
        Input /* "input" */,
        ModifierError /* "modifier_error" */,
        NetworkError /* "network_error" */,
        NLG /* "nlg" */,
        System /* "system" */,
        TimeOut /* "timeout" */,
        Unauthorized /* "unauthorized" */,
        UniProxy /* "uniproxy" */,
        ScenarioError /* "scenario_error" */,
        Critical /* "critical" */,
        NotFound /* "not_found" */,
        Parse /* "parse" */,
        VersionMismatch /* "version_mismatch" */,
    };

public:
    ~TError() = default;
    TError() = default;
    TError(TError&& error) = default;
    TError(const TError& error)
        : Type(error.Type)
        , HttpCode(error.HttpCode)
    {
        ErrorMsg << error.ErrorMsg;
    }

    explicit TError(HttpCodes httpCode)
        : HttpCode(httpCode)
    {
    }

    explicit TError(EType type, TStringBuf errorMsg = {})
        : Type{type}
    {
        ErrorMsg << errorMsg;
    }

    TError& operator=(const TError& error) {
        Type = error.Type;
        HttpCode = error.HttpCode;

        ErrorMsg.clear();
        ErrorMsg.reserve(error.ErrorMsg.size());
        ErrorMsg << error.ErrorMsg;
        return *this;
    }

    // FIXME (petrk@): Find out how this works with TStringBuilder lacking move assignment operator.
    // (Falling back on copy assignment?)
    TError& operator=(TError&& error) = default;

    template <typename T>
    TError&& operator<<(const T& t) {
        ErrorMsg << t;
        return std::move(*this);
    }

public:
    EType Type = EType::Http;
    TMaybe<HttpCodes> HttpCode;
    TStringBuilder ErrorMsg;
};

using TStatus = TMaybe<TError>;

inline TStatus Success() {
    return Nothing();
}

TError ErrorFromResponseResult(NHttpFetcher::TResponse::EResult result, const TString& msg,
                               NHttpFetcher::TResponse::THttpCode httpCode);

template <typename T>
class TErrorOr {
public:
    using TSelfType = TErrorOr<T>;
    using TType = std::variant<TError, T>;

    struct TBadValue : public yexception {
        using yexception::yexception;
    };

public:
    TErrorOr(const T& value)
        : Result_{value}
    {
    }

    TErrorOr(T&& value)
        : Result_{std::move(value)}
    {
    }

    TErrorOr(const TError& error)
        : Result_{error}
    {
    }

    TErrorOr(TError&& error)
        : Result_{std::move(error)}
    {
    }

    TErrorOr(const TSelfType& rhs) = default;
    TErrorOr(TSelfType&& rhs) = default;

    TSelfType& operator=(const TSelfType& rhs) = default;
    TSelfType& operator=(TSelfType&& rhs) = default;

    bool IsSuccess() const noexcept {
        return std::get_if<TError>(&Result_) == nullptr;
    }

    TStatus Status() const {
        if (const TError* e = std::get_if<TError>(&Result_)) {
            return *e;
        }

        return Success();
    }

    const TError* Error() const {
        return std::get_if<TError>(&Result_);
    }

    TError* Error() {
        return std::get_if<TError>(&Result_);
    }

    TStatus MoveTo(T& result) {
        struct TVisitor {
            explicit TVisitor(T& dst)
                : Dst{dst}
            {
            }

            TStatus operator()(TError& error) {
                return std::move(error);
            }
            TStatus operator()(T& obj) {
                Dst = std::move(obj);
                return Success();
            }

            T& Dst;
        };
        return std::visit(TVisitor(result), static_cast<std::variant<TError, T>&>(Result_));
    }

    TStatus MoveTo(TMaybe<T>& result) {
        struct TVisitor {
            explicit TVisitor(TMaybe<T>& dst)
                : Dst{dst}
            {
            }

            TStatus operator()(TError& error) {
                return std::move(error);
            }
            TStatus operator()(T& obj) {
                Dst = std::move(obj);
                return Success();
            }

            TMaybe<T>& Dst;
        };
        return std::visit(TVisitor(result), static_cast<std::variant<TError, T>&>(Result_));
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

    template <typename FF> friend decltype(auto) Visit(FF&& f, TErrorOr<T>& v) {
        return std::visit(f, v.Result_);
    }
    template <typename FF> friend decltype(auto) Visit(FF&& f, const TErrorOr<T>& v) {
        return std::visit(f, v.Result_);
    }

private:
    TType Result_;
};

} // namespace NAlice
