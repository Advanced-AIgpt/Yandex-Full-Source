#pragma once

#include <alice/bass/libs/video_content/protos/rows.pb.h>
#include <alice/bass/libs/ydb_helpers/path.h>

#include <util/generic/strbuf.h>
#include <util/generic/string.h>
#include <util/generic/vector.h>

#include <cstddef>

namespace NYdb {
namespace NScheme {
class TSchemeClient;
} // namespace NScheme

namespace NTable {
class TTableClient;
} // namespace NTable
} // namespace NYdb

namespace NVideoContent {
inline constexpr TStringBuf YT_PROXY = "arnold";
inline constexpr TStringBuf VIDEO_TABLE_ITEMS_YT = "video_items";
inline constexpr TStringBuf VIDEO_TABLES_PREFIX = "video";
inline const TVector<TString> VIDEO_TABLE_KINOPOISK_PRIMARY_KEYS{"KinopoiskId"};

TString GetYDBToken();

// Sorts tables with names starting with |prefix| lexicographically,
// except |latest|, and keeps at most top |keep| among them.
bool DropOldTables(NYdb::NScheme::TSchemeClient& schemeClient, NYdb::NTable::TTableClient& tableClient,
                   const NYdbHelpers::TPath& database, TStringBuf prefix, TStringBuf latest, size_t numKeep);

// Sorts directories with names starting with |prefix|
// lexicographically, except |latest|, and keeps at most top |keep|
// among them.
bool DropOldDirectoriesWithTables(NYdb::NScheme::TSchemeClient& schemeClient, NYdb::NTable::TTableClient& tableClient,
                                  const NYdbHelpers::TPath& database, TStringBuf prefix, TStringBuf latest,
                                  size_t keep);

struct TVideoKeysTableTraits {
    using TScheme = NProtos::TVideoKeyRow;
    using TYTScheme = TScheme;

    static inline constexpr TStringBuf NAME = "video_keys";
    static inline constexpr TStringBuf YT_NAME = NAME;
    static inline const TVector<TString> PRIMARY_KEYS{"ProviderName", "ProviderItemId", "HumanReadableId"};
};

using TLatestVideoItemsYTScheme = NProtos::TVideoItemRowV5YT;

struct TVideoItemsV5TableTraits {
    using TScheme = NProtos::TVideoItemRowV5YDb;
    using TYTScheme = TLatestVideoItemsYTScheme;

    static inline constexpr TStringBuf NAME = "video_items_v5";
    static inline constexpr TStringBuf YT_NAME = VIDEO_TABLE_ITEMS_YT;
    static inline const TVector<TString> PRIMARY_KEYS{"Id"};
};

using TVideoItemsLatestTableTraits = TVideoItemsV5TableTraits;

struct TProviderItemIdIndexTableTraits {
    using TScheme = NProtos::TProviderItemIdIndexRow;

    static inline constexpr TStringBuf NAME = "piid_index";
    static inline const TVector<TString> PRIMARY_KEYS{"ProviderName", "ProviderItemId", "Type", "Id"};
};

struct THumanReadableIdIndexTableTraits {
    using TScheme = NProtos::THumanReadableIdIndexRow;

    static inline constexpr TStringBuf NAME = "hrid_index";
    static inline const TVector<TString> PRIMARY_KEYS{"ProviderName", "HumanReadableId", "Type", "Id"};
};

struct TKinopoiskIdIndexTableTraits {
    using TScheme = NProtos::TKinopoiskIdIndexRow;

    static inline constexpr TStringBuf NAME = "kpid_index";
    static inline const TVector<TString> PRIMARY_KEYS{"KinopoiskId", "ProviderName", "Type", "Id"};
};

struct TVideoSerialsTableTraits {
    using TScheme = NProtos::TSerialDescriptorRow;
    using TYTScheme = TScheme;

    static inline constexpr TStringBuf NAME = "video_serials";
    static inline constexpr TStringBuf YT_NAME = NAME;
    static inline const TVector<TString> PRIMARY_KEYS{"ProviderName", "SerialId"};
};

struct TVideoSeasonsTableTraits {
    using TScheme = NProtos::TSeasonDescriptorRow;
    using TYTScheme = TScheme;

    static inline constexpr TStringBuf NAME = "video_seasons";
    static inline constexpr TStringBuf YT_NAME = NAME;
    static inline const TVector<TString> PRIMARY_KEYS{"ProviderName", "SerialId", "ProviderNumber"};
};

struct TVideoEpisodesTableTraits {
    using TScheme = NProtos::TEpisodeDescriptorRow;
    using TYTScheme = TScheme;

    static inline constexpr TStringBuf NAME = "video_episodes";
    static inline constexpr TStringBuf YT_NAME = NAME;
    static inline const TVector<TString> PRIMARY_KEYS{"ProviderItemId"};
};

struct TProviderUniqueItemsTableTraitsV2 {
    using TScheme = NProtos::TProviderUniqueVideoItemRow;
    using TYTScheme = TScheme;

    static inline constexpr TStringBuf NAME = "video_provider_unique_items_v2";
    static inline constexpr TStringBuf YT_NAME = "video_provider_unique_items";
    static inline const TVector<TString> PRIMARY_KEYS{"ProviderName", "ProviderItemId"};

};

} // namespace NVideoContent
