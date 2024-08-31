#include <alice/bass/forms/market/client.h>
#include <alice/bass/forms/market/client/base_client.h>
#include <alice/bass/util/error.h>

namespace NBASS {

namespace NMarket {

namespace {

// TODO: Дубликация кода с THttpException пропадет после переноса клиентов из market_choice на новый стиль
TError GetResponseError(const NHttpFetcher::TResponse::TRef& response)
{
    Y_ASSERT(response->IsError());
    TStringBuf errorMessage;
    if (response->IsTimeout()) {
        errorMessage = TStringBuf("Timeout error: ");
    } else if (!response->IsHttpOk()) {
        errorMessage = TStringBuf("Response code is not 200: ");
    } else {
        errorMessage = TStringBuf("cannot fetch data: ");
    }
    return TError(
        NBASS::TError::EType::MARKETERROR,
        TStringBuilder() << errorMessage << response->GetErrorText());
}

}

TBaseResponse::TBaseResponse(const NHttpFetcher::TResponse::TRef response)
{
    if (response->IsTimeout()) {
        ythrow THttpTimeoutException(TStringBuilder() << TStringBuf("Timeout error: ") << response->GetErrorText());
    } else if (response->IsError()) {
        LOG(ERR) << "Can not get data from market" << Endl;
        Error = GetResponseError(response);
        return;
    }
    RawData.ConstructInPlace(std::move(response->Data));
}

const TError& TBaseResponse::GetError() const
{
    return Error.GetRef();
}

bool TBaseResponse::HasError() const
{
    return Error.Defined();
}

TBaseJsonResponse::TBaseJsonResponse(const NHttpFetcher::TResponse::TRef response)
    : TBaseResponse(response)
{
    if (!HasError()) {
        NSc::TValue data = NSc::TValue::FromJson(RawData.GetRef());
        if (data.IsNull()) {
            LOG(ERR) << "Can not parse data from market" << Endl;
            Error = TError(
                NBASS::TError::EType::MARKETERROR,
                TStringBuf("cannot_parse_data"));
            return;
        }
        if (!data["error"].IsNull()) {
            LOG(ERR) << "Interal Market Error: " << RawData << Endl;
            Error = TError(
                NBASS::TError::EType::MARKETERROR,
                TStringBuf("internal_error"));
            return;
        }
        Data.ConstructInPlace(std::move(data));
    }
}

////////////////////////////////////////////////////////////////////////////////

TReportRequest::TReportRequest(
        NHttpFetcher::TRequestPtr httpRequest,
        TStringBuf place,
        EMarketType marketType)
    : HttpRequest(std::move(httpRequest))
    , Handle(HttpRequest->Fetch())
    , Place(place)
    , MarketType(marketType)
{
}

TReportResponse TReportRequest::Wait()
{
    return TReportResponse(Handle->Wait(), Place, MarketType);
}

} // namespace NMarket

} // namespace NBASS
