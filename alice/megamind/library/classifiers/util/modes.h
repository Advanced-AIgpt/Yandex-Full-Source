#pragma once

#include <alice/megamind/library/context/context.h>
#include <alice/megamind/library/experiments/flags.h>
#include <alice/megamind/protos/common/device_state.pb.h>

namespace NAlice {

bool IsActiveScenario(const ISession* session, const TStringBuf name, ui64 activeScenarioTimeoutMs,
                      const ui64 serverTimeMs);
inline bool IsActiveScenario(const IContext& ctx, const TString& name) {
    return IsActiveScenario(ctx.Session(), name,
                            ctx.ScenarioConfig(name).GetDialogManagerParams().GetActiveScenarioTimeoutSeconds() *
                                1'000,
                            ctx.SpeechKitRequest()->GetRequest().GetAdditionalOptions().GetServerTimeMs());
}

bool IsPlayerOwnerScenario(const TDeviceState& deviceState, const TStringBuf name);
inline bool IsPlayerOwnerScenario(const IContext& ctx, const TStringBuf name) {
    return IsPlayerOwnerScenario(ctx.SpeechKitRequest().DeviceState(), name);
}

bool IsModalModeAllowed(const ISession* session, i32 maxActivityTurns, const TString& name);
inline bool IsModalModeAllowed(const IContext& ctx, const TString& name) {
    return IsModalModeAllowed(ctx.Session(), ctx.ScenarioConfig(name).GetDialogManagerParams().GetMaxActivityTurns(),
                              name);
}

inline bool IsPlayerOwnerPriorityModeAllowed(const IContext& ctx, const TString& name) {
    return ctx.ScenarioConfig(name).GetDialogManagerParams().GetIsPlayerOwnerPriorityAllowed() &&
        !ctx.HasExpFlag(EXP_DISABLE_PLAYER_OWNER_PRIORITY);
}

inline bool IsInModalMode(const ISession* session, i32 maxActivityTurns, const TString& name,
                          const ui64 activeScenarioTimeoutMs, const ui64 serverTimeMs) {
    return IsActiveScenario(session, name, activeScenarioTimeoutMs, serverTimeMs) &&
           IsModalModeAllowed(session, maxActivityTurns, name);
}
inline bool IsInModalMode(const IContext& ctx, const TString& name) {
    return IsActiveScenario(ctx, name) && IsModalModeAllowed(ctx, name);
}

inline bool IsInPlayerOwnerPriorityMode(const IContext& ctx, const TString& name) {
    return IsPlayerOwnerScenario(ctx, name) && IsPlayerOwnerPriorityModeAllowed(ctx, name);
}

} // namespace NAlice
