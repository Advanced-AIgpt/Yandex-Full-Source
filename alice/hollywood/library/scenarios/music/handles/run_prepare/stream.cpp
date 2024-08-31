#include "impl.h"

namespace NAlice::NHollywood::NMusic::NImpl {

namespace {

bool AddStreamSlot(const TScenarioRunRequestWrapper& request, TFrame& frame, const TString& streamName) {
    if (const auto* stream = RADIO_STREAM_MOCKS.FindPtr(streamName); stream) {
        if (request.ClientInfo().IsSmartSpeaker() || request.ClientInfo().IsNavigator()) { // Answer with actual rup stream
            frame.AddSlot(TSlot{ToString(NAlice::NMusic::SLOT_RADIO_SEEDS),
                                ToString(NAlice::NMusic::SLOT_RADIO_SEEDS_TYPE),
                                TSlot::TValue{streamName}});
        } else { // Answer with mock
            if (*stream == "#personal") {
                frame.AddSlot(TSlot{ToString(NAlice::NMusic::SLOT_PERSONALITY),
                                    ToString(NAlice::NMusic::SLOT_PERSONALITY_TYPE),
                                    TSlot::TValue{"is_personal"}});
            } else if (*stream) {
                frame.AddSlot(TSlot{ToString(NAlice::NMusic::SLOT_SPECIAL_PLAYLIST),
                                    ToString(NAlice::NMusic::SLOT_SPECIAL_PLAYLIST_TYPE),
                                    TSlot::TValue{*stream}});
            }
            // No slots added for onyourwave mock
        }
        return true;
    }
    return false;
}

} // namespace

TMaybe<NScenarios::TScenarioRunResponse> TRunPrepareHandleImpl::TryAddStreamSlotWithFallback() {
    Y_ENSURE(Frame_.Name() == MUSIC_PLAY_FRAME);

    const auto streamSlot = Frame_.FindSlot(NAlice::NMusic::SLOT_STREAM);
    if (!streamSlot) {
        // we don't ask for a stream
        return Nothing();
    }

    if (AddStreamSlot(Request_, Frame_, streamSlot->Value.AsString())) {
        // assigned to Features/Intent at run_render to aid postclassification
        Frame_.AddSlot({ORIGINAL_INTENT, "string", TSlot::TValue{MUSIC_PLAY_FRAME}});
        return Nothing();
    } else {
        LOG_WARNING(Logger_) << "Unexpected value in " << NAlice::NMusic::SLOT_STREAM << " slot: "
                            << streamSlot->Value.AsString();
        return CreateIrrelevantResponseMusicNotFound();
    }
}

} // namespace NAlice::NHollywood::NMusic::NImpl
