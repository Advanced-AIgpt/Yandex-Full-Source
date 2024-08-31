#include "defs.h"

#include <alice/bass/forms/player_command/player_command.sc.h>

#include <alice/bass/forms/directives.h>
#include <alice/bass/forms/player_command.h>
#include <alice/bass/forms/video/defs.h>
#include <alice/bass/forms/video/player_command.h>
#include <alice/bass/forms/video/utils.h>

#include <alice/bass/libs/config/config.h>

#include <alice/bass/libs/logging_v2/logger.h>

namespace NBASS {

TResultValue TPlayerRewindCommandHandler::Do(TRequestHandler& r) {
    NPlayerCommand::SetPlayerCommandProductScenario(r.Ctx());
    if (!r.Ctx().ClientFeatures().SupportsPlayerRewindDirective()) {
        LOG(ERR) << "player_rewind is unsupported on this device";
        r.Ctx().AddErrorBlock(
            TError(TError::EType::NOTSUPPORTED, TStringBuf("unsupported_operation")),
            NSc::Null()
        );
        return TResultValue();
    }

    if (!NPlayerCommand::AssertPlayerCommandIsSupported(NPlayerCommand::PLAYER_REWIND, r.Ctx())) {
        return TResultValue();
    }

    bool isVideoPlayerScreen = (NVideo::GetCurrentScreen(r.Ctx()) == NVideo::EScreenId::VideoPlayer);

    if (isVideoPlayerScreen && NVideo::GetCurrentVideoItemType(r.Ctx()) == NVideo::EItemType::TvStream) {
        r.Ctx().AddAttention(NVideo::ATTENTION_CANNOT_REWIND_TV_STREAM);
        return TResultValue();
    }

    if (isVideoPlayerScreen && NVideo::GetCurrentVideoItemType(r.Ctx()) == NVideo::EItemType::CameraStream) {
        r.Ctx().AddAttention(NVideo::ATTENTION_CANNOT_REWIND_CAMERA_STREAM);
        return TResultValue();
    }

    TSlot* time = r.Ctx().GetSlot(TStringBuf("time"));
    TSlot* rewindTypeSlot = r.Ctx().GetSlot(TStringBuf("rewind_type"));
    NPlayerCommand::ERewindType rewindType;
    if (IsSlotEmpty(rewindTypeSlot)) {
        rewindType = NPlayerCommand::ERewindType::None;
    } else if (!TryFromString(rewindTypeSlot->Value.GetString(), rewindType)) {
        TString err = TStringBuilder() << "Unexpected rewind_type value " << rewindTypeSlot->Value.GetString();
        LOG(ERR) << err << Endl;
        return TError(TError::EType::INVALIDPARAM, err);
    }

    NPlayerCommand::TPlayerRewindCommand command;
    switch (rewindType) {
    case NPlayerCommand::ERewindType::Forward:
    case NPlayerCommand::ERewindType::Backward:
    case NPlayerCommand::ERewindType::Absolute:
        command->Type() = ToString(rewindType);
        break;
    case NPlayerCommand::ERewindType::None: //XXX: in first quick implementation none direction means forward
        command->Type() = ToString(NPlayerCommand::ERewindType::Absolute);
        break;
    }
    //TODO: think about reducing copy-paste with timers implementation
    TDuration d;
    if (IsSlotEmpty(time)) {
        constexpr TDuration defaultRewindDuration = TDuration::Seconds(10);
        d = defaultRewindDuration;
    } else {
        d += TDuration::Hours(1) * time->Value["hours"].GetNumber();
        d += TDuration::Minutes(1) * time->Value["minutes"].GetNumber();
        d += TDuration::Seconds(1) * time->Value["seconds"].GetNumber();
    }

    command->Amount() = d.Seconds();

    if (isVideoPlayerScreen) {
        return NVideo::RewindVideo(r.Ctx(), command);
    }
    r.Ctx().AddCommand<TPlayerRewindDirective>(NPlayerCommand::PLAYER_REWIND, command.Value());
    return TResultValue();
}

}
