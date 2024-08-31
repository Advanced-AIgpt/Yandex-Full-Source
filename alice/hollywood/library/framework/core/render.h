#pragma once

//
// HOLLYWOOD FRAMEWORK
// Base render class information
//
#include <alice/hollywood/library/nlg/nlg.h>
#include <alice/hollywood/library/nlg/nlg_render_history.h>

#include <util/generic/maybe.h>
#include <util/generic/vector.h>

namespace NAppHost {

//
// Forward declarations
//
class IServiceContext;

} // namespace NAppHost


//
// forward defarations for protobuf
//
namespace NAlice::NScenarios {

class TLayout_TCard;
class TScenarioResponseBody;
class TScenarioRunResponse;
class TActionSpace;
class TStackEngine;

} // namespace NAlice::NScenarios

namespace NAlice::NRenderer {

class TDivRenderData;
class TRenderResponse;

} // namespace NAlice::NRenderer


namespace NAlice::NData {

class TScenarioData;

} // namespace NAlice::NRenderer

namespace NAlice::NHollywoodFw {

namespace NPrivate {

class TRenderImpl;
class TNodeCaller;

} // namespace NPrivate

//
// Forward declarartions
//
class TRequest;
class TScenario;
class TDirectivesWrapper;
class TServerDirectivesWrapper;

//
// Base Hollywood render class definition
//
class TRender : public TNonCopyable {
friend class TScenario;
public:
    ~TRender();

    const TRequest& GetRequest() const {
        return Request_;
    }

    // Get direct link to MM_SCENARIO_RESPONSE body
    // TODO This function will be removed soon
    // It can be used to build custom answers (directives, etc) currently not supported
    // By Hollywood framework
    // Please contact @d-dima for more details
    NScenarios::TScenarioResponseBody& GetResponseBody() {
        return ResponseBody_;
    }

    //
    // Phrase rendering functions
    //
    void MakeComplexVar(TStringBuf varName, const NJson::TJsonValue& jsonContext);

    // Create response from Render and compiled NLG file
    void CreateFromNlg(TStringBuf nlgName, TStringBuf phrase, const google::protobuf::Message& msgProto);
    void CreateFromNlg(TStringBuf nlgName, TStringBuf phrase, const NJson::TJsonValue& jsonContext);
    // Add suggestions from NLG
    void AddSuggestion(TStringBuf nlgName, TStringBuf suggestPhraseName,
                       TStringBuf typeText = "",
                       TStringBuf name = "",
                       TStringBuf imageUrl = "");
    void AppendDiv2FromNlg(TStringBuf nlgName, TStringBuf cardName, const google::protobuf::Message& proto, bool hideBorders = true);

    //
    // Attach  ellipsis/more frame(s) to final answer
    //
    void AddEllipsisFrame(const TString& ellipsis, const TString& actionName = TString{});
    void AddEllipsisFrame(const TString& ellipsis, TTypedSemanticFrame tsf, const TString& actionName = TString{});
    void AddCallbackFrame(const TString& name, const TSemanticFrame& frame, const TString& actionName = TString{});
    void AddCallback(const NScenarios::TCallbackDirective& callback, const TString& actionName = TString{});

    //
    // Attach directives to response
    //
    TDirectivesWrapper& Directives();
    TServerDirectivesWrapper& ServerDirectives();

    //
    // Set data for DivRender
    // OBSOLETE funtion, will be removed soon.
    // Use TReturnValueRender().AddDivRender()
    //
    void AddDivRender(NRenderer::TDivRenderData&& renderData);
    // Get results of DivRender
    const TVector<std::shared_ptr<NRenderer::TRenderResponse>>& GetDivRenderResponse() const {
        return DivRenderResponse_;
    }

    //
    // Set ScenarioData and other protobufs
    //
    void SetScenarioData(NData::TScenarioData&& scenarioData);
    void SetStackEngine(NScenarios::TStackEngine&& stackEngine);

    //
    // Adds ActionSpace
    //
    void AddActionSpace(const TString& spaceName, NScenarios::TActionSpace&& actionSpace);

    // Set "Should listen" flag to answer, see https://st.yandex-team.ru/ALICE-10003
    void SetShouldListen(bool listenFlag);

    // Internal functions
    void BuildAnswer(NAlice::NScenarios::TScenarioResponseBody* response, NAppHost::IServiceContext& ctx, const NPrivate::TNodeCaller& caller);
    NPrivate::TRenderImpl* GetImpl() {
        return Impl_.get();
    }

    // Render a phrase and return direct pointer to the phrase
    NNlg::TRenderPhraseResult RenderPhrase(TStringBuf nlgName, TStringBuf phraseName, const google::protobuf::Message& msgProto);
    NNlg::TRenderPhraseResult RenderPhrase(TStringBuf nlgName, TStringBuf phraseName, const NJson::TJsonValue& jsonContext);

private:
    // ctor, created automatically from framework core
    TRender(NPrivate::TNodeCaller& caller, const TMaybe<NHollywood::TCompiledNlgComponent>& nlg);

private:
    const TRequest& Request_;
    NAlice::NScenarios::TScenarioResponseBody& ResponseBody_;
    std::unique_ptr<NPrivate::TRenderImpl> Impl_;
    TVector<std::shared_ptr<NRenderer::TRenderResponse>> DivRenderResponse_;
    std::unique_ptr<TDirectivesWrapper> Directives_;
    std::unique_ptr<TServerDirectivesWrapper> ServerDirectives_;
};

} // namespace NAlice::NHollywoodFw
