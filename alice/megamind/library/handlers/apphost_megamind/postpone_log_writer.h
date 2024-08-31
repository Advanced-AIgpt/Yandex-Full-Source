#pragma once

#include "node.h"

#include <alice/megamind/library/apphost_request/node.h>
#include <alice/megamind/library/globalctx/globalctx.h>
#include <alice/megamind/library/registry/registry.h>
#include <alice/megamind/library/util/status.h>

namespace NAlice::NMegamind {

namespace NImpl {

void LogResponse(NJson::TJsonValue&& responseJson, const bool haveErrorsInResponse, TRTLogger& logger,
                 const int statusCode);
} // namespace NImpl

void RegisterPostponeLogWriterHander(IGlobalCtx& globalCtx, TRegistry& registry);

class TAppHostPostponeLogWriter : public TAppHostNodeHandler {
public:
    TAppHostPostponeLogWriter(IGlobalCtx& globalCtx);

    TStatus Execute(IAppHostCtx& ahCtx) const override;

};

} // namespace NAlice::NMegamind
