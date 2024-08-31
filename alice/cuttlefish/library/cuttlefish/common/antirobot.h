#pragma once

#include <apphost/lib/proto_answers/http.pb.h>
#include <util/generic/strbuf.h>
#include <util/generic/maybe.h>

#include <alice/cuttlefish/library/protos/antirobot.pb.h>
#include <alice/cuttlefish/library/protos/session.pb.h>

#include "http_utils.h"


namespace NAlice::NCuttlefish::NAppHostServices {

class TAntirobotClient {
public:
    static TMaybe<NAppHostHttp::THttpRequest> CreateRequest(
        const NAliceProtocol::TSessionContext& ctx,
        const NAliceProtocol::TAntirobotInputSettings&,
        const NAliceProtocol::TAntirobotInputData&
    );

    static bool ParseResponseTo(const NAppHostHttp::THttpResponse&, NAliceProtocol::TRobotnessData*);
};

}  // namespace NAlice::NCuttlefish::NAppHostServices
