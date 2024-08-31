#pragma once

#include <alice/cuttlefish/library/protos/context_load.pb.h>


namespace NAlice::NCuttlefish {
    void Censore(NAliceProtocol::TContextLoadResponse&);
    inline TString CensoredContextLoadResponseStr(const NAliceProtocol::TContextLoadResponse& origMessage) {
        NAliceProtocol::TContextLoadResponse censoredMessage(origMessage);
        Censore(censoredMessage);
        return censoredMessage.ShortUtf8DebugString();
    }
}
