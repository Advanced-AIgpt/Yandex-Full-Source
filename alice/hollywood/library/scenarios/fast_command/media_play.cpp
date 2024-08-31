#include "media_play.h"

#include "common.h"

namespace NAlice::NHollywood::NMediaPlay {

namespace {

constexpr TStringBuf MEDIA_PLAY_FRAME_NAME = "alice.media_play";

constexpr TStringBuf TUNE_ID_SLOT = "tune_id";
constexpr TStringBuf LOCATION_ID_SLOT = "location_id";

const THashMap<TString, TString> TUNE_ID_TO_STREAM_URL = {
    {"stereopair_ready", "https://storage.mds.yandex.net/get-music-shots/3472782/stereoready_1632489505.mp3"},
};

void TryAddStartMultiroomDirective(TRTLogger& logger, const NScenarios::TInterfaces& interfaces,
                                   const TFrame& frame, TResponseBodyBuilder& bodyBuilder) {
    constexpr TStringBuf WILL_NOT_ADD = "Will not add start_multiroom directive, reason: ";

    const auto locationIdSlot = frame.FindSlot(LOCATION_ID_SLOT);
    if (!locationIdSlot) {
        LOG_INFO(logger) << WILL_NOT_ADD << "no location_id slot";
        return;
    }
    const auto& locationId = locationIdSlot->Value.AsString();

    if (!interfaces.GetMultiroomAudioClient()) {
        LOG_WARN(logger) << WILL_NOT_ADD << "client doesn't support MultiroomAudioClient";
        return;
    }

    NScenarios::TDirective directive;
    auto& startMultiroomDirective = *directive.MutableStartMultiroomDirective();
    startMultiroomDirective.SetRoomId(locationId);
    bodyBuilder.AddDirective(std::move(directive));

    LOG_INFO(logger) << "Added start_multiroom directive with location id " << locationId.Quote();
}

void TryAddAudioPlayDirective(TRTLogger& logger, const NScenarios::TInterfaces& interfaces,
                              const TFrame& frame, TResponseBodyBuilder& bodyBuilder) {
    constexpr TStringBuf WILL_NOT_ADD = "Will not add audio_play directive, reason: ";

    const auto tuneIdSlot = frame.FindSlot(TUNE_ID_SLOT);
    if (!tuneIdSlot) {
        LOG_INFO(logger) << WILL_NOT_ADD << "no tune_id slot";
        return;
    }
    const auto& tuneId = tuneIdSlot->Value.AsString();

    if (!interfaces.GetHasAudioClient()) {
        LOG_WARN(logger) << WILL_NOT_ADD << "client doesn't support AudioClient";
        return;
    }

    const auto iter = TUNE_ID_TO_STREAM_URL.find(tuneId);
    if (iter == TUNE_ID_TO_STREAM_URL.end()) {
        LOG_ERROR(logger) << WILL_NOT_ADD << "stream url for tune not found";
        return;
    }
    const TString& streamUrl = iter->second;

    NScenarios::TDirective directive;
    auto& audioPlayDirective = *directive.MutableAudioPlayDirective();
    audioPlayDirective.MutableStream()->SetUrl(streamUrl);
    // TODO(sparkle): other fields?..
    bodyBuilder.AddDirective(std::move(directive));

    LOG_INFO(logger) << "Added audio_play directive with tune id " << tuneId.Quote();
}

} // namespace

bool TryProcessMediaPlayFrame(TFastCommandScenarioRunContext& fastCommandScenarioRunContext) {
    const auto& request = fastCommandScenarioRunContext.Request;
    auto& logger = fastCommandScenarioRunContext.Logger;
    const auto frameProto = request.Input().FindSemanticFrame(MEDIA_PLAY_FRAME_NAME);
    if (!frameProto) {
        return false;
    }
    LOG_INFO(logger) << "See media_play frame, will create its response";

    const auto frame = TFrame::FromProto(*frameProto);
    auto& bodyBuilder = fastCommandScenarioRunContext.RunResponseBuilder.CreateResponseBodyBuilder(&frame);

    TryAddStartMultiroomDirective(logger, request.Interfaces(), frame, bodyBuilder);
    TryAddAudioPlayDirective(logger, request.Interfaces(), frame, bodyBuilder);
    return true;
}

} // namespace NAlice::NHollywood::NMediaPlay
