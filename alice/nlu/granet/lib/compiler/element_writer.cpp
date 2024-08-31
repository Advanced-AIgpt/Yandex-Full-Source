#include "element_writer.h"
#include <dict/nerutil/tstimer.h>
#include <library/cpp/iterator/enumerate.h>
#include <util/digest/sequence.h>
#include <util/generic/hash.h>
#include <util/generic/iterator_range.h>
#include <util/generic/stack.h>
#include <util/generic/xrange.h>
#include <util/generic/ymath.h>
#include <util/string/builder.h>

namespace NGranet::NCompiler {

static const size_t EMPTY_RULE_TRIE_INDEX = 0;

// ~~~~

void WriteGrammarElements(const TDeque<TCompilerElement>& srcElements, TGrammarData* data) {
    DEBUG_TIMER("WriteGrammarElements");
    Y_ENSURE(data);

    Y_ENSURE(EMPTY_RULE_TRIE_INDEX == data->RuleTriePool.size());
    data->RuleTriePool.emplace_back();

    THashMap<TVector<TElementRuleTag>, NNlu::TInterval> tagPoolRanges;

    for (const TCompilerElement& srcElement : srcElements) {
        TElementWriter(srcElement, data, &tagPoolRanges).Write();
    }
}

// ~~~~ TElementWriter ~~~~

TElementWriter::TElementWriter(const TCompilerElement& srcElement, TGrammarData* data,
        THashMap<TVector<TElementRuleTag>, NNlu::TInterval>* tagPoolRanges)
    : SrcElement(srcElement)
    , Data(data)
    , TagPoolRanges(tagPoolRanges)
{
    Y_ENSURE(Data);
    Y_ENSURE(TagPoolRanges);
}

void TElementWriter::Write() {
    Y_ENSURE(SrcElement.IsCompiled);
    Y_ENSURE(SrcElement.Id == Data->Elements.size());

    const TCompilerElementParams& srcParams = SrcElement.Params;

    Element = &Data->Elements.emplace_back();
    Element->Id = SrcElement.Id;
    Element->Name = SrcElement.Name;
    Element->EntityName = srcParams.EntityName;
    SetFlags(&Element->Flags, EF_ANCHOR_TO_BEGIN, srcParams.AnchorToBegin);
    SetFlags(&Element->Flags, EF_ANCHOR_TO_END, srcParams.AnchorToEnd);
    SetFlags(&Element->Flags, EF_ENABLE_FILLERS, srcParams.EnableFillers);
    SetFlags(&Element->Flags, EF_COVER_FILLERS, srcParams.CoverFillers);
    SetFlags(&Element->Flags, EF_ENABLE_EDGE_FILLERS, srcParams.EnableEdgeFillers);
    Element->EnableSynonymFlagsMask = srcParams.EnableSynonymFlagsMask;
    Element->EnableSynonymFlags = srcParams.EnableSynonymFlags;
    Element->Quantity = srcParams.Quantity;
    Element->Level = SrcElement.Level;
    if (SrcElement.Definition != nullptr) {
        Element->SourceForSlots = SrcElement.Definition->SourceForSlots;
    }

    WriteRules();
}

void TElementWriter::WriteRules() {
    if (SrcElement.Rules.empty()) {
        SetRuleTrie(EMPTY_RULE_TRIE_INDEX);
        return;
    }
    CollectRules();
    ConvertMarkups();
    SortRules();
    RemoveConflictedRules();
    CalculateLogProb();
    WriteData();
    WriteMarkups();
    WriteFlags();
    CollectChildren();
    BuildRuleTrie();
}

void TElementWriter::CollectRules() {
    Y_ENSURE(Rules.empty());

    Rules.reserve(SrcElement.Rules.size());
    for (const TCompiledRule& rule : SrcElement.Rules) {
        Rules.emplace_back().Rule = &rule;
    }
}

void TElementWriter::ConvertMarkups() {
    for (TRuleInfo& rule : Rules) {
        TStack<ui16> tagBeginStack;
        TVector<TElementRuleTag> tags;
        rule.FilteredChain.reserve(rule.Rule->Chain.size());
        for (const TTokenId token : rule.Rule->Chain) {
            if (!NTokenId::IsSlotEdge(token)) {
                rule.FilteredChain.push_back(token);
                continue;
            }
            const ui16 pos = rule.FilteredChain.size();
            if (token == TOKEN_SLOT_BEGIN) {
                tagBeginStack.push(pos);
                continue;
            }
            TElementRuleTag tag;
            tag.Interval = {tagBeginStack.top(), pos};
            tag.Tag = NTokenId::ToSlotMarkupStringId(token);
            tags.push_back(tag);
            tagBeginStack.pop();
        }
        rule.TagPoolRange = AddRuleMarkup(tags);
    }
}

NNlu::TInterval TElementWriter::AddRuleMarkup(const TVector<TElementRuleTag>& markup) {
    if (markup.empty()) {
        return {};
    }
    const auto [it, isNew] = TagPoolRanges->try_emplace(markup);
    if (isNew) {
        const size_t start = Data->TagPool.size();
        it->second = {start, start + markup.size()};
        Extend(markup, &Data->TagPool);
    }
    return it->second;
}

void TElementWriter::SortRules() {
    StableSort(Rules, [](const TRuleInfo& rule1, const TRuleInfo& rule2) {
        if (rule1.FilteredChain != rule2.FilteredChain) {
            return rule1.FilteredChain < rule2.FilteredChain;
        }
        const TListItemParams& params1 = rule1.Rule->ListItemParams;
        const TListItemParams& params2 = rule2.Rule->ListItemParams;
        if (params1.IsForced != params2.IsForced) {
            return params1.IsForced;
        }
        if (params1.Weight != params2.Weight) {
            return params1.Weight > params2.Weight;
        }
        if (params1.IsNegative != params2.IsNegative) {
            return params1.IsNegative;
        }
        return false;
    });
}

void TElementWriter::RemoveConflictedRules() {
    // Rules with same chain must have same flags IsForced and IsNegative.
    for (size_t i = 0; i < Rules.size(); ++i) {
        const TRuleInfo& rule1 = Rules[i];
        const TListItemParams& params1 = rule1.Rule->ListItemParams;
        for (size_t j = i + 1; j < Rules.size(); ++j) {
            const TRuleInfo& rule2 = Rules[j];
            const TListItemParams& params2 = rule2.Rule->ListItemParams;
            if (rule1.FilteredChain != rule2.FilteredChain) {
                break;
            }
            if (params1.IsForced != params2.IsForced || params1.IsNegative != params2.IsNegative) {
                Rules.erase(Rules.begin() + j);
                --j;
            }
        }
    }
}

void TElementWriter::CalculateLogProb() {
    double weightTotal = 0;
    for (const TRuleInfo& rule : Rules) {
        weightTotal += rule.Rule->ListItemParams.Weight;
    }
    const double epsilon = 1e-20;
    const double probabilityTotal = log(Max(epsilon, weightTotal));
    for (TRuleInfo& rule : Rules) {
        const double weight = rule.Rule->ListItemParams.Weight;
        const double logProb = log(Max(epsilon, weight)) - probabilityTotal;
        Element->RulesLogProbs.push_back(static_cast<float>(logProb));
    }
}

void TElementWriter::WriteData() {
    for (const TRuleInfo& rule : Rules) {
        if (rule.Rule->ListItemParams.DataType != 0 || rule.Rule->ListItemParams.DataValue != 0) {
            Element->Flags |= EF_HAS_DATA;
            break;
        }
    }
    if (!Element->Flags.HasFlags(EF_HAS_DATA)) {
        return;
    }
    for (const TRuleInfo& rule : Rules) {
        Element->DataTypes.push_back(rule.Rule->ListItemParams.DataType);
        Element->DataValues.push_back(rule.Rule->ListItemParams.DataValue);
    }
}

void TElementWriter::WriteMarkups() {
    for (const TRuleInfo& rule : Rules) {
        if (!rule.TagPoolRange.Empty()) {
            Element->Flags |= EF_HAS_TAGS;
            break;
        }
    }
    if (!Element->Flags.HasFlags(EF_HAS_TAGS)) {
        return;
    }
    for (const TRuleInfo& rule : Rules) {
        Element->TagPoolRanges.push_back(rule.TagPoolRange);
    }
}

void TElementWriter::WriteFlags() {
    for (const auto& [i, rule] : Enumerate(Rules)) {
        if (rule.Rule->ListItemParams.IsForced) {
            Element->ForcedRules.Set(i);
        }
        if (rule.Rule->ListItemParams.IsNegative) {
            Element->NegativeRules.Set(i);
        }
        if (rule.Rule->BagItemParams.IsRequired) {
            Element->SetOfRequiredRules |= static_cast<ui32>(1u << i);
        }
        if (rule.Rule->BagItemParams.IsLimited) {
            Element->SetOfLimitedRules |= static_cast<ui32>(1u << i);
        }
    }
}

void TElementWriter::CollectChildren() {
    TSet<TElementId> children;
    for (const TRuleInfo& rule : Rules) {
        for (const TTokenId id : rule.FilteredChain) {
            if (NTokenId::IsElement(id)) {
                children.insert(NTokenId::ToElementId(id));
            }
        }
    }
    Element->ElementsInRules = ToVector<TElementId>(children);
}

void TElementWriter::BuildRuleTrie() {
    TRuleTrie::TBuilder builder;
    for (ui32 i = 0; i < Rules.size();) {
        TRuleIndexes indexes;
        indexes.RuleIndex = i;
        const TVector<TTokenId>& chain = Rules[i].FilteredChain;
        i++;
        while (i < Rules.size() && Rules[i].FilteredChain == chain) {
            i++;
        }
        indexes.RuleCount = i - indexes.RuleIndex;
        builder.Add(chain, indexes);
    }
    TBufferOutput buffer;
    builder.Save(buffer);
    Data->RuleTriePool.emplace_back(TBlob::FromBuffer(buffer.Buffer()));
    SetRuleTrie(Data->RuleTriePool.size() - 1);
}

void TElementWriter::SetRuleTrie(size_t trieIndex) {
    Element->RuleTrieIndex = trieIndex;
    Element->RulesBeginIterator = TSearchIterator<TRuleTrie>(Data->RuleTriePool[trieIndex]);
}

} // namespace NGranet::NCompiler
