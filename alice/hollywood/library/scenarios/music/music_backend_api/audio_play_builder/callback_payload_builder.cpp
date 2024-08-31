#include "callback_payload_builder.h"

#include <alice/hollywood/library/scenarios/music/music_backend_api/content_id/content_id.h>
#include <alice/hollywood/library/scenarios/music/music_backend_api/play_audio/play_audio.h>

#include <alice/library/proto/protobuf.h>

#include <util/string/builder.h>

namespace NAlice::NHollywood::NMusic {

TCallbackPayloadBuilder& TCallbackPayloadBuilder::AddPlayAudioEvent(
    const TString& from, const TContentId& contentId,
    const TString& trackId, const TMaybe<TString>& albumId,
    const TMaybe<TString>& albumType, const TString& playId,
    const TStringBuf uid, const bool incognito, const bool shouldSaveProgress,
    const TMaybe<TStringBuf>& radioSessionId, const TMaybe<TStringBuf>& batchId)
{
    auto builder = TPlayAudioEventBuilder()
        .From(from)
        .Context(ContentTypeToText(contentId.GetType()))
        .ContextItem(contentId.GetId())
        .TrackId(trackId)
        .PlayId(playId) // It is important to have a different random PlayId for each playback, even for the
                        // same trackId to have correct statistics reports about music usage
        .Uid(TString(uid))
        .Incognito(incognito)
        .ShouldSaveProgress(shouldSaveProgress);
    if (albumId) {
        builder.AlbumId(albumId.GetRef());
    }
    if (albumType) {
        builder.AlbumType(albumType.GetRef());
    }
    if (contentId.GetType() == TContentId_EContentType_Playlist) {
        builder.PlaylistId(contentId.GetId());
    }
    if (radioSessionId) {
        builder.RadioSessionId(TString{*radioSessionId});
    }
    if (batchId) {
        builder.BatchId(TString{*batchId});
    }
    *Payload_.AddEvents()->MutablePlayAudioEvent() = builder.BuildProto();
    return *this;
}

TCallbackPayloadBuilder& TCallbackPayloadBuilder::AddGenerativeFeedbackEvent(
    TGenerativeFeedbackEvent_EType eventType, const TString& generativeStationId,
    const TString& streamId, const TMaybe<TString>& guestOAuthTokenEncrypted)
{
    auto& event = *Payload_.AddEvents()->MutableGenerativeFeedbackEvent();
    event.SetType(eventType);
    event.SetGenerativeStationId(generativeStationId);
    event.SetStreamId(streamId);
    if (guestOAuthTokenEncrypted) {
        event.SetGuestOAuthTokenEncrypted(*guestOAuthTokenEncrypted);
    }
    return *this;
}

TCallbackPayloadBuilder& TCallbackPayloadBuilder::AddRadioStartedFeedbackEvent(const TString& stationId,
                                                                               const TString& radioSessionId,
                                                                               const TMaybe<TString>& guestOAuthTokenEncrypted)
{
    auto& event = *Payload_.AddEvents()->MutableRadioFeedbackEvent();
    event.SetType(TRadioFeedbackEvent_EType_RadioStarted);
    event.SetStationId(stationId);
    event.SetRadioSessionId(radioSessionId);
    if (guestOAuthTokenEncrypted) {
        event.SetGuestOAuthTokenEncrypted(*guestOAuthTokenEncrypted);
    }
    return *this;
}

TCallbackPayloadBuilder& TCallbackPayloadBuilder::AddRadioFeedbackEvent(
    const TRadioFeedbackEvent_EType eventType, const TString& stationId,
    const TString& batchId, const TString& trackId, const TString& albumId,
    const TString& radioSessionId, const TMaybe<TString>& guestOAuthTokenEncrypted)
{
    auto& event = *Payload_.AddEvents()->MutableRadioFeedbackEvent();
    event.SetType(eventType);
    event.SetStationId(stationId);
    event.SetBatchId(batchId);
    event.SetTrackId(TStringBuilder{} << trackId << ':' << albumId);
    event.SetRadioSessionId(radioSessionId);
    if (guestOAuthTokenEncrypted) {
        event.SetGuestOAuthTokenEncrypted(*guestOAuthTokenEncrypted);
    }
    return *this;
}

TCallbackPayloadBuilder& TCallbackPayloadBuilder::AddShotFeedbackEvent(const TExtraPlayable_TShot& shot,
                                                                       TShotFeedbackEvent_EType eventType,
                                                                       const TStringBuf uid) {
    auto& event = *Payload_.AddEvents()->MutableShotFeedbackEvent();
    event.SetShotId(shot.GetId());
    event.SetEventId(shot.GetEventId());
    event.SetType(eventType);
    event.SetFrom(shot.GetFrom());
    event.SetPrevTrackId(shot.GetPrevTrackId());
    event.SetNextTrackId(shot.GetNextTrackId());
    event.SetContext(shot.GetContext());
    event.SetContextItem(shot.GetContextItem());
    event.SetUid(TString(uid));
    return *this;
}

google::protobuf::Struct TCallbackPayloadBuilder::Build() {
    return MessageToStruct(Payload_);
}

} // namespace NAlice::NHollywood::NMusic
