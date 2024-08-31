#pragma once

#include <apphost/lib/proto_answers/http.pb.h>

namespace NAlice::NCuttlefish::NAppHostServices {

class TTvmTool {
public:
    static NAppHostHttp::THttpRequest CheckServiceTicket(const TStringBuf serviceTicket);
};

}  // namespace NAlice::NCuttlefish::NAppHostServices
