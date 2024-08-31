#include "factor_calcer.h"
#include "dssm_cos_factors.h"
#include "is_dssm_index_factor.h"

namespace NNlg {

namespace {

static TFactorCalcerCtx GetMinimalCtx(size_t numTurns, const TVector<TString>& factorDssmModelNames) {
    static const float x = 0.0;
    static TVector<TVector<TString>> contexts {TVector<TString>(numTurns)};
    static TVector<TString> replies { "" };

    TFactorCalcerCtx ctx(contexts, replies);
    ctx.QueryContext = { TVector<TString>(numTurns) };
    ctx.Contexts = { TVector<TString>(numTurns) };
    ctx.Replies = { "" };
    for (const auto& name : factorDssmModelNames) {
        auto& dssmCtx = ctx.DssmFactorCtxs[name];
        dssmCtx.QueryEmbedding = &x;
        dssmCtx.ContextEmbeddings = { &x };
        dssmCtx.ReplyEmbeddings = { &x };
        dssmCtx.Dimension = 1;
    }
    ctx.DssmIndexNames = { "" };
    ctx.KnnIndexNames = { "" };
    ctx.StaticFactors = { &x };
    ctx.NumStaticFactors = 0;
    return ctx;
}

static TVector<TString> GetFactorDssmModelNames(const TVector<IFactorPtr>& factors) {
    TVector<TString> names;
    for (const auto& factor : factors) {
        if (const auto* f = dynamic_cast<const TDssmCosFactors*>(factor.Get())) {
            names.push_back(f->GetDssmModelName());
        }
    }
    return names;
}

static THashMap<TString, std::pair<size_t, size_t>> CalcFactorsLocation(const TVector<IFactorPtr>& factors, size_t numTurns) {
    THashMap<TString, std::pair<size_t, size_t>> result;
    auto ctx = GetMinimalCtx(numTurns, GetFactorDssmModelNames(factors));
    TVector<TVector<float>> features(1);
    for (const auto& factor : factors) {
        size_t begin = features[0].size();
        factor->AppendFactors(ctx, &features);
        size_t end = features[0].size();
        result.emplace(factor->GetName(), std::pair<size_t, size_t>{begin, end});
    }
    return result;
}

static size_t CalcNumFactors(THashMap<TString, std::pair<size_t, size_t>> factorsLocation) {
    size_t numFactors = 0;
    for (const auto& [name, location] : factorsLocation) {
        numFactors = std::max(numFactors, location.second);
    }
    return numFactors;
}

}

TFactorCalcer::TFactorCalcer(TVector<IFactorPtr> factors, size_t numTurns)
    : Factors(std::move(factors))
    , NumTurns(numTurns)
    , FactorsLocation(CalcFactorsLocation(Factors, NumTurns))
    , NumFactors(CalcNumFactors(FactorsLocation))
{
}

void TFactorCalcer::CalcFactors(TFactorCalcerCtx* ctx, TVector<TVector<float>>* factors) const {
    size_t numDocs = factors->size();
    Y_VERIFY(ctx->Contexts.size() == numDocs);
    Y_VERIFY(ctx->Replies.size() == numDocs);
    for (const auto& pair : ctx->DssmFactorCtxs) {
        const auto& dssmCtx = pair.second;
        Y_VERIFY(dssmCtx.ContextEmbeddings.size() == numDocs);
        Y_VERIFY(dssmCtx.ReplyEmbeddings.size() == numDocs);
    }

    if (numDocs == 0) {
        return;
    }
    ctx->QueryContext.resize(NumTurns);
    for (auto& context : ctx->Contexts) {
        context.resize(NumTurns);
    }
    size_t numFactors = 0;
    for (auto& doc : *factors) {
        Y_VERIFY(numFactors == 0 || doc.size() + NumFactors == numFactors);
        numFactors = doc.size() + NumFactors;
        doc.reserve(numFactors);
    }
    for (const auto& factor : Factors) {
        factor->AppendFactors(*ctx, factors);
    }
    Y_VERIFY(std::all_of(factors->begin(), factors->end(), [&](const auto& doc) { return doc.size() == numFactors; }));
}

const std::pair<size_t, size_t>* TFactorCalcer::GetFactorsLocation(const TString& factorName) const {
    return FactorsLocation.FindPtr(factorName);
}

}
