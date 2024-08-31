#pragma once

#include <alice/nlu/granet/lib/grammar/grammar_data.h>

namespace NGranet::NCompiler {

// ~~~~ TElementSkipParamsCalculator ~~~~

class TElementSkipParamsCalculator {
public:
    explicit TElementSkipParamsCalculator(TGrammarData* data);

    void Process();

private:
    float CalculateLogProbOfElementSkip(TGrammarElement& element) const;
    float CalculateLogProbOfRuleSkip(const TVector<TTokenId>& rule) const;
    const TRuleTrie& GetElementRules(const TGrammarElement& element) const;

private:
    TGrammarData& Data;
    TVector<TGrammarElement>& Elements;
};

} // namespace NGranet::NCompiler
