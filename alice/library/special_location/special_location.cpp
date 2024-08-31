#include "special_location.h"

namespace NAlice {

TSpecialLocation::TSpecialLocation(TStringBuf str) {
    if (str == TStringBuf("nearby")) {
        Value = EType::NEARBY;
    } else if (str == TStringBuf("near_me")) {
        Value = EType::NEAR_ME;
    } else if (str == TStringBuf("nearest")) {
        Value = EType::NEAREST;
    } else if (str == TStringBuf("home")) {
        Value = EType::HOME;
    } else if (str == TStringBuf("work")) {
        Value = EType::WORK;
    } else if (str == TStringBuf("my_neighborhood")) {
        Value = EType::CURRENT_LOCALITY;
    } else if (str == TStringBuf("my_country")) {
        Value = EType::CURRENT_COUNTRY;
    } else {
        Value = EType::ERROR;
    }
}

TStringBuf TSpecialLocation::AsString() const {
    switch (Value) {
        case EType::NEARBY:
            return TStringBuf("nearby");
        case EType::NEAR_ME:
            return TStringBuf("near_me");
        case EType::NEAREST:
            return TStringBuf("nearest");
        case EType::HOME:
            return TStringBuf("home");
        case EType::WORK:
            return TStringBuf("work");
        case EType::CURRENT_LOCALITY:
            return TStringBuf("my_neighborhood");
        case EType::CURRENT_COUNTRY:
            return TStringBuf("my_country");
        case EType::ERROR:
            return TStringBuf();
    }
}

TSpecialLocation TSpecialLocation::GetNamedLocation(TStringBuf slotValue, TStringBuf slotType) {
    if (slotType != TStringBuf("special_location") && slotType != TStringBuf("named_location")) {
        return TSpecialLocation(TSpecialLocation::EType::ERROR);
    }
    return TSpecialLocation(slotValue);
}

}
