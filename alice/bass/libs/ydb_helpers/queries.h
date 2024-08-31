#pragma once

#include "exception.h"
#include "path.h"
#include "protobuf.h"
#include "settings.h"
#include "visitors.h"

#include <ydb/public/sdk/cpp/client/ydb_table/table.h>

#include <util/generic/string.h>
#include <util/generic/vector.h>
#include <util/stream/str.h>
#include <util/string/builder.h>
#include <util/string/printf.h>

namespace NYdbHelpers {

struct TQueryOptions {
    TQueryOptions();
    TQueryOptions(NYdb::NTable::TTxControl txControl);
    TQueryOptions(NYdb::NTable::TRetryOperationSettings retryPolicy);
    TQueryOptions(const NYdb::NTable::TTxControl& txControl, const NYdb::NTable::TRetryOperationSettings& retryPolicy);

    NYdb::NTable::TTxControl TxControl;
    NYdb::NTable::TRetryOperationSettings RetryPolicy = DefaultYdbRetrySettings();
};

struct IDataQuery {
    virtual ~IDataQuery() = default;

    virtual NYdb::TStatus Execute(NYdb::NTable::TSession& session, const TQueryOptions& options) const = 0;
};

template <typename TProto>
class TUpsertQuery : public IDataQuery {
public:
    explicit TUpsertQuery(const TTablePath& table) {
        const auto type = ProtoToStructType<TProto>();
        Query = Sprintf(R"(
                        PRAGMA TablePathPrefix("%s");
                        DECLARE $items AS "List<%s>";
                        UPSERT INTO [%s] SELECT * FROM AS_TABLE($items);
                        )",
                        table.Database.c_str(), NYdb::FormatType(type).c_str(), table.Name.c_str());
    }

    void Add(const TProto& row) {
        Rows.push_back(row);

        TProtobufSizeVisitor visitor;
        VisitProtoFields(row, visitor);
        ByteSize += visitor.Size;
    }

    size_t GetNumRows() const {
        return Rows.size();
    }

    size_t GetByteSize() const {
        return ByteSize;
    }

    bool Empty() const {
        return GetNumRows() == 0;
    }

    void Clear() {
        Rows.clear();
        ByteSize = 0;
    }

    // IDataQuery overrides:
    NYdb::TStatus Execute(NYdb::NTable::TSession& session, const TQueryOptions& options) const override {
        auto result = session.PrepareDataQuery(Query).GetValueSync();
        if (!result.IsSuccess())
            return result;

        const auto items = ProtosToList(Rows);

        auto query = result.GetQuery();
        auto params = session.GetParamsBuilder().AddParam("$items", items).Build();

        return query.Execute(options.TxControl, std::move(params)).GetValueSync();
    }

private:
    TString Query;
    TVector<TProto> Rows;
    size_t ByteSize = 0;
};

class TPreparedQuery {
public:
    TPreparedQuery(const TString& query, NYdb::NTable::TSession& session)
        : PrepareResult(session.PrepareDataQuery(query).GetValueSync())
        , Session(session) {
    }

    bool IsPrepared() const {
        return PrepareResult.IsSuccess();
    }

    const NYdb::NTable::TPrepareQueryResult& GetPrepareResult() const {
        return PrepareResult;
    }

    NYdb::NTable::TDataQueryResult Execute(const NYdb::NTable::TTxControl& txControl) {
        Y_ASSERT(IsPrepared());
        auto query = PrepareResult.GetQuery();
        return query.Execute(txControl).GetValueSync();
    }

    template <typename TParamsBuilder>
    NYdb::NTable::TDataQueryResult Execute(const NYdb::NTable::TTxControl& txControl, TParamsBuilder&& builder) {
        Y_ASSERT(IsPrepared());
        auto query = PrepareResult.GetQuery();
        auto params = builder(Session);
        return query.Execute(txControl, std::move(params)).GetValueSync();
    }

private:
    NYdb::NTable::TPrepareQueryResult PrepareResult;
    NYdb::NTable::TSession& Session;
};

} // namespace NYdbHelpers
