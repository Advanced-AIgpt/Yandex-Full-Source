#pragma once

#include <alice/hollywood/library/scenarios/music/music_backend_api/multiroom.h>
#include <alice/hollywood/library/scenarios/music/proto/music_context.pb.h>
#include <alice/megamind/protos/scenarios/directives.pb.h>

#include <util/generic/maybe.h>

namespace NAlice::NHollywood::NMusic {

void FillGlagolMetadata(::NAlice::NScenarios::TAudioPlayDirective_TAudioPlayMetadata_TGlagolMetadata& glagolMetadata,
                        TMaybe<TContentId> contentId, TMaybe<TString> prevTrackId,
                        TMaybe<TString> nextTrackId, TMaybe<bool> shuffled,
                        TMaybe<ERepeatType> repeatType
);

class TAudioPlayBuilder {
public:
    TAudioPlayBuilder(const TQueueItem& item, const i32 offsetMs,
                      TMaybe<TContentId> contentId, TMaybe<TString> prevTrackId,
                      TMaybe<TString> nextTrackId, TMaybe<bool> shuffled, TMaybe<ERepeatType> repeatType,
                      const bool needSetPauseAtStart = false, const TMultiroomTokenWrapper* multiroomToken = nullptr);

    TAudioPlayBuilder& AddOnStartedCallback(google::protobuf::Struct payload);
    TAudioPlayBuilder& AddOnStoppedCallback(google::protobuf::Struct payload);
    TAudioPlayBuilder& AddOnFinishedCallback(google::protobuf::Struct payload);
    TAudioPlayBuilder& AddOnFailedCallback(google::protobuf::Struct payload);

    NScenarios::TDirective BuildProto();

private:
    NScenarios::TDirective Proto_;
    NScenarios::TAudioPlayDirective& AudioPlayProto_;
};

} // NAlice::NHollywood::NMusic
