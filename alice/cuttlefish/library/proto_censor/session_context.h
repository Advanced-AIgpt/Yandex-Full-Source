#pragma once

#include <alice/cuttlefish/library/protos/session.pb.h>

namespace NAlice::NCuttlefish {
    void Censore(NAliceProtocol::TSessionContext&);
    inline TString CensoredSessionContextStr(const NAliceProtocol::TSessionContext& ctxOrig) {
        NAliceProtocol::TSessionContext ctx(ctxOrig);
        Censore(ctx);
        return ctx.ShortUtf8DebugString();

    }

}
