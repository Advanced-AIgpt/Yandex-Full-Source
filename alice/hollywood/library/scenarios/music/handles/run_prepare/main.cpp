#include "main.h"
#include "impl.h"

#include <alice/hollywood/library/scenarios/music/common.h>
#include <alice/hollywood/library/scenarios/music/fairy_tales/semantic_frames.h>
#include <alice/hollywood/library/scenarios/music/generative/generative.h>
#include <alice/hollywood/library/scenarios/music/music_play_anaphora.h>
#include <alice/hollywood/library/scenarios/music/music_request_builder/music_request_builder.h>
#include <alice/hollywood/library/scenarios/music/util/music_proxy_request.h>

#include <alice/library/experiments/flags.h>
#include <alice/library/music/defs.h>

#include <alice/megamind/protos/blackbox/blackbox.pb.h>


namespace NAlice::NHollywood::NMusic {

namespace NImpl {

std::variant<THttpProxyRequest, NScenarios::TScenarioRunResponse, TString> TRunPrepareHandleImpl::Do() {
    const TFrame* musicPlayFrame = FindFrame(MUSIC_PLAY_FRAME);
    const TFrame* fixlistFrame = FindFrame(MUSIC_PLAY_FIXLIST_FRAME);
    const TFrame* anaphoraFrame = FindFrame(MUSIC_PLAY_ANAPHORA_FRAME);
    const TFrame* fairytaleFrame = FindFrame(MUSIC_PLAY_FAIRYTALE_FRAME);

    // TODO(sparkle): move to onboarding.cpp?
    if (FindFrame(FORCE_EXIT_FRAME) && GetScenarioStateOrDummy().GetOnboardingState().GetInOnboarding()) {
        LOG_INFO(Logger_) << "Interrupting music onboarding due to force exit";
        return CreateIrrelevantResponseMusicNotFound();
    }

    // queries like "следующий", "предыдущий", "стоп", "продолжи", ...
    if (auto resp = HandlePlayerCommand()) {
        return std::move(*resp);
    }

    // each callback is a special request type with its own logic
    if (auto resp = HandleCallback()) {
        return std::move(*resp);
    }

    // if we have an explicit onboarding frame, try to handle it
    if (auto resp = HandleOnboardingFrames()) {
        return std::move(*resp);
    }

    // try to fall into 'like genre' onboarding
    if (musicPlayFrame) {
        Frame_ = *musicPlayFrame;
        if (auto resp = TryHandleMusicComplexLikeDislikeNoSearch()) {
            return std::move(*resp);
        }
    }

    const auto* currentTrack = CheckAndGetMusicPlayAnaphoraTrack(anaphoraFrame, Request_);
    const bool fairytaleReask = fairytaleFrame && UserInfo_ && UserInfo_->GetHasYandexPlus();
    if (!musicPlayFrame && !fixlistFrame && !currentTrack && !fairytaleReask) {
        LOG_WARNING(Logger_) << "Failed to get music_play, music_play_fixlist, music_play_anaphora, or music_play_fairytale semantic frame";
        return CreateIrrelevantResponseMusicNotFound();
    }

    // define which frame we want to send to BASS
    // the `if`s ORDER IS VERY IMPORTANT
    if (fixlistFrame) {
        // "fixlist" substitutes the frame with hardcoded content
        if (const auto slot = fixlistFrame->FindSlot(NAlice::NMusic::SLOT_SPECIAL_ANSWER_INFO)) {
            const auto fixlist = NJson::ReadJsonFastTree(slot->Value.AsString());
            Frame_ = CreateSpecialAnswerFrame(fixlist, Request_.Interfaces().GetHasAudioClient());
            LOG_DEBUG(Logger_) << "Created fixlist frame: " << JsonStringFromProto(Frame_.ToProto());
        } else {
            LOG_WARNING(Logger_) << "Missing " << NAlice::NMusic::SLOT_SPECIAL_ANSWER_INFO << " slot in "
                                << MUSIC_PLAY_FIXLIST_FRAME << " semantic frame";
            return CreateIrrelevantResponseMusicNotFound();
        }
    } else if (currentTrack) {
        Frame_ = TransformMusicPlayAnaphora(*anaphoraFrame, *currentTrack);
        LOG_DEBUG(Logger_) << "Created anaphora frame: " << JsonStringFromProto(Frame_.ToProto());
    } else if (fairytaleReask) {
        // "reasking" is clarification of what user wants to listen
        Frame_ = *fairytaleFrame;
        LOG_DEBUG(Logger_) << "Created fairytale reask frame: " << JsonStringFromProto(Frame_.ToProto());
    } else {
        // queries like 'включи рок', 'включи джаз', etc.
        if (auto resp = HandleThinClientRadio()) {
            return std::move(*resp);
        }

        Frame_ = *musicPlayFrame;
        LOG_DEBUG(Logger_) << "Created music play frame: " << JsonStringFromProto(Frame_.ToProto());
    }

    // "reasking" is clarification of what user wants to listen
    if (auto resp = HandleReask()) {
        return std::move(*resp);
    }

    // process generative music request
    // TODO(sparkle): move to generative.cpp?
    if (musicPlayFrame) {
        if (const auto stationSlot = musicPlayFrame->FindSlot(::NAlice::NMusic::SLOT_GENERATIVE_STATION);
                stationSlot &&
                Request_.HasExpFlag(NExperiments::EXP_HW_MUSIC_THIN_CLIENT_GENERATIVE))
        {
            if (Request_.Interfaces().GetHasAudioClient()) {
                LOG_INFO(Logger_) << "Handling MusicPlay generative SemanticFrame...";
                return *HandleThinClientGenerative(Ctx_, Request_, Nlg_, stationSlot->Value.AsString(), /* isNewContentRequestedByUser= */ true);
            } else {
                LOG_INFO(Logger_) << "No audio_client feature found for generative music, answering with hardcoded phrase";
                return CreateNotSupportedResponse("generative_music_not_supported");
            }
        }
    }

    // add stream slot if needed
    if (Frame_.Name() == MUSIC_PLAY_FRAME) {
        if (auto resp = TryAddStreamSlotWithFallback()) {
            return std::move(*resp);
        }
    }

    // process fairy tales requests
    if (Frame_.Name() == MUSIC_PLAY_FAIRYTALE_FRAME) {
        if (auto resp = HandleFairyTale()) {
            return std::move(*resp);
        }
    } else if (FindFrame(ALICE_FAIRY_TALE_ONDEMAND_FRAME)) {
        if (auto resp = HandleFairyTaleOnDemand()) {
            return std::move(*resp);
        }
    }

    // "activate_multiroom" slot is used by legacy BASS multiroom
    // XXX(sparkle): remove when thin multiroom is released
    TryFillActivateMultiroomSlot(Frame_);

    // handle object slot
    if (auto objectTypeSlot = Frame_.FindSlot(NAlice::NMusic::SLOT_OBJECT_TYPE)) {
        // try handle object slot in THIN player
        if (auto resp = TryHandleThinClientMusicPlayObject()) {
            return std::move(*resp);
        }

        // try handle object slot in THICK player
        // generative music is not supported in THICK player at all
        if (objectTypeSlot->Value.AsString() == TMusicPlayObjectTypeSlot_EValue_Name(TMusicPlayObjectTypeSlot_EValue_Generative)) {
            return CreateIrrelevantResponseMusicNotFound();
        }

        // radio music is supported in THICK player as legacy
        if (objectTypeSlot->Value.AsString() == TMusicPlayObjectTypeSlot_EValue_Name(TMusicPlayObjectTypeSlot_EValue_Radio)) {
            auto objectIdSlot = Frame_.FindSlot(NAlice::NMusic::SLOT_OBJECT_ID);
            return MakeBassRadioResponseWithContinueArguments(objectIdSlot->Value.AsString());
        }

        // other music is supported via "/vins" BASS handler
        return PrepareBassVinsRequest(
            Logger_,
            Request_,
            Frame_,
            /* sourceTextProvider= */ nullptr,
            Meta_,
            /* imageSearch= */ false,
            AppHostParams_
        );
    }

    // try to fallback to onboarding once again...
    if (auto resp = TryHandleMusicComplexLikeDislikeWithSearch()) {
        return std::move(*resp);
    }

    // "subgraph flag" is a string that affects AppHost graph execution
    // and passes control to the subgraph
    if (auto subgraphFlag = TryGetSubgraphFlag()) {
        LOG_INFO(Logger_) << "Should invoke subgraph, add flag " << *subgraphFlag;
        return *subgraphFlag;
    }

    // send Frame_ to BASS
    return CreateBassRunRequest();
}

} // namespace NImpl

void TRunPrepareHandle::Do(TScenarioHandleContext& ctx) const {
    const auto result = NImpl::TRunPrepareHandleImpl{ctx}.Do();

    struct {
        TScenarioHandleContext& Ctx;

        // normal course of action: go to BASS next
        void operator()(const THttpProxyRequest& bassRequest) {
            AddBassRequestItems(Ctx, bassRequest);
        }

        // early Irrelevant exit or callback/command processing to apply/continue stage
        void operator()(const NScenarios::TScenarioRunResponse& response) {
            Ctx.ServiceCtx.AddProtobufItem(response, RESPONSE_ITEM);
        }

        // go to some subgraph (defined by flag)
        void operator()(const TString& flag) {
            Ctx.ServiceCtx.AddFlag(flag);
        }
    } visitor{ctx};

    std::visit(visitor, result);
}

} // namespace NAlice::NHollywood::NMusic
