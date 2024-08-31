#include "service_instance_repository.h"

#include <util/string/printf.h>
#include <util/string/subst.h>

namespace NRTLog {
    using namespace NYdb;
    using namespace NYdb::NTable;
    using namespace NThreading;
    using namespace NRTLogEvents;

    TServiceInstanceRepository::TServiceInstanceRepository(NYdb::NTable::TTableClient tableClient, const TString& database)
        : TableClient(std::move(tableClient))
        , Query(ReplaceAll(R"(
                    DECLARE $service_name AS String;
                    DECLARE $host_name AS String;

                    $existing_id = (SELECT id
                                    FROM [{name_to_id_table}]
                                    WHERE service_name = $service_name AND host_name = $host_name);
                    $max_id = (SELECT MAX(id) FROM [{id_to_name_table}]);
                    $new_id = ($max_id ?? 0) + 1;
                    $to_insert = (SELECT
                                      $service_name AS service_name,
                                      $host_name AS host_name,
                                      $new_id AS id);

                    UPSERT INTO [{name_to_id_table}]
                    SELECT * FROM $to_insert WHERE $existing_id IS NULL;

                    UPSERT INTO [{id_to_name_table}]
                    SELECT * FROM $to_insert WHERE $existing_id IS NULL;

                    SELECT Unwrap(IF($existing_id IS NOT NULL, $existing_id, $new_id)) AS id;
    )", std::initializer_list<std::pair<TString, TString>> {
            {"{name_to_id_table}", database + "/instance_name_to_id"},
            {"{id_to_name_table}", database + "/instance_id_to_name"}
        }))
    {
    }

    TServiceInstanceRepository::TServiceInstanceRepository(const TYdbSettings& ydbSettings)
        : TServiceInstanceRepository(TTableClient(TDriver(ToDriverConfig(ydbSettings))), ydbSettings.Database)
    {
    }

    ui64 TServiceInstanceRepository::Resolve(const TInstanceDescriptor& instance) {
        static const auto ydbClientTimeout = TDuration::Seconds(20);

        TMaybe <ui64> result;
        const auto queryStatus = TableClient.RetryOperationSync([&](TSession s) -> TStatus {
            auto params = TParamsBuilder()
                .AddParam("$service_name")
                    .String(instance.GetServiceName())
                    .Build()
                .AddParam("$host_name")
                    .String(instance.GetHostName())
                    .Build()
                .Build();

            auto prepareResult = s.PrepareDataQuery(Query,
                                                    TPrepareDataQuerySettings().ClientTimeout(ydbClientTimeout))
                                  .ExtractValueSync();
            if (!prepareResult.IsSuccess()) {
                return prepareResult;
            }
            const auto queryResult = prepareResult.GetQuery().Execute(
                        TTxControl::BeginTx(TTxSettings::SerializableRW()).CommitTx(),
                        std::move(params),
                        TExecDataQuerySettings().ClientTimeout(ydbClientTimeout)).ExtractValueSync();
            if (queryResult.IsSuccess()) {
                TResultSetParser parser = queryResult.GetResultSetParser(0);
                TValueParser& columnParser = parser.ColumnParser(0);
                Y_VERIFY(parser.TryNextRow());
                result = columnParser.GetUint64();
            }
            return queryResult;
        }, TRetryOperationSettings().GetSessionClientTimeout(ydbClientTimeout));
        Y_ENSURE(queryStatus.IsSuccess(),
                 Sprintf("failed to resolve instance id, status [%lu], issues [%s], is transport error [%d]",
                         static_cast<size_t>(queryStatus.GetStatus()),
                         queryStatus.GetIssues().ToString().c_str(),
                         queryStatus.IsTransportError()));
        Y_VERIFY(result.Defined());
        return *result;
    }
}
