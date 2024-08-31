#pragma once

#include "context.h"

#include <alice/hollywood/library/nlg/nlg_wrapper.h>
#include <alice/hollywood/library/response/response_builder.h>
#include <alice/megamind/protos/scenarios/directives.pb.h>

#include <memory>
#include <utility>

namespace NAlice::NHollywood {
class TPushDirectiveBuilder;
} // namespace NAlice::NHollywood

namespace NAlice::NHollywood::NReminders {

class THandlerResult final {
public:
    class IBackend;
    using TBackendPtr = std::unique_ptr<IBackend>;
    using TRunResponsePtr = std::unique_ptr<NScenarios::TScenarioRunResponse>;

    class IBackend {
    public:
        virtual ~IBackend() = default;
        virtual TRunResponsePtr CreateResponse() = 0;
    };

public:
    THandlerResult(TBackendPtr backend);

    TRunResponsePtr CreateResponse();

    static TBackendPtr Irrelevant();
    static TBackendPtr Error(const TString& type, const TString& message);
    template <typename T, typename... TArgs>
    static std::unique_ptr<T> CreateBackend(TArgs&& ...args) {
        return std::make_unique<T>(std::forward<TArgs>(args)...);
    }

private:
    TBackendPtr Backend_;
};

class TResultBackendAction final : public THandlerResult::IBackend {
public:
    TResultBackendAction();

    TResultBackendAction& AddFrameAction(const TString& name, NScenarios::TFrameAction&& action);
    TResultBackendAction& AddDirective(NScenarios::TDirective directive);

    // Overrides IBackend.
    THandlerResult::TRunResponsePtr CreateResponse() override;

private:
    TRunResponseBuilder Builder_;
    TResponseBodyBuilder& BodyBuilder_;
};


class TResultBackendNlg final : public THandlerResult::IBackend {
public:
    TResultBackendNlg(THandlerContext& ctx, const TString& tmpl, const TString& phrase, const TString* rngId);

    TResultBackendNlg& AddFrameAction(const TString& name, NScenarios::TFrameAction&& action);
    TResultBackendNlg& AddServerDirective(NScenarios::TServerDirective directive);
    TResultBackendNlg& AddPushDirective(TPushDirectiveBuilder& builder);
    TResultBackendNlg& AddDirective(NScenarios::TDirective directive);
    TNlgData& NlgData();

    // Overrides IBackend.
    THandlerResult::TRunResponsePtr CreateResponse() override;

private:
    TString Template_;
    TString Phrase_;
    TMaybe<TRng> Rng_;
    TNlgWrapper NlgWrapper_;
    TRunResponseBuilder Builder_;
    TResponseBodyBuilder& BodyBuilder_;
    TNlgData NlgData_;
};

} // namespace NAlice::NHollywood::NReminders
