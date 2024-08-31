#pragma once

//
// HOLLYWOOD FRAMEWORK
// Internal render class information, not for public usage
//
#include "request.h"
#include "error.h"

#include <alice/hollywood/library/nlg/nlg.h>
#include <alice/hollywood/library/nlg/nlg_render_history.h>

#include <alice/megamind/protos/scenarios/action_space.pb.h>
#include <alice/megamind/protos/scenarios/response.pb.h>
#include <alice/megamind/protos/scenarios/stack_engine.pb.h>
#include <alice/protos/api/renderer/api.pb.h>
#include <alice/protos/data/scenario/data.pb.h>

#include <library/cpp/protobuf/json/proto2json.h>

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
namespace NAlice::NHollywoodFw::NPrivate {

class TNodeCaller;

//
// Internal interface for Directives containes
//
class TDirectiveWrapper {
public:
    explicit TDirectiveWrapper(const TString& name)
        : Name_(name)
    {
    }
    virtual ~TDirectiveWrapper() = default;
    virtual void Attach(NAlice::NScenarios::TScenarioResponseBody& response) = 0;
    const TString& GetName() const {
        return Name_;
    }
protected:
    TString Name_;
};

//
// Simplest Directives wrapper based on TDirectiveWrapper
// Acceptable for all directives with only one field 'Name'
//
template <class Ret>
class TSimpleDirectiveWrapper: public TDirectiveWrapper {
public:
    TSimpleDirectiveWrapper(const TString& name, Ret(NScenarios::TDirective::*fn))
        : TDirectiveWrapper(name)
        , Fn_(fn)
    {
    }
    virtual void Attach(NAlice::NScenarios::TScenarioResponseBody& response) override {
        NScenarios::TDirective directive;
        auto& directive2 = *((directive.*Fn_)());
        directive2.SetName(Name_);
        *(response.MutableLayout()->AddDirectives()) = std::move(directive);
    };
private:
    Ret(NScenarios::TDirective::*Fn_);
};

template <typename T, typename Ret>
class TCustomDirectiveWrapper final : public TDirectiveWrapper {
public:
    TCustomDirectiveWrapper(T&& directive, Ret(NScenarios::TDirective::*fn))
        : TDirectiveWrapper(directive.GetName())
        , Directive_(std::move(directive))
        , Fn_(fn)
    {
    }
    void Attach(NAlice::NScenarios::TScenarioResponseBody& response) override {
        NScenarios::TDirective directive;
        auto& directive2 = *((directive.*Fn_)());
        directive2.Swap(&Directive_);
        *(response.MutableLayout()->AddDirectives()) = std::move(directive);
    };
private:
    T Directive_;
    Ret(NScenarios::TDirective::*Fn_);
};

//
// Implementation of rendering
//
class TRenderImpl {
public:
    TRenderImpl(const TRequest& request, const TMaybe<NHollywood::TCompiledNlgComponent>& nlg);
    ~TRenderImpl();

    void AddSuggestion(TStringBuf nlgName, TStringBuf suggestPhraseName, TStringBuf typeText, TStringBuf name, TStringBuf imageUrl);
    void AppendDiv2FromNlg(TStringBuf nlgName, TStringBuf cardName, const google::protobuf::Message& proto, bool hideBorders);
    NNlg::TRenderPhraseResult RenderPhrase(TStringBuf nlgName, TStringBuf phraseName, const NJson::TJsonValue& jsonContext);
    void SetPhraseOutput(const NNlg::TRenderPhraseResult& output);
    void MakeComplexVar(TStringBuf varName, const NJson::TJsonValue& jsonContext) {
        ComplexRoot_.InsertValue(varName, jsonContext);
    }

    void BuildAnswer(NAlice::NScenarios::TScenarioResponseBody* response, NAppHost::IServiceContext& ctx, const NPrivate::TNodeCaller& caller);

public:
    struct TEllipsis {
        TEllipsis(const TString& ellipsis, const TString& actionName)
            : Ellipsis{ellipsis}
            , ActionName{actionName}
        {
        }
        TEllipsis(const TString& ellipsis, const TString& actionName, TTypedSemanticFrame&& tsf)
            : Ellipsis{ellipsis}
            , ActionName{actionName}
            , Tsf{tsf}
        {
        }
        TEllipsis(const TString& ellipsis, const TString& actionName, NScenarios::TCallbackDirective&& cb)
            : Ellipsis{ellipsis}
            , ActionName{actionName}
            , Callback{cb}
        {
        }

        TString Ellipsis;
        TString ActionName;
        TMaybe<TTypedSemanticFrame> Tsf;
        TMaybe<NScenarios::TCallbackDirective> Callback;

        void ToFrameAction(NScenarios::TFrameAction& frameAction);
    };
    TVector<TEllipsis> EllipsisFrames;

    // For direct access from unit tests
    TString TextResponse;
    TString VoiceResponse;

    // Additional directives
    TVector<std::shared_ptr<TDirectiveWrapper>> Directives;

    // Data for Div renderer
    TVector<NRenderer::TDivRenderData> DivRenderData;

    // Scenario data
    TMaybe<NData::TScenarioData> ScenarioData;
    TMaybe<NScenarios::TStackEngine> StackEngine;

    // Mapping from ActionSpaceId to corresponding ActionSpace
    THashMap<TString, NScenarios::TActionSpace> ActionSpacesData;

    bool ShouldListen = false;

private:
    // Suggests
    struct TSuggestData {
        TString Suggest;
        TString TypeText;
        TString Name;
        TString ImageUrl;
    };
    TVector<TSuggestData> Suggests_;

    const TRequest& Request_;
    const TMaybe<NHollywood::TCompiledNlgComponent>& Nlg_;
    NJson::TJsonValue ComplexRoot_;

    // Result of CreateFromNlg()
    TVector<NScenarios::TLayout_TCard> Cards_;
    TMaybe<google::protobuf::Struct> Div2Templates_;
    TMaybe<google::protobuf::Struct> Div2Palette_;

    NHollywood::TNlgRenderHistoryRecordStorage NlgRenderHistoryRecordStorage_;
};

inline NJson::TJsonValue Proto2Json(const google::protobuf::Message& msgProto) {
    constexpr auto fieldNameGenerator = [](const google::protobuf::FieldDescriptor& field) -> TString {
        if (field.has_json_name()) {
            return field.json_name();
        }
        return field.name();
    };

    NProtobufJson::TProto2JsonConfig config;
    config.MapAsObject = true;
    config.NameGenerator = fieldNameGenerator;

    TString jsonStr;
    try {
        NProtobufJson::Proto2Json(msgProto, jsonStr, config);
    } catch(...) {
        HW_ERROR("Proto2Json conversion failed");
    }
    return NJson::ReadJsonFastTree(jsonStr, /* notClosedBracketIsError */ true);
}

} // namespace NAlice::NHollywoodFw::NPrivate
