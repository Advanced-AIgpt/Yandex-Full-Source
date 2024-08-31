#pragma once

#include "preprocessed_sample.h"
#include "result.h"
#include "state.h"
#include <alice/nlu/granet/lib/utils/utils.h>
#include <alice/nlu/granet/lib/grammar/grammar.h>
#include <alice/nlu/granet/lib/sample/markup.h>

namespace NGranet {

// ~~~~ TParser ~~~~

// Left-to-right Earley's parser.
class TParser : public TMoveOnly {
public:
    TParser(const TPreprocessedSample::TConstRef& preprocessedSample, const TParserTask& task);

    TParser& SetLog(IOutputStream* log, bool isVerbose);

    // Add debug info to TParserFormResult.
    TParser& SetNeedDebugInfo(bool value);

    // Disable some optimizations to properly collect blocker words from samples.
    TParser& SetCollectBlockersMode(bool value);

    TParserTaskResult::TRef Parse();

private:
    void DumpInitialization() const;
    bool IsTaskPromising() const;
    bool IsElementPromising(int pos, TElementId id) const;
    void PrepareChart();
    void PrepareChart(bool isFiller, size_t levelCount, size_t stateLimit);
    const TVector<TParserStateList>& GetChart(bool isFiller) const;
    TVector<TParserStateList>& GetChart(bool isFiller);
    void ProcessPosition(int pos);
    void ProcessPosition(bool isFiller, int pos);
    void BeginRoot(int pos);
    const TParserState* AddBeginningState(bool isFiller, int pos, const TGrammarElement& element,
        float solutionLogProbUpperBound);
    void Predict(bool isFiller, const TParserState& state);
    void Scan(bool isFiller, const TParserState& state);
    void Complete(bool isFiller, const TParserState& state);
    void Dispatch(bool isFiller, const TParserState& state);
    void DispatchNormal(bool isFiller, const TParserState& child);
    void DispatchFiller(const TParserState& filler);
    bool CanPassFiller(const TParserState& state) const;
    void AddNextState(bool isFiller, const TParserStateKey& key, EParserStateEventType type,
        const TParserState& prev, float logProbIncrement = 0, const TParserState* predictedChild = nullptr,
        const TParserState* passedChild = nullptr, TRuleIndexes completeRules = {});
    TParserTaskResult::TRef BuildResult();

private:
    TPreprocessedSample::TConstRef PreprocessedSample;
    TGrammar::TConstRef Grammar;
    const TGrammarData& GrammarData;
    TSample::TConstRef Sample;
    const TVector<TParserVertex>& Vertices;
    const int VertexCount = 0;

    bool CollectBlockersMode = false;

    const TParserTask& Task;
    const TGrammarElement* Root = nullptr;
    const TGrammarElement* UserFiller = nullptr;
    const TGrammarElement* AutoFiller = nullptr;

    bool NeedDebugInfo = true;
    IOutputStream* Log = nullptr;
    bool IsLogVerbose = false;

    TVector<TParserStateList> MainChart;
    TVector<TParserStateList> FillerChart;
    TParserStateDumper StateDumper;
};

} // namespace NGranet
