#include "defs.h"

#include "keys.h"
#include "utils.h"

#include <util/stream/output.h>

namespace NVideoCommon {

TVector<TStringBuf> GetSupportedVideoProviders() {
    TVector<TStringBuf> supportedVideoProviders = {PROVIDER_AMEDIATEKA, PROVIDER_IVI, PROVIDER_KINOPOISK,
                                                   PROVIDER_YAVIDEO, PROVIDER_YAVIDEO_PROXY, PROVIDER_YOUTUBE};
    return supportedVideoProviders;
}

bool DoesProviderHaveUniqueIdsForItems(TStringBuf providerName) {
    return providerName == PROVIDER_KINOPOISK || providerName == PROVIDER_OKKO;
}

// TSeasonDescriptor -----------------------------------------------------------
void TSeasonDescriptor::Ser(NVideoContent::NProtos::TSeasonDescriptor& s) const {
    s.Clear();

    s.SetSerialId(SerialId);
    if (Id)
        s.SetId(*Id);
    s.SetEpisodesCount(EpisodesCount);
    for (const auto& id : EpisodeIds)
        s.AddEpisodeIds(id);
    for (const auto& item : EpisodeItems)
        s.AddEpisodeItems(item.Value().ToJson());
    s.SetIndex(Index);
    s.SetProviderNumber(ProviderNumber);
    s.SetSoon(Soon);

    if (DownloadedAt)
        s.SetDownloadedAtUS(DownloadedAt->MicroSeconds());
    if (UpdateAt)
        s.SetUpdateAtUS(UpdateAt->MicroSeconds());
}

bool TSeasonDescriptor::Des(const NVideoContent::NProtos::TSeasonDescriptor& s) {
    TSeasonDescriptor season;

    season.SerialId = s.GetSerialId();
    if (s.HasId())
        season.Id = s.GetId();
    season.EpisodesCount = s.GetEpisodesCount();
    for (const auto& id : s.GetEpisodeIds())
        season.EpisodeIds.push_back(id);
    for (const auto& json : s.GetEpisodeItems()) {
        NVideoCommon::TVideoItem item;
        if (!::NVideoCommon::Des(json, item))
            return false;
        season.EpisodeItems.push_back(item);
    }
    season.Index = s.GetIndex();
    season.ProviderNumber = s.GetProviderNumber();
    season.Soon = s.GetSoon();

    if (s.HasDownloadedAtUS())
        season.DownloadedAt = TInstant::MicroSeconds(s.GetDownloadedAtUS());
    if (s.HasUpdateAtUS())
        season.UpdateAt = TInstant::MicroSeconds(s.GetUpdateAtUS());

    *this = std::move(season);
    return true;
}

bool TSeasonDescriptor::Ser(const TSeasonKey& key, NVideoContent::NProtos::TSeasonDescriptorRow& row) const {
    TString data;
    {
        NVideoContent::NProtos::TSeasonDescriptor s;
        Ser(s);

        TStringOutput os(data);
        if (!s.SerializeToArcadiaStream(&os))
            return false;
    }

    row.SetProviderName(key.ProviderName);
    row.SetSerialId(key.SerialId);
    row.SetProviderNumber(key.ProviderNumber);
    row.SetSeason(data);

    if (DownloadedAt)
        row.SetDownloadedAtUS(DownloadedAt->MicroSeconds());
    if (UpdateAt)
        row.SetUpdateAtUS(UpdateAt->MicroSeconds());

    return true;
}

// TSerialDescriptor -----------------------------------------------------------
void TSerialDescriptor::Ser(NVideoContent::NProtos::TSerialDescriptor& s) const {
    s.Clear();

    s.SetId(Id);
    for (const auto& season : Seasons)
        season.Ser(*s.AddSeasons());
    s.SetTotalEpisodesCount(TotalEpisodesCount);
    if (MinAge)
        s.SetMinAge(*MinAge);
}

bool TSerialDescriptor::Des(const NVideoContent::NProtos::TSerialDescriptor& s) {
    TSerialDescriptor serial;

    serial.Id = s.GetId();
    for (const auto& season : s.GetSeasons()) {
        NVideoCommon::TSeasonDescriptor seasonDescr;
        if (!seasonDescr.Des(season))
            return false;
        serial.Seasons.push_back(std::move(seasonDescr));
    }

    Sort(serial.Seasons, [](const TSeasonDescriptor& lhs, const TSeasonDescriptor& rhs) {
        return lhs.ProviderNumber < rhs.ProviderNumber;
    });

    serial.TotalEpisodesCount = s.GetTotalEpisodesCount();
    if (s.HasMinAge())
        serial.MinAge = s.GetMinAge();

    *this = std::move(serial);
    return true;
}

bool TSerialDescriptor::Ser(const TSerialKey& key, NVideoContent::NProtos::TSerialDescriptorRow& row) const {
    TString data;
    {
        NVideoContent::NProtos::TSerialDescriptor s;
        Ser(s);

        TStringOutput os(data);
        if (!s.SerializeToArcadiaStream(&os))
            return false;
    }

    row.SetProviderName(key.ProviderName);
    row.SetSerialId(key.SerialId);
    row.SetSerial(data);

    return true;
}
} // namespace NVideoCommon

namespace {
template <typename T>
void OutVector(IOutputStream& os, const TVector<T>& vs) {
    os << "TVector [";
    os << vs.size() << ":";
    for (const auto& v : vs)
        os << " " << v;
    os << "]";
}
} // namespace

template <>
void Out<NVideoCommon::TVideoItem>(IOutputStream& os, const NVideoCommon::TVideoItem& item) {
    os << "TVideoItem [" << item.Value().ToJson() << "]";
}

template <>
void Out<NVideoCommon::TSeasonDescriptor>(IOutputStream& os, const NVideoCommon::TSeasonDescriptor& season) {
    os << "TSeasonDescriptor [";
    os << season.SerialId << ", ";
    os << season.Id << ", ";
    os << season.EpisodesCount << ", ";

    OutVector(os, season.EpisodeIds);
    os << ", ";

    OutVector(os, season.EpisodeItems);
    os << ", ";

    os << season.Index << ", ";
    os << season.ProviderNumber << ", ";
    os << season.Soon;

    if (season.DownloadedAt) {
        os << ", ";
        os << "downloaded at: " << season.DownloadedAt->ToString();
    }

    if (season.UpdateAt) {
        os << ", ";
        os << "update at: " << season.UpdateAt->ToString();
    }

    os << "]";
}

template <>
void Out<NVideoCommon::TSerialDescriptor>(IOutputStream& os, const NVideoCommon::TSerialDescriptor& serial) {
    os << "TSerialDescriptor [";
    os << serial.Id << ", ";

    OutVector(os, serial.Seasons);
    os << ", ";

    os << serial.TotalEpisodesCount << ", ";
    os << serial.MinAge << "]";
}
