#pragma once

#include <util/generic/string.h>
#include <util/generic/vector.h>
#include <util/generic/hash.h>

namespace NNlg {

static const TString BASE_DSSM_MODEL_NAME = "base_model";

struct TDssmFactorCtx {
    const float* QueryEmbedding;
    TVector<const float*> ContextEmbeddings;
    TVector<const float*> ReplyEmbeddings;
    size_t Dimension;
};

struct TSeq2SeqFactorCtx {
    bool IsSeq2Seq;
    float Score;
    size_t NumTokens;
};

struct TFactorCalcerCtx {
    TFactorCalcerCtx(TVector<TVector<TString>>& contexts, TVector<TString>& replies)
        : Contexts(contexts)
        , Replies(replies)
    {
    }

    TFactorCalcerCtx(TVector<TVector<TString>>& contexts, TVector<TString>& replies, TVector<TSeq2SeqFactorCtx>& seq2SeqFactorCtxs)
        : Contexts(contexts)
        , Replies(replies)
        , Seq2SeqFactorCtxs(seq2SeqFactorCtxs)
    {
    }

    TVector<TVector<TString>>& Contexts;
    TVector<TString>& Replies;

    TVector<TString> QueryContext;
    THashMap<TString, TDssmFactorCtx> DssmFactorCtxs;
    TVector<TString> DssmIndexNames;
    TVector<TString> KnnIndexNames;
    TVector<const float*> StaticFactors;
    size_t NumStaticFactors;
    TVector<TSeq2SeqFactorCtx> Seq2SeqFactorCtxs;
};

}
