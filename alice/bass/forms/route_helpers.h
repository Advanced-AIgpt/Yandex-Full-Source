#pragma once

#include <alice/bass/forms/vins.h>

namespace NBASS {
    namespace NRoute {

        bool CheckNavigatorState(TContext& ctx, TStringBuf state);
        const extern TStringBuf WAITING_STATE;

    } // NRoute
} // NBASS
