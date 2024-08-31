#include "nlg_wrapper.h"

#include <alice/hollywood/library/request/experiments.h>

namespace NAlice::NHollywood {

TNlgWrapper::TNlgWrapper(const TCompiledNlgComponent& nlgComponent, TNlgRenderHistoryRecordStorage&& nlgRenderHistoryRecordStorage, IRng& rng, const ELanguage lang)
    : Nlg_{nlgComponent}
    , NlgRenderHistoryRecordStorage_{std::move(nlgRenderHistoryRecordStorage)}
    , Rng_{rng}
    , Lang_{lang}
{}

NNlg::TRenderPhraseResult TNlgWrapper::RenderPhrase(const TStringBuf templateName, const TStringBuf phraseName, const TNlgData& nlgData) {
    NlgRenderHistoryRecordStorage_.TrackRenderPhrase(templateName, phraseName, nlgData, GetLang());
    return Nlg_.RenderPhrase(templateName, phraseName, Lang_, Rng_, nlgData);
}

TNlgWrapper TNlgWrapper::Create(const TCompiledNlgComponent& nlgComponent, const TScenarioBaseRequestWrapper& request, IRng& rng, const ELanguage lang) {
    return TNlgWrapper(
        nlgComponent,
        TNlgRenderHistoryRecordStorage(request.HasExpFlag(EXP_DUMP_NLG_RENDER_CONTEXT)),
        rng,
        lang
    );
}

}
