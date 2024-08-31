#include "content_db.h"

#include "utils.h"

#include <alice/bass/libs/ydb_helpers/settings.h>
#include <alice/bass/libs/ydb_helpers/exception.h>
#include <alice/bass/libs/ydb_helpers/queries.h>
#include <alice/bass/libs/ydb_helpers/table.h>
#include <alice/bass/libs/video_common/kinopoisk_utils.h>
#include <alice/bass/libs/video_common/utils.h>

#include <alice/bass/libs/logging_v2/logger.h>

#include <ydb/public/sdk/cpp/client/ydb_table/table.h>

#include <util/string/printf.h>
#include <util/system/yassert.h>

#include <algorithm>
#include <utility>

namespace NVideoCommon {

namespace {
template <typename TFn, typename TItem, typename... TArgs>
void BatchedUpdate(size_t batchSize, const TFn& fn, TVector<TItem>& items, TArgs&&... args) {
    const size_t n = Min(items.size(), batchSize);
    const size_t from = items.size() - n;

    TVector<TItem> batch{Reserve(n)};
    std::move(items.begin() + from, items.end(), std::back_inserter(batch));
    items.erase(items.begin() + from, items.end());

    fn(batch, std::forward<TArgs>(args)...);
}

} // namespace

// TYdbContentDb ---------------------------------------------------------------
TYdbContentDb::TYdbContentDb(NYdb::NTable::TTableClient& client, const TVideoTablesPaths& videoTables,
                             const TIndexTablesPaths& indexTables)
    : Client(client) {
    FindIdByPiid = Sprintf(R"(
                           PRAGMA TablePathPrefix("%s");

                           DECLARE $providerName AS String;
                           DECLARE $id AS String;
                           DECLARE $type AS "String?";

                           SELECT Id FROM [%s] WHERE ProviderName == $providerName AND
                                                     ProviderItemId == $id AND
                                                     ($type IS NULL OR Type == $type);
                           )",
                           indexTables.Database.c_str(), indexTables.Piids.c_str());
    FindIdsByPiidKeys = Sprintf(R"(
                                PRAGMA TablePathPrefix("%s");

                                DECLARE $Ids AS "List<Struct<ProviderItemId:String,ProviderName:String,Type:String?>>";

                                SELECT piids.Id, ps.ProviderItemId, ps.ProviderName, ps.Type FROM AS_TABLE($Ids) AS ps
                                INNER JOIN [%s] AS piids
                                ON ps.ProviderName == piids.ProviderName AND
                                   ps.ProviderItemId == piids.ProviderItemId
                                WHERE ps.Type IS NULL OR ps.Type == piids.Type
                                )",
                                indexTables.Database.c_str(), indexTables.Piids.c_str());
    FindIdByHrid = Sprintf(R"(
                           PRAGMA TablePathPrefix("%s");

                           DECLARE $providerName AS String;
                           DECLARE $id AS String;
                           DECLARE $type AS "String?";

                           SELECT Id FROM [%s] WHERE ProviderName == $providerName AND
                                                     HumanReadableId == $id AND
                                                     ($type IS NULL OR Type == $type);
                           )",
                           indexTables.Database.c_str(), indexTables.Hrids.c_str());
    FindIdsByHridKeys = Sprintf(R"(
                                PRAGMA TablePathPrefix("%s");

                                DECLARE $Ids AS "List<Struct<HumanReadableId:String,ProviderName:String,Type:String?>>";

                                SELECT hrids.Id, hs.HumanReadableId, hs.ProviderName, hs.Type FROM AS_TABLE($Ids) AS hs
                                INNER JOIN [%s] AS hrids
                                ON hs.ProviderName == hrids.ProviderName AND
                                   hs.HumanReadableId == hrids.HumanReadableId
                                WHERE hs.Type IS NULL OR hs.Type == hrids.Type
                                )",
                                indexTables.Database.c_str(), indexTables.Hrids.c_str());
    FindIdByKpid = Sprintf(R"(
                           PRAGMA TablePathPrefix("%s");

                           DECLARE $providerName AS String;
                           DECLARE $id AS String;
                           DECLARE $type AS "String?";

                           SELECT Id FROM [%s] WHERE ProviderName == $providerName AND
                                                     KinopoiskId == $id AND
                                                     ($type IS NULL OR Type == $type);
                           )",
                           indexTables.Database.c_str(), indexTables.Kpids.c_str());
    FindIdsByKpid = Sprintf(R"(
                            PRAGMA TablePathPrefix("%s");

                            DECLARE $id AS String;

                            SELECT Id FROM [%s] WHERE KinopoiskId == $id
                            )",
                            indexTables.Database.c_str(), indexTables.Kpids.c_str());
    // YDB needs constant list to be on the left side of the INNER JOIN operation!
    FindIdsByKpids = Sprintf(R"(
                             PRAGMA TablePathPrefix("%s");

                             DECLARE $kinopoiskIds AS "List<Struct<KinopoiskId:String>>";

                             SELECT kpids.Id FROM AS_TABLE($kinopoiskIds) AS ks
                             INNER JOIN [%s] AS kpids
                             ON ks.KinopoiskId == kpids.KinopoiskId;
                             )",
                             indexTables.Database.c_str(), indexTables.Kpids.c_str());
    FindIdsByKpidKeys = Sprintf(R"(
                                PRAGMA TablePathPrefix("%s");

                                DECLARE $Ids AS "List<Struct<KinopoiskId:String,ProviderName:String,Type:String?>>";

                                SELECT kpids.Id, ks.KinopoiskId, ks.ProviderName, ks.Type FROM AS_TABLE($Ids) AS ks
                                INNER JOIN [%s] AS kpids
                                ON ks.ProviderName == kpids.ProviderName AND
                                   ks.KinopoiskId == kpids.KinopoiskId
                                WHERE ks.Type IS NULL OR ks.Type == kpids.Type
                                )",
                                indexTables.Database.c_str(), indexTables.Kpids.c_str());
    FindItem = Sprintf(R"(
                       PRAGMA TablePathPrefix("%s");

                       DECLARE $id AS Uint64;

                       SELECT Content, IsVoid FROM [%s] WHERE Id == $id;
                       )",
                       videoTables.Database.c_str(), videoTables.Items.c_str());
    FindSerial = Sprintf(R"(
                         PRAGMA TablePathPrefix("%s");

                         DECLARE $providerName AS String;
                         DECLARE $serialId AS String;

                         SELECT Serial FROM [%s] WHERE ProviderName == $providerName AND
                                                       SerialId == $serialId;
                         )",
                         videoTables.Database.c_str(), videoTables.Serials.c_str());
    FindInitSeasonsByKeys = Sprintf(R"(
                                    PRAGMA TablePathPrefix("%s");

                                    DECLARE $Keys AS
                                        "List<Struct<ProviderName:String,SerialId:String>>";

                                    SELECT seasonsTable.ProviderName,
                                           seasonsTable.SerialId,
                                           seasonsTable.Season,
                                           seasonsTable.ProviderNumber
                                    FROM
                                        (
                                            SELECT ks.ProviderName as ProviderName,
                                                   ks.SerialId as SerialId,
                                                   seasonsTable.ProviderNumber as ProviderNumber
                                            from AS_TABLE($Keys) AS ks
                                            INNER JOIN
                                            (
                                                SELECT seasonsTable.ProviderName,
                                                       seasonsTable.SerialId,
                                                       min(seasonsTable.ProviderNumber) AS ProviderNumber
                                                from [%s] AS seasonsTable
                                                GROUP BY seasonsTable.ProviderName, seasonsTable.SerialId
                                            ) AS seasonsTable
                                            ON seasonsTable.ProviderName == ks.ProviderName AND
                                               seasonsTable.SerialId == ks.SerialId
                                        ) as fs
                                    INNER JOIN [%s] AS seasonsTable
                                    ON seasonsTable.ProviderName == fs.ProviderName AND
                                       seasonsTable.SerialId == fs.SerialId AND
                                       seasonsTable.ProviderNumber == fs.ProviderNumber
                                    )",
                                    videoTables.Database.c_str(),
                                    videoTables.Seasons.c_str(),
                                    videoTables.Seasons.c_str());
    FindSeason = Sprintf(R"(
                         PRAGMA TablePathPrefix("%s");

                         DECLARE $providerName AS String;
                         DECLARE $serialId AS String;
                         DECLARE $providerNumber AS Uint64;

                         SELECT Season FROM [%s] WHERE ProviderName == $providerName AND
                                                       SerialId == $serialId AND
                                                       ProviderNumber == $providerNumber;
                         )",
                         videoTables.Database.c_str(), videoTables.Seasons.c_str());
    FindEpisode = Sprintf(R"(
                         PRAGMA TablePathPrefix("%s");

                         DECLARE $providerItemId AS String;

                         SELECT SeasonNumber, EpisodeNumber FROM [%s] WHERE ProviderItemId == $providerItemId;
                         )",
                         videoTables.Database.c_str(), videoTables.Episodes.c_str());
    // YDB needs constant list to be on the left side of the INNER JOIN operation!
    FindItemsByIds = Sprintf(R"(
                             PRAGMA TablePathPrefix("%s");

                             DECLARE $ids AS "List<Struct<Id:Uint64>>";

                             SELECT ts.Content, ts.IsVoid FROM AS_TABLE($ids) AS idsTable
                             INNER JOIN [%s] AS ts
                             ON ts.Id == idsTable.Id;
                             )",
                             videoTables.Database.c_str(), videoTables.Items.c_str());
    FindProviderUniqueItem = Sprintf(R"(
                                     PRAGMA TablePathPrefix("%s");

                                     DECLARE $providerName As String;
                                     DECLARE $providerItemId AS String;

                                     SELECT Content FROM [%s]
                                     WHERE ProviderName == $providerName AND ProviderItemId == $providerItemId;
                                     )",
                                     videoTables.Database.c_str(), videoTables.ProviderUniqueItems.c_str());
}

NYdb::TStatus
TYdbContentDb::RetryOperationSync(const NYdb::NTable::TTableClient::TOperationSyncFunc& operation) const {
    return NYdbHelpers::RetrySyncWithDefaultSettings(Client, operation);
}

// ids.size() must be >= 0 to make correct query.
// Ydb can return only <= 1000 items, so ids.size() must be <= 1000.
TYdbContentDb::TStatus
TYdbContentDb::FindVideoItemsByKinopoiskIdsImpl(const TVector<TString>& kinopoiskIds,
                                                THashMap<TString, TVector<TVideoItem>>& items) const {
    Y_ENSURE(kinopoiskIds.size() <= MAX_YDB_RESULT_SIZE && !kinopoiskIds.empty());

    TVector<TString> contents;
    const auto status = RetryOperationSync([&](NYdb::NTable::TSession session) -> NYdb::TStatus {
        TVector<ui64> ids;
        const auto findIdsStatus = FindIdsByKinopoiskIdsImpl(session, kinopoiskIds, ids);
        if (!findIdsStatus.IsSuccess() || ids.empty())
            return findIdsStatus;
        return NYdbHelpers::ExecuteSingleSelectDataQuery(
            Client, FindItemsByIds,
            [&](NYdb::NTable::TSession& session) {
                return session.GetParamsBuilder().AddParam("$ids", ToIdsList(ids)).Build();
            } /* paramsBuilder */,
            [&](NYdb::TResultSetParser& parser) {
                Y_ASSERT(parser.ColumnsCount() == 2);
                while (parser.TryNextRow()) {
                    TMaybe<TString> content = parser.ColumnParser(0).GetOptionalString();
                    TMaybe<bool> isVoid = parser.ColumnParser(1).GetOptionalBool();
                    if (isVoid.GetOrElse(false))
                        continue;
                    if (content.Defined())
                        contents.push_back(std::move(*content.Get()));
                }
            } /* onParser */);
    });

    if (!status.IsSuccess()) {
        LOG(ERR) << NYdbHelpers::StatusToString(status);
        return TStatus{EStatus::YDB_ERROR, status};
    }

    if (contents.empty()) {
        LOG(WARNING) << "Can not find any content in db for this kpids." << Endl;
        return {EStatus::NOT_FOUND, status};
    }

    THashMap<TString, TVector<TVideoItem>> tempItems;
    bool hasBrokenData = false;
    for (const TString &content : contents) {
        TVideoItem videoItem;
        if (!Des(content, videoItem)) {
            LOG(ERR) << "BROKEN_DATA: Cannot create videoItem from db content." << Endl;
            hasBrokenData = true;
            continue;
        }
        tempItems[videoItem->MiscIds().Kinopoisk()].push_back(std::move(videoItem));
    }

    items = std::move(tempItems);
    if (hasBrokenData)
        TStatus{EStatus::BROKEN_DATA, status};
    return TStatus{EStatus::SUCCESS, status};
}

THashMap<TString, TVector<TVideoItem>> TYdbContentDb::FindVideoItemsByKinopoiskIds(const THashSet<TString>& kpids) const {
    if (kpids.empty()) {
        LOG(WARNING) << "Empty kinopoisk ids to find in content db." << Endl;
        return {};
    }
    if (kpids.size() > MAX_YDB_RESULT_SIZE) {
        LOG(ERR) << "Max ydb result size exceeded!" << Endl;
        return {};
    }

    TVector<TString> kpidsVector(kpids.begin(), kpids.end());
    THashMap<TString, TVector<TVideoItem>> allItems;
    const auto status = FindVideoItemsByKinopoiskIdsImpl(kpidsVector, allItems);

    return allItems;
}

TYdbContentDb::TStatus TYdbContentDb::FindVideoItem(const TVideoKey& key, TVideoItem& item) const {
    TMaybe<TString> content;
    TMaybe<bool> isVoid;

    const auto status = RetryOperationSync([&](NYdb::NTable::TSession session) -> NYdb::TStatus {
        const auto idStatus = FindIdImpl(session, key);
        const auto& id = idStatus.Id;
        if (!idStatus.IsSuccess() || !id.Defined())
            return idStatus;

        return NYdbHelpers::ExecuteSingleSelectDataQuery(
            session, FindItem,
            [&](NYdb::NTable::TSession& session) {
                return session.GetParamsBuilder().AddParam("$id").Uint64(*id).Build().Build();
            } /* paramsBuilder */,
            [&](NYdb::TResultSetParser& parser) {
                if (!parser.TryNextRow())
                    return;
                Y_ASSERT(parser.ColumnsCount() == 2);
                content = parser.ColumnParser(0).GetOptionalString();
                isVoid = parser.ColumnParser(1).GetOptionalBool();
            } /* onParser */);
    });

    if (!status.IsSuccess())
        return TStatus{EStatus::YDB_ERROR, status};
    if (isVoid.Defined() && *isVoid)
        return TStatus{EStatus::IS_VOID, status};
    if (!content)
        return TStatus{EStatus::NOT_FOUND, status};

    TVideoItem videoItem;
    if (!Des(*content, videoItem))
        return TStatus{EStatus::BROKEN_DATA, status};

    item = videoItem;
    return TStatus{EStatus::SUCCESS, status};
}

TYdbContentDb::TStatus TYdbContentDb::FindVideoItemsImpl(const THashSet<TVideoKey>& keys,
                                                         TVector<std::pair<bool /* isVoid */, TVideoItem>>& items) const {
    Y_ENSURE(keys.size() <= MAX_YDB_RESULT_SIZE && !keys.empty());

    // Check that all keys have the same IdType. Otherwise there is no reason to call this function.
    TVideoKey::EIdType idType = keys.begin()->IdType;
    Y_ENSURE(AllOf(keys, [idType](const auto& key) { return key.IdType == idType; }));

    TVector<std::pair<bool /* isVoid */, TString>> contents;
    const auto status = RetryOperationSync([&](NYdb::NTable::TSession session) -> NYdb::TStatus {
        THashMap<TVideoKey, ui64> idsMap;
        const auto findIdsStatus = FindIds(session, keys, idsMap);
        if (!findIdsStatus.IsSuccess() || idsMap.empty())
            return findIdsStatus;

        TVector<ui64> ids;
        ids.reserve(idsMap.size());
        for (const auto& id : idsMap)
            ids.push_back(id.second);

        return NYdbHelpers::ExecuteSingleSelectDataQuery(
            Client, FindItemsByIds,
            [&](NYdb::NTable::TSession& session) {
                return session.GetParamsBuilder().AddParam("$ids", ToIdsList(ids)).Build();
            } /* paramsBuilder */,
            [&](NYdb::TResultSetParser& parser) {
                Y_ASSERT(parser.ColumnsCount() == 2);
                while (parser.TryNextRow()) {
                    TMaybe<TString> content = parser.ColumnParser(0).GetOptionalString();
                    TMaybe<bool> isVoid = parser.ColumnParser(1).GetOptionalBool();
                    bool isVoidContent = isVoid.GetOrElse(false);
                    if (content)
                        contents.push_back({isVoidContent, std::move(*content)});
                }
            } /* onParser */);
    });

    if (!status.IsSuccess()) {
        LOG(ERR) << NYdbHelpers::StatusToString(status);
        return TStatus{EStatus::YDB_ERROR, status};
    }

    if (contents.empty()) {
        return TStatus{EStatus::NOT_FOUND, status};
    }

    TVector<std::pair<bool /* isVoid */, TVideoItem>> tempItems;
    auto resultStatus = EStatus::SUCCESS;
    for (const auto& content : contents) {
        TVideoItem videoItem;
        if (!Des(content.second, videoItem)) {
            LOG(ERR) << "BROKEN_DATA: Cannot create videoItem from db content." << Endl;
            resultStatus = EStatus::BROKEN_DATA;
            continue;
        }

        tempItems.push_back({content.first, std::move(videoItem)});
    }

    items = std::move(tempItems);

    return TStatus{resultStatus, status};
}

THashMap<TVideoKey, std::pair<bool /* isVoid */, TVideoItem>>
TYdbContentDb::FindVideoItems(const THashSet<TVideoKey>& keys) const {
    THashMap<TVideoKey::EIdType, THashSet<TVideoKey>> keysByIdType;
    for (const auto& key : keys) {
        keysByIdType[key.IdType].insert(key);
    }

    THashMap<TVideoKey, std::pair<bool /* isVoid */, TVideoItem>> tempItems;

    for (const auto& [idType, keysForType] : keysByIdType) {
        if (keysForType.size() > MAX_YDB_RESULT_SIZE) {
            LOG(ERR) << "Max ydb result size exceeded!" << Endl;
            continue;
        }
        TVector<std::pair<bool /* isVoid */, TVideoItem>> itemsForType;
        const auto status = FindVideoItemsImpl(keysForType, itemsForType);
        if (!status.IsSuccess()) {
            LOG(ERR) << "Can not get video item list with videoKey type=" << idType
                << ". YdbContentDb status: " << status.Status << ". Ydb status: "
                << NYdbHelpers::StatusToString(status.YdbStatus);
            continue;
        }
        if (status.IsSuccess() && status.Status != EStatus::NOT_FOUND) {
            for (const auto& itemForType : itemsForType) {
                const auto isVoid = itemForType.first;
                const auto& videoItem = itemForType.second;

                TVideoKey key;
                key.ProviderName = videoItem->ProviderName();
                if (idType != TVideoKey::EIdType::KINOPOISK_ID && videoItem->HasType())
                    key.Type = videoItem->Type();
                key.IdType = idType;
                switch (idType) {
                    case TVideoKey::EIdType::KINOPOISK_ID:
                        key.Id = videoItem->MiscIds().Kinopoisk();
                        break;
                    case TVideoKey::EIdType::HRID:
                        key.Id = videoItem->HumanReadableId();
                        break;
                    case TVideoKey::EIdType::ID:
                        key.Id = videoItem->ProviderItemId();
                        break;
                }

                tempItems[key] = {isVoid, std::move(videoItem)};
            }
        }
    }

    return tempItems;
}

TYdbContentDb::TStatus TYdbContentDb::FindSerialDescriptor(const TSerialKey& key, TSerialDescriptor& serial) const {
    TMaybe<TString> content;
    const auto status = NYdbHelpers::ExecuteSingleSelectDataQuery(
        Client, FindSerial,
        [&](NYdb::NTable::TSession& session) {
            return session.GetParamsBuilder()
                .AddParam("$providerName")
                .String(key.ProviderName)
                .Build()
                .AddParam("$serialId")
                .String(key.SerialId)
                .Build()
                .Build();
        } /* paramsBuilder */,
        [&](NYdb::TResultSetParser& parser) {
            if (!parser.TryNextRow())
                return;
            Y_ASSERT(parser.ColumnsCount() == 1);
            content = parser.ColumnParser(0).GetOptionalString();
        } /* onParser */);

    if (!status.IsSuccess())
        return TStatus{EStatus::YDB_ERROR, status};
    if (!content)
        return TStatus{EStatus::NOT_FOUND, status};

    TStringInput is(*content);

    NVideoContent::NProtos::TSerialDescriptor psd;
    if (!psd.ParseFromArcadiaStream(&is))
        return TStatus{EStatus::BROKEN_DATA, status};

    TSerialDescriptor sd;
    if (!sd.Des(psd))
        return TStatus{EStatus::BROKEN_DATA, status};

    serial = std::move(sd);
    return TStatus{EStatus::SUCCESS, status};
}

THashMap<TSerialKey, TSeasonDescriptor>
TYdbContentDb::FindInitSeasonsBySerialKeys(const THashSet<TSerialKey>& keys) const {
    if (keys.empty()) {
        LOG(WARNING) << "Empty serial keys to find in content db." << Endl;
        return {};
    }
    if (keys.size() > MAX_YDB_RESULT_SIZE) {
        LOG(ERR) << "Max ydb result size exceeded!" << Endl;
        return {};
    }

    THashMap<TSerialKey, TSeasonDescriptor> resultSeasons;
    const auto status = FindInitSeasonsBySerialKeysImpl(keys, resultSeasons);
    if (!status.IsSuccess())
        LOG(ERR) << status << Endl;

    return resultSeasons;
}

// serialKeys.size() must be >= 0 to make correct query.
// Ydb can return only <= 1000 items, so serialKeys.size() must be <= 1000.
TYdbContentDb::TStatus
TYdbContentDb::FindInitSeasonsBySerialKeysImpl(const THashSet<TSerialKey>& serialKeys,
                                               THashMap<TSerialKey, TSeasonDescriptor>& resultSeasons) const {
    Y_ENSURE(serialKeys.size() <= MAX_YDB_RESULT_SIZE && !serialKeys.empty());

    TVector<std::pair<TSerialKey, TString>> contents;
    const auto status = NYdbHelpers::ExecuteSingleSelectDataQuery(
        Client, FindInitSeasonsByKeys,
        [&](NYdb::NTable::TSession& session) {
            return session.GetParamsBuilder().AddParam("$Keys", SerialKeysToList(serialKeys)).Build();
        } /* paramsBuilder */,
        [&](NYdb::TResultSetParser& parser) {
            Y_ASSERT(parser.ColumnsCount() == 4);
            while (parser.TryNextRow()) {
                TMaybe<TString> providerName = parser.ColumnParser(0).GetOptionalString();
                TMaybe<TString> serialId = parser.ColumnParser(1).GetOptionalString();
                TMaybe<TString> season = parser.ColumnParser(2).GetOptionalString();
                TMaybe<ui64> providerNumber = parser.ColumnParser(3).GetOptionalUint64();
                if (providerName && serialId && providerNumber && season) {
                    contents.push_back({TSerialKey{*providerName, *serialId}, std::move(*season)});
                }
            }
        } /* onParser */);

    if (!status.IsSuccess()) {
        LOG(ERR) << NYdbHelpers::StatusToString(status) << Endl;
        return TStatus{EStatus::YDB_ERROR, status};
    }

    if (contents.empty()) {
        return TStatus{EStatus::NOT_FOUND, status};
    }

    THashMap<TSerialKey, TSeasonDescriptor> foundSeasons;
    auto resultStatus = EStatus::SUCCESS;
    for (const auto& [serialKey, seasonDescr] : contents) {
        TStringInput is(seasonDescr);
        NVideoContent::NProtos::TSeasonDescriptor psd;
        if (!psd.ParseFromArcadiaStream(&is)) {
            LOG(ERR) << "BROKEN_DATA: Cannot create season descriptor from db content." << Endl;
            resultStatus = EStatus::BROKEN_DATA;
            continue;
        }

        TSeasonDescriptor sd;
        if (!sd.Des(psd)) {
            LOG(ERR) << "BROKEN_DATA: Cannot create season descriptor from db content." << Endl;
            resultStatus = EStatus::BROKEN_DATA;
            continue;
        }

        foundSeasons[serialKey] = std::move(sd);
    }

    resultSeasons = std::move(foundSeasons);

    return TStatus{resultStatus, status};
}

TYdbContentDb::TStatus TYdbContentDb::FindSeasonDescriptor(const TSeasonKey& key, TSeasonDescriptor& season) const {
    TMaybe<TString> content;
    const auto status = NYdbHelpers::ExecuteSingleSelectDataQuery(
        Client, FindSeason,
        [&](NYdb::NTable::TSession& session) {
            return session.GetParamsBuilder()
                .AddParam("$providerName")
                .String(key.ProviderName)
                .Build()
                .AddParam("$serialId")
                .String(key.SerialId)
                .Build()
                .AddParam("$providerNumber")
                .Uint64(key.ProviderNumber)
                .Build()
                .Build();
        } /* paramsBuilder */,
        [&](NYdb::TResultSetParser& parser) {
            if (!parser.TryNextRow())
                return;
            Y_ASSERT(parser.ColumnsCount() == 1);
            content = parser.ColumnParser(0).GetOptionalString();
        } /* onParser */);

    if (!status.IsSuccess())
        return TStatus{EStatus::YDB_ERROR, status};
    if (!content)
        return TStatus{EStatus::NOT_FOUND, status};

    TStringInput is(*content);
    NVideoContent::NProtos::TSeasonDescriptor psd;
    if (!psd.ParseFromArcadiaStream(&is))
        return TStatus{EStatus::BROKEN_DATA, status};

    TSeasonDescriptor sd;
    if (!sd.Des(psd))
        return TStatus{EStatus::BROKEN_DATA, status};

    season = std::move(sd);
    return TStatus{EStatus::SUCCESS, status};
}

TYdbContentDb::TStatus TYdbContentDb::FindEpisodeNumber(const TString& providerItemId, ui64& season, ui64& episode) const {
    TMaybe<ui64> seasonNumber, episodeNumber;
    const auto status = NYdbHelpers::ExecuteSingleSelectDataQuery(
            Client, FindEpisode,
            [&](NYdb::NTable::TSession& session) {
                return session.GetParamsBuilder()
                        .AddParam("$providerItemId")
                        .String(providerItemId)
                        .Build()
                        .Build();
            } /* paramsBuilder */,
            [&](NYdb::TResultSetParser& parser) {
                if (!parser.TryNextRow())
                    return;
                Y_ASSERT(parser.ColumnsCount() == 2);
                seasonNumber = parser.ColumnParser("SeasonNumber").GetOptionalUint64();
                episodeNumber = parser.ColumnParser("EpisodeNumber").GetOptionalUint64();
            } /* onParser */);
    if (!status.IsSuccess())
        return TStatus{EStatus::YDB_ERROR, status};
    if (!seasonNumber || !episodeNumber)
        return TStatus{EStatus::NOT_FOUND, status};
    season = *seasonNumber;
    episode = *episodeNumber;
    return TStatus{EStatus::SUCCESS, status};
}

TYdbContentDb::TIdStatus TYdbContentDb::FindId(const TVideoKey& key) const {
    TMaybe<ui64> id;
    auto status = RetryOperationSync([&](NYdb::NTable::TSession session) -> NYdb::TStatus {
        const auto status = FindIdImpl(session, key);
        id = status.Id;
        return status;
    });
    return TIdStatus{std::move(status), id};
}

NYdb::TStatus TYdbContentDb::FindIds(NYdb::NTable::TSession& session,
                                     const THashSet<TVideoKey>& keys,
                                     THashMap<TVideoKey, ui64>& ids) const {
    Y_ENSURE(keys.size() <= MAX_YDB_RESULT_SIZE && !keys.empty());

    // Check that all keys have the same IdType. Otherwise there is no reason to call this function.
    TVideoKey::EIdType idType = keys.begin()->IdType;
    Y_ASSERT(AllOf(keys, [idType](const auto& key) { return key.IdType == idType; }));

    // Kinopoisk keys can have no content type from web search, while contentDB will have it.
    const bool needIgnoreType = (idType == TVideoKey::EIdType::KINOPOISK_ID);

    const TString* query = nullptr;
    switch (idType) {
        case TVideoKey::EIdType::ID:
            query = &FindIdsByPiidKeys;
            break;
        case TVideoKey::EIdType::HRID:
            query = &FindIdsByHridKeys;
            break;
        case TVideoKey::EIdType::KINOPOISK_ID:
            query = &FindIdsByKpidKeys;
            break;
    }
    Y_ASSERT(query);

    THashMap<TVideoKey, ui64> tempIds;
    auto status = NYdbHelpers::ExecuteSingleSelectDataQuery(
        session, *query,
        [&](NYdb::NTable::TSession& session) {
            return session.GetParamsBuilder()
                .AddParam("$Ids", KeysToList(keys))
                .Build();
        } /* paramsBuilder */,
        [&](NYdb::TResultSetParser& parser) {
            Y_ASSERT(parser.ColumnsCount() == 4);
            while (parser.TryNextRow()) {
                TMaybe<ui64> videoItemsTableId = parser.ColumnParser(0).GetOptionalUint64();
                TString id = parser.ColumnParser(1).GetString();
                TString providerName = parser.ColumnParser(2).GetString();
                TMaybe<TString> type = parser.ColumnParser(3).GetOptionalString();
                if (videoItemsTableId) {
                    TVideoKey mapKey{providerName, id, idType, type};
                    if (needIgnoreType && type)
                        mapKey.Type = Nothing();
                    if (videoItemsTableId) {
                        Y_ENSURE(!tempIds.contains(mapKey));
                        tempIds[mapKey] = *videoItemsTableId;
                    }
                }
            }
        } /* onParser */);

    if (!status.IsSuccess())
        return status;
    ids = std::move(tempIds);
    return status;
}

TYdbContentDb::TIdStatus TYdbContentDb::FindIdImpl(NYdb::NTable::TSession& session, const TVideoKey& key) const {
    const TString* query = nullptr;

    switch (key.IdType) {
        case TVideoKey::EIdType::ID:
            query = &FindIdByPiid;
            break;
        case TVideoKey::EIdType::HRID:
            query = &FindIdByHrid;
            break;
        case TVideoKey::EIdType::KINOPOISK_ID:
            query = &FindIdByKpid;
            break;
    }

    Y_ASSERT(query);

    TMaybe<ui64> id;
    auto status = NYdbHelpers::ExecuteSingleSelectDataQuery(
        session, *query,
        [&](NYdb::NTable::TSession& session) {
            return session.GetParamsBuilder()
                .AddParam("$providerName")
                .String(key.ProviderName)
                .Build()
                .AddParam("$id")
                .String(key.Id)
                .Build()
                .AddParam("$type")
                .OptionalString(key.Type)
                .Build()
                .Build();
        } /* paramsBuilder */,
        [&](NYdb::TResultSetParser& parser) {
            if (!parser.TryNextRow())
                return;
            Y_ASSERT(parser.ColumnsCount() == 1);
            id = parser.ColumnParser(0).GetOptionalUint64();
        } /* onParser */);
    return TIdStatus{std::move(status), id};
}

NYdb::TStatus TYdbContentDb::FindIdsByKinopoiskIdsImpl(NYdb::NTable::TSession& session,
                                                       const TVector<TString>& kinopoiskIds,
                                                       TVector<ui64>& ids) const
{
    TVector<ui64> tempIds;
    auto status = NYdbHelpers::ExecuteSingleSelectDataQuery(
        session, FindIdsByKpids,
        [&](NYdb::NTable::TSession& session) {
            return session.GetParamsBuilder()
                .AddParam("$kinopoiskIds", ToKinopoiskIdsList(kinopoiskIds.begin(), kinopoiskIds.end()))
                .Build();
        } /* paramsBuilder */,
        [&](NYdb::TResultSetParser& parser) {
            Y_ASSERT(parser.ColumnsCount() == 1);
            while (parser.TryNextRow()) {
                TMaybe<ui64> id = parser.ColumnParser(0).GetOptionalUint64();
                if (id.Defined())
                    tempIds.push_back(std::move(*id.Get()));
            }
        } /* onParser */);
    if (!status.IsSuccess())
        return status;
    ids = std::move(tempIds);
    return status;
}

TYdbContentDb::TStatus TYdbContentDb::FindProviderUniqueVideoItem(const TVideoKey& key, TVideoItem& item) const {
    TMaybe<TString> content;

    const auto status = RetryOperationSync([&](NYdb::NTable::TSession session) -> NYdb::TStatus {
        return NYdbHelpers::ExecuteSingleSelectDataQuery(
            session, FindProviderUniqueItem,
            [&](NYdb::NTable::TSession& session) {
                return session.GetParamsBuilder()
                    .AddParam("$providerName")
                    .String(key.ProviderName)
                    .Build()
                    .AddParam("$providerItemId")
                    .String(key.Id)
                    .Build()
                    .Build();
            } /* paramsBuilder */,
            [&](NYdb::TResultSetParser& parser) {
                if (!parser.TryNextRow())
                    return;
                Y_ASSERT(parser.ColumnsCount() == 1);
                content = parser.ColumnParser(0).GetOptionalString();
            } /* onParser */);
    });

    if (!status.IsSuccess())
        return TStatus{EStatus::YDB_ERROR, status};
    if (!content)
        return TStatus{EStatus::NOT_FOUND, status};

    TVideoItem videoItem;
    if (!Des(*content, videoItem))
        return TStatus{EStatus::BROKEN_DATA, status};

    item = std::move(videoItem);
    return TStatus{EStatus::SUCCESS, status};
}


void DownloadItems(const TVector<TVideoItem>& items, IContentInfoProvidersCache& providers,
                   std::function<void(size_t index, const TVideoItem& item)> onSuccess,
                   std::function<void(size_t index, const TError& error)> onError) {
    Y_ASSERT(onSuccess);
    Y_ASSERT(onError);

    NHttpFetcher::IMultiRequest::TRef multiRequest = NHttpFetcher::WeakMultiRequest();
    Y_ASSERT(multiRequest);

    TVector<TIndexed<std::unique_ptr<IVideoItemHandle>>> handles;
    TVector<IContentInfoProvider*> contentProviders(items.size(), nullptr);
    TVector<TVideoItem> videoItems(items.size());
    TVector<bool> checkAux(items.size(), false);

    for (size_t i = 0; i < items.size(); ++i) {
        const auto& item = items[i];
        const TStringBuf providerName = item->ProviderName();

        auto* provider = providers.GetProvider(providerName);
        if (!provider)
            continue;

        contentProviders[i] = provider;
        if (!provider->IsAuxInfoRequestFeasible(item.Scheme()))
            checkAux[i] = true;
        handles.emplace_back(i, provider->MakeContentInfoRequest(item.Scheme(), multiRequest));
    }

    TVector<TIndexed<std::unique_ptr<IVideoItemHandle>>> auxHandles;
    for (auto& handle : handles) {
        Y_ASSERT(handle.Value);

        const auto index = handle.Index;
        Y_ASSERT(index < items.size());

        auto& videoItem = videoItems[index];
        auto* provider = contentProviders[index];
        Y_ASSERT(provider);

        if (const auto error = handle.Value->WaitAndParseResponse(videoItem)) {
            onError(index, *error);
            continue;
        }

        if (!checkAux[index] || !provider->IsAuxInfoRequestFeasible(videoItem.Scheme())) {
            onSuccess(index, videoItem);
            continue;
        }

        auxHandles.emplace_back(index, provider->MakeAuxInfoRequest(videoItem.Scheme(), multiRequest));
    }

    for (auto& handle : auxHandles) {
        Y_ASSERT(handle.Value);

        const auto index = handle.Index;
        auto& videoItem = videoItems[index];

        if (const auto error = handle.Value->WaitAndParseResponse(videoItem))
            onError(index, *error);
        else
            onSuccess(index, videoItem);
    }
}

void DownloadSerials(
    const TVector<TVideoItem>& items, IContentInfoProvidersCache& providers,
    std::function<void(size_t index, const TSerialKey& key, const TSerialDescriptor& serial)> onSuccess,
    std::function<void(size_t index, const TError& error)> onError) {
    Y_ASSERT(onSuccess);
    Y_ASSERT(onError);

    NHttpFetcher::IMultiRequest::TRef multiRequest = NHttpFetcher::WeakMultiRequest();
    Y_ASSERT(multiRequest);

    TVector<TIndexed<std::unique_ptr<ISerialDescriptorHandle>>> handles;
    for (size_t i = 0; i < items.size(); ++i) {
        const auto& item = items[i];
        if (item->Type() != ToString(EItemType::TvShow))
            continue;

        const TStringBuf providerName = item->ProviderName();

        auto* provider = providers.GetProvider(providerName);
        if (!provider)
            continue;

        handles.emplace_back(i, provider->MakeSerialDescriptorRequest(item.Scheme(), multiRequest));
    }

    for (auto& handle : handles) {
        Y_ASSERT(handle.Value);
        Y_ASSERT(handle.Index < items.size());

        TSerialDescriptor serial;
        if (const auto error = handle.Value->WaitAndParseResponse(serial)) {
            onError(handle.Index, *error);
            continue;
        }

        const auto& item = items[handle.Index];

        const TSerialKey key{item->ProviderName(), item->ProviderItemId()};
        onSuccess(handle.Index, key, serial);
    }
}

void DownloadAllSeasons(
    const TVector<TAllSeasonsDownloadItem>& items, IContentInfoProvidersCache& providers,
    std::function<void(size_t index, const TSeasonKey& key, const TSeasonDescriptor& season)> onSuccess,
    std::function<void(size_t index, const TError& error)> onError) {
    Y_ASSERT(onSuccess);
    Y_ASSERT(onError);

    NHttpFetcher::IMultiRequest::TRef multiRequest = NHttpFetcher::WeakMultiRequest();
    Y_ASSERT(multiRequest);

    TVector<TIndexed<std::unique_ptr<IAllSeasonsDescriptorHandle>>> handles;
    for (size_t i = 0; i < items.size(); ++i) {
        const auto& item = items[i];
        auto* provider = providers.GetProvider(item.Provider);
        if (!provider)
            continue;

        auto handle = provider->MakeAllSeasonsDescriptorRequest(item.Serial, multiRequest);
        handles.emplace_back(i, std::move(handle));
    }

    for (auto& handle : handles) {
        Y_ASSERT(handle.Value);
        Y_ASSERT(handle.Index < items.size());

        const auto& item = items[handle.Index];

        auto serial = item.Serial;
        if (const auto error = handle.Value->WaitAndParseResponse(serial)) {
            onError(handle.Index, *error);
            continue;
        }

        for (const auto& season : serial.Seasons) {
            const TSeasonKey key{item.Provider, season.SerialId, season.ProviderNumber};
            onSuccess(handle.Index, key, season);
        }
    }
}

void DownloadSeasons(
    const TVector<TSeasonDownloadItem>& items, IContentInfoProvidersCache& providers,
    std::function<void(size_t index, const TSeasonKey& key, const TSeasonDescriptor& season)> onSuccess,
    std::function<void(size_t index, const TError& error)> onError) {
    Y_ASSERT(onSuccess);
    Y_ASSERT(onError);

    NHttpFetcher::IMultiRequest::TRef multiRequest = NHttpFetcher::WeakMultiRequest();
    Y_ASSERT(multiRequest);

    TVector<TIndexed<std::unique_ptr<ISeasonDescriptorHandle>>> handles;
    for (size_t i = 0; i < items.size(); ++i) {
        const auto& item = items[i];
        auto* provider = providers.GetProvider(item.Provider);
        if (!provider)
            continue;

        if (!item.Serial)
            continue;

        auto handle = provider->MakeSeasonDescriptorRequest(*item.Serial, item.Season, multiRequest);
        handles.emplace_back(i, std::move(handle));
    }

    for (auto& handle : handles) {
        Y_ASSERT(handle.Value);
        Y_ASSERT(handle.Index < items.size());

        const auto& item = items[handle.Index];

        TSeasonDescriptor season = item.Season;
        if (const auto error = handle.Value->WaitAndParseResponse(season)) {
            onError(handle.Index, *error);
            continue;
        }

        const TSeasonKey key{item.Provider, season.SerialId, season.ProviderNumber};
        onSuccess(handle.Index, key, season);
    }
}

bool IsProviderSupportedByVideoItemsTable(TStringBuf name) {
    return name == PROVIDER_IVI || name == PROVIDER_AMEDIATEKA || name == PROVIDER_KINOPOISK;
}

bool IsTypeSupportedByVideoItemsTable(EItemType type) {
    return IsTypeSupportedByVideoItemsTable(ToString(type));
}

bool IsTypeSupportedByVideoItemsTable(TStringBuf type) {
    return type == ToString(EItemType::Movie) || type == ToString(EItemType::TvShow);
}
} // namespace NVideoCommon

template <>
void Out<NVideoCommon::TVideoTablesPaths>(IOutputStream& os, const NVideoCommon::TVideoTablesPaths& paths) {
    os << "TVideoTablesPaths [";
    os << "database: " << paths.Database << ", ";
    os << "items: " << paths.Items << ", ";
    os << "serials: " << paths.Serials << ", ";
    os << "seasons: " << paths.Seasons;
    os << "]";
}

template <>
void Out<NVideoCommon::TYdbContentDb::TStatus>(IOutputStream& os, const NVideoCommon::TYdbContentDb::TStatus& status) {
    os << "TStatus [";
    os << "Status: " << status.Status;
    if (!status.YdbStatus.IsSuccess())
        os << ", YdbStatus: " << NYdbHelpers::StatusToString(status.YdbStatus);
    os << "]";
}

template <>
void Out<NVideoCommon::TYdbContentDb::TIdStatus>(IOutputStream& os,
                                                 const NVideoCommon::TYdbContentDb::TIdStatus& status) {
    os << "TIdStatus [";
    os << "Status: " << NYdbHelpers::StatusToString(status) << ", " << status.Id;
    os << "]";
}
