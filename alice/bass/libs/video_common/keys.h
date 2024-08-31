#pragma once

#include "defs.h"

#include <util/generic/hash.h>
#include <util/generic/maybe.h>
#include <util/generic/strbuf.h>
#include <util/system/types.h>

namespace NVideoCommon {

namespace NImpl {
template <typename TValue>
size_t Hash(const TValue& value) {
    return THash<TValue>()(value);
}

template <typename TValue, typename... TValues>
size_t Hash(const TValue& value, const TValues&... values) {
    return CombineHashes(Hash(value), Hash(values...));
}
} // namespace NImpl

struct TVideoKey final {
    enum class EIdType { ID /* "id" */, HRID /* "hrid" */, KINOPOISK_ID /* "kinopoisk_id" */ };

    TVideoKey() = default;

    TVideoKey(TStringBuf providerName, TStringBuf id, EIdType idType, const TMaybe<TString>& type)
        : ProviderName(providerName)
        , Id(id)
        , IdType(idType)
        , Type(type) {
    }

    static TMaybe<TVideoKey> TryFromProviderItemId(TStringBuf providerName, TVideoItemConstScheme item);
    static TMaybe<TVideoKey> TryFromHumanReadableId(TStringBuf providerName, TVideoItemConstScheme item);
    static TMaybe<TVideoKey> TryFromKinopoiskId(TStringBuf providerName, TVideoItemConstScheme item);

    static TMaybe<TVideoKey> TryAny(TStringBuf providerName, TVideoItemConstScheme item);

    bool operator<(const TVideoKey& rhs) const {
        if (ProviderName != rhs.ProviderName)
            return ProviderName < rhs.ProviderName;
        if (Id != rhs.Id)
            return Id < rhs.Id;
        if (IdType != rhs.IdType)
            return IdType < rhs.IdType;
        if (!Type.Defined() && rhs.Type.Defined())
            return true;
        if (Type.Defined() && rhs.Type.Defined())
            return *Type < *rhs.Type;
        return false;
    }

    bool operator==(const TVideoKey& rhs) const {
        return ProviderName == rhs.ProviderName && Id == rhs.Id && IdType == rhs.IdType && Type == rhs.Type;
    }

    size_t Hash() const {
        return NImpl::Hash(ProviderName, Id, static_cast<size_t>(IdType), Type.GetOrElse(TString{}));
    }

    TString ProviderName;
    TString Id;
    EIdType IdType = EIdType::ID;
    TMaybe<TString> Type;
};

struct TSerialKey final {
    TSerialKey() = default;

    TSerialKey(TStringBuf providerName, TStringBuf serialId)
        : ProviderName(providerName)
        , SerialId(serialId) {
    }

    bool operator<(const TSerialKey& rhs) const {
        if (ProviderName != rhs.ProviderName)
            return ProviderName < rhs.ProviderName;
        return SerialId < rhs.SerialId;
    }

    bool operator==(const TSerialKey& rhs) const {
        return ProviderName == rhs.ProviderName && SerialId == rhs.SerialId;
    }

    size_t Hash() const {
        return NImpl::Hash(ProviderName, SerialId);
    }

    TString ProviderName;
    TString SerialId;
};

struct TSeasonKey final {
    TSeasonKey() = default;

    TSeasonKey(TStringBuf providerName, TStringBuf serialId, ui64 providerNumber)
        : ProviderName(providerName)
        , SerialId(serialId)
        , ProviderNumber(providerNumber) {
    }

    bool operator<(const TSeasonKey& rhs) const {
        if (ProviderName != rhs.ProviderName)
            return ProviderName < rhs.ProviderName;
        if (SerialId != rhs.SerialId)
            return SerialId < rhs.SerialId;
        return ProviderNumber < rhs.ProviderNumber;
    }

    bool operator==(const TSeasonKey& rhs) const {
        return ProviderName == rhs.ProviderName && SerialId == rhs.SerialId && ProviderNumber == rhs.ProviderNumber;
    }

    size_t Hash() const {
        return NImpl::Hash(ProviderName, SerialId, ProviderNumber);
    }

    TString ProviderName;
    TString SerialId;
    ui64 ProviderNumber = 0;
};

} // namespace NVideoCommon

template <>
struct THash<NVideoCommon::TVideoKey> {
    size_t operator()(const NVideoCommon::TVideoKey& k) const {
        return k.Hash();
    }
};

template <>
struct THash<NVideoCommon::TSerialKey> {
    size_t operator()(const NVideoCommon::TSerialKey& k) const {
        return k.Hash();
    }
};

template <>
struct THash<NVideoCommon::TSeasonKey> {
    size_t operator()(const NVideoCommon::TSeasonKey& k) const {
        return k.Hash();
    }
};
