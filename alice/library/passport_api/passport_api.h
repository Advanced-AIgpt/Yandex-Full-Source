#pragma once

#include <alice/bass/libs/fetcher/request.h>

#include <util/generic/maybe.h>
#include <util/generic/string.h>
#include <util/system/yassert.h>

namespace NAlice {

class TContext;

class TPassportAPI {
public:
    struct TError {
        enum class EType {
            // Something is wrong with environment.
            InternalError,
            // Not even possible to create a valid request.
            BadParams,
            // Passport does not respond.
            NoResponse,
            // Something is wrong with passport response.
            ResponseError
        };

        explicit TError(EType type)
            : Type(type) {
        }

        TError(EType type, const TString& message)
            : Type(type)
            , Message(message) {
        }

        EType Type;
        TString Message;
    };

    struct TResult {
        TResult() = default;

        explicit TResult(const TError::EType type)
            : Error(TError(type)) {
        }

        explicit TResult(const TError& error)
            : Error(error) {
        }

        TResult(const TString& uid, const TString& code)
            : UID(uid)
            , Code(code) {
            Y_ASSERT(!uid.empty());
            Y_ASSERT(!code.empty());
        }

        TMaybe<TError> Error;

        // Following fields are non-empty when !Error.
        TString UID;
        TString Code;
    };

    virtual ~TPassportAPI() = default;

    virtual TResult RegisterKolonkish(NHttpFetcher::TRequestPtr request, TStringBuf consumer,
                                      TStringBuf userAuthorizationHeader, TStringBuf userIP);
};

} // namespace NAlice
