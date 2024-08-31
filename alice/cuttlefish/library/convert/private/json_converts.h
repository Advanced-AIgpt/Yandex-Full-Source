#pragma once
#include <type_traits>
#include <alice/cuttlefish/library/convert/private/traits.h>
#include <library/cpp/json/json_value.h>
#include <alice/cuttlefish/library/convert/rapid_node.h>
#include <util/string/cast.h>
#include <util/string/type.h>


namespace NAlice::NCuttlefish::NConvert::NPrivate {


template <typename T, typename = void>
struct TJsonConverts { };


template <>
struct TJsonConverts<TString> {
    static inline const TString& GetSafe(const NJson::TJsonValue& x) {
        return x.GetStringSafe();
    }
    static inline TString GetSoft(const NJson::TJsonValue& x) {
        return x.GetStringRobust();
    }

    static inline TString GetSafe(const TRapidJsonNode& x) {
        return ToString(x.GetValue());
    }
    static inline TString GetSoft(const TRapidJsonNode& x) {
        return ToString(x.GetValue());
    }
};


template <typename T>
struct TJsonConverts<T, IfSignedInt<T>> {
    static inline T GetSafe(const NJson::TJsonValue& x) {
        return x.GetIntegerSafe();
    }
    static inline T GetSoft(const NJson::TJsonValue& x) {
        if (x.IsString())
            return FromString<T>(x.GetString());
        return GetSafe(x);
    }

    static inline T GetSafe(const TRapidJsonNode& x) {
        Y_ENSURE(x.GetType() == T_NUMBER);
        return FromString<T>(x.GetValue());
    }
    static inline T GetSoft(const TRapidJsonNode& x) {
        return FromString<T>(x.GetValue());
    }
};


template <typename T>
struct TJsonConverts<T, IfUnsignedInt<T>> {
    static inline T GetSafe(const NJson::TJsonValue& x) {
        return x.GetUIntegerSafe();
    }
    static inline T GetSoft(const NJson::TJsonValue& x) {
        if (x.IsString())
            return FromString<T>(x.GetString());
        return GetSafe(x);
    }

    static inline T GetSafe(const TRapidJsonNode& x) {
        Y_ENSURE(x.GetType() == T_NUMBER);
        return FromString<T>(x.GetValue());
    }
    static inline T GetSoft(const TRapidJsonNode& x) {
        return FromString<T>(x.GetValue());
    }
};


template <typename T>
struct TJsonConverts<T, IfFloat<T>> {
    static inline T GetSafe(const NJson::TJsonValue& x) {
        return x.GetDoubleSafe();
    }

    static inline T GetSoft(const NJson::TJsonValue& x) {
        if (x.IsString())
            return FromString<T>(x.GetString());
        return GetSafe(x);
    }

    static inline T GetSafe(const TRapidJsonNode& x) {
        Y_ENSURE(x.GetType() == T_NUMBER);
        return FromString<T>(x.GetValue());
    }
    static inline T GetSoft(const TRapidJsonNode& x) {
        return FromString<T>(x.GetValue());
    }
};


template <>
struct TJsonConverts<bool> {
    static inline bool GetSafe(const NJson::TJsonValue& x) {
        return x.GetBooleanSafe();
    }
    static inline bool GetSoft(const NJson::TJsonValue& x) {
        if (x.IsString())
            return IsTrue(x.GetString());
        if (x.IsInteger())
            return x.GetInteger();
        return GetSafe(x);
    }
    static inline bool GetSafe(const TRapidJsonNode& x) {
        Y_ENSURE(x.GetType() == T_BOOLEAN);
        return x.GetBoolValue();
    }
    static inline bool GetSoft(const TRapidJsonNode& x) {
        switch (x.GetType()) {
            case T_STRING:
                return IsTrue(x.GetValue());
            case T_NUMBER:
                return FromString<double>(x.GetValue());
            default:
                return x.GetBoolValue();
        }
    }
};

}  // namespace NAlice::NCuttlefish::NConvert::NPrivate