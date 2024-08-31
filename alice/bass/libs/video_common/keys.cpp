#include "keys.h"

#include <util/stream/output.h>

namespace NVideoCommon {
// TVideoKey -------------------------------------------------------------------
// static
TMaybe<TVideoKey> TVideoKey::TryFromProviderItemId(TStringBuf providerName, TVideoItemConstScheme item) {
    if (!item.HasProviderItemId() || item.ProviderItemId()->empty())
        return Nothing();
    TMaybe<TString> type;
    if (item.HasType())
        type = item.Type();
    return TVideoKey(providerName, item.ProviderItemId(), TVideoKey::EIdType::ID, type);
}

// static
TMaybe<TVideoKey> TVideoKey::TryFromHumanReadableId(TStringBuf providerName, TVideoItemConstScheme item) {
    if (!item.HasHumanReadableId() || item.HumanReadableId()->empty())
        return Nothing();
    TMaybe<TString> type;
    if (item.HasType())
        type = item.Type();
    return TVideoKey(providerName, item.HumanReadableId(), TVideoKey::EIdType::HRID, type);
}

// static
TMaybe<TVideoKey> TVideoKey::TryFromKinopoiskId(TStringBuf providerName, TVideoItemConstScheme item) {
    if (!item.HasMiscIds() || !item.MiscIds().HasKinopoisk() || item.MiscIds().Kinopoisk()->empty())
        return Nothing();
    TMaybe<TString> type;
    if (item.HasType())
        type = item.Type();
    return TVideoKey(providerName, item.MiscIds().Kinopoisk(), TVideoKey::EIdType::KINOPOISK_ID, type);
}

// static
TMaybe<TVideoKey> TVideoKey::TryAny(TStringBuf providerName, TVideoItemConstScheme item) {
    if (const auto key = TryFromProviderItemId(providerName, item))
        return key;
    if (const auto key = TryFromHumanReadableId(providerName, item))
        return key;
    if (const auto key = TryFromKinopoiskId(providerName, item))
        return key;
    return Nothing();
}

} // namespace NVideoCommon

template <>
void Out<NVideoCommon::TVideoKey>(IOutputStream& out, const NVideoCommon::TVideoKey& key) {
    out << "TVideoKey [" << key.ProviderName << ", " << key.Id << ", " << key.IdType << ", " << key.Type << "]";
}

template <>
void Out<NVideoCommon::TSerialKey>(IOutputStream& out, const NVideoCommon::TSerialKey& key) {
    out << "TSerialKey [" << key.ProviderName << ", " << key.SerialId << "]";
}

template <>
void Out<NVideoCommon::TSeasonKey>(IOutputStream& out, const NVideoCommon::TSeasonKey& key) {
    out << "TSeasonKey [" << key.ProviderName << ", " << key.SerialId << ", " << key.ProviderNumber << "]";
}
