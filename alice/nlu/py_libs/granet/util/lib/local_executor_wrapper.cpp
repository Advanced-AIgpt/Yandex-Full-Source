#include "local_executor_wrapper.h"

#include <alice/nlu/granet/lib/parser/result.h>
#include <library/cpp/threading/local_executor/local_executor.h>

#include <utility>

namespace NParallelGranet {

using namespace NGranet;
using namespace std;

using TLocalExecutorPtr = TAtomicSharedPtr<NPar::TLocalExecutor>;

TVector<TString> ParseInParallel(const TGrammar::TRef& grammar, TVector<TIntrusivePtr<TSample>>& samples, int nThreads, int blockSize, bool needValues, bool needTypes) {
    TVector<TString> results(samples.size());

    const TLocalExecutorPtr localExecutor = new NPar::TLocalExecutor();
    localExecutor->RunAdditionalThreads(nThreads);

    NPar::TLocalExecutor::TExecRangeParams blockParams(0, samples.size());
    blockParams.SetBlockSize(blockSize);

    localExecutor->ExecRange([&](int blockIdx) {
        const int blockStart = blockIdx * blockParams.GetBlockSize();
        const int nextBlockStart = Min<ui64>(blockStart + blockParams.GetBlockSize(), blockParams.LastId);
        for (int i = blockStart; i < nextBlockStart; i++) {
            TVector<TParserFormResult::TConstRef> forms = ParseSample(grammar, samples[i]);
            // in verstehen tasks we check only one form, so results needed only for this form
            if (forms.empty() || !forms[0]->IsPositive()) {
                continue;
            }
            const auto slots = forms[0]->GetBestVariant()->ToMarkup();

            TSampleMarkup sample_markup;
            sample_markup.Text = samples[i]->GetText();
            sample_markup.Slots = slots;
            results[i] = sample_markup.PrintMarkup(needValues, needTypes);
        }
    }, 0, blockParams.GetBlockCount(), NPar::TLocalExecutor::WAIT_COMPLETE);
    return results;
}


TVector<TParts> ParseToAlmostMatchedPartsInParallel(const TGrammar::TRef& grammar, TVector<TIntrusivePtr<TSample> >& samples, int nThreads, int blockSize) {
    TVector<TParts> results(samples.size());
    const TString filler = "(.)*";

    const TLocalExecutorPtr localExecutor = new NPar::TLocalExecutor();
    localExecutor->RunAdditionalThreads(nThreads);

    NPar::TLocalExecutor::TExecRangeParams blockParams(0, samples.size());
    blockParams.SetBlockSize(blockSize);

    localExecutor->ExecRange([&](int blockIdx) {

        const int blockStart = blockIdx * blockParams.GetBlockSize();
        const int nextBlockStart = Min<ui64>(blockStart + blockParams.GetBlockSize(), blockParams.LastId);

        for (int i = blockStart; i < nextBlockStart; i++) {
            TParts& parts = results[i];
            TVector<TParserFormResult::TConstRef> forms = ParseSample(grammar, samples[i]);

            if (forms.empty() || !forms[0]->IsPositive()) {
                NNlu::TInterval interval;
                interval.Begin = 0;
                interval.End = samples[i]->GetText().length();
                parts.emplace_back(make_pair(interval, false));
                continue;
            }

            const TVector<TSlotMarkup> markups = forms[0]->GetBestVariant()->ToNonterminalMarkup(grammar);
            size_t start = 0;

            for (const auto& markup: markups) {
                if (markup.Name != filler) {
                    continue;
                }

                size_t begin = markup.Interval.Begin;
                size_t end = markup.Interval.End;

                if (begin - start > 0) {
                    NNlu::TInterval interval;
                    interval.Begin = start;
                    interval.End = begin;
                    parts.emplace_back(make_pair(interval, true));
                }

                if (end - begin > 0) {
                    parts.emplace_back(make_pair(markup.Interval, false));
                }

                start = end;
            }

            if (samples[i]->GetText().length() > start) {
                NNlu::TInterval interval;
                interval.Begin = start;
                interval.End = samples[i]->GetText().length();
                parts.emplace_back(make_pair(interval, true));
            }
        }
    }, 0, blockParams.GetBlockCount(), NPar::TLocalExecutor::WAIT_COMPLETE);
    return results;
}

TVector<TParserFormResult::TConstRef> ParseToResultsInParallel(const TGrammar::TRef& grammar, TVector<TSample::TRef>& samples, int nThreads, int blockSize) {
    TVector<TParserFormResult::TConstRef> results(samples.size());

    const TLocalExecutorPtr localExecutor = new NPar::TLocalExecutor();
    localExecutor->RunAdditionalThreads(nThreads);

    NPar::TLocalExecutor::TExecRangeParams blockParams(0, samples.size());
    blockParams.SetBlockSize(blockSize);

    localExecutor->ExecRange([&](int blockIdx) {
        const int blockStart = blockIdx * blockParams.GetBlockSize();
        const int nextBlockStart = Min<ui64>(blockStart + blockParams.GetBlockSize(), blockParams.LastId);
        for (int i = blockStart; i < nextBlockStart; i++) {
            TVector<TParserFormResult::TConstRef> forms = ParseSample(grammar, samples[i]);
            // in verstehen tasks we check only one form, so results needed only for this form
            if (forms.empty()) {
                continue;
            }
            results[i] = forms[0];
        }
    }, 0, blockParams.GetBlockCount(), NPar::TLocalExecutor::WAIT_COMPLETE);
    return results;
}

}
