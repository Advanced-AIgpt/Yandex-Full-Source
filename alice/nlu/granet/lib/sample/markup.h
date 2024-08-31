#pragma once

#include "tag.h"
#include <alice/nlu/granet/lib/utils/flag_utils.h>
#include <alice/nlu/libs/interval/interval.h>
#include <alice/nlu/libs/tuple_like_type/tuple_like_type.h>
#include <util/digest/sequence.h>
#include <util/generic/set.h>
#include <util/generic/vector.h>

namespace NGranet {

// ~~~~ ESlotPrintingOption ~~~~

// Examples:
//   text with 'slot'(name)                             - 0
//   text with 'slot'(name:value)                       - SPO_NEED_VALUES
//   text with 'slot'(name/type:value)                  - SPO_NEED_VALUES | SPO_NEED_TYPES
//   text with 'slot'(name:value1;value2)               - SPO_NEED_VALUES | SPO_NEED_VARIANTS
//   text with 'slot'(name/type1;type2:value1;value2)   - SPO_NEED_VALUES | SPO_NEED_TYPES | SPO_NEED_VARIANTS
enum ESlotPrintingOption : ui32 {
    SPO_NEED_VALUES         = FLAG32(0),
    SPO_NEED_TYPES          = FLAG32(1),
    SPO_NEED_VARIANTS       = FLAG32(2),
};

Y_DECLARE_FLAGS(ESlotPrintingOptions, ESlotPrintingOption);
Y_DECLARE_OPERATORS_FOR_FLAGS(ESlotPrintingOptions);

const ESlotPrintingOptions SPO_NEED_ALL = SPO_NEED_VALUES | SPO_NEED_TYPES | SPO_NEED_VARIANTS;

inline ESlotPrintingOptions MakeSlotPrintingOptions(bool needValues, bool needTypes, bool needVariants) {
    return FlagsIf(SPO_NEED_VALUES, needValues)
        | FlagsIf(SPO_NEED_TYPES, needTypes)
        | FlagsIf(SPO_NEED_VARIANTS, needVariants);
}

// ~~~~ TSlotMarkup ~~~~

struct TSlotMarkup {
    NNlu::TInterval Interval;
    TString Name;
    TVector<TString> Types;
    TVector<TString> Values;

    DECLARE_TUPLE_LIKE_TYPE(TSlotMarkup, Interval, Name, Types, Values);

    // Empty Type should be treated as unknown.
    bool HasTypes() const {
        return !Types.empty();
    }

    // Empty Value should be treated as unknown.
    bool HasValues() const {
        return !Values.empty();
    }

    TString PrintMarkup(ESlotPrintingOptions options) const;
    TString PrintMarkup(bool needValues, bool needTypes) const; // deprecated
    void ReadMarkup(TStringBuf str);
    void MakeSafeForPrint();
};

// ~~~~ TSlotMarkup functions ~~~~

IOutputStream& operator<<(IOutputStream& out, const TSlotMarkup& slot);

TSlotMarkup ReadSlotMarkup(const NNlu::TInterval& interval, TStringBuf str);

TTag ToTag(const TSlotMarkup& slot, ESlotPrintingOptions options);
TVector<TTag> ToTags(const TVector<TSlotMarkup>& slots, ESlotPrintingOptions options);
TSlotMarkup ToSlotMarkup(const TTag& tag);
TVector<TSlotMarkup> ToSlotMarkups(const TVector<TTag>& tags);

// ~~~~ TSampleMarkup ~~~~

struct TSampleMarkup {
    bool IsPositive = false;
    TString Text;
    TVector<TSlotMarkup> Slots;

    DECLARE_TUPLE_LIKE_TYPE(TSampleMarkup, IsPositive, Text, Slots);

    // Print text with slots (without positive/negative info)
    TString PrintMarkup(ESlotPrintingOptions options) const;
    TString PrintMarkup(bool needValues, bool needTypes) const; // deprecated

    // Print positive/negative prefix and text with slots if positive.
    TString PrintForReport(ESlotPrintingOptions options) const;
    TString PrintForReport(bool needValues, bool needTypes) const; // deprecated

    // Read text with slots (without positive/negative info)
    bool TryReadMarkup(TStringBuf str);

    void MakeSafeForPrint();

    // Empty Type and Value in markup (not result) are treated as undefined (correspond to any).
    bool CheckResult(const TSampleMarkup& result, bool compareSlotsByTop) const;

    bool HasTypes() const;
    bool HasValues() const;
};

// ~~~~ TSampleMarkup functions ~~~~

TSampleMarkup ReadSampleMarkup(bool isPositive, TStringBuf str);

} // namespace NGranet

template <>
struct THash<NGranet::TSlotMarkup>: public TTupleLikeTypeHash {
};

template <>
struct THash<TVector<NGranet::TSlotMarkup>>: public TSimpleRangeHash {
};

template <>
struct THash<NGranet::TSampleMarkup>: public TTupleLikeTypeHash {
};
