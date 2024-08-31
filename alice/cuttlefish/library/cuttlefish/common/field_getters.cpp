#include "field_getters.h"


namespace NAlice::NCuttlefish::NAppHostServices {

    const TString& GetUuid(const NAliceProtocol::TSessionContext& ctx) {
        const auto& info = ctx.GetUserInfo();
        if (info.HasVinsApplicationUuid()) {
            return info.GetVinsApplicationUuid();
        }
        return info.GetUuid();
    }

}  // namespace NAlice::NCuttlefish::NAppHostServices
