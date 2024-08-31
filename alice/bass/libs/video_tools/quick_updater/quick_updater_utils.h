#pragma once

#include <alice/bass/libs/video_common/defs.h>
#include <alice/bass/libs/video_common/keys.h>
#include <alice/bass/libs/video_common/utils.h>
#include <alice/bass/libs/video_common/content_db.h>
#include <alice/bass/libs/ydb_helpers/table.h>

#include <ydb/public/sdk/cpp/client/ydb_table/table.h>

#include <util/generic/maybe.h>
#include <util/generic/strbuf.h>
#include <util/generic/string.h>

#include <utility>

namespace NQuickUpdater {

struct TKeyedSerialDescriptor {
    TKeyedSerialDescriptor() = default;

    template <typename TKey, typename TDescr>
    TKeyedSerialDescriptor(TKey&& key, TDescr&& descr)
        : Key(std::forward<TKey>(key))
        , Descr(std::forward<TDescr>(descr)) {
    }

    NVideoCommon::TSerialKey Key;
    NVideoCommon::TSerialDescriptor Descr;
};

struct TKeyedSeasonDescriptor {
    TKeyedSeasonDescriptor() = default;

    template <typename TKey, typename TDescr>
    TKeyedSeasonDescriptor(TKey&& key, TDescr&& descr)
        : Key(std::forward<TKey>(key))
        , Descr(std::forward<TDescr>(descr)) {
    }

    NVideoCommon::TSeasonKey Key;
    NVideoCommon::TSeasonDescriptor Descr;
};

struct TTvShowAllInfo {
    TTvShowAllInfo() = default;

    explicit TTvShowAllInfo(const NVideoCommon::TVideoItem& item)
        : Item(item)
    {
    }

    TTvShowAllInfo(const NVideoCommon::TVideoItem& item, const NVideoCommon::TSerialDescriptor& serialDescr);

    NVideoCommon::TVideoItem Item;
    TKeyedSerialDescriptor SerialDescr;
    TVector<TKeyedSeasonDescriptor> SeasonDescrs;
    bool Success = true;
};

struct ITransactionAction {
    virtual ~ITransactionAction() = default;
    virtual void Execute() = 0;
    virtual void Rollback() noexcept = 0;
    virtual void Commit()  = 0;
};

class TTransaction {
public:
    explicit TTransaction(std::unique_ptr<ITransactionAction>&& action)
        : Action{std::move(action)}
    {
    }

    TTransaction(TTransaction&& rhs) = default;

    ~TTransaction() {
        if (NeedsRollback)
            Action->Rollback();
    }

    template <typename TAction, typename... Args>
    static TTransaction MakeTransaction(Args&&... args) {
        return TTransaction{std::make_unique<TAction>(std::forward<Args>(args)...)};
    }

    void Execute() {
        if (!Action) {
            LOG(ERR) << "Trying to execute an empty transaction!" << Endl;
            return;
        }

        NeedsRollback = true;
        Action->Execute();
    }

    void Commit() {
        if (!Action) {
            LOG(ERR) << "Trying to commit an empty transaction!" << Endl;
            return;
        }
        Action->Commit();
        NeedsRollback = false;
    }

    void Rollback() {
        if (NeedsRollback) {
            Action->Rollback();
            NeedsRollback = false;
        }
    }

private:
    std::unique_ptr<ITransactionAction> Action;
    bool NeedsRollback = false;
};


TVector<TTvShowAllInfo> DownloadAll(const TVector<NVideoCommon::TVideoItem>& items,
                                    NVideoCommon::IContentInfoProvidersCache& providers);

class TSerialsForUpdateStatus : public NYdb::TStatus {
public:
    template <typename TItems>
    TSerialsForUpdateStatus(NYdb::TStatus&& status, TItems&& items)
        : NYdb::TStatus(std::move(status))
        , Items(std::forward<TItems>(items)) {
    }

    const TVector<NVideoCommon::TVideoItem>& GetItems() const {
        return Items;
    }

private:
    TVector<NVideoCommon::TVideoItem> Items;
};

TSerialsForUpdateStatus GetSerialsForUpdate(NYdb::NTable::TTableClient& client,
                                            const NYdbHelpers::TTablePath& itemsPath,
                                            const NYdbHelpers::TTablePath& seasonsPath, TInstant timestamp);

TMaybe<TString> GetNextSnapshotName(TStringBuf curr);

NYdb::TStatus CopyTables(NYdb::NTable::TTableClient& tableClient, NYdb::NScheme::TSchemeClient& schemeClient,
                         const NYdbHelpers::TTablePath& from, const NYdbHelpers::TTablePath& to,
                         std::unique_ptr<TTransaction> &transaction);

bool UpdateTvShow(NYdb::NTable::TTableClient& client, const NYdbHelpers::TTablePath& snapshot, ui64 id,
                  const TTvShowAllInfo& info);

void UpdateContentDbTvShows(NVideoCommon::TYdbContentDb& db, NYdb::NTable::TTableClient& tableClient,
                            const NYdbHelpers::TTablePath& snapshot, const TVector<TTvShowAllInfo>& infos);

} // namespace NQuickUpdater
