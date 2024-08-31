#pragma once

#include "defs.h"
#include "keys.h"
#include "utils.h"

#include <alice/bass/libs/video_content/common.h>

#include <alice/bass/libs/ydb_helpers/path.h>

#include <ydb/public/sdk/cpp/client/ydb_driver/driver.h>

#include <util/generic/hash.h>
#include <util/generic/noncopyable.h>
#include <util/generic/singleton.h>
#include <util/system/rwlock.h>
#include <util/system/types.h>
#include <util/system/yassert.h>
#include <util/thread/singleton.h>

#include <cstddef>
#include <functional>
#include <utility>

namespace NYdb::NTable {
class TSession;
class TTableClient;
} // namespace NYdb::NTable

namespace NVideoCommon {

inline constexpr size_t MAX_YDB_RESULT_SIZE = 1000;

struct TIndexTablesPaths {
    TIndexTablesPaths() = default;
    TIndexTablesPaths(const NYdbHelpers::TPath& database, const NYdbHelpers::TPath& piids,
                      const NYdbHelpers::TPath& hrids, const NYdbHelpers::TPath& kpids)
        : Database(database)
        , Piids(piids)
        , Hrids(hrids)
        , Kpids(kpids) {
    }

    static TIndexTablesPaths MakeDefault(const NYdbHelpers::TPath& database) {
        return TIndexTablesPaths{database, TString{NVideoContent::TProviderItemIdIndexTableTraits::NAME},
                                 TString{NVideoContent::THumanReadableIdIndexTableTraits::NAME},
                                 TString{NVideoContent::TKinopoiskIdIndexTableTraits::NAME}};
    }

    static TIndexTablesPaths MakeDefault(const NYdbHelpers::TTablePath& snapshot) {
        const auto& database = snapshot.Database;
        const auto& name = snapshot.Name;
        return TIndexTablesPaths{database,
                                 NYdbHelpers::Join(name, NVideoContent::TProviderItemIdIndexTableTraits::NAME),
                                 NYdbHelpers::Join(name, NVideoContent::THumanReadableIdIndexTableTraits::NAME),
                                 NYdbHelpers::Join(name, NVideoContent::TKinopoiskIdIndexTableTraits::NAME)};
    }

    NYdbHelpers::TTablePath PiidsPath() const {
        return {Database, Piids};
    }

    NYdbHelpers::TTablePath HridsPath() const {
        return {Database, Hrids};
    }

    NYdbHelpers::TTablePath KpidsPath() const {
        return {Database, Kpids};
    }

    template <typename TFn>
    void ForEachPath(TFn&& fn) const {
        for (const auto& path : {PiidsPath(), HridsPath(), KpidsPath()})
            fn(path);
    }

    NYdbHelpers::TPath Database;

    NYdbHelpers::TPath Piids;
    NYdbHelpers::TPath Hrids;
    NYdbHelpers::TPath Kpids;
};

struct TVideoTablesPaths {
    TVideoTablesPaths() = default;
    TVideoTablesPaths(const NYdbHelpers::TPath& database, const NYdbHelpers::TPath& items,
                      const NYdbHelpers::TPath& serials, const NYdbHelpers::TPath& seasons,
                      const NYdbHelpers::TPath& episodes, const NYdbHelpers::TPath& providerUniqueItems)
        : Database(database)
        , Items(items)
        , Serials(serials)
        , Seasons(seasons)
        , Episodes(episodes)
        , ProviderUniqueItems(providerUniqueItems)
    {
    }

    static TVideoTablesPaths MakeDefault(const NYdbHelpers::TPath& database) {
        return TVideoTablesPaths{database, TString{NVideoContent::TVideoItemsLatestTableTraits::NAME},
                                 TString{NVideoContent::TVideoSerialsTableTraits::NAME},
                                 TString{NVideoContent::TVideoSeasonsTableTraits::NAME},
                                 TString{NVideoContent::TVideoEpisodesTableTraits::NAME},
                                 TString{NVideoContent::TProviderUniqueItemsTableTraitsV2::NAME}};
    }

    static TVideoTablesPaths MakeDefault(const NYdbHelpers::TTablePath& snapshot) {
        const auto& database = snapshot.Database;
        const auto& name = snapshot.Name;
        return TVideoTablesPaths{
            database, NYdbHelpers::Join(name, TString{NVideoContent::TVideoItemsLatestTableTraits::NAME}),
            NYdbHelpers::Join(name, TString{NVideoContent::TVideoSerialsTableTraits::NAME}),
            NYdbHelpers::Join(name, TString{NVideoContent::TVideoSeasonsTableTraits::NAME}),
            NYdbHelpers::Join(name, TString{NVideoContent::TVideoEpisodesTableTraits::NAME}),
            NYdbHelpers::Join(name, TString{NVideoContent::TProviderUniqueItemsTableTraitsV2::NAME})};
    }

    NYdbHelpers::TTablePath ItemsPath() const {
        return {Database, Items};
    }

    NYdbHelpers::TTablePath SerialsPath() const {
        return {Database, Serials};
    }

    NYdbHelpers::TTablePath SeasonsPath() const {
        return {Database, Seasons};
    }

    NYdbHelpers::TTablePath EpisodesPath() const {
        return {Database, Episodes};
    }

    NYdbHelpers::TTablePath ProviderUniqueItemsPath() const {
        return {Database, ProviderUniqueItems};
    }

    template <typename TFn>
    void ForEachPath(TFn&& fn) const {
        for (const auto& path : {ItemsPath(), SerialsPath(), SeasonsPath(), EpisodesPath(), ProviderUniqueItemsPath()})
            fn(path);
    }

    NYdbHelpers::TPath Database;

    NYdbHelpers::TPath Items;
    NYdbHelpers::TPath Serials;
    NYdbHelpers::TPath Seasons;
    NYdbHelpers::TPath Episodes;
    NYdbHelpers::TPath ProviderUniqueItems;
};

class TYdbContentDb final {
public:
    enum class EStatus {
        SUCCESS /* "success" */,

        BROKEN_DATA /* "broken_data" */,
        NOT_FOUND /* "not_found" */,
        YDB_ERROR /* "ydb_error" */,

        IS_VOID /* "is_void" */
    };

    struct TStatus {
        TStatus(EStatus status, NYdb::TStatus ydbStatus)
            : Status(status)
            , YdbStatus(ydbStatus) {
            if (Status == EStatus::SUCCESS)
                Y_ASSERT(YdbStatus.IsSuccess());
        }

        bool IsSuccess() const {
            return Status == EStatus::SUCCESS;
        }

        bool IsVoid() const {
            return Status == EStatus::IS_VOID;
        }

        EStatus Status;
        NYdb::TStatus YdbStatus;
    };

    struct TIdStatus final : public NYdb::TStatus {
        TIdStatus(NYdb::TStatus&& status, const TMaybe<ui64>& id)
            : NYdb::TStatus(std::move(status))
            , Id(id) {
        }

        TMaybe<ui64> Id;
    };

    TYdbContentDb(NYdb::NTable::TTableClient& client, const TVideoTablesPaths& videoTables,
                  const TIndexTablesPaths& indexTables);

    TStatus FindVideoItem(const TVideoKey& key, TVideoItem& item) const;
    THashMap<TVideoKey, std::pair<bool /* isVoid */, TVideoItem>> FindVideoItems(const THashSet<TVideoKey>& keys) const;
    THashMap<TString, TVector<TVideoItem>> FindVideoItemsByKinopoiskIds(const THashSet<TString>& kpids) const;
    TStatus FindSerialDescriptor(const TSerialKey& key, TSerialDescriptor& serial) const;
    TStatus FindSeasonDescriptor(const TSeasonKey& key, TSeasonDescriptor& season) const;
    TStatus FindEpisodeNumber(const TString& providerItemId, ui64& season, ui64& episode) const;
    THashMap<TSerialKey, TSeasonDescriptor> FindInitSeasonsBySerialKeys(const THashSet<TSerialKey>& keys) const;
    TIdStatus FindId(const TVideoKey& key) const;
    TStatus FindProviderUniqueVideoItem(const TVideoKey& key, TVideoItem& item) const;

private:
    TIdStatus FindIdImpl(NYdb::NTable::TSession& session, const TVideoKey& key) const;
    NYdb::TStatus FindIdsByKinopoiskIdsImpl(NYdb::NTable::TSession& session,
                                            const TVector<TString>& kinopoiskIds, TVector<ui64>& ids) const;
    TStatus FindVideoItemsByKinopoiskIdsImpl(const TVector<TString>& kinopoiskIds,
                                             THashMap<TString, TVector<TVideoItem>>& items) const;
    TYdbContentDb::TStatus
    FindInitSeasonsBySerialKeysImpl(const THashSet<TSerialKey>& serialKeys,
                                    THashMap<TSerialKey, TSeasonDescriptor>& resultSeasons) const;
    // The keys must not be empty or exceed MAX_YDB_RESULT_SIZE.
    NYdb::TStatus FindIds(NYdb::NTable::TSession& session, const THashSet<TVideoKey>& keys,
                          THashMap<TVideoKey, ui64>& ids) const;
    // The keys must not be empty or exceed MAX_YDB_RESULT_SIZE.
    // The keys must have the same IdType.
    TStatus FindVideoItemsImpl(const THashSet<TVideoKey>& keys,
                               TVector<std::pair<bool /* isVoid */, TVideoItem>>& items) const;

    NYdb::TStatus RetryOperationSync(const NYdb::NTable::TTableClient::TOperationSyncFunc& operation) const;

private:
    NYdb::NTable::TTableClient& Client;

    TString FindIdByPiid;
    TString FindIdsByPiidKeys;
    TString FindIdByHrid;
    TString FindIdsByHridKeys;
    TString FindIdByKpid;
    TString FindIdsByKpid;
    TString FindIdsByKpids;
    TString FindIdsByKpidKeys;

    TString FindItem;
    TString FindSerial;
    TString FindSeason;
    TString FindEpisode;
    TString FindInitSeasonsByKeys;
    TString FindItemsByIds;
    TString FindProviderUniqueItem;
};

struct TAllSeasonsDownloadItem {
    TAllSeasonsDownloadItem() = default;

    TAllSeasonsDownloadItem(const TString& provider, const TSerialDescriptor& serial)
        : Provider(provider)
        , Serial(serial) {
    }

    TString Provider;
    TSerialDescriptor Serial;
};

struct TSeasonDownloadItem {
    TSeasonDownloadItem() = default;

    TSeasonDownloadItem(const TString& provider, TIntrusivePtr<TSerialDescriptor> serial,
                        const TSeasonDescriptor& season)
        : Provider(provider)
        , Serial(std::move(serial))
        , Season(season) {
        Y_ASSERT(Serial);
    }

    TString Provider;
    TIntrusivePtr<TSerialDescriptor> Serial;
    TSeasonDescriptor Season;
};

void DownloadItems(const TVector<TVideoItem>& items, IContentInfoProvidersCache& providers,
                   std::function<void(size_t index, const TVideoItem& item)> onSuccess,
                   std::function<void(size_t index, const TError& error)> onError);

void DownloadSerials(
    const TVector<TVideoItem>& items, IContentInfoProvidersCache& providers,
    std::function<void(size_t index, const TSerialKey& key, const TSerialDescriptor& serial)> onSuccess,
    std::function<void(size_t index, const TError& error)> onError);

void DownloadAllSeasons(
    const TVector<TAllSeasonsDownloadItem>& items, IContentInfoProvidersCache& providers,
    std::function<void(size_t index, const TSeasonKey& key, const TSeasonDescriptor& season)> onSuccess,
    std::function<void(size_t index, const TError& error)> onError);

void DownloadSeasons(
    const TVector<TSeasonDownloadItem>& items, IContentInfoProvidersCache& providers,
    std::function<void(size_t index, const TSeasonKey& key, const TSeasonDescriptor& season)> onSuccess,
    std::function<void(size_t index, const TError& error)> onError);

bool IsProviderSupportedByVideoItemsTable(TStringBuf name);
bool IsTypeSupportedByVideoItemsTable(EItemType type);
bool IsTypeSupportedByVideoItemsTable(TStringBuf type);

} // namespace NVideoCommon
