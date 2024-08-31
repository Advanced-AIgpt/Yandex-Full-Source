//
// HOLLYWOOD FRAMEWORK
// Additional render object
// Render object contains request and scenario specific data like NLG
//

#include "render.h"
#include "render_impl.h"

#include "error.h"
#include "node_caller.h"
#include "request.h"

#include <alice/hollywood/library/framework/core/codegen/gen_directives.pb.h>
#include <alice/hollywood/library/framework/core/codegen/gen_server_directives.pb.h>

#include <alice/hollywood/library/response/response_builder.h>
#include <alice/library/json/json.h>
#include <alice/protos/endpoint/capability.pb.h>

namespace NAlice::NHollywoodFw {

/*
    TRender ctor
*/
TRender::TRender(NPrivate::TNodeCaller& caller, const TMaybe<NHollywood::TCompiledNlgComponent>& nlg)
    : Request_(caller.GetBaseRequest())
    , ResponseBody_(*caller.GetBaseResponseProto())
    , Impl_(new NPrivate::TRenderImpl(caller.GetBaseRequest(), nlg))
    , Directives_(new TDirectivesWrapper)
    , ServerDirectives_(new TServerDirectivesWrapper)
{
    // Add DivRenderData if exists in caller.GetApphostCtx()
    const auto& items = caller.GetApphostCtx().GetProtobufItemRefs(NHollywood::RENDER_DATA_RESULT);
    for (const auto& it : items) {
        DivRenderResponse_.push_back(std::make_shared<NRenderer::TRenderResponse>(ParseProto<NRenderer::TRenderResponse>(it.Raw())));
    }
}

/*
    TRender dtor
*/
TRender::~TRender() {
}

/*
    Convert complex protobuf variable into internal NLG Context variable
    This function is used for variables like "datetime", "citypreparse", etc.
*/
void TRender::MakeComplexVar(TStringBuf varName, const NJson::TJsonValue& jsonContext) {
    Impl_->MakeComplexVar(varName, jsonContext);
}

/*
    Render phrase and return direct pointer to textvoice answer
*/
NNlg::TRenderPhraseResult TRender::RenderPhrase(TStringBuf nlgName, TStringBuf phraseName, const google::protobuf::Message& msgProto) {
    return RenderPhrase(nlgName, phraseName, NPrivate::Proto2Json(msgProto));
}

NNlg::TRenderPhraseResult TRender::RenderPhrase(TStringBuf nlgName, TStringBuf phraseName, const NJson::TJsonValue& jsonContext) {
    return Impl_->RenderPhrase(nlgName, phraseName, jsonContext);
}

/*
    Create response using NLG template and Protobuf struct
    @params [IN] nlgName - NLG file name
            [IN] phrase - NLG phrase
            [IN] msgProto - protobuf to add to context (usually == renderer args)
    Note:
    1. NLG file must be registered in scenario ctor using TScenario::SetNlgRegistration() function
    2. This function doesn't use default Proto->JSON translator from alice/util because protobuf converts int64 values as a string.
    See https://github.com/protocolbuffers/protobuf/issues/2679 for more details
*/
void TRender::CreateFromNlg(TStringBuf nlgName, TStringBuf phrase, const google::protobuf::Message& msgProto) {
    CreateFromNlg(nlgName, phrase, NPrivate::Proto2Json(msgProto));
}

/*
    Create response using NLG template and json context
    @params [IN] nlgName - NLG file name
            [IN] phrase - NLG phrase
            [IN] jsonContext - NLG context
    Note:
    1. NLG file must be registered in scenario ctor using TScenario::SetNlgRegistration() function
*/
void TRender::CreateFromNlg(TStringBuf nlgName, TStringBuf phraseName, const NJson::TJsonValue& jsonContext) {
    const auto output = Impl_->RenderPhrase(nlgName, phraseName, jsonContext);
    Impl_->SetPhraseOutput(output);
}

/*
    Add suggestion button to output
*/
void TRender::AddSuggestion(TStringBuf nlgName,
                            TStringBuf suggestPhraseName,
                            TStringBuf typeText /*= ""*/,
                            TStringBuf name /*= ""*/,
                            TStringBuf imageUrl /*= ""*/) {
    Impl_->AddSuggestion(nlgName, suggestPhraseName, typeText, name, imageUrl);
}

void TRender::AppendDiv2FromNlg(TStringBuf nlgName, TStringBuf cardName, const google::protobuf::Message& proto, bool hideBorders) {
    Impl_->AppendDiv2FromNlg(nlgName, cardName, proto, hideBorders);
}

void TRender::SetShouldListen(bool listenFlag) {
    Impl_->ShouldListen = listenFlag;
}

// TODO: OBSOLETE, will be removed soon
void TRender::AddDivRender(NRenderer::TDivRenderData&& renderData) {
    Impl_->DivRenderData.push_back(std::move(renderData));
}

void TRender::SetScenarioData(NData::TScenarioData&& scenarioData) {
    Impl_->ScenarioData.ConstructInPlace(std::move(scenarioData));
}

void TRender::SetStackEngine(NScenarios::TStackEngine&& stackEngine) {
    Impl_->StackEngine.ConstructInPlace(std::move(stackEngine));
}

void TRender::AddActionSpace(const TString& spaceName, NScenarios::TActionSpace&& actionSpace) {
    Impl_->ActionSpacesData[spaceName] = std::move(actionSpace);
}

// Attach  ellipsis/more frame(s) to final answer
void TRender::AddEllipsisFrame(const TString& ellipsis, const TString& actionName /*= TString{}*/) {
    Impl_->EllipsisFrames.emplace_back(ellipsis, actionName);
}

void TRender::AddEllipsisFrame(const TString& ellipsis, TTypedSemanticFrame tsf, const TString& actionName /*= TString{}*/) {
    Impl_->EllipsisFrames.emplace_back(ellipsis, actionName, std::move(tsf));
}

void TRender::AddCallbackFrame(const TString& name, const TSemanticFrame& frame, const TString& actionName /*= TString{}*/) {
    NScenarios::TCallbackDirective callback;
    callback.SetName(TString{NHollywood::FRAME_CALLBACK});
    (*callback.MutablePayload()->mutable_fields())["frame"].set_string_value(JsonStringFromProto(frame));
    Impl_->EllipsisFrames.emplace_back(name, actionName, std::move(callback));
}

void TRender::AddCallback(const NScenarios::TCallbackDirective& callback, const TString& actionName /*= TString{}*/) {
    NScenarios::TCallbackDirective callbackCopy = callback;
    Impl_->EllipsisFrames.emplace_back("", actionName, std::move(callbackCopy));
}

TDirectivesWrapper& TRender::Directives() {
    return *Directives_;
}

TServerDirectivesWrapper& TRender::ServerDirectives() {
    return *ServerDirectives_;
}

/*
    Buld a final response
    This is internal function, called aumatically from Finalize() stage
*/
void TRender::BuildAnswer(NAlice::NScenarios::TScenarioResponseBody* response, NAppHost::IServiceContext& ctx, const NPrivate::TNodeCaller& caller) {
    Impl_->BuildAnswer(response, ctx, caller);
    Directives_->BuildAnswer(*response);
    ServerDirectives_->BuildAnswer(*response);
}

} // namespace NAlice::NHollywoodFw
