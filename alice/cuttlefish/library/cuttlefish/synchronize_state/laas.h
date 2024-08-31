#pragma once
#include "utils.h"
#include <library/cpp/json/json_reader.h>
#include <apphost/lib/proto_answers/http.pb.h>
#include <alice/cuttlefish/library/protos/context_load.pb.h>
#include <alice/cuttlefish/library/protos/session.pb.h>
#include <alice/cuttlefish/library/protos/events.pb.h>
#include <alice/cuttlefish/library/utils/string_utils.h>
#include <util/string/builder.h>
#include <util/string/ascii.h>


namespace NAlice::NCuttlefish::NAppHostServices {

class TLaas {
public:

    static void AddWifinetsToQuery(TStringBuilder& out, const NAliceProtocol::TSessionContext& sessionCtx)
    {
        bool started = false;
        for (const auto& it : sessionCtx.GetDeviceInfo().GetWifiNetworks()) {
            if (!(it.HasMac() && it.HasSignalStrength()))
                continue;
            if (!started) {
                out << "&wifinetworks=";
                started = true;
            } else {
                out << ",";
            }
            WriteWithSkipping(out, it.GetMac(), ':');
            out << ':' << it.GetSignalStrength();
        }
    }

    static NAppHostHttp::THttpRequest CreateRequest(
        const NAliceProtocol::TSessionContext& sessionCtx,
        const NAliceProtocol::TContextLoadLaasRequestOptions& options
    ) {
        const NAliceProtocol::TUserInfo& userInfo = sessionCtx.GetUserInfo();

        TStringBuilder path;
        path << "/region?real-ip=";

        if (const auto& connInfo = sessionCtx.GetConnectionInfo(); connInfo.HasPredefinedIpAddress()) {
            path << connInfo.GetPredefinedIpAddress();
        } else {
            path << connInfo.GetIpAddress();
        }

        path << "&uuid=" << userInfo.GetUuid();

        if (sessionCtx.GetClientType() == NAliceProtocol::TSessionContext::CLIENT_TYPE_QUASAR) {
            path << "&service=quasar";
        }

        if (userInfo.HasPuid()) {
            path << "&puid=" << userInfo.GetPuid();
        }

        AddWifinetsToQuery(path, sessionCtx);

        // https://st.yandex-team.ru/VOICESERV-3682
        if (options.GetUseCoordinatesFromIoT() && userInfo.HasLongitude() && userInfo.HasLatitude()) {
            path << "&lat=" << userInfo.GetLatitude()
                 << "&lon=" << userInfo.GetLongitude()
                 << "&location_accuracy=1"
                 << "&location_recency=0";
        }

        NAppHostHttp::THttpRequest req;
        req.SetMethod(NAppHostHttp::THttpRequest::Get);
        req.SetScheme(NAppHostHttp::THttpRequest::Http);
        req.SetPath(std::move(path));

        if (userInfo.HasYuid()) {
            AddHeader(req, "Cookie", TStringBuilder() << "yandexuid=" << userInfo.GetYuid());
        }

        return req;
    }

};

}  // namespace NAlice::NCuttlefish::NAppHostServices
