#include "result.h"
#include "debug.h"
#include <alice/nlu/granet/lib/utils/string_utils.h>
#include <library/cpp/iterator/enumerate.h>
#include <util/string/join.h>
#include <util/string/printf.h>
#include <util/string/strip.h>

namespace NGranet {

// ~~~~ TResultSlot ~~~~

TTag TResultSlot::ToTag() const {
    return {Interval, Name};
}

void TResultSlot::Dump(IOutputStream* log, const TSample::TConstRef& sample, const TString& indent) const {
    Y_ENSURE(log);
    *log << indent << Name << ":" << Endl;
    *log << indent << "  Interval: " << Interval << Endl;
    *log << indent << "  Text: " << sample->GetTextByIntervalOnTokens(Interval) << Endl;
    *log << indent << "  Data:" << Endl;
    for (const TResultSlotValue& value : Data) {
        *log << indent << "    " << value.Type << ": " << value.Value << Endl;
    }
}

// ~~~~ TParserVariant ~~~~

TVector<TSlotMarkup> TParserVariant::ToMarkup() const {
    TVector<TSlotMarkup> markupSlots(Reserve(Slots.size()));
    for (const TResultSlot& slot : Slots) {
        TSlotMarkup& markupSlot = markupSlots.emplace_back();
        markupSlot.Interval = Sample->ConvertPositionToText(slot.Interval);
        markupSlot.Name = slot.Name;
        if (slot.Data.empty()) {
            continue;
        }
        for (const auto& value : slot.Data) {
            markupSlot.Values.push_back(value.Value);
            markupSlot.Types.push_back(value.Type);
        }
    }
    Sort(markupSlots);
    return markupSlots;
}

TVector<TSlotMarkup> TParserVariant::ToNonterminalMarkup(const TGrammar::TConstRef& grammar) const {
    TVector<TSlotMarkup> markupSlots;
    ElementTree->ToNonterminalMarkup(grammar, Sample, markupSlots);
    Sort(markupSlots);
    return markupSlots;
}

void TParserVariant::Dump(const TGrammar::TConstRef& grammar, IOutputStream* log, const TString& indent) const {
    Y_ENSURE(log);
    *log << indent << "LogProb: " << Sprintf("%.2f", LogProb) << Endl;
    *log << indent << "Slots:" << Endl;
    for (const TResultSlot& slot : Slots) {
        slot.Dump(log, Sample, indent + "  ");
    }
    *log << indent << "Tree:" << Endl;
    ElementTree->DumpTree(grammar, Sample, log, indent + "  ");
}


// ~~~~ TParserTaskResult ~~~~

void TParserTaskResult::DumpDebugInfo(IOutputStream* log, const TString& indent) const {
    if (DebugInfo) {
        *log << indent << "  DebugInfo:" << Endl;
        DebugInfo->Dump(log, indent + "    ");
    } else {
        *log << indent << "  No debug info" << Endl;
    }
}

const TParserDebugInfo::TConstRef& TParserTaskResult::GetDebugInfo() const {
    return DebugInfo;
}

void TParserTaskResult::SetDebugInfo(const TParserDebugInfo::TConstRef& debugInfo) {
    DebugInfo = debugInfo;
}

// ~~~~ TParserFormResult ~~~~

TSampleMarkup TParserFormResult::ToMarkup() const {
    TSampleMarkup markup;
    markup.IsPositive = IsPositive();
    markup.Text = Sample->GetText();
    if (IsPositive()) {
        markup.Slots = GetBestVariant()->ToMarkup();
    }
    return markup;
}

void TParserFormResult::Dump(IOutputStream* log, const TString& indent) const {
    Y_ENSURE(log);
    *log << indent << "Parser result for form " << Name << ":" << Endl;
    *log << indent << "  Result: " << ToMarkup().PrintForReport(0) << Endl;
    *log << indent << "  IsInternal: " << FormatBool(GetIsInternal()) << Endl;
    *log << indent << "  Variant count: " << Variants.size() << Endl;
    if (!Variants.empty()) {
        *log << indent << "  BestVariant:" << Endl;
        GetBestVariant()->Dump(Grammar, log, indent + "    ");
    }
    DumpDebugInfo(log, indent);
}

// ~~~~ TParserEntityResult ~~~~

void TParserEntityResult::SetName(TStringBuf name) {
    TParserTaskResult::SetName(name);
    for (TEntity& entity : Entities) {
        entity.Type = Name;
    }
}

TSampleMarkup TParserEntityResult::ToMarkup() const {
    TSampleMarkup markup;
    markup.IsPositive = IsPositive();
    markup.Text = Sample->GetText();
    for (const TEntity& entity : Entities) {
        TSlotMarkup& slot = markup.Slots.emplace_back();
        slot.Interval = Sample->ConvertPositionToText(entity.Interval);
        slot.Name = entity.Type;
        slot.Values.push_back(entity.Value);
    }
    Sort(markup.Slots);
    return markup;
}

void TParserEntityResult::Dump(IOutputStream* log, const TString& indent) const {
    Y_ENSURE(log);
    *log << indent << "Parser result for entity " << Name << ":" << Endl;
    *log << indent << "  Result: " << ToMarkup().PrintForReport(0) << Endl;
    *log << indent << "  IsInternal: " << FormatBool(GetIsInternal()) << Endl;
    *log << indent << "  Entities:" << Endl;
    Sample->DumpEntities(Entities, log, indent + "    ");
    DumpDebugInfo(log, indent);
}

// ~~~~ Helpers ~~~~

TParserTaskResult::TRef CreateParserEmptyResult(const TParserTask& task, const TSample::TConstRef& sample,
    const TGrammar::TConstRef& grammar)
{
    if (task.Type == PTT_FORM) {
        return TParserFormResult::Create(task.Name, task.IsInternal, sample, grammar, {});
    } else if (task.Type == PTT_ENTITY) {
        return TParserEntityResult::Create(task.Name, task.IsInternal, sample, grammar, {});
    } else {
        Y_ENSURE(false);
        return nullptr;
    }
}

} // namespace NGranet
