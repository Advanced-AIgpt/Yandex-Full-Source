#pragma once

#include "vins.h"

#include <alice/bass/forms/common/blackbox_api.h>
#include <alice/bass/forms/common/data_sync_api.h>

namespace NBASS {

namespace NPlayerCommand {

inline constexpr TStringBuf MUSIC_PLAYER = "music";
inline constexpr TStringBuf RADIO_PLAYER = "radio";
inline constexpr TStringBuf VIDEO_PLAYER = "video";
inline constexpr TStringBuf STOP_PLAYER = "stop";
inline constexpr TStringBuf BLUETOOTH_PLAYER = "bluetooth";

inline constexpr TStringBuf PLAYER_NEXT_TRACK = "player_next_track";
inline constexpr TStringBuf PLAYER_PREV_TRACK = "player_previous_track";
inline constexpr TStringBuf PLAYER_REWIND = "player_rewind";

inline constexpr TStringBuf SLOT_NAME_MUSIC_PLAYER_ONLY = "music_player_only";
inline constexpr TStringBuf SLOT_TYPE_FLAG = "flag";

TStringBuf SelectPlayer(TContext& ctx, NSc::TValue* lastWatchedVideo);
bool AssertPlayerCommandIsSupported(TStringBuf command, TContext& ctx);
void SetPlayerCommandProductScenario(TContext& ctx);

} // namespace NPlayerCommand

class TPlayerSimpleCommandHandler: public IHandler {
public:
    TResultValue Do(TRequestHandler& r) override;
    TResultValue ExecuteCommand(TContext& ctx, TStringBuf command, NSc::TValue* output);

    static void Register(THandlersMap* handlers);
};

class TPlayerAuthorizedCommandHandler: public IHandler {
public:
    TPlayerAuthorizedCommandHandler(THolder<TBlackBoxAPI> blackBoxAPI, THolder<TDataSyncAPI> dataSyncAPI);

    TResultValue Do(TRequestHandler& r) override;

    static void Register(THandlersMap* handlers);

private:
    THolder<TBlackBoxAPI> BlackBoxAPI;
    THolder<TDataSyncAPI> DataSyncAPI;
};

class TPlayerContinueCommandHandler: public IHandler {
public:
    TResultValue Do(TRequestHandler& r) override;

    static void Register(THandlersMap* handlers);
};

class TPlayerNextPrevCommandHandler: public TContinuableHandler {
public:
    IContinuation::TPtr Prepare(TRequestHandler& r) override;

    static void Register(THandlersMap* handlers);
};

class TPlayerRewindCommandHandler : public IHandler {
public:
    TResultValue Do(TRequestHandler& r) override;

    static void Register(THandlersMap* handlers);
};

} // namespace NBASS
