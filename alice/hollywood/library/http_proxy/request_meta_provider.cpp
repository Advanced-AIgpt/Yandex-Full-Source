#include "request_meta_provider.h"

namespace NAlice::NHollywood {

TRequestMetaProvider::TRequestMetaProvider(const NScenarios::TRequestMeta& meta)
    : Meta_(meta)
{
}

const TString& TRequestMetaProvider::GetRequestId() const {
    return Meta_.GetRequestId();
}

const TString& TRequestMetaProvider::GetClientIP() const {
    return Meta_.GetClientIP();
}

const TString& TRequestMetaProvider::GetOAuthToken() const {
    return Meta_.GetOAuthToken();
}

const TString& TRequestMetaProvider::GetUserTicket() const {
    return Meta_.GetUserTicket();
}

} // namespace NAlice::NHollywood
