#pragma once

#include "vins.h"

namespace NBASS {

// Form with no slots that VINS sends to BASS when user start new session.
// New session is detected as return back to assistant after long break.
class TSessionStartFormHandler : public IHandler {
public:
    TSessionStartFormHandler();

    TResultValue Do(TRequestHandler& r) override;

    static void Register(THandlersMap* handlers);

    static TContext::TPtr SetAsResponse(TContext& ctx);
    static TContext::TPtr SetSessionStartAsResponse(TContext& ctx);
};

}
