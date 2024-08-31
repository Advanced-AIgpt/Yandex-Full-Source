#include "pers_basket_entry.h"

#include <alice/bass/libs/logging_v2/logger.h>
#include <library/cpp/scheme/scheme_cast.h>

namespace NBASS {

namespace NMarket {

NSc::TValue TPersBasketEntry::ToJson() const
{
    NSc::TValue result;
    result[TStringBuf("text")] = Text;
//    result[TStringBuf("meta_info")] =  NJsonConverters::ToTValue(MetaMap);
    return result;
}

void TPersBasketEntry::FromJson(const NSc::TValue& json)
{
    Text = json[TStringBuf("text")].GetString();
//    NJsonConverters::FromTValue(json[TStringBuf("meta_info")], MetaMap, true /* validate */ );
}

NSc::TValue TPersBasketEntryWithId::ToJson() const
{
    NSc::TValue result = TPersBasketEntry::ToJson();
    result[TStringBuf("added_at")] = AddedAt;
    result[TStringBuf("id")] =  Id;
    return result;
}

void TPersBasketEntryWithId::FromJson(const NSc::TValue& json)
{
    TPersBasketEntry::FromJson(json);
    AddedAt = json[TStringBuf("added_at")].GetString();
    Id = json[TStringBuf("id")].GetIntNumber();
}

TPersBasketEntryResponse::TPersBasketEntryResponse(const NSc::TValue& json)
{
    for (const auto& raw : json["result"]["entries"].GetArray()) {
        TPersBasketEntryWithId entry;
        entry.FromJson(raw);
        push_back(entry);
    }
}

} // namespace NMarket

} // namespace NBASS
