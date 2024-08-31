#pragma once

#include <alice/cuttlefish/library/protos/megamind.pb.h>

namespace NAlice::NCuttlefish {
    void Censore(NAlice::TSpeechKitRequestProto&);
    inline void Censore(NAliceProtocol::TMegamindRequest& mmRequest) {
        if (mmRequest.HasRequestBase()) {
            Censore(*mmRequest.MutableRequestBase());
        }
    }
}
