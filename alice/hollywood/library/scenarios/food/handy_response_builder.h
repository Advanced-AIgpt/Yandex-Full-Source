#pragma once

#include <alice/hollywood/library/base_scenario/scenario.h>
#include <alice/hollywood/library/nlg/nlg_data.h>
#include <alice/hollywood/library/request/request.h>
#include <alice/hollywood/library/response/response_builder.h>
#include <alice/megamind/protos/scenarios/response.pb.h>

namespace NAlice::NHollywood::NFood {

class THandyResponseBuilder {
public:
    THandyResponseBuilder(TScenarioHandleContext& ctx, const TScenarioRunRequestWrapper& request, TStringBuf nlgTemplate);

    void SetIsIrrelevant(bool value = true);
    void SetShouldListen(bool value = true);
    void SetCommitArguments(const google::protobuf::Message& args);
    void SetState(const google::protobuf::Message& state);
    void AddAction(TStringBuf name, const NScenarios::TFrameAction& action);

    void AddTypeTextSuggest(const TString& text);

    void AddDirective(NScenarios::TDirective&& directive);
    void AddOpenUriDirective(TStringBuf uri);

    const NJson::TJsonValue& NlgCtx() const {
        return NlgData_.Context;
    }
    NJson::TJsonValue& NlgCtx() {
        return NlgData_.Context;
    }

    const TNlgData& NlgData() const {
        return NlgData_;
    }
    TNlgData& NlgData() {
        return NlgData_;
    }

    const TNlgWrapper& NlgWrapper() const {
        return NlgWrapper_;
    }
    TNlgWrapper& NlgWrapper() {
        return NlgWrapper_;
    }

    void RenderNlg(TStringBuf nlgPhrase);

    const NScenarios::TScenarioResponseBody& ResponseBody() const {
        return ResponseBodyBuilder_.GetResponseBody();
    }
    NScenarios::TScenarioResponseBody& ResponseBody() {
        return ResponseBodyBuilder_.GetResponseBody();
    }

    void WriteResponse() &&;

private:
    TScenarioHandleContext& Ctx;
    TString NlgTemplate;
    TNlgData NlgData_;
    TNlgWrapper NlgWrapper_;
    TMaybe<google::protobuf::Any> CommitArguments;
    TRunResponseBuilder ResponseBuilder_;
    TResponseBodyBuilder& ResponseBodyBuilder_;
};

} // namespace NAlice::NHollywood::NFood
