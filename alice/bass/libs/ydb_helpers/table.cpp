#include "table.h"

#include <google/protobuf/descriptor.h>

#include <util/generic/maybe.h>
#include <util/system/compiler.h>
#include <util/system/yassert.h>

using namespace NYdb;

namespace NYdbHelpers {

NYdb::NTable::TTxControl SerializableRW() {
    return NYdb::NTable::TTxControl::BeginTx(NYdb::NTable::TTxSettings::SerializableRW()).CommitTx();
}

NTable::TTableDescription CreateTableDescription(const TTableSchema& schema, const TVector<TString>& primaryKeys) {
    NTable::TTableBuilder builder;

    for (const auto& column : schema.Columns)
        builder.AddNullableColumn(column.Name, column.YdbType);
    builder.SetPrimaryKeyColumns(primaryKeys);

    return builder.Build();
}

NYdb::TStatus CheckTableExists(NYdb::NTable::TTableClient& client, const TTablePath& table,
                               const NYdb::NTable::TRetryOperationSettings& settings) {
    return client.RetryOperationSync(
        [&](NTable::TSession session) -> NYdb::TStatus {
            return session.DescribeTable(table.FullPath()).GetValueSync();
        },
        settings);
}

NYdb::TStatus CheckTableExistsOrFail(NYdb::NTable::TTableClient& client, const TTablePath& table,
                                     const NYdb::NTable::TRetryOperationSettings& settings) {
    return ThrowOnError(CheckTableExists(client, table, settings));
}

NYdb::TStatus CopyTable(NYdb::NTable::TTableClient& client, const TTablePath& from, const TTablePath& to,
                        const NYdb::NTable::TRetryOperationSettings& settings) {
    return client.RetryOperationSync(
        [&](NTable::TSession session) { return session.CopyTable(from.FullPath(), to.FullPath()).GetValueSync(); },
        settings);
}

NYdb::TStatus CopyTableOrFail(NYdb::NTable::TTableClient& client, const TTablePath& from, const TTablePath& to,
                              const NYdb::NTable::TRetryOperationSettings& settings) {
    return ThrowOnError(CopyTable(client, from, to, settings));
}

NYdb::TStatus DropTable(NTable::TTableClient& client, const TTablePath& table,
                        const NYdb::NTable::TRetryOperationSettings& settings) {
    return client.RetryOperationSync(
        [&](NTable::TSession session) { return session.DropTable(table.FullPath()).GetValueSync(); }, settings);
}

NYdb::TStatus DropTableOrFail(NTable::TTableClient& client, const TTablePath& table,
                              const NYdb::NTable::TRetryOperationSettings& settings) {
    return ThrowOnError(DropTable(client, table, settings));
}

NYdb::TStatus ExecuteDataQuery(NTable::TTableClient& client, const TString& query, const TQueryOptions& options) {
    return client.RetryOperationSync([&options, &query](NTable::TSession session) {
        return session.ExecuteDataQuery(query, options.TxControl).GetValueSync();
    }, options.RetryPolicy);
}

NYdb::TStatus ExecuteDataQueryOrFail(NTable::TTableClient& client, const TString& query,
                                     const TQueryOptions& options) {
    return ThrowOnError(ExecuteDataQuery(client, query, options));
}

NYdb::TStatus ExecuteDataQuery(NYdb::NTable::TTableClient& client, const IDataQuery& query,
                               const TQueryOptions& options) {
    return client.RetryOperationSync(
        [&options, &query](NTable::TSession session) { return query.Execute(session, options); }, options.RetryPolicy);
}

NYdb::TStatus ExecuteDataQueryOrFail(NYdb::NTable::TTableClient& client, const IDataQuery& query,
                                     const TQueryOptions& options) {
    return ThrowOnError(ExecuteDataQuery(client, query, options));
}

NYdb::TStatus ExecuteSelectQuery(NTable::TTableClient& client, const TString& query, TOnSelectResult resultCb,
                                 const TQueryOptions& options) {
    TMaybe<NYdb::NTable::TDataQueryResult> res;
    auto cb = [&options, &query, &res](NYdb::NTable::TSession session) {
        res.ConstructInPlace(session.ExecuteDataQuery(query, options.TxControl).GetValueSync());
        return *res;
    };

    NYdb::TStatus status{client.RetryOperationSync(cb, options.RetryPolicy)};
    if (res && resultCb) {
        resultCb(*res);
    }
    return status;
}

NYdb::TStatus ExecuteSelectQueryOrFail(NTable::TTableClient& client, const TString& query,
                                       TOnSelectResult resultHandler, const TQueryOptions& options) {
    return ThrowOnError(ExecuteSelectQuery(client, query, resultHandler, options));
}

void MakeDirectoryOrFail(NYdb::NScheme::TSchemeClient& client, const NYdbHelpers::TTablePath& path) {
    ThrowOnError(client.MakeDirectory(path.FullPath()).GetValueSync());
}

NYdb::TStatus RemoveDirectory(NYdb::NScheme::TSchemeClient& client, const NYdbHelpers::TTablePath& path) {
    return client.RemoveDirectory(path.FullPath()).GetValueSync();
}

void RemoveDirectoryWithTablesOrFail(NYdb::NTable::TTableClient& tableClient,
                                     NYdb::NScheme::TSchemeClient& schemeClient, const NYdbHelpers::TTablePath& path) {
    using EType = NYdb::NScheme::ESchemeEntryType;

    const auto status = ListDirectory(schemeClient, path.FullPath(), [&](const NYdb::NScheme::TSchemeEntry& entry) {
        const NYdbHelpers::TTablePath npath(path.Database, Join(path.Name, entry.Name));
        switch (entry.Type) {
            case EType::Directory:
                RemoveDirectoryWithTablesOrFail(tableClient, schemeClient, npath);
                break;
            case EType::Table:
                DropTableOrFail(tableClient, npath);
                break;
            case EType::Unknown:
                [[fallthrough]];
            case EType::PqGroup:
            case EType::Topic:
                [[fallthrough]];
            case EType::SubDomain:
                [[fallthrough]];
            case EType::RtmrVolume:
                [[fallthrough]];
            case EType::BlockStoreVolume:
                [[fallthrough]];
            case EType::CoordinationNode:
                [[fallthrough]];
            case EType::ColumnTable:
                [[fallthrough]];
            case EType::Sequence:
                [[fallthrough]];
            case EType::Replication:
                break;
        }
    });

    ThrowOnError(status);
    ThrowOnError(RemoveDirectory(schemeClient, path));
}

} // namespace NYdbHelpers
