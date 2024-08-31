#pragma once

#include <util/generic/strbuf.h>
#include <util/generic/string.h>

namespace NAlice {
/// Helper "enum class" for special location with methods.
struct TSpecialLocation {
    enum class EType : ui8 {
        ERROR            /* "" */,
        NEARBY           /* "nearby" */,            ///< around some specific place
        NEAR_ME          /* "near_me" */,           ///< around user (get my coords from meta)
        NEAREST          /* "nearest" */,           ///< nearest one to user (get my coords from meta)
        HOME             /* "home" */,              ///< user's home address (datasync or bass db)
        WORK             /* "work" */,              ///< user's work address (datasync or bass db)
        CURRENT_LOCALITY /* "my_neighborhood" */,   ///< user's current city or distrcit (not home city)
        CURRENT_COUNTRY  /* "my_country" */,        ///< user's current country (not home country)
    } Value;

    explicit TSpecialLocation(TStringBuf str);

    explicit TSpecialLocation(EType value)
        : Value(value) {
    }

    operator EType() const {
        return Value;
    }

    operator TStringBuf() const {
        return AsString();
    }

    TStringBuf AsString() const;

    bool IsError() const {
        return Value == TSpecialLocation::EType::ERROR;
    }

    bool IsNearLocation() const {
        return Value == TSpecialLocation::EType::NEARBY || Value == TSpecialLocation::EType::NEAR_ME ||
               Value == TSpecialLocation::EType::NEAREST;
    }

    bool IsUserAddress() const {
        return Value == TSpecialLocation::EType::HOME || Value == TSpecialLocation::EType::WORK;
    }

    bool IsCurrentGeo() const {
        return Value == TSpecialLocation::EType::CURRENT_COUNTRY ||
               Value == TSpecialLocation::EType::CURRENT_LOCALITY;
    }

    /** Get enum value of named special location if any.
     */
    static TSpecialLocation GetNamedLocation(const TStringBuf slotValue, const TStringBuf slotType);

    /** Check, whether slot contains any special location
     */
    static bool IsSpecialLocation(const TStringBuf slotValue, const TStringBuf slotType) {
        return GetNamedLocation(slotValue, slotType) != TSpecialLocation::EType::ERROR;
    }

    /** Check, whether slot contains special value for "near" location
     */
    static bool IsNearLocation(const TStringBuf slotValue, const TStringBuf slotType) {
        return GetNamedLocation(slotValue, slotType).IsNearLocation();
    }

    /** Check, whether slot contains special value for "home" or "work" location
     */
    static bool IsUserAddress(const TStringBuf slotValue, const TStringBuf slotType) {
        return GetNamedLocation(slotValue, slotType).IsUserAddress();
    }
};

}
