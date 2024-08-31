#include "builder.h"

#include <alice/megamind/library/experiments/flags.h>
#include <alice/megamind/library/serializers/meta.h>

#include <alice/megamind/protos/common/content_properties.pb.h>
#include <alice/megamind/protos/common/response_error_message.pb.h>

#include <alice/library/analytics/scenario/builder.h>
#include <alice/library/experiments/flags.h>
#include <alice/library/experiments/utils.h>
#include <alice/library/json/json.h>
#include <alice/library/response/defs.h>
#include <alice/library/video_common/defs.h>

#include <google/protobuf/util/json_util.h>

#include <library/cpp/json/json_writer.h>
#include <library/cpp/protobuf/json/json2proto.h>
#include <library/cpp/protobuf/json/proto2json.h>

#include <util/generic/guid.h>
#include <util/generic/hash.h>
#include <util/string/join.h>

namespace NAlice {

namespace {

using namespace NVideoCommon;

template <typename TToType, typename TProtoContainer>
TVector<TToType> DecodeProtos(const TProtoContainer& protos) {
    TVector<TToType> result(Reserve(protos.size()));
    for (const auto& proto : protos) {
        result.push_back(TToType::FromProto(proto));
    }
    return result;
}

} // namespace

// TResponseBuilder ------------------------------------------------------------
TResponseBuilder::TResponseBuilder(const TSpeechKitRequest& skr, const TRequest& requestModel,
                                   const TString& scenarioName,
                                   TResponseBuilderProto& storage, const NMegamind::IGuidGenerator& guidGenerator,
                                   const TMaybe<TString>& serializerScenarioName)
    : ProtoStorage{storage}
{
    InitProtoFromScratch(skr, requestModel, scenarioName, guidGenerator, serializerScenarioName);
}

void TResponseBuilder::Reset(const TSpeechKitRequest& skr, const TRequest& requestModel,
                             const NMegamind::IGuidGenerator& guidGenerator) {
    TString scenarioName = Storage().GetScenarioName();
    Storage().Clear();
    InitProtoFromScratch(skr, requestModel, scenarioName, guidGenerator);
}

void TResponseBuilder::Reset(const TSpeechKitRequest& skr, const TRequest& requestModel,
                             const NMegamind::IGuidGenerator& guidGenerator,
                             const TString& scenarioName) {
    Storage().Clear();
    InitProtoFromScratch(skr, requestModel, scenarioName, guidGenerator);
}

// private
TResponseBuilder::TResponseBuilder(const TSpeechKitRequest& skr, const TRequest& requestModel,
                                   TResponseBuilderProto& storage,
                                   const NMegamind::IGuidGenerator& guidGenerator)
    : ProtoStorage{storage}
{
    InitRequiredFields(skr, requestModel, guidGenerator);
}

void TResponseBuilder::InitProtoFromScratch(const TSpeechKitRequest& skr, const TRequest& requestModel,
                                            const TString& scenarioName,
                                            const NMegamind::IGuidGenerator& guidGenerator,
                                            const TMaybe<TString>& serializerScenarioName) {
    auto& response = *Storage().MutableResponse();

    Storage().SetScenarioName(scenarioName);

    response.MutableHeader()->SetRequestId(skr->GetHeader().GetRequestId());
    response.MutableHeader()->SetDialogId(skr->GetHeader().GetDialogId());
    response.MutableHeader()->SetRefMessageId(skr->GetHeader().GetRefMessageId());
    response.MutableHeader()->SetSessionId(skr->GetHeader().GetSessionId());
    const auto* flag = skr.ExpFlags().FindPtr(NExperiments::ANALYTICS_INFO);
    if (flag && flag->Defined()) {
        NMegamind::TExpFlagsToStructVisitor{*response.MutableResponse()->MutableExperiments()}
            .Visit(skr->GetRequest().GetExperiments());
    }

    InitRequiredFields(skr, requestModel, guidGenerator, serializerScenarioName);
}

void TResponseBuilder::InitRequiredFields(const TSpeechKitRequest& speechKitRequest,
                                          const TRequest& requestModel,
                                          const NMegamind::IGuidGenerator& guidGenerator,
                                          const TMaybe<TString>& serializerScenarioName) {
    const auto& srcResponse = Storage().GetResponse();
    const auto& srcResponseHeader = srcResponse.GetHeader();
    auto& dstHeader = *Storage().MutableResponse()->MutableHeader();

    if (!srcResponseHeader.HasResponseId()) {
        dstHeader.SetResponseId(guidGenerator.GenerateGuid());
    }

    const auto& requestHeader = speechKitRequest->GetHeader();
    if (requestHeader.HasSequenceNumber()) {
        dstHeader.SetSequenceNumber(requestHeader.GetSequenceNumber());
    }

    const auto& request = speechKitRequest->GetRequest();
    const auto& event = speechKitRequest.Event();
    if (requestModel.GetDisableVoiceSession()) {
        Storage().SetShouldAddOutputSpeech(false);
    } else {
        const bool isVoiceInput = event.HasType() && event.GetType() == EEventType::voice_input;
        Storage().SetShouldAddOutputSpeech(request.HasVoiceSession()
                                               ? request.GetVoiceSession()
                                               : isVoiceInput);
    }

    // If client use any special params to force disable should_listen (e.g TFrameRequestParams),
    // we should set should_listen=false and block any changes (from TResponseBuilder::ShouldListen) by ForceDisableShouldListen parameter.
    // Otherwise we set the default or already specified value.
    if (requestModel.GetDisableShouldListen()) {
        Storage().SetForceDisableShouldListen(true);
        Storage().MutableResponse()->MutableVoiceResponse()->SetShouldListen(false);
    } else {
        Storage().MutableResponse()->MutableVoiceResponse()->SetShouldListen(
            srcResponse.GetVoiceResponse().GetShouldListen());
    }

    SerializerMeta = NMegamind::TSerializerMeta(serializerScenarioName.Defined()
                                                    ? *serializerScenarioName
                                                    : Storage().GetScenarioName(),
                                                srcResponseHeader.GetRequestId(),
                                                speechKitRequest.ClientInfo(),
                                                requestModel.GetIotUserInfo(),
                                                request.GetSmartHomeInfo(),
                                                request.GetAdditionalOptions(),
                                                guidGenerator);
    SpeechKitProtoSerializer = NMegamind::TSpeechKitProtoSerializer(SerializerMeta);
}

IResponseBuilder& TResponseBuilder::AddSimpleText(const TString& text, bool appendTts) {
    return AddSimpleText(text, text, appendTts);
}

IResponseBuilder& TResponseBuilder::AddSimpleText(const TString& text, const TString& tts, bool appendTts) {
    // Voice
    if (tts && Storage().GetShouldAddOutputSpeech()) {
        auto& outputSpeech = *Storage().MutableResponse()->MutableVoiceResponse()->MutableOutputSpeech();
        outputSpeech.SetType(NResponse::SIMPLE);
        const auto oldTts = outputSpeech.GetText();
        outputSpeech.SetText((appendTts && oldTts) ? (oldTts + " " + tts) : tts);
    }

    // Text
    if (text) {
        auto& card = *Storage().MutableResponse()->MutableResponse()->AddCards();
        card.SetType(NResponse::SIMPLE_TEXT);
        card.SetText(text);
    }

    return *this;
}

IResponseBuilder& TResponseBuilder::SetResponseErrorMessage(const TResponseErrorMessage& responseErrorMessage) {
    Storage().MutableResponse()->MutableResponse()->MutableResponseErrorMessage()->CopyFrom(responseErrorMessage);
    return *this;
}

IResponseBuilder& TResponseBuilder::ShouldListen(bool value) {
    if (Storage().GetForceDisableShouldListen()) {
        return *this;
    }
    Storage().MutableResponse()->MutableVoiceResponse()->SetShouldListen(value);
    return *this;
}

IResponseBuilder& TResponseBuilder::SetIsTrashPartial(bool value) {
    Storage().MutableResponse()->MutableVoiceResponse()->SetIsTrashPartial(value);
    return *this;
}

IResponseBuilder& TResponseBuilder::SetContentProperties(const TContentProperties& contentProperties) {
    Storage().MutableResponse()->MutableContentProperties()->CopyFrom(contentProperties);
    // backward compatibility
    Storage().MutableResponse()->SetContainsSensitiveData(contentProperties.GetContainsSensitiveDataInRequest() ||
                                                          contentProperties.GetContainsSensitiveDataInResponse());
    return *this;
}

const TSpeechKitResponseProto& TResponseBuilder::GetSKRProto() const {
    return Storage().GetResponse();
}

TSpeechKitResponseProto& TResponseBuilder::GetSKRMutableProto() {
    return *Storage().MutableResponse();
}

TString TResponseBuilder::GetRenderedSpeech() const {
    if (Storage().GetResponse().HasVoiceResponse()) {
        const auto& voiceResponse = Storage().GetResponse().GetVoiceResponse();
        if (voiceResponse.HasOutputSpeech()) {
            return voiceResponse.GetOutputSpeech().GetText();
        }
    }
    return Default<TString>();
}

TString TResponseBuilder::GetRenderedText() const {
    const auto& cards = Storage().GetResponse().GetResponse().GetCards();
    if (!cards.empty()) {
        return cards[0].GetText();
    }
    return Default<TString>();
}

TString TResponseBuilder::GetRenderedResponse() const {
    auto speech = GetRenderedSpeech();
    return speech ? speech : GetRenderedText();
}

bool TResponseBuilder::GetShouldListen(bool def) const {
    if (Storage().GetResponse().HasVoiceResponse()) {
        const auto& voiceResponse = Storage().GetResponse().GetVoiceResponse();
        if (voiceResponse.HasShouldListen()) {
            return voiceResponse.GetShouldListen();
        }
    }
    return def;
}

IResponseBuilder& TResponseBuilder::AddError(const TString& errorType, const TString& message) {
    auto& meta = *Storage().MutableResponse()->MutableResponse()->AddMeta();
    meta.SetType(NResponse::TYPE_ERROR);
    meta.SetErrorType(errorType);
    meta.SetMessage(message);
    return *this;
}

IResponseBuilder& TResponseBuilder::AddAnalyticsAttention(const TString& type) {
    auto& meta = *Storage().MutableResponse()->MutableResponse()->AddMeta();
    meta.SetType(NResponse::TYPE_ATTENTION);
    meta.SetAttentionType(type);
    return *this;
}

IResponseBuilder& TResponseBuilder::AddMeta(const TString& type, const NJson::TJsonValue& payload) {
    auto& meta = *Storage().MutableResponse()->MutableResponse()->AddMeta();
    meta.SetType(type);
    *meta.MutablePayload() = JsonToProto<google::protobuf::Struct>(payload, /* validateUtf8= */ true);
    return *this;
}

// static
TResponseBuilder TResponseBuilder::FromProto(const TSpeechKitRequest& request, const TRequest& requestModel,
                                             TResponseBuilderProto& proto,
                                             const NMegamind::IGuidGenerator& guidGenerator) {
    if (!proto.HasResponse())
        ythrow yexception() << "No Response field in serialized TResponseBuilder";
    return TResponseBuilder{request, requestModel, proto, guidGenerator};
}

TStatus TResponseBuilder::PutSemanticFrame(const TSemanticFrame& frame) {
    Storage().MutableSemanticFrame()->CopyFrom(frame);
    return Success();
}

TMaybe<TSemanticFrame> TResponseBuilder::GetSemanticFrame() const {
    if (!Storage().HasSemanticFrame()) {
        return Nothing();
    }
    return Storage().GetSemanticFrame();
}

TStatus TResponseBuilder::PutActions(const ::google::protobuf::Map<TString, NScenarios::TFrameAction>& actions) {
    *Storage().MutableActions() = actions;
    return Success();
}

::google::protobuf::Map<TString, NScenarios::TFrameAction> TResponseBuilder::GetActions() const {
    return Storage().GetActions();
}

IResponseBuilder& TResponseBuilder::PutAction(const TString& key, const NScenarios::TFrameAction& action) {
    (*Storage().MutableActions())[key] = action;
    return *this;
}

TStatus TResponseBuilder::PutEntities(const ::google::protobuf::RepeatedPtrField<TClientEntity>& entities) {
    *Storage().MutableEntities() = entities;
    return Success();
}

::google::protobuf::RepeatedPtrField<TClientEntity> TResponseBuilder::GetEntities() const {
    return Storage().GetEntities();
}

IResponseBuilder& TResponseBuilder::AddDirective(const NMegamind::IDirectiveModel& directive) {
    // TODO(alkapov): handle possible convert exceptions
    *Storage().MutableResponse()->MutableResponse()->AddDirectives() =
        SpeechKitProtoSerializer.Serialize(directive);
    return *this;
}

IResponseBuilder& TResponseBuilder::AddProtobufUniproxyDirective(const NMegamind::IDirectiveModel& model) {
    auto directive = SpeechKitProtoSerializer.SerializeProtobufUniproxyDirective(model);
    if (directive.Defined()) {
        directive->Swap(Storage().MutableResponse()->MutableVoiceResponse()->AddUniproxyDirectives());
    }
    return *this;
}

IResponseBuilder& TResponseBuilder::AddFrontDirective(const NMegamind::IDirectiveModel& directive) {
    // TODO(alkapov): handle possible convert exceptions
    const auto& directives = Storage().GetResponse().GetResponse().GetDirectives();
    google::protobuf::RepeatedPtrField<NAlice::NSpeechKit::TDirective> newDirectives;
    *newDirectives.Add() = SpeechKitProtoSerializer.Serialize(directive);
    newDirectives.Add(directives.begin(), directives.end());

    Storage().MutableResponse()->MutableResponse()->MutableDirectives()->Swap(&newDirectives);
    return *this;
}

IResponseBuilder& TResponseBuilder::AddDirectiveToVoiceResponse(const NMegamind::IDirectiveModel& directive) {
    *Storage().MutableResponse()->MutableVoiceResponse()->AddDirectives() =
        SpeechKitProtoSerializer.Serialize(directive);
    return *this;
}

IResponseBuilder& TResponseBuilder::AddCard(const NMegamind::ICardModel& card) {
    // TODO(alkapov): handle possible convert exceptions
    *Storage().MutableResponse()->MutableResponse()->AddCards() =
        SpeechKitProtoSerializer.Serialize(card);
    return *this;
}

IResponseBuilder& TResponseBuilder::AddSuggest(const NMegamind::IButtonModel& button) {
    // TODO(alkapov): handle possible convert exceptions
    *Storage().MutableResponse()->MutableResponse()->MutableSuggest()->AddItems() =
        SpeechKitProtoSerializer.Serialize(button);
    return *this;
}

IResponseBuilder& TResponseBuilder::SetOutputSpeech(const TString& text) {
    auto& storage = Storage();
    if (storage.GetShouldAddOutputSpeech()) {
        auto& outputSpeech = *storage.MutableResponse()->MutableVoiceResponse()->MutableOutputSpeech();
        outputSpeech.SetType(NResponse::SIMPLE);
        outputSpeech.SetText(text);
    }
    return *this;
}

IResponseBuilder& TResponseBuilder::SetOutputEmotion(const TString& outputEmotion) {
    Storage().MutableResponse()->MutableVoiceResponse()->SetOutputEmotion(outputEmotion);
    return *this;
}

IResponseBuilder& TResponseBuilder::SetDirectivesExecutionPolicy(EDirectivesExecutionPolicy value) {
    GetSKRMutableProto().MutableResponse()->SetDirectivesExecutionPolicy(value);
    return *this;
}

IResponseBuilder& TResponseBuilder::SetProductScenarioName(const TString& psn) {
    Storage().SetProductScenarioName(psn);
    return *this;
}

} // namespace NAlice
