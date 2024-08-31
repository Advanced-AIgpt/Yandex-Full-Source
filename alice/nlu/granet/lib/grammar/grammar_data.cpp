#include "grammar_data.h"
#include "grammar.h"
#include <alice/nlu/granet/lib/utils/utils.h>
#include <util/generic/array_ref.h>
#include <util/generic/xrange.h>
#include <util/string/builder.h>
#include <util/string/join.h>

namespace NGranet {

// ~~~~ TSlotDescription ~~~~

void TSlotDescription::Dump(IOutputStream* log, const TString& indent) const {
    Y_ENSURE(log);
    *log << indent << "TSlotDescription:" << Endl;
    *log << indent << "  Name: " << Name << Endl;
    *log << indent << "  DataTypes:" << Endl;
    for (const TString& type : DataTypes) {
        *log << indent << "    " << type << Endl;
    }
    *log << indent << "  MatchingType: " << MatchingType << Endl;
    *log << indent << "  ConcatenateStrings: " << ConcatenateStrings << Endl;
    *log << indent << "  KeepVariants: " << KeepVariants << Endl;
}

// ~~~~ TParserTask ~~~~

IOutputStream& operator<<(IOutputStream& out, const TParserTaskKey& key) {
    out << key.Type << ' ' << key.Name;
    return out;
}

// ~~~~ TParserTask ~~~~

TParserTaskKey TParserTask::GetTaskKey() const {
    return {Type, Name};
}

void TParserTask::Dump(const TGrammar& grammar, IOutputStream* log, const TString& indent) const {
    Y_ENSURE(log);
    *log << indent << "TParserTask:" << Endl;
    *log << indent << "  Name: " << Name << Endl;
    *log << indent << "  Type: " << Type << Endl;
    *log << indent << "  Index: " << Index << Endl;
    *log << indent << "  Root: " << grammar.GetElementNameForLog(Root) << Endl;
    *log << indent << "  UserFiller: " << grammar.GetElementNameForLog(UserFiller) << Endl;
    *log << indent << "  AutoFiller: " << grammar.GetElementNameForLog(AutoFiller) << Endl;
    *log << indent << "  EnableGranetParser: " << EnableGranetParser << Endl;
    *log << indent << "  EnableAliceTagger: " << EnableAliceTagger << Endl;
    *log << indent << "  EnableAutoFiller: " << EnableAutoFiller << Endl;
    *log << indent << "  EnableSynonymFlagsMask: " << EnableSynonymFlagsMask << Endl;
    *log << indent << "  EnableSynonymFlags: " << EnableSynonymFlags << Endl;
    *log << indent << "  KeepOverlapped: " << KeepOverlapped << Endl;
    *log << indent << "  KeepVariants: " << KeepVariants << Endl;
    *log << indent << "  IsAction: " << IsAction << Endl;
    *log << indent << "  IsFixlist: " << IsFixlist << Endl;
    *log << indent << "  IsConditional: " << IsConditional << Endl;
    *log << indent << "  IsInternal: " << IsInternal << Endl;
    *log << indent << "  Fresh: " << Fresh << Endl;
    *log << indent << "  Freshness: " << Freshness << Endl;
    *log << indent << "  Slots:" << Endl;
    for (const TSlotDescription& slot : Slots) {
        slot.Dump(log, indent + "    ");
    }
    *log << indent << "  DependsOnEntities: " << JoinSeq(", ", DependsOnEntities) << Endl;
}

// ~~~~ TQuantityParams ~~~~

TString TQuantityParams::GetNormalizedText() const {
    if (MinCount == 0) {
        if (MaxCount == 1) {
            return "?";
        } else if (MaxCount == Max<ui8>()) {
            return "*";
        }
    } else if (MinCount == 1) {
        if (MaxCount == 1) {
            return "";
        } else if (MaxCount == Max<ui8>()) {
            return "+";
        }
    }
    if (MinCount == MaxCount) {
        return TStringBuilder() << '<' << ToInt(MinCount) << '>';
    }
    return TStringBuilder() << '<' << ToInt(MinCount) << ',' << ToInt(MaxCount) << '>';
}

// ~~~~ TGrammarElement ~~~~

static void DumpRules(const TGrammarElement& element, const TGrammar& grammar, const TTokenPool& tokenPool,
    IOutputStream* log, bool verbose, const TString& indent = "")
{
    Y_ENSURE(log);

    if (element.RuleTrieIndex == NPOS) {
        *log << indent << "Rule trie undefined" << Endl;
        return;
    }
    *log << indent << "Rules:" << Endl;

    size_t counter = 0;
    for (const auto& [rule, data] : grammar.GetData().RuleTriePool[element.RuleTrieIndex]) {
        *log << indent;
        if (!verbose && counter >= 5) {
            *log << "..." << Endl;
            break;
        }
        counter++;

        for (const TTokenId id : rule) {
            *log << "  ";
            if (NTokenId::IsElement(id)) {
                *log << grammar.GetData().Elements[NTokenId::ToElementId(id)].Name;
            } else {
                *log << tokenPool.PrintWordToken(id);
            }
        }
        for (const size_t i : xrange(data.RuleCount)) {
            const size_t ruleIndex = data.RuleIndex + i;
            *log << (i == 0 ? "  # " : " | ");
            *log << "index: " << ruleIndex;
            *log << ", logprob: " << element.RulesLogProbs[ruleIndex];
            *log << (element.ForcedRules.Get(ruleIndex) ? ", forced" : "");
            *log << (element.NegativeRules.Get(ruleIndex) ? ", negative" : "");
            const ui32 ruleAsFlag = data.GetRuleIndexAsFlag();
            *log << (HasFlag(element.SetOfRequiredRules, ruleAsFlag) ? ", required" : "");
            *log << (HasFlag(element.SetOfLimitedRules, ruleAsFlag) ? ", limited" : "");
            *log << (HasFlag(element.SetOfPossibleEmptyRequiredRules, ruleAsFlag) ? ", can be empty" : "");
        }
        *log << Endl;
    }
}

void TGrammarElement::Dump(const TGrammar& grammar, const TTokenPool& tokenPool,
    IOutputStream* log, bool verbose, const TString& indent) const
{
    Y_ENSURE(log);

    *log << indent << "TGrammarElement:" << Endl;
    *log << indent << "  Id: " << Id << Endl;
    *log << indent << "  Name: " << Name << Endl;
    *log << indent << "  ElementsInRules: " << JoinSeq(", ", ElementsInRules) << Endl;
    *log << indent << "  Flags: " << FormatFlags(Flags) << Endl;
    *log << indent << "  EntityName: " << EntityName << Endl;
    *log << indent << "  DataTypes:" << Endl;
    for (const size_t i : xrange(DataTypes.GetGroupCount())) {
        *log << indent << "    " << DataTypes.GetGroupLength(i) << " rules: "
            << grammar.GetData().StringPool[DataTypes.GetGroupValue(i)] << Endl;
    }
    *log << indent << "  DataValues:" << Endl;
    for (const size_t i : xrange(DataValues.GetGroupCount())) {
        *log << indent << "    " << DataValues.GetGroupLength(i) << " rules: "
            << grammar.GetData().StringPool[DataValues.GetGroupValue(i)] << Endl;
    }
    *log << indent << "  TagPoolRanges:" << Endl;
    for (const size_t i : xrange(TagPoolRanges.GetGroupCount())) {
        *log << indent << "    " << TagPoolRanges.GetGroupLength(i) << " rules: " << TagPoolRanges.GetGroupValue(i) << Endl;
    }
    *log << indent << "  Quantity: " << Quantity.GetNormalizedText() << Endl;
    *log << indent << "  SetOfRequiredRules: " << Bin(SetOfRequiredRules, HF_ADDX) << Endl;
    *log << indent << "  SetOfLimitedRules:  " << Bin(SetOfLimitedRules, HF_ADDX) << Endl;
    *log << indent << "  Level: " << Level << Endl;
    *log << indent << "  SourceForSlots:" << Endl;
    for (const TSlotDescriptionId& slotId : SourceForSlots) {
        const TParserTask& task = grammar.GetData().GetTasks(slotId.TaskType)[slotId.TaskIndex];
        *log << indent << "    " << task.Type << " " << task.Name << " " << task.Slots[slotId.SlotIndex].Name << Endl;
    }
    *log << indent << "  CanSkip: " << CanSkip << Endl;
    *log << indent << "  LogProbOfSkip: " << LogProbOfSkip << Endl;
    *log << indent << "  SetOfPossibleEmptyRequiredRules: " << Bin(SetOfPossibleEmptyRequiredRules, HF_ADDX) << Endl;
    *log << indent << "  LogProbOfRequiredRulesSkip:" << Endl;
    for (const auto& [ruleIndexFlag, logProbOfSkip] : LogProbOfRequiredRulesSkip ) {
        *log << indent << "    " << Bin(ruleIndexFlag, HF_ADDX) << ", " << logProbOfSkip << Endl;
    }
    DumpRules(*this, grammar, tokenPool, log, verbose, indent + "  ");
}

// ~~~~ TTokenToElementsMap ~~~~

bool TTokenToElementsMap::IsEmpty() const {
    return TokenToOffset.empty();
}

void TTokenToElementsMap::AddTokenUncommonElementsToSet(TTokenId token, TDynBitMap* set) const {
    if (IsEmpty()) {
        return;
    }
    Y_ASSERT(set);
    Y_ASSERT(ElementSetPool.at(0) == UNDEFINED_ELEMENT_ID);
    for (ui32 i = TokenToOffset.Value(token, 0); ElementSetPool[i] != UNDEFINED_ELEMENT_ID; i++) {
        set->Set(ElementSetPool[i]);
    }
}


void TTokenToElementsMap::Dump(const TTokenPool& tokenPool, IOutputStream* log, const TString& indent) const {
    Y_ENSURE(log);
    *log << indent << "TTokenToElementsMap:" << Endl;
    *log << indent << "  ElementSetPool:" << Endl;
    for (size_t i = 0; i < ElementSetPool.size(); i++) {
        if (i == 0 || ElementSetPool[i - 1] == UNDEFINED_ELEMENT_ID) {
            *log << indent << "    offset " << i << ": ";
        } else {
            *log << ", ";
        }
        if (ElementSetPool[i] != UNDEFINED_ELEMENT_ID) {
            *log << ElementSetPool[i];
        } else {
            *log << Endl;
        }
    }
    *log << indent << "  Token to elements:" << Endl;
    for (const TTokenId tokenId : OrderedSetOfKeys(TokenToOffset)) {
        *log << indent << "    " << tokenPool.PrintWordToken(tokenId) << ": ";
        const ui32 from = TokenToOffset.at(tokenId);
        for (ui32 i = from; ElementSetPool[i] != UNDEFINED_ELEMENT_ID; i++) {
            if (i - from < 20) {
                *log << (i != from ? ", ": "") << ElementSetPool[i];
            } else {
                const size_t count = i + FindIndex(MakeArrayRef(ElementSetPool).Slice(i), UNDEFINED_ELEMENT_ID);
                *log << ", ... (" << count << " elements)";
            }
        }
        *log << Endl;
    }
    TVector<TElementId> common;
    Y_FOR_EACH_BIT(id, CommonElements) {
        common.push_back(id);
    }
    *log << indent << "  Common elements: " << JoinSeq(", ", common) << Endl;
}

// ~~~~ TGrammarOptimizationInfo ~~~~

void TGrammarOptimizationInfo::Dump(const TTokenPool& tokenPool, IOutputStream* log, const TString& indent) const {
    Y_ENSURE(log);
    *log << indent << "TGrammarOptimizationInfo:" << Endl;
    *log << indent << "  SpecificWordToElements:" << Endl;
    SpecificWordToElements.Dump(tokenPool, log, indent + "    ");
    *log << indent << "  FirstWordToElements:" << Endl;
    FirstWordToElements.Dump(tokenPool, log, indent + "    ");
}

} // namespace NGranet
