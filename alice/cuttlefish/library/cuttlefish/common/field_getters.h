#pragma once
#include <alice/cuttlefish/library/protos/session.pb.h>


namespace NAlice::NCuttlefish::NAppHostServices {

    const TString& GetUuid(const NAliceProtocol::TSessionContext& ctx);

}  // namespace NAlice::NCuttlefish::NAppHostServices
