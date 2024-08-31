#pragma once

#include <alice/bass/forms/context/context.h>
#include <alice/library/network/headers.h>
#include <library/cpp/svnversion/svnversion.h>

namespace NBASS::NMusic {

struct TCommonHeaders {
    TString Authorization;
    TString UserIp;
    TString RequestId;
    TString MusicClient;
};

inline TCommonHeaders CreateCommonHeaders(const TContext& ctx) {
    return {
        .Authorization = ctx.UserAuthorizationHeader(),
        .UserIp = ctx.UserIP(),
        .RequestId = ctx.ReqId(),
        .MusicClient = TString::Join(TStringBuf("YandexAssistant/"), GetArcadiaLastChange()),
    };
}

inline void AddHeaders(const TCommonHeaders& commonHeaders, NHttpFetcher::TRequest* request) {
    request->AddHeader(NAlice::NNetwork::HEADER_AUTHORIZATION, commonHeaders.Authorization);
    request->AddHeader(NAlice::NNetwork::HEADER_X_FORWARDED_FOR, commonHeaders.UserIp);
    request->AddHeader(NAlice::NNetwork::HEADER_X_REQUEST_ID, commonHeaders.RequestId);
    request->AddHeader(NAlice::NNetwork::HEADER_X_YANDEX_MUSIC_CLIENT, commonHeaders.MusicClient);
}

} // namespace NBASS::NMusic
