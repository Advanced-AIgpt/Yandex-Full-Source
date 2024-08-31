#pragma once

#include <alice/nlu/granet/lib/grammar/grammar.h>
#include <alice/nlu/granet/lib/grammar/multi_grammar.h>
#include <alice/nlu/granet/lib/parser/parser.h>
#include <alice/nlu/granet/lib/parser/result.h>
#include <alice/nlu/granet/lib/sample/sample.h>
#include <util/generic/hash.h>
#include <util/generic/hash_set.h>
#include <util/generic/noncopyable.h>

namespace NGranet {

class TMultiParser : public TThrRefBase {
public:
    using TRef = TIntrusivePtr<TMultiParser>;
    using TConstRef = TIntrusiveConstPtr<TMultiParser>;

public:
    static TRef Create(const TMultiPreprocessedSample::TRef& preprocessedSample, bool ensureEntities);
    static TRef Create(const TMultiGrammar::TConstRef& multiGrammar, const TSample::TRef& sample, bool ensureEntities);
    static TRef Create(const TGrammar::TConstRef& grammar, const TSample::TRef& sample, bool ensureEntities);

    TMultiParser& SetLog(IOutputStream* log, bool isVerbose);

    // Add debug info to TParserFormResult.
    TMultiParser& SetNeedDebugInfo(bool value);

    // Disable some optimizations to properly collect blocker words from samples.
    TMultiParser& SetCollectBlockersMode(bool value);

    const TMultiGrammar::TConstRef& GetMultiGrammar();

    TParserTaskResult::TRef ParseTask(EParserTaskType type, TStringBuf name);

    TVector<TParserEntityResult::TConstRef> ParseEntities();
    TParserEntityResult::TRef ParseEntity(TStringBuf name);

    TVector<TParserFormResult::TConstRef> ParseForms();
    TParserFormResult::TRef ParseForm(TStringBuf name);

private:
    TMultiParser(const TMultiPreprocessedSample::TRef& preprocessedSample, bool ensureEntities);

    TParserEntityResult::TRef ParseEntity(TStringBuf name, const TMultiGrammar::TTaskInfo& task);
    TParserFormResult::TRef ParseForm(TStringBuf name, const TMultiGrammar::TTaskInfo& task);
    TParserTaskResult::TRef ParseTask(TStringBuf name, const TMultiGrammar::TTaskInfo& task);
    void ParseDependencies(const TMultiGrammar::TTaskInfo& form);

private:
    TMultiPreprocessedSample::TRef Sample;
    bool ShouldEnsureEntities = false;

    IOutputStream* Log = nullptr;
    bool IsLogVerbose = false;
    bool NeedDebugInfo = true;
    bool CollectBlockersMode = false;
};

} // namespace NGranet
