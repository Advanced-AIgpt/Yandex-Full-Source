#include "seq2seq_factor.h"
#include <alice/boltalka/extsearch/base/factors/factors_gen.h>

namespace NNlg {

TSeq2SeqFactor::TSeq2SeqFactor() {}

void TSeq2SeqFactor::AppendFactors(const TFactorCalcerCtx& ctx, TVector<TVector<float>>* factors) const {
    for (size_t i = 0; i < factors->size(); ++i) {
        if (ctx.Seq2SeqFactorCtxs.size() == 0) {
            (*factors)[i].push_back(0);
            (*factors)[i].push_back(0.0);
            continue;
        }

        auto& seq2SeqCtx = ctx.Seq2SeqFactorCtxs[i];
        (*factors)[i].push_back(seq2SeqCtx.IsSeq2Seq ? 1 : 0);
        (*factors)[i].push_back(seq2SeqCtx.IsSeq2Seq ? seq2SeqCtx.Score : 0.0);
    }
}

}
