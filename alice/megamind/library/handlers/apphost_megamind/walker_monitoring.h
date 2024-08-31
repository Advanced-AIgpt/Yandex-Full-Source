#pragma once

#include "node.h"

#include <alice/megamind/library/apphost_request/node.h>

namespace NAlice::NMegamind {

class TAppHostWalkerMonitoringNodeHandler : public TAppHostNodeHandler {
public:
    TAppHostWalkerMonitoringNodeHandler(IGlobalCtx& globalCtx);

    TStatus Execute(IAppHostCtx& ahCtx) const override;

    static TMaybe<TString> TextHasError(const TString& text);
};

} // namespace NAlice::NMegamind
