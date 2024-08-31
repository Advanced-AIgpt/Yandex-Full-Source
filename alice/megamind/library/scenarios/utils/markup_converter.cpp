#include "markup_converter.h"

namespace NAlice::NMegamind {

void ConvertExternalMarkup(const NBg::NProto::TExternalMarkupProto& begemotMarkup,
                           NScenarios::TBegemotExternalMarkup& externalMarkup)
{
#define SetSimpleField(src, dst, FieldName)       \
    if (src.Has##FieldName()) {                   \
        dst.Set##FieldName(src.Get##FieldName()); \
    }

    SetSimpleField(begemotMarkup, externalMarkup, OriginalRequest);
    SetSimpleField(begemotMarkup, externalMarkup, ProcessedRequest);

#define SetRepeatedFields(src, dst, FieldName, FieldCopy, depth) \
    for (const auto& srcField##depth : src.Get##FieldName()) {   \
        auto& dstField##depth = *dst.Add##FieldName();           \
        FieldCopy                                                \
    }

#define SetMessageField(src, dst, FieldName, FieldCopy, depth) \
    if (src.Has##FieldName()) {                                \
        auto& dstMessage##depth = *dst.Mutable##FieldName();   \
        auto& srcMessage##depth = src.Get##FieldName();        \
        FieldCopy                                              \
    }

    SetRepeatedFields(begemotMarkup, externalMarkup, Tokens, {
        SetSimpleField(srcField0, dstField0, Text);
        SetSimpleField(srcField0, dstField0, BeginChar);
        SetSimpleField(srcField0, dstField0, EndChar);
    },
    0);
    SetRepeatedFields(begemotMarkup, externalMarkup, Delimiters, {
        SetSimpleField(srcField0, dstField0, Text);
        SetSimpleField(srcField0, dstField0, BeginChar);
        SetSimpleField(srcField0, dstField0, EndChar);
    },
    0);

#define FillTokenSpan(srcField, dstField)      \
    SetSimpleField(srcField, dstField, Begin); \
    SetSimpleField(srcField, dstField, End);

    SetRepeatedFields(begemotMarkup, externalMarkup, Morph, {
        SetMessageField(srcField0, dstField0, Tokens, {
            FillTokenSpan(srcMessage1, dstMessage1);
        },
        1);
        SetRepeatedFields(srcField0, dstField0, Lemmas, {
            SetSimpleField(srcField1, dstField1, Text);
            SetSimpleField(srcField1, dstField1, Language);
            SetRepeatedFields(srcField1, dstField1, Grammems, {
                dstField2 = srcField2;
            },
            2);
            SetRepeatedFields(srcField1, dstField1, Forms, {
                SetSimpleField(srcField2, dstField2, Text);
                SetRepeatedFields(srcField2, dstField2, Grammems, {
                    dstField3 = srcField3;
                },
                3);
            },
            2);
        },
        1);
    },
    0);
    SetRepeatedFields(begemotMarkup, externalMarkup, GeoAddr, {
        SetMessageField(srcField0, dstField0, Tokens, {
            FillTokenSpan(srcMessage1, dstMessage1);
        },
        1);
        SetSimpleField(srcField0, dstField0, Type);
        SetRepeatedFields(srcField0, dstField0, Fields, {
            SetMessageField(srcField1, dstField1, Tokens, {
                FillTokenSpan(srcMessage2, dstMessage2);
            },
            2);
            SetSimpleField(srcField1, dstField1, Type);
            SetSimpleField(srcField1, dstField1, Name);
        },
        1);
    },
    0);
    SetRepeatedFields(begemotMarkup, externalMarkup, GeoAddrRoute, {
        SetSimpleField(srcField0, dstField0, Type);
        SetSimpleField(srcField0, dstField0, Transport);
        SetSimpleField(srcField0, dstField0, GeoAddrFrom);
        SetSimpleField(srcField0, dstField0, GeoAddrTo);
        SetSimpleField(srcField0, dstField0, GeoAddrIn);
    },
    0);
    SetRepeatedFields(begemotMarkup, externalMarkup, Fio, {
        SetSimpleField(srcField0, dstField0, Type);
        SetSimpleField(srcField0, dstField0, FirstName);
        SetSimpleField(srcField0, dstField0, LastName);
        SetSimpleField(srcField0, dstField0, Patronymic);
        SetMessageField(srcField0, dstField0, Tokens, {
            FillTokenSpan(srcMessage1, dstMessage1);
        },
        1);
    },
    0);
    SetMessageField(begemotMarkup, externalMarkup, Porn, {
        SetSimpleField(srcMessage0, dstMessage0, IsPornoQuery);
    },
    0);
    SetMessageField(begemotMarkup, externalMarkup, DirtyLang, {
        if (srcMessage0.HasClass()) {
            dstMessage0.SetDirtyLangClass(srcMessage0.GetClass());
        }
    },
    0);
    SetRepeatedFields(begemotMarkup, externalMarkup, MeasurementUnits, {
        SetSimpleField(srcField0, dstField0, Unit);
        SetSimpleField(srcField0, dstField0, UnitDenom);
        SetSimpleField(srcField0, dstField0, Property);
        SetSimpleField(srcField0, dstField0, Value);
        SetMessageField(srcField0, dstField0, Tokens, {
            FillTokenSpan(srcMessage1, dstMessage1);
        },
        1);
        SetMessageField(srcField0, dstField0, Range, {
            SetSimpleField(srcMessage1, dstMessage1, From);
            SetSimpleField(srcMessage1, dstMessage1, To);
        },
        1);
    },
    0);
    SetRepeatedFields(begemotMarkup, externalMarkup, Date, {
        SetSimpleField(srcField0, dstField0, Day);
        SetSimpleField(srcField0, dstField0, RelativeDay);
        SetSimpleField(srcField0, dstField0, Month);
        SetSimpleField(srcField0, dstField0, RelativeMonth);
        SetSimpleField(srcField0, dstField0, Year);
        SetSimpleField(srcField0, dstField0, RelativeYear);
        SetSimpleField(srcField0, dstField0, Week);
        SetSimpleField(srcField0, dstField0, RelativeWeek);
        SetSimpleField(srcField0, dstField0, Century);
        SetSimpleField(srcField0, dstField0, BeforeCommonEra);
        SetSimpleField(srcField0, dstField0, Hour);
        SetSimpleField(srcField0, dstField0, Min);
        SetSimpleField(srcField0, dstField0, Prep);
        SetSimpleField(srcField0, dstField0, IntervalBegin);
        SetSimpleField(srcField0, dstField0, IntervalEnd);
        SetMessageField(srcField0, dstField0, Tokens, {
            FillTokenSpan(srcMessage1, dstMessage1);
        },
        1);
        SetMessageField(srcField0, dstField0, Duration, {
            SetSimpleField(srcMessage1, dstMessage1, Type);
            SetSimpleField(srcMessage1, dstMessage1, Day);
            SetSimpleField(srcMessage1, dstMessage1, Month);
            SetSimpleField(srcMessage1, dstMessage1, Year);
            SetSimpleField(srcMessage1, dstMessage1, Hour);
            SetSimpleField(srcMessage1, dstMessage1, Min);
            SetSimpleField(srcMessage1, dstMessage1, Week);
            SetSimpleField(srcMessage1, dstMessage1, Times);
        },
        1);
    },
    0);

#undef SetSimpleField
#undef SetMessageField
#undef FillTokenSpan
#undef SetRepeatedFields
}

} // namespace NAlice::NMegamind
