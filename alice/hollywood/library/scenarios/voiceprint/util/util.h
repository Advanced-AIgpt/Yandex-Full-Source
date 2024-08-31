#pragma once

#include <alice/hollywood/library/scenarios/voiceprint/common.h>
#include <alice/hollywood/library/scenarios/voiceprint/proto/voiceprint.pb.h>

#include <alice/hollywood/library/base_scenario/scenario.h>
#include <alice/hollywood/library/nlg/nlg_wrapper.h>
#include <alice/hollywood/library/response/response_builder.h>

#include <alice/megamind/protos/guest/guest_data.pb.h>
#include <alice/megamind/protos/guest/guest_options.pb.h>
#include <alice/megamind/protos/scenarios/request.pb.h>
#include <alice/megamind/protos/scenarios/response.pb.h>
#include <alice/protos/data/scenario/voiceprint/personalization_data.pb.h>

#include <util/generic/maybe.h>
#include <util/generic/strbuf.h>
#include <util/generic/string.h>

namespace NAlice::NHollywood::NVoiceprint {

enum class EIrrelevantType {
    UnknownStage /* "unknown_stage" */,
    UnsupportedSurface /* "unsupported_surface" */,
    UnsupportedFeature /* "unsupported_feature" */,
    NotActive /* "not_active_scenario" */,
    UnexpectedFrame /* "unexpected_frame" */,
};

class TVoiceprintHandleContext : public NNonCopyable::TNonCopyable {
public:
    TVoiceprintHandleContext(TScenarioHandleContext& ctx);

    const TFrame* FindFrame(TStringBuf frameName);
    const TSemanticFrame* FindSemanticFrame(TStringBuf frameName);

public:
    TScenarioHandleContext& Ctx;
    TRTLogger& Logger;
    const NScenarios::TScenarioRunRequest RequestProto;
    const TScenarioRunRequestWrapper Request;
    TNlgWrapper Nlg;

    const TBlackBoxUserInfo* UserInfo;
    TVoiceprintState ScenarioStateProto;

private:
    THashMap<TString, std::unique_ptr<TFrame>> Frames_;
    THashMap<TString, const TSemanticFrame*> SemanticFrames_;
};

bool IsCollectingPhrases(const TVoiceprintState& scState);

TMaybe<TString> GetUserNameFromDataSync(
    const TScenarioRunRequestWrapper& request,
    const TString& ownerUid,
    const TString& requesterUid,
    const TGuestData* guestData,
    const TString& persId
);

void UpdateVoiceprintEnrollState(TVoiceprintEnrollState& enrollState);
NAlice::NData::NVoiceprint::EGender UpdateGender(TVoiceprintEnrollState::EGender oldGender);
TVoiceprintEnrollState::EGender RollbackGender(NAlice::NData::NVoiceprint::EGender gender);

bool HasMatch(const TGuestOptions* guestOptions);
bool IsBioCapabilitySupported(const TScenarioRunRequestWrapper& request);
bool IsValidRegionImpl(const TScenarioHandleContext& ctx, const TScenarioRunRequestWrapper& request);

void Irrelevant(TRTLogger& logger, const TScenarioRunRequestWrapper& request, TRunResponseBuilder& builder, EIrrelevantType type, TStringBuf msg = {});

void AddSlot(NJson::TJsonArray& out, const TStringBuf& name, NJson::TJsonValue value);

void AddEnrollmentFinishDirective(
    TRTLogger& logger,
    const TVoiceprintEnrollState& enrollState,
    TResponseBodyBuilder& bodyBuilder,
    bool sendGuestEnrollmentFinishFrame
);

void AddSaveVoiceprintDirective(TRTLogger& logger, const TVoiceprintEnrollState& enrollState, TResponseBodyBuilder& bodyBuilder);

NScenarios::TServerDirective MakeUpdateDatasyncDirective(
    TRTLogger& logger,
    const TString& key,
    const TString& value,
    NScenarios::TServerDirective::TMeta::EApplyFor applyFor
);

} // namespace NAlice::NHollywood::NVoiceprint
