#include "audio_play.h"
#include "glagol_metadata.h"

#include <alice/library/proto/protobuf.h>

using NAlice::NHollywood::NMusic::TCallbackPayload;
using NAlice::NHollywood::NMusic::TMusicQueueWrapper;
using NAlice::NHollywood::NMusic::TTrackUrl;
using NAlice::NScenarios::TAudioPlayDirective;
using NAlice::NScenarios::TCallbackDirective;

namespace NAlice::NHollywoodFw::NMusic {

namespace {

constexpr TStringBuf SCENARIO_META_CONTENT_ID = "content_id";
constexpr TStringBuf SCENARIO_META_MUSIC = "music";
constexpr TStringBuf SCENARIO_META_OWNER = "owner";
constexpr TStringBuf SCENARIO_META_QUEUE_ITEM = "queue_item";

constexpr TStringBuf ON_FAILED_CALLBACK = "music_thin_client_on_failed";
constexpr TStringBuf ON_FINISHED_CALLBACK = "music_thin_client_on_finished";
constexpr TStringBuf ON_STARTED_CALLBACK = "music_thin_client_on_started";
constexpr TStringBuf ON_STOPPED_CALLBACK = "music_thin_client_on_stopped";

void FillCallback(TCallbackDirective& directive, const TCallbackPayload& callback, TStringBuf name) {
    directive.SetName(name.data(), name.size());
    directive.SetIgnoreAnswer(true);
    *directive.MutablePayload() = MessageToStruct(callback);
}

} // namespace

TAudioPlayBuilder::TAudioPlayBuilder(const TMusicQueueWrapper& musicQueue) {
    Directive_.SetScreenType(NScenarios::TAudioPlayDirective_EScreenType_Music);
    Directive_.SetName("music");

    const auto& item = musicQueue.CurrentItem();

    // fill scenario meta
    auto& scenarioMeta = *Directive_.MutableScenarioMeta();
    scenarioMeta[SCENARIO_META_OWNER] = SCENARIO_META_MUSIC;
    scenarioMeta[SCENARIO_META_QUEUE_ITEM] = ProtoToBase64String(item);
    scenarioMeta[SCENARIO_META_CONTENT_ID] = ProtoToBase64String(musicQueue.ContentId());

    // fill stream
    auto& stream = *Directive_.MutableStream();
    stream.SetUrl(item.GetUrlInfo().GetUrl());
    stream.SetExpiringAtMs(item.GetUrlInfo().GetExpiringAtMs());
    stream.SetStreamType(NAlice::NScenarios::TAudioPlayDirective_TStream_TStreamType_Track);

    if (item.GetUrlInfo().GetUrlFormat() == TTrackUrl::UrlFormatHls) {
        stream.SetStreamFormat(TAudioPlayDirective::TStream::HLS);
    } else {
        stream.SetStreamFormat(TAudioPlayDirective::TStream::MP3);
    }

    stream.SetId(item.GetTrackId()); // Stream.Id must be a one-to-one mapping to the globally unique trackId to enable
                                     // correct work for the tracks prefetch on the client side

    if (item.HasNormalization()) {
        stream.MutableNormalization()->SetIntegratedLoudness(item.GetNormalization().GetIntegratedLoudness());
        stream.MutableNormalization()->SetTruePeak(item.GetNormalization().GetTruePeak());
    }

    // TODO(sparkle): offsetMs?

    // fill metadata
    auto& meta = *Directive_.MutableAudioPlayMetadata();
    *meta.MutableGlagolMetadata() = BuildGlagolMetadata(musicQueue);
    meta.SetTitle(item.GetTitle());
    meta.SetSubTitle(TMusicQueueWrapper::ArtistName(item));
    meta.SetArtImageUrl(item.GetCoverUrl());
    meta.SetHideProgressBar(item.GetType() == "generative");
}

TAudioPlayBuilder& TAudioPlayBuilder::AddOnStartedCallback(const TCallbackPayload& callback) {
    auto& onStarted = *Directive_.MutableCallbacks()->MutableOnPlayStartedCallback();
    FillCallback(onStarted, callback, ON_STARTED_CALLBACK);
    return *this;
}

TAudioPlayBuilder& TAudioPlayBuilder::AddOnStoppedCallback(const TCallbackPayload& callback) {
    auto& onStopped = *Directive_.MutableCallbacks()->MutableOnPlayStoppedCallback();
    FillCallback(onStopped, callback, ON_STOPPED_CALLBACK);
    return *this;
}

TAudioPlayBuilder& TAudioPlayBuilder::AddOnFailedCallback(const TCallbackPayload& callback) {
    auto& onFailed = *Directive_.MutableCallbacks()->MutableOnFailedCallback();
    FillCallback(onFailed, callback, ON_FAILED_CALLBACK);
    return *this;
}

TAudioPlayBuilder& TAudioPlayBuilder::AddOnFinishedCallback(const TCallbackPayload& callback) {
    auto& onFinished = *Directive_.MutableCallbacks()->MutableOnPlayFinishedCallback();
    FillCallback(onFinished, callback, ON_FINISHED_CALLBACK);
    return *this;
}

NScenarios::TAudioPlayDirective&& TAudioPlayBuilder::Build() && {
    return std::move(Directive_);
}

} // namespace NAlice::NHollywoodFw::NMusic
