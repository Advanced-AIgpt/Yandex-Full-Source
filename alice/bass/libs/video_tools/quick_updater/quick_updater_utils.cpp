#include "quick_updater_utils.h"

#include <alice/bass/libs/logging_v2/logger.h>
#include <alice/bass/libs/video_common/defs.h>

#include <alice/bass/libs/ydb_helpers/queries.h>
#include <alice/bass/libs/ydb_helpers/settings.h>

#include <ydb/public/sdk/cpp/client/ydb_driver/driver.h>
#include <ydb/public/sdk/cpp/client/ydb_scheme/scheme.h>

#include <util/string/builder.h>

using namespace NVideoCommon;
using namespace NYdbHelpers;

namespace NQuickUpdater {

namespace {

bool TvShowsLess(const TVideoItem& lhs, const TVideoItem& rhs) {
    if (lhs->ProviderName().Get() != rhs->ProviderName().Get())
        return lhs->ProviderName().Get() < rhs->ProviderName().Get();
    return lhs->ProviderItemId().Get() < rhs->ProviderItemId().Get();
}

bool TvShowsEqual(const TVideoItem& lhs, const TVideoItem& rhs) {
    return lhs->ProviderName().Get() == rhs->ProviderName().Get() &&
           lhs->ProviderItemId().Get() == rhs->ProviderItemId().Get();
}

class TCopyTableAction : public ITransactionAction {
public:
    TCopyTableAction(NYdb::NTable::TTableClient& client, const TTablePath& from, const TTablePath& to)
        : Client(client)
        , FromPath(from)
        , ToPath(to)
    {
    }

    void Execute() override {
        CopyTableOrFail(Client, FromPath, ToPath);
    }

    void Rollback() noexcept override {
        LOG(INFO) << "Rolling back creation of table " << ToPath << "..." << Endl;
        const auto status = DropTable(Client, ToPath);
        if (!status.IsSuccess()) {
            LOG(ERR) << "Failed to drop copied table: " << ToPath << ": " << StatusToString(status) << Endl;
        }
    }

    void Commit() override {
    }

private:
    NYdb::NTable::TTableClient& Client;
    const TTablePath FromPath;
    const TTablePath ToPath;
};

class TMkDirAction : public ITransactionAction {
public:
    TMkDirAction(NYdb::NScheme::TSchemeClient& client, const TTablePath& path)
        : Client(client)
        , Path(path)
    {
    }

protected:
    void Execute() override {
        const auto status = Client.MakeDirectory(Path.FullPath()).GetValueSync();
        ThrowOnError(status);
    }

    void Rollback() noexcept override {
        LOG(INFO) << "Rolling back creation of directory " << Path << "..." << Endl;
        const auto status = Client.RemoveDirectory(Path.FullPath()).GetValueSync();
        if (!status.IsSuccess()) {
            LOG(ERR) << "Failed to remove created directory: " << Path << ": " << StatusToString(status) << Endl;
        }
    }

    void Commit() override {
    }

private:
    NYdb::NScheme::TSchemeClient& Client;
    const TTablePath Path;
};

class TTransactionsSeq : public ITransactionAction {
public:
    template <typename TAction, typename... TArgs>
    void Emplace(TArgs&&... args) {
        Transactions.push_back(TTransaction::MakeTransaction<TAction>(std::forward<TArgs>(args)...));
    }

    void Execute() override {
        for (auto& transaction : Transactions)
            transaction.Execute();
    }

    void Rollback() noexcept override {
        for (auto RevIter = Transactions.rbegin(); RevIter != Transactions.rend(); ++RevIter)
            RevIter->Rollback();
    }

    void Commit() override {
        for (auto& transaction : Transactions)
            transaction.Commit();
    }

private:
    TVector<TTransaction> Transactions;
};

// Parses pairs of provider name and serial id from |result|, updates
// correspondingly last versions of these fields.
//
// Returns true iff at least one non-empty result was parsed.
bool ParseProviderNameSerialIds(NYdb::NTable::TDataQueryResult& result, TVector<TVideoItem>& items,
                                TMaybe<TString>& lastProviderName, TMaybe<TString>& lastSerialId) {
    Y_ASSERT(result.GetResultSets().size() == 1);
    auto parser = result.GetResultSetParser(0);

    bool updated = false;
    while (parser.TryNextRow()) {
        Y_ASSERT(parser.ColumnsCount() == 2);
        updated = true;

        auto providerName = parser.ColumnParser(0).GetOptionalString();
        auto serialId = parser.ColumnParser(1).GetOptionalString();

        if (!providerName || !serialId)
            continue;

        TVideoItem item;
        item->ProviderName() = *providerName;
        item->ProviderItemId() = *serialId;
        item->Type() = ToString(EItemType::TvShow);
        items.push_back(std::move(item));

        lastProviderName = std::move(providerName);
        lastSerialId = std::move(serialId);
    }

    return updated;
}

bool ParseItemIds(NYdb::NTable::TDataQueryResult& result, TVector<TVideoItem>& items, TMaybe<ui64>& lastId) {
    Y_ASSERT(result.GetResultSets().size() == 1);
    auto parser = result.GetResultSetParser(0);

    bool updated = false;
    while (parser.TryNextRow()) {
        Y_ASSERT(parser.ColumnsCount() == 3);
        updated = true;

        const auto id = parser.ColumnParser(0).GetOptionalUint64();
        const auto providerName = parser.ColumnParser(1).GetOptionalString();
        const auto providerItemId = parser.ColumnParser(2).GetOptionalString();

        if (!id || !providerName || !providerItemId)
            continue;

        lastId = id;

        TVideoItem item;
        item->ProviderName() = *providerName;
        item->ProviderItemId() = *providerItemId;
        item->Type() = ToString(EItemType::TvShow);
        items.push_back(std::move(item));
    }

    return updated;
}

NYdb::TStatus GetItemsForUpdate(NYdb::NTable::TTableClient& client, const NYdbHelpers::TTablePath& path,
                                TInstant timestamp, TVector<TVideoItem>& items) {
    // YDb returns only a limited amount of items. To retrieve all the items required for update, we query the DB
    // multiple times, moving a start window each time (lastId parameter).
    const TString query = Sprintf(R"(
                                     PRAGMA TablePathPrefix("%s");

                                     DECLARE $timestamp AS Uint64;
                                     DECLARE $lastId AS "Uint64?";

                                     SELECT Id, ProviderName, ProviderItemId FROM [%s] WHERE
                                         Type == "tv_show" AND UpdateAtUS <= $timestamp
                                         AND ($lastId IS NULL OR Id > $lastId)
                                     ORDER BY Id;
                                     )",
                                  path.Database.c_str(), path.Name.c_str());

    return RetrySyncWithDefaultSettings(client, [&](NYdb::NTable::TSession session) -> NYdb::TStatus {
        NYdbHelpers::TPreparedQuery preparedQuery{query, session};
        if (!preparedQuery.IsPrepared())
            return preparedQuery.GetPrepareResult();

        TMaybe<ui64> lastId;
        bool updated = true;
        TMaybe<NYdb::NTable::TDataQueryResult> result;
        while (updated) {
            result = preparedQuery.Execute(NYdbHelpers::SerializableRW(), [&](NYdb::NTable::TSession& session) {
                return session.GetParamsBuilder()
                    .AddParam("$timestamp")
                    .Uint64(timestamp.MicroSeconds())
                    .Build()
                    .AddParam("$lastId")
                    .OptionalUint64(lastId)
                    .Build()
                    .Build();
            } /* paramsBuilder */);
            if (!result->IsSuccess())
                return *result;
            updated = ParseItemIds(*result, items, lastId);
        }

        Y_ASSERT(result);
        return *result;
    });
}

NYdb::TStatus GetItemsForUpdateBySeasons(NYdb::NTable::TTableClient& client, const NYdbHelpers::TTablePath& path,
                                         TInstant timestamp, TVector<TVideoItem>& items) {
    // YDb returns only a limited amount of items. To retrieve all the items required for update, we query the DB
    // multiple times, moving a start window each time (lastProviderName and lastSerialId parameters).
    const TString query = Sprintf(R"(
                                     PRAGMA TablePathPrefix("%s");

                                     DECLARE $timestamp AS Uint64;
                                     DECLARE $lastProviderName AS "String?";
                                     DECLARE $lastSerialId AS "String?";

                                     SELECT ProviderName, SerialId FROM [%s] WHERE
                                             (  $lastProviderName IS NULL
                                                OR (ProviderName == $lastProviderName AND SerialId > $lastSerialId)
                                                OR ProviderName > $lastProviderName
                                             )
                                         AND
                                             UpdateAtUS <= $timestamp
                                     ORDER BY ProviderName, SerialId;
                                     )",
                                  path.Database.c_str(), path.Name.c_str());

    return RetrySyncWithDefaultSettings(client, [&](NYdb::NTable::TSession session) -> NYdb::TStatus {
        NYdbHelpers::TPreparedQuery preparedQuery{query, session};
        if (!preparedQuery.IsPrepared())
            return preparedQuery.GetPrepareResult();

        TMaybe<TString> lastProviderName;
        TMaybe<TString> lastSerialId;

        bool updated = true;
        TMaybe<NYdb::NTable::TDataQueryResult> result;
        while (updated) {
            result = preparedQuery.Execute(NYdbHelpers::SerializableRW(), [&](NYdb::NTable::TSession& session) {
                return session.GetParamsBuilder()
                    .AddParam("$timestamp")
                    .Uint64(timestamp.MicroSeconds())
                    .Build()
                    .AddParam("$lastProviderName")
                    .OptionalString(lastProviderName)
                    .Build()
                    .AddParam("$lastSerialId")
                    .OptionalString(lastSerialId)
                    .Build()
                    .Build();
            } /* paramsBuilder */);
            if (!result->IsSuccess())
                return *result;

            updated = ParseProviderNameSerialIds(*result, items, lastProviderName, lastSerialId);
        }

        Y_ASSERT(result);
        return *result;
    });
}

} // namespace

TTvShowAllInfo::TTvShowAllInfo(const TVideoItem &item, const TSerialDescriptor &serialDescr)
    : Item(item)
{
    TStringBuf providerName = item->ProviderName();
    TStringBuf serialId = item->ProviderItemId();
    SerialDescr = {TSerialKey{providerName, serialId}, serialDescr};
    for (const auto& season : serialDescr.Seasons) {
        TSeasonKey seasonKey{providerName, serialId, season.ProviderNumber};
        SeasonDescrs.emplace_back(seasonKey, season);
    }
}

TVector<TTvShowAllInfo> DownloadAll(const TVector<TVideoItem>& items, IContentInfoProvidersCache& providers) {
    TVector<TTvShowAllInfo> infos;

    DownloadItems(items, providers,
                  [&](size_t /* index */, const TVideoItem& item) { infos.emplace_back(item); } /* onSuccess */,
                  [&](size_t index, const TError& error) {
                      Y_ASSERT(index < items.size());
                      LOG(ERR) << "Failed to download video item " << items[index].Value().ToJson() << ": " << error
                               << Endl;
                  });

    {
        TVector<TVideoItem> serials;
        for (const auto& info : infos)
            serials.push_back(info.Item);

        DownloadSerials(serials, providers,
                        [&](size_t index, const TSerialKey& key, const TSerialDescriptor& serial) {
                            Y_ASSERT(index < infos.size());
                            infos[index].SerialDescr = TKeyedSerialDescriptor{key, serial};
                        } /* onSuccess */,
                        [&](size_t index, const TError& error) {
                            Y_ASSERT(index < infos.size());
                            LOG(ERR) << "Failed to download serial descriptor for: "
                                     << infos[index].Item.Value().ToJson() << ": " << error << Endl;
                            infos[index].Success = false;
                        } /* onError */);
        EraseIf(infos, [&](const TTvShowAllInfo& info) { return !info.Success; });
    }

    {
        TVector<TAllSeasonsDownloadItem> items;
        for (const auto& info : infos)
            items.emplace_back(TString{*info.Item->ProviderName()}, info.SerialDescr.Descr);

        DownloadAllSeasons(items, providers,
                           [&](size_t index, const TSeasonKey& key, const TSeasonDescriptor& season) {
                               Y_ASSERT(index < infos.size());
                               infos[index].SeasonDescrs.emplace_back(key, season);
                           } /* onSuccess */,
                           [&](size_t index, const TError& error) {
                               Y_ASSERT(index < infos.size());
                               LOG(ERR) << "Failed to download season descriptor for: "
                                        << infos[index].Item.Value().ToJson() << ": " << error << Endl;
                               infos[index].Success = false;
                           } /* onError */);
        EraseIf(infos, [&](const TTvShowAllInfo& info) { return !info.Success; });
    }

    return infos;
}

NYdb::TStatus DropTvShow(NYdb::NTable::TSession& session, NYdb::NTable::TTxControl tx,
                         const NYdbHelpers::TTablePath& snapshot, ui64 id, const TVideoItem& tvShow) {
    const TString query =
        Sprintf(R"(
                PRAGMA TablePathPrefix("%s");

                DECLARE $id AS Uint64;
                DECLARE $providerName AS String;
                DECLARE $serialId AS String;

                DELETE FROM [%s] WHERE ProviderName == $providerName AND SerialId == $serialId;
                DELETE FROM [%s] WHERE ProviderName == $providerName AND SerialId == $serialId;
                DELETE FROM [%s] WHERE Id == $id;
                DELETE FROM [%s] WHERE ProviderName == $providerName AND
                                       (ProviderItemId == $serialId OR ParentTvShowId == $serialId);
                )",
                snapshot.Database.c_str(),
                NYdbHelpers::Join(snapshot.Name, NVideoContent::TVideoSeasonsTableTraits::NAME).c_str(),
                NYdbHelpers::Join(snapshot.Name, NVideoContent::TVideoSerialsTableTraits::NAME).c_str(),
                NYdbHelpers::Join(snapshot.Name, NVideoContent::TVideoItemsLatestTableTraits::NAME).c_str(),
                NYdbHelpers::Join(snapshot.Name, NVideoContent::TProviderUniqueItemsTableTraitsV2::NAME).c_str());

    NYdbHelpers::TPreparedQuery q{query, session};
    if (!q.IsPrepared())
        return q.GetPrepareResult();
    return q.Execute(tx, [&](NYdb::NTable::TSession& session) {
        return session.GetParamsBuilder()
            .AddParam("$id")
            .Uint64(id)
            .Build()
            .AddParam("$providerName")
            .String(TString{*tvShow->ProviderName()})
            .Build()
            .AddParam("$serialId")
            .String(TString{*tvShow->ProviderItemId()})
            .Build()
            .Build();
    } /* paramsBuilder */);
}

template <typename TTableTraits>
TMaybe<NYdb::TStatus> UpsertToIndexTable(NYdb::NTable::TSession& session, NYdb::NTable::TTxControl tx,
                                         const NYdbHelpers::TTablePath& snapshot, const TVideoItem& item, ui64 id) {
    using TScheme = typename TTableTraits::TScheme;

    TScheme row;
    if (!Ser(item, id, row))
        return Nothing();

    NYdbHelpers::TUpsertQuery<TScheme> query{Join(snapshot, TTableTraits::NAME)};
    query.Add(row);
    return query.Execute(session, NYdbHelpers::TQueryOptions{tx});
}

bool UpdateTvShow(NYdb::NTable::TTableClient& client, const NYdbHelpers::TTablePath& snapshot, ui64 id,
                  const TTvShowAllInfo& info) {
    using TItemTraits = NVideoContent::TVideoItemsLatestTableTraits;

    // Serialize everything before insertion to database.
    TItemTraits::TScheme item;
    Ser(info.Item, id, false /* isVoid */, item);

    bool needsUpdateProviderUniqueItems = DoesProviderHaveUniqueIdsForItems(info.Item->ProviderName());
    TVector<NVideoContent::TProviderUniqueItemsTableTraitsV2::TScheme> puis;
    auto addPui = [&puis](const TVideoItem& item) {
        NVideoContent::TProviderUniqueItemsTableTraitsV2::TScheme row;
        if (!Ser(item, row)) {
            LOG(ERR) << "Failed to serialize provider unique item: " << item << Endl;
            return false;
        }
        puis.push_back(row);
        return true;
    };

    if (needsUpdateProviderUniqueItems) {
        if (!addPui(info.Item))
            return false;
    }

    NVideoContent::TVideoSerialsTableTraits::TScheme serial;
    {
        const auto& key = info.SerialDescr.Key;
        const auto& descr = info.SerialDescr.Descr;
        if (!descr.Ser(key, serial)) {
            LOG(ERR) << "Failed to serialize serial descriptor" << Endl;
            return false;
        }
    }

    TVector<NVideoContent::TVideoSeasonsTableTraits::TScheme> seasons;

    for (const auto& seasonDescr : info.SeasonDescrs) {
        NVideoContent::TVideoSeasonsTableTraits::TScheme season;

        const auto& key = seasonDescr.Key;
        const auto& descr = seasonDescr.Descr;
        if (!descr.Ser(key, season)) {
            LOG(ERR) << "Failed to serialize season descriptor" << Endl;
            return false;
        }
        seasons.emplace_back(std::move(season));

        if (needsUpdateProviderUniqueItems) {
            for (const auto& episode : seasonDescr.Descr.EpisodeItems) {
                if (!addPui(episode))
                    return false;
            }
        }
    }

    const auto status = RetrySyncWithDefaultSettings(client, [&](NYdb::NTable::TSession session) -> NYdb::TStatus {
        auto beginResult = session.BeginTransaction(NYdb::NTable::TTxSettings::SerializableRW()).GetValueSync();
        if (!beginResult.IsSuccess())
            return beginResult;
        auto tx = beginResult.GetTransaction();

        {
            const auto status = DropTvShow(session, NYdb::NTable::TTxControl::Tx(tx), snapshot, id, info.Item);
            if (!status.IsSuccess())
                return status;
        }

        {
            NYdbHelpers::TUpsertQuery<TItemTraits::TScheme> query{Join(snapshot, TItemTraits::NAME)};
            query.Add(item);
            const auto status = query.Execute(session, NYdbHelpers::TQueryOptions{NYdb::NTable::TTxControl::Tx(tx)});
            if (!status.IsSuccess())
                return status;
        }

        {
            const auto status = UpsertToIndexTable<NVideoContent::TProviderItemIdIndexTableTraits>(
                session, NYdb::NTable::TTxControl::Tx(tx), snapshot, info.Item, id);
            if (status.Defined() && !status->IsSuccess())
                return *status;
        }
        {
            const auto status = UpsertToIndexTable<NVideoContent::THumanReadableIdIndexTableTraits>(
                session, NYdb::NTable::TTxControl::Tx(tx), snapshot, info.Item, id);
            if (status.Defined() && !status->IsSuccess())
                return *status;
        }
        {
            const auto status = UpsertToIndexTable<NVideoContent::TKinopoiskIdIndexTableTraits>(
                session, NYdb::NTable::TTxControl::Tx(tx), snapshot, info.Item, id);
            if (status.Defined() && !status->IsSuccess())
                return *status;
        }

        {
            NYdbHelpers::TUpsertQuery<NVideoContent::TVideoSerialsTableTraits::TScheme> query{
                Join(snapshot, NVideoContent::TVideoSerialsTableTraits::NAME)};
            query.Add(serial);
            const auto status = query.Execute(session, NYdbHelpers::TQueryOptions{NYdb::NTable::TTxControl::Tx(tx)});
            if (!status.IsSuccess())
                return status;
        }

        {
            NYdbHelpers::TUpsertQuery<NVideoContent::TVideoSeasonsTableTraits::TScheme> query{
                Join(snapshot, NVideoContent::TVideoSeasonsTableTraits::NAME)};
            for (const auto& season : seasons)
                query.Add(season);

            const auto status = query.Execute(session, NYdbHelpers::TQueryOptions{NYdb::NTable::TTxControl::Tx(tx)});
            if (!status.IsSuccess())
                return status;
        }

        {
            NYdbHelpers::TUpsertQuery<NVideoContent::TProviderUniqueItemsTableTraitsV2::TScheme> query{
                Join(snapshot, NVideoContent::TProviderUniqueItemsTableTraitsV2::NAME)};
            for (const auto& pui : puis)
                query.Add(pui);

            const auto status = query.Execute(session, NYdbHelpers::TQueryOptions{NYdb::NTable::TTxControl::Tx(tx)});
            if (!status.IsSuccess())
                return status;
        }

        return tx.Commit().GetValueSync();
    });

    if (!status.IsSuccess()) {
        LOG(ERR) << "Failed to insert tv show to the database: " << NYdbHelpers::StatusToString(status) << Endl;
        return false;
    }

    return true;
}

NYdb::TStatus CopyTables(NYdb::NTable::TTableClient& tableClient, NYdb::NScheme::TSchemeClient& schemeClient,
                         const NYdbHelpers::TTablePath& from, const NYdbHelpers::TTablePath& to,
                         std::unique_ptr<TTransaction> &transaction) {
    LOG(INFO) << "Copying tables " << from << " to " << to << "..." << Endl;

    auto transactions = std::make_unique<TTransactionsSeq>();
    transactions->Emplace<TMkDirAction>(schemeClient, to);

    const auto status =
        NYdbHelpers::ListDirectory(schemeClient, from.FullPath(), [&](const NYdb::NScheme::TSchemeEntry& entry) {
            const auto tfrom = Join(from, entry.Name);
            const auto tto = Join(to, entry.Name);
            transactions->Emplace<TCopyTableAction>(tableClient, tfrom, tto);
        });

    if (status.IsSuccess()) {
        transaction = std::make_unique<TTransaction>(std::move(transactions));
        transaction->Execute();
    }
    return status;
}

TSerialsForUpdateStatus GetSerialsForUpdate(NYdb::NTable::TTableClient& client,
                                            const NYdbHelpers::TTablePath& itemsPath,
                                            const NYdbHelpers::TTablePath& seasonsPath, TInstant timestamp) {
    TVector<TVideoItem> items;
    if (auto status = GetItemsForUpdate(client, itemsPath, timestamp, items); !status.IsSuccess())
        return TSerialsForUpdateStatus{std::move(status), std::move(items)};

    auto status = GetItemsForUpdateBySeasons(client, seasonsPath, timestamp, items);
    if (!status.IsSuccess())
        return TSerialsForUpdateStatus{std::move(status), std::move(items)};

    Sort(items, TvShowsLess);
    items.erase(std::unique(items.begin(), items.end(), TvShowsEqual), items.end());

    return TSerialsForUpdateStatus{std::move(status), std::move(items)};
}

void UpdateContentDbTvShows(TYdbContentDb& db, NYdb::NTable::TTableClient& tableClient,
                            const NYdbHelpers::TTablePath& snapshot, const TVector<TTvShowAllInfo>& infos) {
    for (const auto& info : infos) {
        Y_ASSERT(info.Success);

        const auto key = NVideoCommon::TVideoKey::TryAny(info.Item->ProviderName(), info.Item.Scheme());
        if (!key) {
            LOG(ERR) << "Failed to create key for: " << info.Item.Value().ToJsonPretty() << Endl;
            continue;
        }

        const auto idStatus = db.FindId(*key);
        if (!idStatus.IsSuccess()) {
            LOG(ERR) << "Failed to find id status: " << StatusToString(idStatus) << Endl;
            continue;
        }

        const auto& id = idStatus.Id;
        if (!id.Defined()) {
            LOG(ERR) << "No id for an item" << Endl;
            continue;
        }

        if (!UpdateTvShow(tableClient, snapshot, *id, info))
            LOG(INFO) << "Failed to update tv show" << Endl;
    }
}

} // namespace NQuickUpdater
