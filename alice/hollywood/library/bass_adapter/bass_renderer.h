#pragma once

#include <alice/hollywood/library/base_scenario/scenario.h>
#include <alice/hollywood/library/nlg/nlg.h>
#include <alice/hollywood/library/nlg/nlg_data.h>
#include <alice/hollywood/library/request/request.h>
#include <alice/hollywood/library/response/response_builder.h>
#include <alice/library/logger/logger.h>

#include <alice/megamind/protos/scenarios/directives.pb.h>
#include <alice/megamind/protos/scenarios/request_meta.pb.h>
#include <alice/megamind/protos/scenarios/response.pb.h>

namespace NAlice::NHollywood {

namespace NImpl {

class IServerDirectiveParser {
public:
    virtual ~IServerDirectiveParser() = default;

    virtual bool MakeDirective(const NJson::TJsonValue& data, TResponseBodyBuilder& bodyBuilder) const = 0;
    virtual TStringBuf GetDirectiveName() const = 0;
};

class TUpdateDatasyncDirectiveParser : public IServerDirectiveParser {
public:
    bool MakeDirective(const NJson::TJsonValue& data, TResponseBodyBuilder& bodyBuilder) const override;

    TStringBuf GetDirectiveName() const override {
        return "UpdateDatasyncDirective";
    }
};

class TSendPushDirectiveParser : public IServerDirectiveParser {
public:
    bool MakeDirective(const NJson::TJsonValue& data, TResponseBodyBuilder& bodyBuilder) const override;

    TStringBuf GetDirectiveName() const override {
        return "SendPushDirective";
    }
};

TMaybe<TFrame> ParseBassForm(const NJson::TJsonValue& bassForm);

bool AddServerDirective(TResponseBodyBuilder& bodyBuilder,
                        TRTLogger& logger,
                        const IServerDirectiveParser& directiveParser,
                        const NJson::TJsonValue& data);

} // namespace NImpl

class TBassResponseRenderer {
public:
    using TServerDirectiveParsersMapping = THashMap<TString, THolder<NImpl::IServerDirectiveParser>>;

    TBassResponseRenderer(const TScenarioBaseRequestWrapper& baseRequest,
                          const TScenarioInputWrapper& input,
                          IResponseBuilder& builder,
                          TRTLogger& logger,
                          bool suggestAutoAction = true,
                          bool reduceWhitespaceInCards = false,
                          bool addFistTrackObject = true);

    void Render(const TStringBuf nlgTemplateName, const TStringBuf nlgPhraseName,
                const NJson::TJsonValue& bassResponse,
                const TString& analyticsIntentName = Default<TString>(),
                const TString& analyticsScenarioName = Default<TString>(),
                bool processSuggestsOnError = false);

    void Render(const TStringBuf nlgTemplateName,
                const NJson::TJsonValue& bassResponse,
                const TString& analyticsIntentName = Default<TString>(),
                const TString& analyticsScenarioName = Default<TString>(),
                const TStringBuf nlgDefaultPhraseName = "render_result",
                bool processSuggestsOnError = false);

    [[nodiscard]] static TMaybe<TResponseBodyBuilder::TSuggest> CreateSuggest(TNlgWrapper& nlg,
                                                                              const TStringBuf nlgTemplateName,
                                                                              const TStringBuf type,
                                                                              const TStringBuf analyticsTypeAction,
                                                                              bool autoAction,
                                                                              const NJson::TJsonValue& data,
                                                                              const TNlgData& nlgData,
                                                                              const NAlice::NScenarios::TInterfaces* interfaces = nullptr,
                                                                              const TMaybe<TStringBuf> screenId = Nothing());

    void SetContextValue(TStringBuf name, NJson::TJsonValue value);

private:
    bool AddRenderedDivCards(const TStringBuf nlgTemplateName,
                             TResponseBodyBuilder& bodyBuilder,
                             const NJson::TJsonValue::TArray& bassResponseBlocks,
                             const TNlgData& nlgData);

    bool AddRenderedDiv2Cards(const TStringBuf nlgTemplateName,
                             TResponseBodyBuilder& bodyBuilder,
                             const NJson::TJsonValue::TArray& bassResponseBlocks,
                             const TNlgData& nlgData);

    [[nodiscard]] TMaybe<TResponseBodyBuilder::TSuggest> CreateSuggest(const TStringBuf nlgTemplateName,
                                                                       const NJson::TJsonValue& suggestBlock,
                                                                       const TNlgData& nlgData);

    TNlgData CreateNlgData() const;

private:
    const TScenarioBaseRequestWrapper& BaseRequest_;
    const TScenarioInputWrapper& Input_;
    const bool ReduceWhitespaceInCards_;
    IResponseBuilder& Builder_;
    TRTLogger& Logger_;
    TNlgData NlgData_;

    // TODO(bas1330): remove this tmp flags after MEGAMIND-853
    bool SuggestAutoAction_; // add action directive on suggest with uri creation

    const bool AddFirstTrackObject_;

    static const TServerDirectiveParsersMapping PreErrorServerDirectiveParsers;
    static const TServerDirectiveParsersMapping PostErrorServerDirectiveParsers;
};

} // namespace NAlice::NHollywood
