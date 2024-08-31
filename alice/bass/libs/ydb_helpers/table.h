#pragma once

#include "exception.h"
#include "path.h"
#include "queries.h"
#include "settings.h"

#include <ydb/public/sdk/cpp/client/ydb_scheme/scheme.h>
#include <ydb/public/sdk/cpp/client/ydb_table/table.h>

#include <util/generic/maybe.h>
#include <util/generic/string.h>
#include <util/generic/vector.h>
#include <util/generic/noncopyable.h>

namespace google::protobuf {
class Descriptor;
} // namespace google::protobuf

namespace NYdbHelpers {

NYdb::NTable::TTxControl SerializableRW();

NYdb::NTable::TTableDescription CreateTableDescription(const TTableSchema& schema,
                                                       const TVector<TString>& primaryKeys);

template <typename TProto>
NYdb::NTable::TTableDescription CreateTableDescription(const TVector<TString>& primaryKeys) {
    return CreateTableDescription(TTableSchema{*TProto::descriptor()}, primaryKeys);
}

template <typename TTableTraits>
NYdb::NTable::TTableDescription CreateTableDescription() {
    return CreateTableDescription<typename TTableTraits::TScheme>(TTableTraits::PRIMARY_KEYS);
}

template <typename TProto>
NYdb::TStatus CreateTable(NYdb::NTable::TTableClient& client, const TTablePath& table,
                          const TVector<TString>& primaryKeys,
                          const NYdb::NTable::TRetryOperationSettings& settings = DefaultYdbRetrySettings()) {
    return client.RetryOperationSync(
        [&](NYdb::NTable::TSession session) {
            auto description = CreateTableDescription<TProto>(primaryKeys);
            return session.CreateTable(table.FullPath(), std::move(description)).GetValueSync();
        },
        settings);
}

template <typename TProto>
NYdb::TStatus CreateTableOrFail(NYdb::NTable::TTableClient& client, const TTablePath& table,
                                const TVector<TString>& primaryKeys,
                                const NYdb::NTable::TRetryOperationSettings& settings = DefaultYdbRetrySettings()) {
    return ThrowOnError(CreateTable<TProto>(client, table, primaryKeys, settings));
}

template <typename TTableTraits>
NYdb::TStatus CreateTable(NYdb::NTable::TTableClient& client, const TTablePath& table,
                          const NYdb::NTable::TRetryOperationSettings& settings = DefaultYdbRetrySettings()) {
    return client.RetryOperationSync(
        [&](NYdb::NTable::TSession session) {
            auto description = CreateTableDescription<TTableTraits>();
            return session.CreateTable(table.FullPath(), std::move(description)).GetValueSync();
        },
        settings);
}

template <typename TTableTraits>
NYdb::TStatus CreateTableOrFail(NYdb::NTable::TTableClient& client, const TTablePath& table,
                                const NYdb::NTable::TRetryOperationSettings& settings = DefaultYdbRetrySettings()) {
    return ThrowOnError(CreateTable<TTableTraits>(client, table, settings));
}

NYdb::TStatus CheckTableExists(NYdb::NTable::TTableClient& client, const TTablePath& table,
                               const NYdb::NTable::TRetryOperationSettings& settings = DefaultYdbRetrySettings());
NYdb::TStatus
CheckTableExistsOrFail(NYdb::NTable::TTableClient& client, const TTablePath& table,
                       const NYdb::NTable::TRetryOperationSettings& settings = DefaultYdbRetrySettings());

NYdb::TStatus CopyTable(NYdb::NTable::TTableClient& client, const TTablePath& from, const TTablePath& to,
                        const NYdb::NTable::TRetryOperationSettings& settings = DefaultYdbRetrySettings());
NYdb::TStatus CopyTableOrFail(NYdb::NTable::TTableClient& client, const TTablePath& from, const TTablePath& to,
                              const NYdb::NTable::TRetryOperationSettings& settings = DefaultYdbRetrySettings());

NYdb::TStatus DropTable(NYdb::NTable::TTableClient& client, const TTablePath& table,
                        const NYdb::NTable::TRetryOperationSettings& settings = DefaultYdbRetrySettings());
NYdb::TStatus DropTableOrFail(NYdb::NTable::TTableClient& client, const TTablePath& table,
                              const NYdb::NTable::TRetryOperationSettings& settings = DefaultYdbRetrySettings());

NYdb::TStatus ExecuteDataQuery(NYdb::NTable::TTableClient& client, const TString& query, const TQueryOptions& options);
NYdb::TStatus ExecuteDataQueryOrFail(NYdb::NTable::TTableClient& client, const TString& query,
                                     const TQueryOptions& txControl);

NYdb::TStatus ExecuteDataQuery(NYdb::NTable::TTableClient& client, const IDataQuery& query,
                               const TQueryOptions& txControl);
NYdb::TStatus ExecuteDataQueryOrFail(NYdb::NTable::TTableClient& client, const IDataQuery& query,
                                     const TQueryOptions& options);

using TOnSelectResult = std::function<void(NYdb::NTable::TDataQueryResult&)>;

NYdb::TStatus ExecuteSelectQuery(NYdb::NTable::TTableClient& client, const TString& query,
                                 TOnSelectResult resultHandler, const TQueryOptions& options);

NYdb::TStatus ExecuteSelectQueryOrFail(NYdb::NTable::TTableClient& client, const TString& query,
                                       TOnSelectResult resultHandler, const TQueryOptions& options);

void MakeDirectoryOrFail(NYdb::NScheme::TSchemeClient& client, const NYdbHelpers::TTablePath& path);
NYdb::TStatus RemoveDirectory(NYdb::NScheme::TSchemeClient& client, const NYdbHelpers::TTablePath& path);

void RemoveDirectoryWithTablesOrFail(NYdb::NTable::TTableClient& tableClient,
                                     NYdb::NScheme::TSchemeClient& schemeClient, const NYdbHelpers::TTablePath& path);

template <typename TFn>
NYdb::TStatus ListDirectory(NYdb::NScheme::TSchemeClient& client, const TString& path, TFn&& fn) {
    const auto result = client.ListDirectory(path).ExtractValueSync();
    if (!result.IsSuccess())
        return result;
    for (const auto& entry : result.GetChildren())
        fn(entry);
    return result;
}

template <typename TFn>
NYdb::TStatus ListTables(NYdb::NScheme::TSchemeClient& client, const TString& path, TFn&& fn) {
    return ListDirectory(client, path, [&](const NYdb::NScheme::TSchemeEntry& entry) {
        if (entry.Type == NYdb::NScheme::ESchemeEntryType::Table)
            fn(entry.Name);
    });
}

// Query must contains only one select statement.
template <typename TParamsBuilder, typename TOnParser>
NYdb::TStatus ExecuteSingleSelectDataQuery(NYdb::NTable::TSession& session, const TString& query,
                                           TParamsBuilder&& paramsBuilder, TOnParser&& onParser) {
    NYdbHelpers::TPreparedQuery q{query, session};
    if (!q.IsPrepared())
        return q.GetPrepareResult();

    const auto status = q.Execute(NYdbHelpers::SerializableRW(), std::forward<TParamsBuilder>(paramsBuilder));
    if (!status.IsSuccess())
        return status;

    Y_ASSERT(status.GetResultSets().size() == 1);

    auto parser = status.GetResultSetParser(0);

    onParser(parser);

    if (parser.TryNextRow())
        LOG(WARNING) << "Not all result rows from ydb were processed by onParser method." << Endl;

    return status;
}

template <typename TParamsBuilder, typename TOnParser>
NYdb::TStatus ExecuteSingleSelectDataQuery(NYdb::NTable::TTableClient& client, const TString& query,
                                           TParamsBuilder&& paramsBuilder, TOnParser&& onParser) {
    return RetrySyncWithDefaultSettings(client, [&](NYdb::NTable::TSession session) -> NYdb::TStatus {
        return ExecuteSingleSelectDataQuery(session, query, std::forward<TParamsBuilder>(paramsBuilder),
                                            std::forward<TOnParser>(onParser));
    });
}

template <typename TFn>
NYdb::TStatus ListDirectories(NYdb::NScheme::TSchemeClient& client, const TString& path, TFn&& fn) {
    return ListDirectory(client, path, [&](const NYdb::NScheme::TSchemeEntry& entry) {
        if (entry.Type == NYdb::NScheme::ESchemeEntryType::Directory)
            fn(entry.Name);
    });
}

template <typename TProto>
class TTableWriter {
public:
    static constexpr size_t BATCH_NUM_ROWS = 10000;
    static constexpr size_t BATCH_BYTE_SIZE = 1000000;

    TTableWriter(NYdb::NTable::TTableClient& client, const TTablePath& table, const TQueryOptions& options)
        : Client{client}
        , Query{table}
        , QueryOptions{options} {
    }

    TTableWriter(NYdb::NTable::TTableClient& client, const TTablePath& table)
        : Client{client}
        , Query{table} {
    }

    ~TTableWriter() {
        Flush(true /* force */);
    }

    void AddRow(const TProto& row) {
        Query.Add(row);
        Flush(false /* force */);
    }

    NYdb::TStatus Flush() {
        if (Query.Empty()) {
            return NYdb::TStatus{NYdb::EStatus::SUCCESS, {} /* issues */};
        }

        auto status = ExecuteDataQueryOrFail(Client, Query, QueryOptions);
        Query.Clear();
        return status;
    }

private:
    void Flush(bool force) {
        if (Query.Empty())
            return;
        if (!force && Query.GetNumRows() < BATCH_NUM_ROWS && Query.GetByteSize() < BATCH_BYTE_SIZE)
            return;
        Flush();
    }

    NYdb::NTable::TTableClient& Client;
    TUpsertQuery<TProto> Query;

    TQueryOptions QueryOptions;
};

class TScopedDirectory : private TMoveOnly {
public:
    TScopedDirectory(TScopedDirectory&& directory)
        : Client_(directory.Client_)
        , Path_(directory.Path_)
        , Release_(directory.Release_) {
        directory.Release();
    }

    ~TScopedDirectory() {
        if (!Release_)
            RemoveDirectory(Client_, Path_);
    }

    TScopedDirectory& operator=(TScopedDirectory&&) noexcept = delete;

    static TScopedDirectory Make(NYdb::NScheme::TSchemeClient& client, const NYdbHelpers::TTablePath& path) {
        MakeDirectoryOrFail(client, path);
        return {client, path};
    }

    const NYdbHelpers::TTablePath& Path() const {
        return Path_;
    }

    void Release() {
        Release_ = true;
    }

private:
    TScopedDirectory(NYdb::NScheme::TSchemeClient& client, const NYdbHelpers::TTablePath& path)
        : Client_(client)
        , Path_(path) {
    }

private:
    NYdb::NScheme::TSchemeClient& Client_;
    NYdbHelpers::TTablePath Path_;

    bool Release_ = false;
};

class TScopedTable : private TMoveOnly {
public:
    TScopedTable(TScopedTable&& table)
        : Client_(table.Client_)
        , Path_(table.Path_)
        , Release_(table.Release_) {
        table.Release();
    }

    ~TScopedTable() {
        if (!Release_)
            DropTable(Client_, Path_);
    }

    TScopedTable& operator=(TScopedTable&&) noexcept = delete;

    template <typename TProto>
    static TScopedTable Make(NYdb::NTable::TTableClient& client, const NYdbHelpers::TTablePath& path,
                             const TVector<TString>& primaryKeys) {
        CreateTableOrFail<TProto>(client, path, primaryKeys);
        return {client, path};
    }

    template <typename TTableTraits>
    static TScopedTable Make(NYdb::NTable::TTableClient& client, const NYdbHelpers::TTablePath& path) {
        CreateTableOrFail<TTableTraits>(client, path);
        return {client, path};
    }

    const NYdbHelpers::TTablePath& Path() const {
        return Path_;
    }

    void Release() {
        Release_ = true;
    }

private:
    TScopedTable(NYdb::NTable::TTableClient& client, const NYdbHelpers::TTablePath& path)
        : Client_(client)
        , Path_(path) {
    }

private:
    NYdb::NTable::TTableClient& Client_;
    NYdbHelpers::TTablePath Path_;

    bool Release_ = false;
};

} // namespace NYdbHelpers
