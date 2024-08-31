#include "impl.h"

#include <alice/hollywood/library/framework/framework_migration.h>
#include <alice/hollywood/library/scenarios/music/music_backend_api/music_common.h>
#include <alice/hollywood/library/scenarios/music/util/util.h>

#include <alice/library/analytics/common/product_scenarios.h>

#include <alice/megamind/protos/common/required_messages/required_messages.pb.h>
#include <alice/megamind/protos/blackbox/blackbox.pb.h>

namespace NAlice::NHollywood::NMusic::NImpl {

namespace {

static const NAlice::NMegamind::TRequiredMessages requiredProtoMessages;

const TStringBuf DEFAULT_AMBIENT_SOUND = "playlist/103372440:1919";
const TStringBuf DEFAULT_AMBIENT_SOUND_TYPE = "playlist";

const THashMap<TStringBuf, TStringBuf> PLAYBACK_TYPE_TO_OBJECT_TYPE = {
    {"track", "Track"},
    {"album", "Album"},
    {"artist", "Artist"},
    {"playlist", "Playlist"},
};

TMaybe<TScenarioState> TryUnpackScenarioState(TRTLogger& logger, const TScenarioRunRequestWrapper& request) {
    TScenarioState scenarioState;
    if (ReadScenarioState(request.BaseRequestProto(), scenarioState)) {
        TryInitPlaybackContextBiometryOptions(logger, scenarioState);
        return scenarioState;
    }
    return Nothing();
}

}

TRunPrepareHandleImpl::TRunPrepareHandleImpl(TScenarioHandleContext& ctx)
    : Ctx_{ctx}
    , Logger_{Ctx_.Ctx.Logger()}
    , Meta_{Ctx_.RequestMeta}
    , AppHostParams_{Ctx_.AppHostParams}
    , RequestProto_{GetOnlyProtoOrThrow<NScenarios::TScenarioRunRequest>(ctx.ServiceCtx, REQUEST_ITEM)}
    , Request_{RequestProto_, Ctx_.ServiceCtx}
    , Nlg_{TNlgWrapper::Create(Ctx_.Ctx.Nlg(), Request_, Ctx_.Rng, Ctx_.UserLang)}
    , MusicResources_{Ctx_.Ctx.ScenarioResources<TMusicResources>()}

    , UserInfo_{GetUserInfoProto(Request_)}
    , ScenarioState_{TryUnpackScenarioState(Logger_, Request_)}
    , EmptyScenarioStateDummy_{}
    , MusicQueue_{Logger_, *GetScenarioStateOrDummy().MutableQueue()}
    , Frame_{MUSIC_PLAY_FRAME}
{}

TScenarioState& TRunPrepareHandleImpl::GetScenarioStateOrDummy() {
    return ScenarioState_ ? *ScenarioState_ : EmptyScenarioStateDummy_;
}

void TRunPrepareHandleImpl::PatchFrameIfNeeded(TFrame& frame) {
    if (frame.Name() == MUSIC_PLAY_FRAME) {
        HasMusicPlayAmbientSoundRequest_ = TAmbientSoundMusicPlayPatcher(frame, Request_, Logger_).Patch();
    }
}

const TFrame* TRunPrepareHandleImpl::FindFrame(TStringBuf frameName) {
    if (!Frames_.contains(frameName)) {
        if (const auto frameProto = Request_.Input().FindSemanticFrame(frameName)) {
            SemanticFrames_[frameName] = frameProto.Get();
            auto frame = TFrame::FromProto(*frameProto);
            PatchFrameIfNeeded(frame);
            Frames_[frameName] = std::make_unique<TFrame>(std::move(frame));
        } else {
            Frames_[frameName] = nullptr;
        }
    }
    return Frames_.at(frameName).get();
}

const TSemanticFrame* TRunPrepareHandleImpl::FindSemanticFrame(TStringBuf frameName) {
    if (!FindFrame(frameName)) {
        return nullptr;
    }
    return SemanticFrames_.at(frameName);
}

void TRunPrepareHandleImpl::AddPrefixToSearchText(const TString& prefix, const TSlot& searchText) {
    const auto newSearchText = TSlot{
        ToString(NAlice::NMusic::SLOT_SEARCH_TEXT),
        ToString(NAlice::SLOT_STRING_TYPE),
        TSlot::TValue{TString::Join(prefix, " ", searchText.Value.AsString())}
    };
    Frame_.RemoveSlots(ToString(NAlice::NMusic::SLOT_SEARCH_TEXT));
    Frame_.AddSlot(newSearchText);
}

} // namespace NAlice::NHollywood::NMusic::NImpl
