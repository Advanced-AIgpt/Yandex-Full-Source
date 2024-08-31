#pragma once

#include <alice/hollywood/library/scenarios/music/proto/music_context.pb.h>
#include <alice/hollywood/library/scenarios/music/proto/play_audio.pb.h>
#include <alice/megamind/protos/scenarios/directives.pb.h>

#include <util/generic/maybe.h>

namespace NAlice::NHollywood::NMusic {

class TCallbackPayloadBuilder {
public:
    TCallbackPayloadBuilder& AddPlayAudioEvent(const TString& from, const TContentId& contentId,
                                               const TString& trackId,
                                               const TMaybe<TString>& albumId, const TMaybe<TString>& albumType,
                                               const TString& playId, const TStringBuf uid, const bool incognito,
                                               const bool shouldSaveProgress, const TMaybe<TStringBuf>& radioSessionId = Nothing(),
                                               const TMaybe<TStringBuf>& batchId = Nothing());

    TCallbackPayloadBuilder& AddGenerativeFeedbackEvent(TGenerativeFeedbackEvent_EType eventType, const TString& generativeStationId,
                                                        const TString& streamId, const TMaybe<TString>& guestOAuthTokenEncrypted);

    TCallbackPayloadBuilder& AddRadioStartedFeedbackEvent(const TString& stationId,
                                                          const TString& radioSessionId,
                                                          const TMaybe<TString>& guestOAuthTokenEncrypted);

    TCallbackPayloadBuilder& AddRadioFeedbackEvent(const TRadioFeedbackEvent_EType eventType, const TString& stationId,
                                                   const TString& batchId, const TString& trackId,
                                                   const TString& albumId, const TString& radioSessionId,
                                                   const TMaybe<TString>& guestOAuthTokenEncrypted);

    TCallbackPayloadBuilder& AddShotFeedbackEvent(const TExtraPlayable_TShot& shot, TShotFeedbackEvent_EType eventType,
                                                  const TStringBuf uid);

    google::protobuf::Struct Build();

private:
    TCallbackPayload Payload_;
};

} // namespace NAlice::NHollywood::NMusic
