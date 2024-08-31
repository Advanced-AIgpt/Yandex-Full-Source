#include "nlg_search_factor_calcer.h"
#include "factor_calcer.h"
#include "basic_text_factors.h"
#include "text_intersect_factors.h"
#include "dssm_cos_factors.h"
#include "pronoun_factors.h"
#include "rus_lister_factors.h"
#include "is_dssm_index_factor.h"
#include "is_knn_index_factor.h"
#include "informativeness_factor.h"
#include "seq2seq_factor.h"

namespace NNlg {

namespace {

static const size_t NlgSearchNumTurns = 3;

static TVector<IFactorPtr> CreateNlgFactors(IFactorPtr rusListerFactors,
                                            const TVector<TString>& baseDssmModelNames,
                                            const TVector<TString>& factorDssmModelNames) {
    TVector<IFactorPtr> factors = {
        new TDssmCosFactors(baseDssmModelNames[0]), // temporary hack to keep current reranker's feature order
        new TBasicTextFactors,
        new TTextIntersectionFactors,
        new TPronounFactors,
        rusListerFactors
    };
    for (size_t i = 1; i < baseDssmModelNames.size(); ++i) {
        factors.push_back(new TDssmCosFactors(baseDssmModelNames[i]));
    }
    for (const auto& dssmModel : factorDssmModelNames) {
        factors.push_back(new TDssmCosFactors(dssmModel));
    }
    for (size_t i = 1; i < baseDssmModelNames.size(); ++i) {
        factors.push_back(new TIsDssmIndexFactor(baseDssmModelNames[i]));
    }
    factors.push_back(new TInformativenessFactor(NNlg::DSSM_QUERY_REPLY_COSINE_FACTOR_SHIFT));
    factors.push_back(new TSeq2SeqFactor());
    factors.push_back(new TIsKnnIndexFactor("movie"));
    return factors;
}

}

TFactorCalcer* CreateNlgSearchFactorCalcer(const TString& rusListerMapFilename, const TVector<TString>& baseDssmModelNames, const TVector<TString>& factorDssmModelNames) {
    return new TFactorCalcer(CreateNlgFactors(new TRusListerFactors(rusListerMapFilename), baseDssmModelNames, factorDssmModelNames), NlgSearchNumTurns);
}

TFactorCalcer* CreateNlgSearchFactorCalcer(const TVector<TString>& rusListerMap, const TVector<TString>& baseDssmModelNames, const TVector<TString>& factorDssmModelNames) {
    return new TFactorCalcer(CreateNlgFactors(new TRusListerFactors(rusListerMap), baseDssmModelNames, factorDssmModelNames), NlgSearchNumTurns);
}

}
