#include "util.h"

#include <alice/hollywood/library/biometry/client_biometry.h>
#include <alice/hollywood/library/environment_state/endpoint.h>
#include <alice/hollywood/library/personal_data/personal_data.h>
#include <alice/hollywood/library/request/experiments.h>
#include <alice/hollywood/library/request/request.h>
#include <alice/hollywood/library/resources/geobase.h>

#include <alice/library/geo/geodb.h>

#include <alice/megamind/protos/scenarios/directives.pb.h>
#include <alice/protos/endpoint/capabilities/bio/capability.pb.h>

#include <kernel/geodb/countries.h>
#include <util/string/cast.h>
#include <util/string/join.h>

namespace NAlice::NHollywood::NVoiceprint {

namespace {

TVoiceprintState ConstructState(const TScenarioRunRequestWrapper& request) {
    TVoiceprintState state;
    const auto& rawState = request.BaseRequestProto().GetState();
    if (rawState.Is<TVoiceprintState>() && !request.IsNewSession()) {
        rawState.UnpackTo(&state);
    }

    return state;
}

} // namespace

TVoiceprintHandleContext::TVoiceprintHandleContext(TScenarioHandleContext& ctx)
    : Ctx{ctx}
    , Logger{Ctx.Ctx.Logger()}
    , RequestProto{GetOnlyProtoOrThrow<NScenarios::TScenarioRunRequest>(ctx.ServiceCtx, REQUEST_ITEM)}
    , Request{RequestProto, Ctx.ServiceCtx}
    , Nlg{TNlgWrapper::Create(Ctx.Ctx.Nlg(), Request, Ctx.Rng, Ctx.UserLang)}

    , UserInfo{GetUserInfoProto(Request)}
    , ScenarioStateProto{ConstructState(Request)}
{}

const TFrame* TVoiceprintHandleContext::FindFrame(TStringBuf frameName) {
    if (!Frames_.contains(frameName)) {
        if (const auto frameProto = Request.Input().FindSemanticFrame(frameName)) {
            SemanticFrames_[frameName] = frameProto.Get();
            auto frame = TFrame::FromProto(*frameProto);
            Frames_[frameName] = std::make_unique<TFrame>(std::move(frame));
        } else {
            Frames_[frameName] = nullptr;
        }
    }
    return Frames_.at(frameName).get();
}

const TSemanticFrame* TVoiceprintHandleContext::FindSemanticFrame(TStringBuf frameName) {
    if (!FindFrame(frameName)) {
        return nullptr;
    }
    return SemanticFrames_.at(frameName);
}

void UpdateVoiceprintEnrollState(TVoiceprintEnrollState& enrollState) {
    auto oldGender = enrollState.GetGender();
    auto updatedGender = UpdateGender(oldGender);
    if (enrollState.GetGenderMementoReady() == updatedGender
        || enrollState.GetGenderMementoReady() != NAlice::NData::NVoiceprint::EGender::Undefined)
    {
        enrollState.SetGenderMementoReady(updatedGender);
    }
}

NAlice::NData::NVoiceprint::EGender UpdateGender(TVoiceprintEnrollState::EGender oldGender) {
    switch (oldGender) {
        case TVoiceprintEnrollState::Undefined:
            return NAlice::NData::NVoiceprint::EGender::Undefined;
        case TVoiceprintEnrollState::Male:
            return NAlice::NData::NVoiceprint::EGender::Male;
        case TVoiceprintEnrollState::Female:
            return NAlice::NData::NVoiceprint::EGender::Female;
        default:
            throw yexception() << "Unexpected TVoiceprintEnrollState::EGender value: " << TVoiceprintEnrollState::EGender_Name(oldGender);
    }
}

TVoiceprintEnrollState::EGender RollbackGender(NAlice::NData::NVoiceprint::EGender gender) {
    switch (gender) {
        case NAlice::NData::NVoiceprint::EGender::Undefined:
            return TVoiceprintEnrollState::Undefined;
        case NAlice::NData::NVoiceprint::EGender::Male:
            return TVoiceprintEnrollState::Male;
        case NAlice::NData::NVoiceprint::EGender::Female:
            return TVoiceprintEnrollState::Female;
        default:
            throw yexception() << "Unexpected NAlice::NData::NVoiceprint::EGender value: " << NAlice::NData::NVoiceprint::EGender_Name(gender);
    }
}

bool IsCollectingPhrases(const TVoiceprintState& scState) {
    return scState.HasVoiceprintEnrollState() && scState.GetVoiceprintEnrollState().GetCurrentStage() != TVoiceprintEnrollState::NotStarted;
}

TMaybe<TString> GetUserNameFromDataSync(
    const TScenarioRunRequestWrapper& request,
    const TString& ownerUid,
    const TString& requesterUid,
    const TGuestData* guestData,
    const TString& persId
)
{
    const auto clientInfo = request.ClientInfo();
    if (ownerUid == requesterUid) {
        return GetOwnerNameFromDataSync(request);
    } else {
        if (!guestData || !guestData->HasRawPersonalData() || guestData->GetRawPersonalData().Empty()) {
            return Nothing();
        }

        const auto dsGuestUserNameKey = ToPersonalDataKey(clientInfo, NAlice::NDataSync::EEnrollmentSpecificKey::UserName, persId);
        NJson::TJsonValue rawPersonalDataMap;
        rawPersonalDataMap.SetType(NJson::JSON_MAP);
        Y_ENSURE(NJson::ReadJsonTree(guestData->GetRawPersonalData(), &rawPersonalDataMap));

        const NJson::TJsonValue* result;
        if (!rawPersonalDataMap.GetValuePointer(dsGuestUserNameKey, &result)) {
            return Nothing();
        } else {
            return result->GetStringSafe();
        }
    }
}

bool HasMatch(const TGuestOptions* guestOptions) {
    return guestOptions && guestOptions->GetStatus() == NAlice::TGuestOptions::Match;
}

bool IsBioCapabilitySupported(const TScenarioRunRequestWrapper& request) {
    if (!request.HasExpFlag(EXP_HW_VOICEPRINT_ENABLE_BIO_CAPABILITY)) {
        return false;
    }
    return SupportsClientBiometry(request);
}

bool IsValidRegionImpl(const TScenarioHandleContext& ctx, const TScenarioRunRequestWrapper& request) {
    const auto& geoLookup = ctx.Ctx.GlobalContext().CommonResources().Resource<TGeobaseResource>().GeobaseLookup();
    auto userRegion = GetUserLocation(request).UserRegion();
    return NAlice::IsValidId(userRegion) && geoLookup.IsIdInRegion(userRegion, NGeoDB::RUSSIA_ID);
}

void Irrelevant(TRTLogger& logger, const TScenarioRunRequestWrapper& request, TRunResponseBuilder& builder, EIrrelevantType type, TStringBuf msg) {
    const TString typeStr = ToString(type);
    LOG_INFO(logger) << "Drop as irrelevant because " << typeStr << (msg ? TString::Join(": ", msg) : "");
    builder.SetIrrelevant();

    auto& bodyBuilder = builder.CreateResponseBodyBuilder();
    TNlgData nlgData{logger, request};
    nlgData.Context["type"] = typeStr;
    bodyBuilder.AddRenderedTextWithButtonsAndVoice(NLG_IRRELEVANT, "render_result", /* buttons= */{}, nlgData);
}

void AddSlot(NJson::TJsonArray& out, const TStringBuf& name, NJson::TJsonValue value) {
    auto slot = NJson::TJsonMap();
    slot["name"] = name;
    slot["value"] = value;
    out.AppendValue(std::move(slot));
}

void AddSaveVoiceprintDirective(TRTLogger& logger, const TVoiceprintEnrollState& enrollState, TResponseBodyBuilder& bodyBuilder) {
    NScenarios::TDirective directive;
    auto& saveVPDirective = *directive.MutableSaveVoiceprintDirective();
    saveVPDirective.SetUserId(enrollState.GetUid());
    saveVPDirective.SetPersId(enrollState.GetPersId());
    if (enrollState.GetGuestPuid().Empty()) {
        saveVPDirective.SetUserType(NScenarios::TSaveVoiceprintDirective::Owner);
    } else {
        saveVPDirective.SetUserType(NScenarios::TSaveVoiceprintDirective::Guest);
    }
    auto& mutableRequests = *saveVPDirective.MutableRequestIds();
    const auto requestsIds = enrollState.GetRequestIds();
    for (const auto& request : requestsIds) {
        *mutableRequests.Add() = request;
    }
    LOG_INFO(logger) << "save_voiceprint directive: " << directive;
    bodyBuilder.AddDirective(std::move(directive));
}

void AddEnrollmentFinishDirective(
    TRTLogger& logger,
    const TVoiceprintEnrollState& enrollState,
    TResponseBodyBuilder& bodyBuilder,
    bool sendGuestEnrollmentFinishFrame
)
{
    NScenarios::TDirective directive;
    auto& enrollFinishDirective = *directive.MutableEnrollmentFinishDirective();
    enrollFinishDirective.SetPersId(enrollState.GetPersId());
    enrollFinishDirective.SetSendGuestEnrollmentFinishFrame(sendGuestEnrollmentFinishFrame);
    LOG_INFO(logger) << "AddEnrollmentFinishDirective: " << directive;
    bodyBuilder.AddDirective(std::move(directive));
    bodyBuilder.AddTtsPlayPlaceholderDirective();
}

NScenarios::TServerDirective MakeUpdateDatasyncDirective(
    TRTLogger& logger,
    const TString& key,
    const TString& value,
    NScenarios::TServerDirective::TMeta::EApplyFor applyFor
)
{
    NScenarios::TServerDirective directive;
    auto& mutableUpdateDS = *directive.MutableUpdateDatasyncDirective();
    mutableUpdateDS.SetKey(key);
    mutableUpdateDS.SetStringValue(value);
    mutableUpdateDS.SetMethod(NScenarios::TUpdateDatasyncDirective::Put);
    directive.MutableMeta()->SetApplyFor(applyFor);
    LOG_INFO(logger) << "update_datasync directive: " << directive;
    return directive;
}

} // namespace NAlice::NHollywood::NVoiceprint