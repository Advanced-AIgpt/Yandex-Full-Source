#pragma once

#include <util/generic/string.h>
#include <util/generic/vector.h>

namespace NAlice {

struct TRecognizedSlot {
    struct TVariantValue {
        TString Type;
        TString Value;
    };

    TString Name;
    TVector<TVariantValue> Variants;

    ui32 Begin;
    ui32 End;
};

TString GetSlotValue(const TVector<TString>& tokens, ui32 begin, ui32 end);

TString PackVariantsValue(const TVector<TRecognizedSlot::TVariantValue>& values);
TVector<TRecognizedSlot::TVariantValue> UnPackVariantsValue(const TString& value);

} // namespace NAlice
