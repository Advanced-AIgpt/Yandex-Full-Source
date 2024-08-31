#pragma once

#include <alice/hollywood/library/nlg/nlg_data.h>
#include <alice/hollywood/library/nlg/nlg_render_history.h>
#include <alice/hollywood/library/nlg/nlg.h>
#include <alice/library/util/rng.h>

namespace NAlice::NHollywood {

class TNlgWrapper {
public:
    NNlg::TRenderPhraseResult RenderPhrase(const TStringBuf templateName, const TStringBuf phraseName,
                                     const TNlgData& nlgData);

    NNlg::TRenderCardResult RenderCard(const TStringBuf templateName, const TStringBuf cardName,
                                 const TNlgData& nlgData, const bool reduceWhitespace = false) {
        NlgRenderHistoryRecordStorage_.TrackRenderCard(templateName, cardName, nlgData, GetLang());
        return Nlg_.RenderCard(templateName, cardName, Lang_, Rng_, nlgData, reduceWhitespace);
    }

    bool HasPhrase(const TStringBuf templateId, const TStringBuf phraseId) const {
        return Nlg_.HasPhrase(templateId, phraseId, Lang_);
    }

    bool HasCard(const TStringBuf templateId, const TStringBuf cardId) const {
        return Nlg_.HasCard(templateId, cardId, Lang_);
    }

    ELanguage GetLang() const {
        return Lang_;
    }

    const TVector<NScenarios::TAnalyticsInfo::TNlgRenderHistoryRecord>& GetNlgRenderHistoryRecords() const {
        return NlgRenderHistoryRecordStorage_.GetTrackedRecords();
    }
private:
    explicit TNlgWrapper(const TCompiledNlgComponent& nlgComponent, TNlgRenderHistoryRecordStorage&& nlgRenderHistoryRecordStorage, IRng& rng, const ELanguage lang);
public:
    static TNlgWrapper Create(const TCompiledNlgComponent& nlgComponent, const TScenarioBaseRequestWrapper& request, IRng& rng, const ELanguage lang);
private:
    const TCompiledNlgComponent& Nlg_;
    TNlgRenderHistoryRecordStorage NlgRenderHistoryRecordStorage_;
    IRng& Rng_;
    const ELanguage Lang_;
};

} // namespace NAlice::NHollywood
