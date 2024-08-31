#pragma once

#include <alice/bass/forms/external_skill/enums.h>
#include <alice/bass/forms/external_skill/fwd.h>
#include <alice/bass/forms/geoaddr.h>
#include <alice/bass/forms/vins.h>
#include <alice/bass/forms/parallel_handler.h>

#include <alice/bass/libs/source_request/handle.h>

#include <contrib/libs/pire/pire/pire.h>

#include <search/alice/serp_summarizer/runtime/proto/bin/server.pb.h>

#include <util/generic/maybe.h>

namespace NBASS {

class TSearchFormHandler: public IHandler, public IParallelHandler {
public:
    explicit TSearchFormHandler(IThreadPool& threadPool);

    TResultValue Do(TRequestHandler& r) override;
    TResultValue DoSetup(TSetupContext& ctx) override; // IHandler

    // IParallelHandler overrides:
    TTryResult TryToHandle(TContext& ctx) override;
    TString Name() const override { return "search"; }

    static void Register(THandlersMap* handlers, IGlobalContext& globalCtx);
    static void Init();

    /** It create a new search form with a given text (or if text is empty get it from meta.utterance).
     * Beware that the meta.utterance is the whole request text said by user (i.e. "дорогая алиса
     * не могла бы ты показать новости футбола в томске").
     * @see TContext::SetResponseForm()
     * @param[in|out] ctx is the original context which won't use as output but used as callback_form slot.
     * @param[in] callbackSlot is a flag which adds current context as callback slot.
     * @param[in] text is search request text (if empty get it from meta.utterance)
     * @return pointer to the new context, or nullptr if failed to create the response form.
     */
    static TContext::TPtr SetAsResponse(TContext& ctx, bool callbackSlot, TStringBuf text = TStringBuf());
    static TContext::TPtr SetAsExternalSkillActivationResponse(TContext& ctx, bool callbackSlot, TStringBuf text = TStringBuf());

private:
    TContext* Context = nullptr;
    TContext::TSlot* Answer = nullptr;
    TString Query;
    TStringBuf Tld;
    bool DisableChangeIntent = false;
    bool IsAndroid = false;
    bool IsIos = false;
    bool IsTouch = false;
    bool IsSmartSpeaker = false;
    bool IsElariWatch = false;
    bool IsTouchSearch = false;
    bool IsTvPluggedIn = false;
    bool IsYaAuto = false;
    IThreadPool& ThreadPool;

    TResultValue NothingFound(bool addAttention, TResultValue defaultResult = TResultValue(), bool disableFacts = false);

    bool AddFactoid(NSc::TValue& searchResult);
    bool AddFactoidAppHost(NSc::TValue& appHostResult);

    bool AddRichFact(NSc::TArray& docs, bool important);

    bool AddSuggestFact(NSc::TArray& docs, bool important, bool summarizationTrigger);
    bool AddSuggestFactAppHost(const NSc::TValue& appHostResult);
    bool AddSuggestFactImpl(const NSc::TValue* snippet);
    bool AddSummarizationAnswer(const NSc::TValue& searchResult);
    bool AddSummarization(const NSc::TValue& summarization, ::NSearch::NAlice::NSerpSummarizer::TServerResponse& response);
    bool AddSummarizationAsync(const NSc::TValue& summarization, ::NSearch::NAlice::NSerpSummarizer::TServerResponse& response);
    bool ShouldAnswerWithSummarization(const NSc::TValue& summarization) const;

    bool AddWikipediaFact(NSc::TArray& docs);

    bool AddEntityFact(NSc::TArray& docs);
    bool AddEntityFactAppHost(const NSc::TValue& appHostResult);
    bool AddEntityFactImpl(const NSc::TValue* snippet);

    bool AddCaloriesFact(NSc::TArray& docs);
    bool AddCaloriesFactAppHost(const NSc::TValue& appHostResult);
    bool AddCaloriesFactImpl(const NSc::TValue* snippet);

    bool AddDistanceFact(NSc::TArray& docs);

    bool AddUnitsConverter(NSc::TArray& docs);
    bool AddUnitsConverterAppHost(const NSc::TValue& appHostResult);
    bool AddUnitsConverterImpl(const NSc::TValue* snippet);

    bool AddTimeDifference(NSc::TArray& docs);
    bool AddTimeDifferenceAppHost(const NSc::TValue& appHostResult);
    bool AddTimeDifferenceImpl(const NSc::TValue* snippet);

    bool AddTableFact(NSc::TArray& docs);

    bool AddAutoRegion(NSc::TArray& docs);
    bool AddAutoRegionAppHost(const NSc::TValue& appHostResult);
    bool AddAutoRegionImpl(const NSc::TValue* snippet);

    bool AddZipCode(NSc::TArray& docs);
    bool AddSportLivescore(NSc::TArray& docs, bool wizplace);
    void AddFactoidPhone(const NSc::TValue& snippet, NSc::TValue* factoid, bool addSuggest) const;
    void AddRelatedFactPromo(const NSc::TValue& snippet, NSc::TValue& factoid) const;

    bool AddMusicSuggest(NSc::TArray& docs);
    NSc::TValue GetMusicSnippetData(NSc::TArray& docs);
    NSc::TValue GetVideoSnippetData(NSc::TArray& docs);

    bool AddObjectAsFact(NSc::TValue& doc);
    int  FindMarketPosition(const NSc::TArray& docs) const;
    bool AddNav(NSc::TValue& doc);

    bool AddCalculator(NSc::TArray& docs);
    bool AddCalculatorAppHost(const NSc::TValue& appHostResult);
    bool AddCalculatorImpl(const NSc::TValue* snippet);

    int  FindTvProgramPosition(const NSc::TArray& docs) const;
    bool AddTvProgram(NSc::TValue& doc);

    bool AddPreRenderedCard(const NSc::TValue& searchResult);

    bool AddSerp(TStringBuf query, NSc::TArray& docs, bool addSerpSuggest = true);
    bool AddRelated(NSc::TValue& data, TContext* const ctx);
    bool AddSearchStubs(NSc::TArray& docs);

    bool AddDivCardImage(const TString& src, ui16 w, ui16 h, NSc::TValue* card) const;
    static TString CreateAvtarIdImageSrc(const NSc::TValue& snippet, ui16 w, ui16 h);

    bool AddRelevantSkills(IRequestHandle<NBASS::NExternalSkill::TServiceResponse> *skillsRequestHandler,
                           NExternalSkill::EDiscoverySourceIntent sourceIntent);

    TString NormalizedQuery();

private:
    bool ApplyFixedAnswers();

private:
    static NSc::TValue Stubs;
    static void LoadBundledSearchStubs();
    TStringBuf ApplyStubs(NSc::TValue& doc, const NSc::TValue& stubs);
    void AddStub(const NSc::TValue& stub, TStringBuf host, TStringBuf url);

private:
    bool TryToSwitchToTranslate(int translateWizardPos, int firstNonWebRes) const;
};

void FillSearchFactorsData(const NAlice::TClientInfo& clientInfo, const NSc::TValue& searchResult, NSc::TValue* factorsData);

bool IsCommercialQuery(TContext& ctx, TStringBuf query);
bool ShouldReadSourceBeforeText(const TStringBuf source, const TStringBuf hostName);

} // NBASS
