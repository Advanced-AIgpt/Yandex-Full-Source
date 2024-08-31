#include "markup.h"
#include <alice/nlu/granet/lib/utils/utils.h>
#include <library/cpp/iterator/zip.h>
#include <util/generic/algorithm.h>
#include <util/string/builder.h>
#include <util/string/cast.h>
#include <util/string/join.h>
#include <util/string/split.h>
#include <util/string/subst.h>

namespace NGranet {

// ~~~~ TSlotMarkup ~~~~

TString TSlotMarkup::PrintMarkup(ESlotPrintingOptions options) const {
    TStringBuilder out;
    out << Name;
    if (options.HasFlags(SPO_NEED_TYPES) && HasTypes()) {
        out << '/' << (options.HasFlags(SPO_NEED_VARIANTS) ? JoinSeq(";", Types) : Types[0]);
    }
    if (options.HasFlags(SPO_NEED_VALUES) && HasValues()) {
        out << ':' << (options.HasFlags(SPO_NEED_VARIANTS) ? JoinSeq(";", Values) : Values[0]);
    }
    return out;
}

TString TSlotMarkup::PrintMarkup(bool needValues, bool needTypes) const {
    return PrintMarkup(MakeSlotPrintingOptions(needValues, needTypes, false));
}

void TSlotMarkup::ReadMarkup(TStringBuf str) {
    const auto& values = str.SplitOff(':');
    if (values != "") {
        Values = StringSplitter(values).Split(';').ToList<TString>();
    }
    Name = TString{str.NextTok('/')};
    if (str != "") {
        Types = StringSplitter(str).Split(';').ToList<TString>();
    }
}

void TSlotMarkup::MakeSafeForPrint() {
    if (AnyOf(Types, [](const TString& type) { return type.find_first_of(TStringBuf(":;)\"\r\n\t")) != NPOS; })) {
        Types.clear();
    }
    if (AnyOf(Values, [](const TString& value) { return value.find_first_of(TStringBuf(";\r\n\t")) != NPOS || ScanTagName(value) < value.length(); })) {
        Values.clear();
    }
}

// ~~~~ TSlotMarkup functions ~~~~

IOutputStream& operator<<(IOutputStream& out, const TSlotMarkup& slot) {
    return out << "{" << slot.Interval << ", \"" << slot.PrintMarkup(0) << "\"}";
}

TSlotMarkup ReadSlotMarkup(const NNlu::TInterval& interval, TStringBuf str) {
    TSlotMarkup slot;
    slot.Interval = interval;
    slot.ReadMarkup(str);
    return slot;
}

TTag ToTag(const TSlotMarkup& slot, ESlotPrintingOptions options) {
    return {slot.Interval, slot.PrintMarkup(options)};
}

TVector<TTag> ToTags(const TVector<TSlotMarkup>& slots, ESlotPrintingOptions options) {
    TVector<TTag> tags(Reserve(slots.size()));
    for (const TSlotMarkup& slot : slots) {
        tags.push_back(ToTag(slot, options));
    }
    return tags;
}

TSlotMarkup ToSlotMarkup(const TTag& tag) {
    return ReadSlotMarkup(tag.Interval, tag.Name);
}

TVector<TSlotMarkup> ToSlotMarkups(const TVector<TTag>& tags) {
    TVector<TSlotMarkup> slots(Reserve(tags.size()));
    for (const TTag& tag : tags) {
        slots.push_back(ToSlotMarkup(tag));
    }
    return slots;
}

// ~~~~ TSampleMarkup ~~~~

TString TSampleMarkup::PrintMarkup(ESlotPrintingOptions options) const {
    return PrintTaggerMarkup(Text, ToTags(Slots, options));
}

TString TSampleMarkup::PrintMarkup(bool needValues, bool needTypes) const {
    return PrintMarkup(MakeSlotPrintingOptions(needValues, needTypes, false));
}

static const TString POSITIVE_PREFIX = "positive: ";
static const TString NEGATIVE_STR = "negative";

TString TSampleMarkup::PrintForReport(ESlotPrintingOptions options) const {
    return IsPositive ? POSITIVE_PREFIX + PrintMarkup(options) : NEGATIVE_STR;
}

TString TSampleMarkup::PrintForReport(bool needValues, bool needTypes) const {
    return PrintForReport(MakeSlotPrintingOptions(needValues, needTypes, false));
}

bool TSampleMarkup::TryReadMarkup(TStringBuf str) {
    TString text;
    TVector<TTag> tags;
    if (!TryReadTaggerMarkup(str, &text, &tags)) {
        return false;
    }
    Text = text;
    Slots = Sorted(ToSlotMarkups(tags));
    return true;
}

void TSampleMarkup::MakeSafeForPrint() {
    SubstGlobal(Text, '\'', '_');
    SubstGlobal(Text, '(', '_');
    SubstGlobal(Text, ')', '_');
    SubstGlobal(Text, '\r', '_');
    SubstGlobal(Text, '\n', '_');
    SubstGlobal(Text, '\t', '_');
    for (TSlotMarkup& slot : Slots) {
        slot.MakeSafeForPrint();
    }
}

static bool CheckResultData(const TVector<TString>& expected, const TVector<TString>& actual, bool compareSlotsByTop) {
    if (expected.empty()) {
        return true;
    }

    if (compareSlotsByTop) {
        if (expected.size() > actual.size()) {
            return false;
        }

        for (const auto& [expectedItem, actualItem] : Zip(expected, actual)) {
            if (expectedItem != actualItem) {
                return false;
            }
        }

        return true;
    } else {
        for (const auto& expectedItem : expected) {
            if (!IsIn(actual.begin(), actual.end(), expectedItem)) {
                return false;
            }
        }

        return true;
    }
}

static bool CheckResultSlot(const TSlotMarkup& expected, const TSlotMarkup& actual, bool compareSlotsByTop) {
    if (expected.Interval != actual.Interval) {
        return false;
    }
    if (expected.Name != actual.Name) {
        return false;
    }

    if (expected.HasTypes() && !CheckResultData(expected.Types, actual.Types, compareSlotsByTop)) {
        return false;
    }
    if (expected.HasValues() && !CheckResultData(expected.Values, actual.Values, compareSlotsByTop)) {
        return false;
    }

    return true;
}

static bool CheckResultSlots(const TVector<TSlotMarkup>& expected, const TVector<TSlotMarkup>& actual, bool compareSlotsByTop) {
    if (expected.size() != actual.size()) {
        return false;
    }
    for (const auto& [expectedSlot, actualSlot] : Zip(expected, actual)) {
        if (!CheckResultSlot(expectedSlot, actualSlot, compareSlotsByTop)) {
            return false;
        }
    }
    return true;
}

bool TSampleMarkup::CheckResult(const TSampleMarkup& result, bool compareSlotsByTop) const {
    if (result.IsPositive != IsPositive || result.Text != Text) {
        return false;
    }
    if (!IsPositive) {
        return true;
    }
    // Check slots fast
    if (CheckResultSlots(Slots, result.Slots, compareSlotsByTop)) {
        return true;
    }
    // Check slots accurate as sets
    return CheckResultSlots(Sorted(Slots), Sorted(result.Slots), compareSlotsByTop);
}

bool TSampleMarkup::HasTypes() const {
    for (const TSlotMarkup& slot : Slots) {
        if (slot.HasTypes()) {
            return true;
        }
    }
    return false;
}

bool TSampleMarkup::HasValues() const {
    for (const TSlotMarkup& slot : Slots) {
        if (slot.HasValues()) {
            return true;
        }
    }
    return false;
}

// ~~~~ TSampleMarkup functions ~~~~

TSampleMarkup ReadSampleMarkup(bool isPositive, TStringBuf str) {
    TSampleMarkup markup;
    if (!markup.TryReadMarkup(str)) {
        ythrow TFromStringException() << "Can not parse sample markup \"" << str << "\". ";
    }
    markup.IsPositive = isPositive;
    return markup;
}

} // namespace NGranet
