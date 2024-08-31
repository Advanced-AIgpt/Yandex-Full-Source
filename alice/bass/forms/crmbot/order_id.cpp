#include "order_id.h"

#include <util/generic/vector.h>
#include <util/string/split.h>
#include <util/string/join.h>

#include <regex>

namespace NBASS::NCrmbot {

TOrderId::TOrderId(TStringBuf source)
{
    TVector<TStringBuf> sourceParts;
    Split(source, " ", sourceParts);
    TString sourceCompressed = JoinRange(TStringBuf(""), sourceParts.begin(), sourceParts.end());

    std::regex yamarketRegex1("YA[0-9]*", std::regex::icase);
    std::regex yamarketRegex2("[0-9]{4,4}-YD[0-9]{6,6}", std::regex::icase);
    std::regex yamarketRegex3("[0-9]{4,4}-LO-[0-9]{6,6}", std::regex::icase);
    std::regex beruRegex("([0-9]{7,9})");
    std::regex dropshipRegex("([0-9]{8,9})/[0-9a-zA-Z]+");
    std::smatch match;

    if (std::regex_match(sourceCompressed.c_str(), match, beruRegex)) {
        Type = EOrderType::BLUE;
        BeruOrderId = match[1].str();
        FullID = sourceCompressed;
    } else if (std::regex_match(sourceCompressed.c_str(), match, dropshipRegex)) {
        Type = EOrderType::BLUE_DROPSHIP;
        BeruOrderId = match[1].str();
        FullID = sourceCompressed;
    } else if (
        std::regex_match(sourceCompressed.c_str(), yamarketRegex1) ||
        std::regex_match(sourceCompressed.c_str(), yamarketRegex2) ||
        std::regex_match(sourceCompressed.c_str(), yamarketRegex3)
    ) {
        Type = EOrderType::GREEN;
        BeruOrderId = "";
        FullID = sourceCompressed;
    } else {
        Type = EOrderType::UNKNOWN;
        BeruOrderId = "";
        FullID = sourceCompressed;
    }
}

TString TOrderId::ToString() const
{
    return FullID;
}

bool TOrderId::HasBeruOrderID() const
{
    return !BeruOrderId.Empty();
}

TStringBuf TOrderId::GetBeruOrderID() const
{
    return BeruOrderId;
}

bool TOrderId::IsUnknown() const
{
    return Type == EOrderType::UNKNOWN;
}

bool TOrderId::IsGreenMarketOrder() const
{
    return Type == EOrderType::GREEN;
}

IOutputStream& operator<<(IOutputStream& strm, const TOrderId& orderId)
{
    return strm << orderId.ToString();
}

}
