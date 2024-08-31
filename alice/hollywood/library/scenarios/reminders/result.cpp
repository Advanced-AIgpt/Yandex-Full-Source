#include "result.h"

#include <alice/hollywood/library/response/push.h>

namespace NAlice::NHollywood::NReminders {
namespace {

void PopulateAnalyticsInfo(NScenarios::IAnalyticsInfoBuilder& ai) {
    ai.SetProductScenarioName("reminder");
}

class TResultBackendError final : public THandlerResult::IBackend {
public:
    TResultBackendError(const TString& type, const TString& message)
        : Type_{type}
        , Message_{message}
    {
    }

    THandlerResult::TRunResponsePtr CreateResponse() override {
        TRunResponseBuilder builder;
        builder.SetError(Type_, Message_);
        return std::move(builder).BuildResponse();
    }

private:
    const TString Type_;
    const TString Message_;
};

class TResultBackendIrrelevant final : public THandlerResult::IBackend {
public:
    THandlerResult::TRunResponsePtr CreateResponse() override {
        TRunResponseBuilder builder;
        builder.SetIrrelevant();
        return std::move(builder).BuildResponse();
    }
};

} // namespace

// THandlerResult --------------------------------------------------------------
// static
THandlerResult::TBackendPtr THandlerResult::Irrelevant() {
    return CreateBackend<TResultBackendIrrelevant>();
}

// static
THandlerResult::TBackendPtr THandlerResult::Error(const TString& type, const TString& message) {
    return CreateBackend<TResultBackendError>(type, message);
}

THandlerResult::THandlerResult(TBackendPtr backend)
    : Backend_{std::move(backend)}
{
}

THandlerResult::TRunResponsePtr THandlerResult::CreateResponse() {
    return Backend_->CreateResponse();
}

// TResultBackendAction -------------------------------------------------------
TResultBackendAction::TResultBackendAction()
    : BodyBuilder_{Builder_.CreateResponseBodyBuilder()}
{
}

TResultBackendAction& TResultBackendAction::AddFrameAction(const TString& name,
                                                           NScenarios::TFrameAction&& action)
{
    BodyBuilder_.AddAction(name, std::move(action));
    return *this;
}

TResultBackendAction& TResultBackendAction::AddDirective(NScenarios::TDirective directive) {
    BodyBuilder_.AddDirective(std::move(directive));
    return *this;
}

THandlerResult::TRunResponsePtr TResultBackendAction::CreateResponse() {
    PopulateAnalyticsInfo(BodyBuilder_.CreateAnalyticsInfoBuilder());
    return std::move(Builder_).BuildResponse();
}

// TResultBackendNlg ----------------------------------------------------------
TResultBackendNlg::TResultBackendNlg(THandlerContext& ctx, const TString& tmpl, const TString& phrase, const TString* rngId)
    : Template_{tmpl}
    , Phrase_{phrase}
    , Rng_{rngId ? TMaybe<TRng>{std::hash<TString>()(*rngId)} : Nothing()}
    , NlgWrapper_{TNlgWrapper::Create(ctx->Ctx.Nlg(), ctx.Request(), Rng_ ? Rng_.GetRef() : ctx->Rng, ctx->UserLang)}
    , Builder_{&NlgWrapper_}
    , BodyBuilder_{Builder_.CreateResponseBodyBuilder()}
    , NlgData_{ctx->Ctx.Logger(), ctx.Request()}
{
}

THandlerResult::TRunResponsePtr TResultBackendNlg::CreateResponse() {
    BodyBuilder_.AddRenderedTextWithButtonsAndVoice(
        Template_, Phrase_,
        TVector<NScenarios::TLayout::TButton>{},
        NlgData_
    );

    if (!BodyBuilder_.HasAnalyticsInfoBuilder()) {
        PopulateAnalyticsInfo(BodyBuilder_.CreateAnalyticsInfoBuilder());
    }

    return std::move(Builder_).BuildResponse();
}

TResultBackendNlg& TResultBackendNlg::AddServerDirective(NScenarios::TServerDirective directive) {
    BodyBuilder_.AddServerDirective(std::move(directive));
    return *this;
}

TResultBackendNlg& TResultBackendNlg::AddPushDirective(TPushDirectiveBuilder& builder) {
    builder.BuildTo(BodyBuilder_);
    return *this;
}

TResultBackendNlg& TResultBackendNlg::AddFrameAction(const TString& name, NScenarios::TFrameAction&& action) {
    BodyBuilder_.AddAction(name, std::move(action));
    return *this;
}

TResultBackendNlg& TResultBackendNlg::AddDirective(NScenarios::TDirective directive) {
    BodyBuilder_.AddDirective(std::move(directive));
    return *this;
}

TNlgData& TResultBackendNlg::NlgData() {
    return NlgData_;
}

} // namespace NAlice::NHollywood::NReminders
