#pragma once

#include "blackbox.h"

#include <alice/bass/libs/fetcher/request.h>

#include <util/generic/variant.h>

namespace NAlice {

class TBlackBoxHttpFetcher {
private:
    using THandleRef = NHttpFetcher::THandle::TRef;
    using TResponseRef = NHttpFetcher::TResponse::TRef;
    using TState = std::variant<TBlackBoxError, THandleRef, TResponseRef>;

public:
    TBlackBoxHttpFetcher();

    TBlackBoxStatus StartRequest(NHttpFetcher::TRequest& request, TStringBuf userIp, const TMaybe<TString>& authToken);

    TBlackBoxErrorOr<TString> Response();

    TBlackBoxErrorOr<TBlackBoxApi::TFullUserInfo> GetFullInfo();
    TBlackBoxErrorOr<TString> GetUid();
    TBlackBoxErrorOr<TString> GetTVM2UserTicket();

private:
    TBlackBoxErrorOr<TString> ResponseContent();

private:
    TState State_;
};

} // namespace NAlice::NBlackBox
