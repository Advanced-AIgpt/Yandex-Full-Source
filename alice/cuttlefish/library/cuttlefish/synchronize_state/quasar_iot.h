#pragma once
#include "utils.h"
#include <library/cpp/json/json_reader.h>
#include <library/cpp/string_utils/quote/quote.h>
#include <apphost/lib/proto_answers/http.pb.h>
#include <alice/cuttlefish/library/protos/context_load.pb.h>
#include <alice/cuttlefish/library/protos/session.pb.h>
#include <alice/cuttlefish/library/protos/events.pb.h>
#include <alice/cuttlefish/library/utils/string_utils.h>
#include <alice/megamind/protos/common/iot.pb.h>

#include <util/string/builder.h>
#include <util/string/ascii.h>


namespace NAlice::NCuttlefish::NAppHostServices {

struct TIoTUserConfig {
    bool Valid = false;
    double Longitude = 0.0;
    double Latitude = 0.0;
};

class TQuasarIoT {
public:
    static NAppHostHttp::THttpRequest CreateRequest(const NAliceProtocol::TSessionContext& sessionCtx, NAppHost::IServiceContext& /*serviceCtx*/)
    {
        NAppHostHttp::THttpRequest req;
        req.SetPath("/v1.0/user/info");
        AddHeader(req, "Accept", "application/protobuf");

        TMaybe<TString> deviceId;
        if (sessionCtx.HasDeviceInfo()) {
            const auto& info = sessionCtx.GetDeviceInfo();
            if (info.HasDeviceId()) {
                deviceId = info.GetDeviceId();
            }
        }

        if (deviceId.Defined()) {
             AddHeader(req, "X-Device-Id", CGIEscapeRet(*deviceId));
        }

        return req;
    }

    static TIoTUserConfig ParseIoTResponse(TStringBuf response) {
        TIoTUserConfig iotConfig;
        NAlice::TIoTUserInfo info;
        Y_PROTOBUF_SUPPRESS_NODISCARD info.ParseFromString(ToString(response));
        if (info.GetHouseholds().size() > 0) {
            iotConfig.Longitude = info.GetHouseholds()[0].GetLongitude();
            iotConfig.Latitude = info.GetHouseholds()[0].GetLatitude();
            iotConfig.Valid = true;
        }

        return iotConfig;
    }
};

}  // namespace NCuttlefish::NAppHostServices
