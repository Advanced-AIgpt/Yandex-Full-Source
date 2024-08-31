#include "socialism.h"

#include <alice/bass/libs/fetcher/request.h>
#include <alice/bass/libs/logging_v2/logger.h>
#include <alice/bass/libs/source_request/handle.h>

#include <util/generic/string.h>
#include <util/string/builder.h>

namespace NBASS {
namespace {

class TFetchProviderTokenHandle : public IRequestHandle<TString> {
public:
    TFetchProviderTokenHandle(NHttpFetcher::TRequestPtr req, TStringBuf passportUid, TStringBuf applicationName)
        : ApplicationName(applicationName)
    {
        req->AddCgiParam(TStringBuf("uid"), passportUid);
        req->AddCgiParam(TStringBuf("application_name"), ApplicationName);
        Handle = req->Fetch();
    }

    TResultValue WaitAndParseResponse(TString* response) override {
        NHttpFetcher::TResponse::TRef httpResponse = Handle->Wait();
        if (httpResponse->IsError()) {
            TString err = TStringBuilder() << "Failed to obtain token for application : " << ApplicationName << ": " << httpResponse->GetErrorText();
            LOG(ERR) << err << Endl;
            return TError(TError::EType::SOCIALISMERROR, err);
        }

        NSc::TValue json = NSc::TValue::FromJson(httpResponse->Data);
        const NSc::TValue& token = json["token"];
        if (!token.IsDict()) {
            LOG(WARNING) << "No token available for application " << ApplicationName << " at SocialAPI for current user" << Endl;
            return TResultValue();
        }
        *response = token["value"].GetString();
        return TResultValue();
    }

private:
    const TStringBuf ApplicationName;
    NHttpFetcher::THandle::TRef Handle;
};

} // namespace

TSocialismRequest FetchSocialismToken(NHttpFetcher::TRequestPtr req, TStringBuf passportUid, TStringBuf applicationName) {
    return std::make_unique<TFetchProviderTokenHandle>(std::move(req), passportUid, applicationName);
}

} // namespace NBASS
