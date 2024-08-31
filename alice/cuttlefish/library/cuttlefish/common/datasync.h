#pragma once
#include "http_utils.h"
#include <apphost/lib/proto_answers/http.pb.h>
#include <library/cpp/json/json_value.h>
#include <util/generic/strbuf.h>
#include <util/generic/maybe.h>


namespace NAlice::NCuttlefish::NAppHostServices {

class TDatasyncClient {
public:
    struct TPersonalSettings {
        TMaybe<bool> DoNotUseUserLogs = Nothing();
    };

    static inline void AddUuidHeader(NAppHostHttp::THttpRequest& req, TStringBuf uuid) {
        AddHeader(req, "X-Uid", TString::Join("uuid:", uuid));
    }

    static inline void AddDeviceIdHeader(NAppHostHttp::THttpRequest& req, TStringBuf uuid) {
        // NOTE: by historical reasons it's not a real device's ID but UUID
        AddHeader(req, "X-Uid", TString::Join("device_id:", uuid));
    }

    static inline void AddUserTicketHeader(NAppHostHttp::THttpRequest& req, TStringBuf userTicket) {
        AddHeader(req, "X-Ya-User-Ticket", TString(userTicket));
    }

    static inline void AddUidHeader(NAppHostHttp::THttpRequest& req, TStringBuf uid) {
        AddHeader(req, "X-Uid", TString(uid));
    }

    static NAppHostHttp::THttpRequest LoadVinsContextsRequest();
    static NAppHostHttp::THttpRequest SaveVinsContextsRequest(const TVector<NJson::TJsonValue>& payloads);

    static NJson::TJsonValue MakeVinsContextsResponseContent(
        TMaybe<TString> addressesResponse,
        TMaybe<TString> keyValueResponse,
        TMaybe<TString> settingsResponse
    );

    static NAppHostHttp::THttpRequest LoadPersonalSettingsRequest();
    static TPersonalSettings ParsePersonalSetingsResponse(TStringBuf response);
};

}  // namespace NAlice::NCuttlefish::NAppHostServices
