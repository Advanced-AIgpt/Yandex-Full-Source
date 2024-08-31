#pragma once
#include <apphost/lib/proto_answers/http.pb.h>


namespace NAlice::NCuttlefish::NAppHostServices {

    void AddHeader(NAppHostHttp::THttpRequest& dst, TString name, TString value);

    bool IsHeader(const NAppHostHttp::THeader& hdr, const TStringBuf name);

}  // namespace NAlice::NCuttlefish::NAppHostServices
