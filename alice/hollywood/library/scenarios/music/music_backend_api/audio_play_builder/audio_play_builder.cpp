#include "audio_play_builder.h"

#include "callback_payload_builder.h"

#include <alice/hollywood/library/scenarios/music/music_backend_api/consts.h>
#include <alice/hollywood/library/scenarios/music/music_backend_api/music_queue/music_queue.h>
#include <alice/hollywood/library/scenarios/music/music_backend_api/music_queue/what_is_playing_answer.h>
#include <alice/hollywood/library/scenarios/music/music_backend_api/play_audio/play_audio.h>

#include <alice/hollywood/library/scenarios/music/proto/play_audio.pb.h>

#include <alice/library/proto/protobuf.h>

namespace NAlice::NHollywood::NMusic {

namespace {

constexpr TStringBuf RENDER_AUDIO_PLAY_DIRECTIVE_NAME = "music";

NScenarios::TAudioPlayDirective_TAudioPlayMetadata_EContentType GetMetaEContentType(const TContentId_EContentType type) {
    switch (type) {
        case TContentId_EContentType_Track:
            return NScenarios::TAudioPlayDirective_TAudioPlayMetadata_EContentType_Track;
        case TContentId_EContentType_Album:
            return NScenarios::TAudioPlayDirective_TAudioPlayMetadata_EContentType_Album;
        case TContentId_EContentType_Artist:
            return NScenarios::TAudioPlayDirective_TAudioPlayMetadata_EContentType_Artist;
        case TContentId_EContentType_Playlist:
            return NScenarios::TAudioPlayDirective_TAudioPlayMetadata_EContentType_Playlist;
        case TContentId_EContentType_Radio:
            return NScenarios::TAudioPlayDirective_TAudioPlayMetadata_EContentType_Radio;
        case TContentId_EContentType_Generative:
            return NScenarios::TAudioPlayDirective_TAudioPlayMetadata_EContentType_Generative;
        case TContentId_EContentType_FmRadio:
            return NScenarios::TAudioPlayDirective_TAudioPlayMetadata_EContentType_FmRadio;
        default:
            ythrow yexception() << "Unsupported ContentId Type=" << TContentId_EContentType_Name(type);
    }
}

NScenarios::TAudioPlayDirective_TAudioPlayMetadata_ERepeatMode GetMetaERepeateMode(const ERepeatType repeatType) {
    switch (repeatType) {
        case ERepeatType::RepeatNone:
            return NScenarios::TAudioPlayDirective_TAudioPlayMetadata_ERepeatMode_None;
        case ERepeatType::RepeatTrack:
            return NScenarios::TAudioPlayDirective_TAudioPlayMetadata_ERepeatMode_One;
        case ERepeatType::RepeatAll:
            return NScenarios::TAudioPlayDirective_TAudioPlayMetadata_ERepeatMode_All;
        default:
            ythrow yexception() << "Unsupported repeat type=" << ERepeatType_Name(repeatType);
    }
}

} // namespace

void FillGlagolMetadata(::NAlice::NScenarios::TAudioPlayDirective_TAudioPlayMetadata_TGlagolMetadata& glagolMetadata,
                        TMaybe<TContentId> contentId, TMaybe<TString> prevTrackId,
                        TMaybe<TString> nextTrackId, TMaybe<bool> shuffled,
                        TMaybe<ERepeatType> repeatType) {
    auto& musicMetadata = *glagolMetadata.MutableMusicMetadata();
    if (contentId.Defined()) {
        musicMetadata.SetId(contentId->GetId());
        musicMetadata.SetType(GetMetaEContentType(contentId->GetType()));
    }
    if (prevTrackId.Defined()) {
        auto& prevTrackInfo = *musicMetadata.MutablePrevTrackInfo();
        prevTrackInfo.SetStreamType(NAlice::NScenarios::TAudioPlayDirective_TStream_TStreamType_Track);
        prevTrackInfo.SetId(prevTrackId.GetRef());
    }
    if (nextTrackId.Defined()) {
        auto& nextTrackInfo = *musicMetadata.MutableNextTrackInfo();
        nextTrackInfo.SetStreamType(NAlice::NScenarios::TAudioPlayDirective_TStream_TStreamType_Track);
        nextTrackInfo.SetId(nextTrackId.GetRef());
    }

    if (shuffled.Defined()) {
        musicMetadata.SetShuffled(shuffled.GetRef());
    }

    if (repeatType.Defined()) {
        musicMetadata.SetRepeatMode(GetMetaERepeateMode(repeatType.GetRef()));
    }
}

TAudioPlayBuilder::TAudioPlayBuilder(const TQueueItem& item, const i32 offsetMs,
                                     TMaybe<TContentId> contentId, TMaybe<TString> prevTrackId,
                                     TMaybe<TString> nextTrackId, TMaybe<bool> shuffled, TMaybe<ERepeatType> repeatType,
                                     const bool needSetPauseAtStart, const TMultiroomTokenWrapper* multiroomToken)
    : Proto_()
    , AudioPlayProto_(*Proto_.MutableAudioPlayDirective())
{
    AudioPlayProto_.SetScreenType(NScenarios::TAudioPlayDirective_EScreenType_Music);

    auto& scenarioMeta = *AudioPlayProto_.MutableScenarioMeta();
    scenarioMeta[SCENARIO_META_OWNER] = SCENARIO_META_MUSIC;
    scenarioMeta[SCENARIO_META_QUEUE_ITEM] = ProtoToBase64String(item);
    if (contentId) {
        scenarioMeta[SCENARIO_META_CONTENT_ID] = ProtoToBase64String(*contentId);
    }

    AudioPlayProto_.SetName(TString(RENDER_AUDIO_PLAY_DIRECTIVE_NAME));
    auto& stream = *AudioPlayProto_.MutableStream();
    stream.SetUrl(item.GetUrlInfo().GetUrl());
    stream.SetExpiringAtMs(item.GetUrlInfo().GetExpiringAtMs());

    if (item.GetUrlInfo().GetUrlFormat() == TTrackUrl::UrlFormatHls) {
        stream.SetStreamFormat(NAlice::NScenarios::TAudioPlayDirective::TStream::HLS);
    } else {
        stream.SetStreamFormat(NAlice::NScenarios::TAudioPlayDirective::TStream::MP3);
    }

    if (item.GetType() == "generative") {
        stream.SetOffsetMs(0);
    } else {
        stream.SetOffsetMs(offsetMs);
    }

    if (item.GetType() == "shot") {
        stream.SetStreamType(NAlice::NScenarios::TAudioPlayDirective_TStream_TStreamType_Shot);
    } else {
        stream.SetStreamType(NAlice::NScenarios::TAudioPlayDirective_TStream_TStreamType_Track);
    }

    stream.SetId(item.GetTrackId()); // Stream.Id must be a one-to-one mapping to the globally unique trackId to enable
                                     // correct work for the tracks prefetch on the client side

    if (item.HasNormalization()) {
        stream.MutableNormalization()->SetIntegratedLoudness(item.GetNormalization().GetIntegratedLoudness());
        stream.MutableNormalization()->SetTruePeak(item.GetNormalization().GetTruePeak());
    }

    auto& meta = *AudioPlayProto_.MutableAudioPlayMetadata();
    meta.SetTitle(item.GetTitle());
    // XXX(stupnik): Clarify what should go here
    meta.SetSubTitle(TMusicQueueWrapper::ArtistName(item));
    meta.SetArtImageUrl(item.GetCoverUrl());
    meta.SetHideProgressBar(item.GetType() == "generative");

    FillGlagolMetadata(*meta.MutableGlagolMetadata(), contentId, prevTrackId, nextTrackId, shuffled, repeatType);

    AudioPlayProto_.SetSetPause(needSetPauseAtStart);

    if (multiroomToken) {
        if (const TStringBuf token = multiroomToken->GetToken()) {
            AudioPlayProto_.SetMultiroomToken(token.data(), token.size());
        }
    }
}

TAudioPlayBuilder& TAudioPlayBuilder::AddOnStartedCallback(google::protobuf::Struct payload) {
    auto& callbacks = *AudioPlayProto_.MutableCallbacks();
    auto& onStarted = *callbacks.MutableOnPlayStartedCallback();
    onStarted.SetName(TString(MUSIC_THIN_CLIENT_ON_STARTED_CALLBACK));
    onStarted.SetIgnoreAnswer(true);
    *onStarted.MutablePayload() = payload;
    return *this;
}

TAudioPlayBuilder& TAudioPlayBuilder::AddOnStoppedCallback(google::protobuf::Struct payload) {
    auto& callbacks = *AudioPlayProto_.MutableCallbacks();
    auto& onStarted = *callbacks.MutableOnPlayStoppedCallback();
    onStarted.SetName(TString(MUSIC_THIN_CLIENT_ON_STOPPED_CALLBACK));
    onStarted.SetIgnoreAnswer(true);
    *onStarted.MutablePayload() = payload;
    return *this;
}

TAudioPlayBuilder& TAudioPlayBuilder::AddOnFinishedCallback(google::protobuf::Struct payload) {
    auto& callbacks = *AudioPlayProto_.MutableCallbacks();
    auto& onStarted = *callbacks.MutableOnPlayFinishedCallback();
    onStarted.SetName(TString(MUSIC_THIN_CLIENT_ON_FINISHED_CALLBACK));
    onStarted.SetIgnoreAnswer(true);
    *onStarted.MutablePayload() = payload;
    return *this;
}

TAudioPlayBuilder& TAudioPlayBuilder::AddOnFailedCallback(google::protobuf::Struct payload) {
    auto& callbacks = *AudioPlayProto_.MutableCallbacks();
    auto& onStarted = *callbacks.MutableOnFailedCallback();
    onStarted.SetName(TString(MUSIC_THIN_CLIENT_ON_FAILED));
    onStarted.SetIgnoreAnswer(true);
    *onStarted.MutablePayload() = payload;
    return *this;
}

NScenarios::TDirective TAudioPlayBuilder::BuildProto() {
    return std::move(Proto_);
}

} // NAlice::NHollywood::NMusic
