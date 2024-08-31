#include "route_helpers.h"

namespace NBASS {
    namespace NRoute {

        bool CheckNavigatorState(TContext& ctx, TStringBuf state) {
            if (ctx.Meta().DeviceState().HasNavigatorState() && ctx.Meta().DeviceState().NavigatorState().HasStates()) {
                for (const auto& stateIter : ctx.Meta().DeviceState().NavigatorState().States()) {
                    if (state == stateIter.Get()) {
                        return true;
                    }
                }
            }
            return false;
        }

        constexpr TStringBuf WAITING_STATE = "waiting_for_route_confirmation";

    } // NRoute
} // NBASS
