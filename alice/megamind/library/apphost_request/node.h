#pragma once

#include "item_adapter.h"

#include <alice/megamind/library/globalctx/fwd.h>
#include <alice/megamind/library/util/status.h>

#include <alice/library/logger/logger.h>

namespace NAlice::NMegamind {

class IAppHostCtx {
public:
    virtual ~IAppHostCtx() = default;

    virtual TRTLogger& Log() = 0;
    virtual IGlobalCtx& GlobalCtx() = 0;
    virtual TItemProxyAdapter& ItemProxyAdapter() = 0;
};

} // namespace NAlice::NMegamind
