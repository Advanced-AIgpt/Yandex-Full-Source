#pragma once

#include "state.h"
#include "state_dumper.h"
#include <alice/nlu/granet/lib/sample/sample.h>

namespace NGranet {

// ~~~~ TParserDebugInfo ~~~~

class TParserDebugInfo : public TThrRefBase {
public:
    using TRef = TIntrusivePtr<TParserDebugInfo>;
    using TConstRef = TIntrusiveConstPtr<TParserDebugInfo>;

public:
    TParserDebugInfo(const TGrammar::TConstRef& grammar, const TSample::TConstRef& sample,
        TVector<TParserStateList>&& mainChart, TVector<TParserStateList>&& fillerChart);

    const TGrammar::TConstRef& GetGrammar() const {
        return Grammar;
    }
    const TSample::TConstRef& GetSample() {
        return Sample;
    }

    TString FindParserBlockerTokenStr() const;
    size_t FindParserBlockerTokenIndex() const;
    TVector<const TParserState*> GetOccurrences() const;

    void Dump(IOutputStream* log, const TString& indent = "") const;

private:
    void DumpNumberOfStates(IOutputStream* log, const TString& indent) const;
    static TString PrintStateCount(const TParserStateList& list);
    void DumpOccurrences(IOutputStream* log, const TString& indent) const;
    static void CollectOccurrences(const TVector<TParserStateList>& chart, TVector<const TParserState*>* result);

private:
    TGrammar::TConstRef Grammar;
    TSample::TConstRef Sample;
    TVector<TParserStateList> MainChart;
    TVector<TParserStateList> FillerChart;
};

} // namespace NGranet
