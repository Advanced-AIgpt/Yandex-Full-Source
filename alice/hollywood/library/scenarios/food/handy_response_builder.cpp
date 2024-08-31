#include "handy_response_builder.h"

#include <alice/library/version/version.h>

namespace NAlice::NHollywood::NFood {

using namespace NAlice::NScenarios;

THandyResponseBuilder::THandyResponseBuilder(TScenarioHandleContext& ctx,
        const TScenarioRunRequestWrapper& request, TStringBuf nlgTemplate)
    : Ctx(ctx)
    , NlgTemplate(nlgTemplate)
    , NlgData_(ctx.Ctx.Logger(), request)
    , NlgWrapper_(TNlgWrapper::Create(ctx.Ctx.Nlg(), request, ctx.Rng, ctx.UserLang))
    , ResponseBuilder_(&NlgWrapper_)
    , ResponseBodyBuilder_(ResponseBuilder_.CreateResponseBodyBuilder())
{
}

void THandyResponseBuilder::SetIsIrrelevant(bool value) {
    ResponseBuilder_.GetMutableFeatures().SetIsIrrelevant(value);
}

void THandyResponseBuilder::SetShouldListen(bool value) {
    ResponseBodyBuilder_.SetShouldListen(value);
}

void THandyResponseBuilder::SetCommitArguments(const google::protobuf::Message& args) {
    CommitArguments.ConstructInPlace().PackFrom(args);
}

void THandyResponseBuilder::SetState(const google::protobuf::Message& state) {
    ResponseBodyBuilder_.GetResponseBody().MutableState()->PackFrom(state);
}

void THandyResponseBuilder::AddAction(TStringBuf name, const NScenarios::TFrameAction& action) {
    (*ResponseBodyBuilder_.GetResponseBody().MutableFrameActions())[TString(name)] = action;
}

void THandyResponseBuilder::AddTypeTextSuggest(const TString& text) {
    ResponseBodyBuilder_.AddTypeTextSuggest(text);
}

void THandyResponseBuilder::AddDirective(NScenarios::TDirective&& directive) {
    ResponseBodyBuilder_.AddDirective(std::move(directive));
}

void THandyResponseBuilder::AddOpenUriDirective(TStringBuf uri) {
    NScenarios::TDirective directive;
    NScenarios::TOpenUriDirective& openUriDirective = *directive.MutableOpenUriDirective();
    openUriDirective.SetName("open_uri");
    openUriDirective.SetUri(TString(uri));
    AddDirective(std::move(directive));
}

void THandyResponseBuilder::RenderNlg(TStringBuf nlgPhrase) {
    ResponseBodyBuilder_.AddRenderedTextWithButtonsAndVoice(NlgTemplate, nlgPhrase, {}, NlgData_);
}

void THandyResponseBuilder::WriteResponse() && {
    auto response = std::move(ResponseBuilder_).BuildResponse();
    response->SetVersion(VERSION_STRING);
    if (CommitArguments) {
        auto body = std::move(*response->MutableResponseBody());
        *response->MutableCommitCandidate()->MutableResponseBody() = std::move(body);
        *response->MutableCommitCandidate()->MutableArguments() = std::move(CommitArguments.GetRef());
    }
    Ctx.ServiceCtx.AddProtobufItem(*response, RESPONSE_ITEM);
}

} // namespace NAlice::NHollywood::NFood
