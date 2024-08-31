#include "skip_params_calculator.h"
#include <dict/nerutil/tstimer.h>
#include <util/generic/xrange.h>
#include <util/stream/trace.h>
#include <util/string/printf.h>

namespace NGranet::NCompiler {

static const float IMPOSSIBLE = -1e6f;

// ~~~~ TElementSkipParamsCalculator ~~~~

TElementSkipParamsCalculator::TElementSkipParamsCalculator(TGrammarData* data)
    : Data(*data)
    , Elements(data->Elements)
{
    Y_ENSURE(data);
}

void TElementSkipParamsCalculator::Process() {
    DEBUG_TIMER("TElementSkipParamsCalculator::Process");
    Y_DBGTRACE(DEBUG, "TElementSkipParamsCalculator::Process:");
    Y_DBGTRACE(DEBUG, "  Possible empty required rules:");

    for (size_t level : xrange(Data.ElementLevelCount)) {
        for (TGrammarElement& element : Elements) {
            if (element.Level == level) {
                element.LogProbOfSkip = CalculateLogProbOfElementSkip(element);
                element.CanSkip = element.LogProbOfSkip > IMPOSSIBLE;
            }
        }
    }

    Y_DBGTRACE(DEBUG, "  Possible empty elements:");
    for (TGrammarElement& element : Elements) {
        if (element.CanSkip) {
            Y_DBGTRACE(DEBUG, Sprintf("%10.2f ", element.LogProbOfSkip) << element.Name);
        }
    }
}

float TElementSkipParamsCalculator::CalculateLogProbOfElementSkip(TGrammarElement& element) const {
    if (element.IsEntity()) {
        return element.Quantity.MinCount == 0 ? 0 : IMPOSSIBLE;
    }

    // Bag
    if (element.SetOfRequiredRules != 0) {
        float result = 0;
        for (const auto& [rule, data] : GetElementRules(element)) {
            const ui32 ruleIndexFlag = data.GetRuleIndexAsFlag();
            if (!HasFlags(element.SetOfRequiredRules, ruleIndexFlag)) {
                continue;
            }
            const float ruleLogProb = CalculateLogProbOfRuleSkip(rule) + element.RulesLogProbs[data.RuleIndex];
            result += ruleLogProb;
            if (ruleLogProb > IMPOSSIBLE) {
                Y_DBGTRACE(DEBUG, Sprintf("%10.2f rule ", ruleLogProb) << data.RuleIndex << " of " << element.Name);
                element.LogProbOfRequiredRulesSkip.push_back({ruleIndexFlag, ruleLogProb});
                element.SetOfPossibleEmptyRequiredRules |= ruleIndexFlag;
            }
        }
        return Max(result, IMPOSSIBLE);
    }

    if (element.Quantity.MinCount == 0) {
        return 0;
    }

    // List
    float bestNegative = IMPOSSIBLE;
    float bestPositive = IMPOSSIBLE;
    bool isNegativeForced = false;
    bool isPositiveForced = false;
    for (const auto& [rule, data] : GetElementRules(element)) {
        const float ruleLogProb = CalculateLogProbOfRuleSkip(rule) + element.RulesLogProbs[data.RuleIndex];
        if (ruleLogProb <= IMPOSSIBLE) {
            continue;
        }
        const bool isForced = element.ForcedRules.Get(data.RuleIndex);
        const bool isNegative = element.NegativeRules.Get(data.RuleIndex);
        float& bestLogProb = isNegative ? bestNegative : bestPositive;
        bool& bestIsForced = isNegative ? isNegativeForced : isPositiveForced;
        if (isForced > bestIsForced) {
            bestIsForced = isForced;
            bestLogProb = ruleLogProb;
        } else if (isForced == bestIsForced) {
            bestLogProb = Max(bestLogProb, ruleLogProb);
        }
    }
    const bool isNegativeWin = isNegativeForced == isPositiveForced
        ? bestPositive < bestNegative
        : isPositiveForced < isNegativeForced;
    if (isNegativeWin) {
        return IMPOSSIBLE;
    }
    return bestPositive * ToFloat(element.Quantity.MinCount);
}

float TElementSkipParamsCalculator::CalculateLogProbOfRuleSkip(const TVector<TTokenId>& rule) const {
    float result = 0;
    for (const TTokenId id : rule) {
        if (!NTokenId::IsElement(id)) {
            return IMPOSSIBLE;
        }
        result += Elements[NTokenId::ToElementId(id)].LogProbOfSkip;
    }
    return Max(result, IMPOSSIBLE);
}

const TRuleTrie& TElementSkipParamsCalculator::GetElementRules(const TGrammarElement& element) const {
    Y_ENSURE(element.RuleTrieIndex != NPOS);
    return Data.RuleTriePool[element.RuleTrieIndex];
}

} // namespace NGranet::NCompiler
