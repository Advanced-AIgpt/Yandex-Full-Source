#pragma once

#include <library/cpp/scheme/scheme.h>
#include <util/generic/hash.h>
#include <util/generic/string.h>
#include <util/generic/vector.h>

namespace NBASS {

namespace NMarket {

using TPersBasketEntryId = i64;

struct TPersBasketEntry {
    TString Text;
    THashMap<TString, TString> MetaMap;

    TPersBasketEntry() = default;
    explicit TPersBasketEntry(TStringBuf text) : Text(text) {}

    NSc::TValue ToJson() const;
    void FromJson(const NSc::TValue& json);
};

struct TPersBasketEntryWithId : public TPersBasketEntry {
    TString AddedAt;
    TPersBasketEntryId Id;

    NSc::TValue ToJson() const;
    void FromJson(const NSc::TValue& json);
};

class TPersBasketEntryResponse : public TVector<TPersBasketEntryWithId> {
public:
    TPersBasketEntryResponse() = default;
    explicit TPersBasketEntryResponse(const NSc::TValue& json);
};

} // namespace NMarket

} // namespace NBASS
