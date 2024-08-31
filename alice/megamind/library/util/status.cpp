#include "status.h"

#include <util/generic/strbuf.h>

namespace NAlice {

TError ErrorFromResponseResult(NHttpFetcher::TResponse::EResult result, const TString& msg,
                               NHttpFetcher::TResponse::THttpCode httpCode) {
    switch (result) {
        case NHttpFetcher::TResponse::EResult::Timeout:
            return TError{TError::EType::TimeOut} << msg;
        case NHttpFetcher::TResponse::EResult::HttpError:
            return TError{TError::EType::Http} << msg;
        case NHttpFetcher::TResponse::EResult::DataError:
            // DataError can rewrite http errors in fetcher. We should check http code ( != 0).
            // https://a.yandex-team.ru/arc/trunk/arcadia/alice/bass/libs/fetcher/neh_detail.cpp?rev=6982997#L650
            if (httpCode != 0) {
                return TError{TError::EType::Http} << msg;
            }
            return TError{TError::EType::DataError} << msg;
        case NHttpFetcher::TResponse::EResult::ParsingError:
            return TError{TError::EType::Parse} << msg;
        case NHttpFetcher::TResponse::EResult::EmptyResponse:
            return TError{TError::EType::Empty} << msg;
        case NHttpFetcher::TResponse::EResult::NetworkResolutionError:
            return TError{TError::EType::NetworkError} << msg;
        case NHttpFetcher::TResponse::EResult::Ok:
            return TError{TError::EType::Logic} << msg;
    }
}
} // namespace NAlice

template <>
void Out<NAlice::TError>(IOutputStream& out, const NAlice::TError& error) {
    out << error.ErrorMsg << TStringBuf(" (") << error.Type;
    if (error.HttpCode.Defined()) {
        out << TStringBuf(", ") << error.HttpCode;
    }
    out << ')';
}
