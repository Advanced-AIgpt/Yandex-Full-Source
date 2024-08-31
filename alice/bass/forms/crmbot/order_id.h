#pragma once

#include <util/generic/string.h>

namespace NBASS::NCrmbot{

class TOrderId
{
public:
    TOrderId(TStringBuf source);
    TString ToString() const;

    bool HasBeruOrderID() const;
    TStringBuf GetBeruOrderID() const;
    bool IsGreenMarketOrder() const;
    bool IsUnknown() const;

private:
    enum class EOrderType {
        BLUE, BLUE_DROPSHIP, GREEN, UNKNOWN
    };
    TString BeruOrderId;
    TString FullID;
    EOrderType Type;
};

IOutputStream& operator<<(IOutputStream& strm, const TOrderId& orderId);

}
