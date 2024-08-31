#pragma once

#include <alice/cuttlefish/library/cuttlefish/common/metrics.h>
#include <alice/cuttlefish/library/logging/log_context.h>
#include <apphost/lib/proto_answers/http.pb.h>
#include <util/generic/maybe.h>
#include <util/generic/strbuf.h>

namespace NAlice::NCuttlefish::NAppHostServices {

class TBlackboxClient {
public:
    struct TOAuthResponse {
        bool Valid = false;
        TString Uid;
        TString StaffLogin;
        TString UserTicket;
        bool IsBetaTester;
    };

    static NAppHostHttp::THttpRequest GetUidForOAuth(TStringBuf token, TStringBuf userIp);
    static NAppHostHttp::THttpRequest GetUidForSessionId(TStringBuf sessionId, TStringBuf userIp, TStringBuf origin);
    static TOAuthResponse ParseResponse(TStringBuf response);
};

class TBlackboxResponseParser {
public:
    TBlackboxResponseParser(TStringBuf itemType);

    TMaybe<TBlackboxClient::TOAuthResponse> TryParse(
        const NAppHost::IServiceContext& ahContext,
        TSourceMetrics& metrics,
        TLogContext logContext) const;

private:
    TStringBuf ItemType;
};

}  // namespace NAlice::NCuttlefish::NAppHostServices
