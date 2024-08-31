#include "dialogs.h"

#include "ttls.h"

#include <alice/wonderlogs/library/builders/dialogs.h>
#include <alice/wonderlogs/library/common/utils.h>
#include <alice/wonderlogs/library/yt/utils.h>
#include <alice/wonderlogs/protos/megamind_prepared.pb.h>
#include <alice/wonderlogs/protos/private_user.pb.h>
#include <alice/wonderlogs/protos/request_stat.pb.h>
#include <alice/wonderlogs/protos/uniproxy_prepared.pb.h>
#include <alice/wonderlogs/sdk/utils/speechkit_utils.h>

#include <alice/megamind/library/handlers/utils/logs_util.h>
#include <alice/megamind/library/response/utils.h>

#include <library/cpp/json/yson/json2yson.h>
#include <library/cpp/protobuf/yt/proto2yt.h>
#include <library/cpp/threading/future/async.h>
#include <library/cpp/threading/future/future.h>
#include <library/cpp/threading/future/subscription/wait_all_or_exception.h>

#include <mapreduce/yt/library/operation_tracker/operation_tracker.h>

#include <util/charset/unidata.h>
#include <util/charset/utf8.h>
#include <util/generic/size_literals.h>
#include <util/string/builder.h>

#include <algorithm>
#include <random>

namespace NAlice::NWonderlogs::NImpl {

TIds TIds::Generate(const ui64 seed) {
    std::mt19937_64 generator(seed);
    std::uniform_int_distribution<ui64> distribution;
    const auto generateGuid = [](const ui64 num1, const ui64 num2) {
        TGUID guid;

        ui64* dw = reinterpret_cast<ui64*>((&guid)->dw);

        WriteUnaligned<ui64>(&dw[0], num1);
        WriteUnaligned<ui64>(&dw[1], num2);
        return guid;
    };

    return {.Puid = ToString(distribution(generator)),
            .Uuid = GetGuidAsString(generateGuid(distribution(generator), distribution(generator))),
            .DeviceId = GetGuidAsString(generateGuid(distribution(generator), distribution(generator)))};
}

} // namespace NAlice::NWonderlogs::NImpl

namespace {

using namespace NAlice::NWonderlogs;

class TDialogsMapper : public NYT::IMapper<NYT::TTableReader<TWonderlog>, NYT::TTableWriter<NYT::TNode>> {
public:
    class TInputOutputTables {
    public:
        TInputOutputTables(const TString& wonderlogsTable, const TString& dialogsTable,
                           const TString& bannedDialogsTable, const TString& errorTable)
            : WonderlogsTable(wonderlogsTable)
            , DialogsTable(dialogsTable)
            , BannedDialogsTable(bannedDialogsTable)
            , ErrorTable(errorTable) {
        }
        NYT::TMapOperationSpec AddToOperationSpec(NYT::TMapOperationSpec&& operationSpec) {
            // https://a.yandex-team.ru/arc/trunk/arcadia/alice/logs/copy_dialog_logs/lib/dialog_operations.py?rev=7003164#L48

            const auto errorSchema =
                NYT::TTableSchema()
                    .AddColumn(NYT::TColumnSchema().Name("process").Type(NYT::VT_STRING, /* required= */ false))
                    .AddColumn(NYT::TColumnSchema().Name("reason").Type(NYT::VT_STRING, /* required= */ false))
                    .AddColumn(NYT::TColumnSchema().Name("message").Type(NYT::VT_STRING, /* required= */ false))
                    .AddColumn(NYT::TColumnSchema().Name("uuid").Type(NYT::VT_STRING, /* required= */ false))
                    .AddColumn(NYT::TColumnSchema().Name("message_id").Type(NYT::VT_STRING, /* required= */ false))
                    .AddColumn(
                        NYT::TColumnSchema().Name("megamind_request_id").Type(NYT::VT_STRING, /* required= */ false))
                    .AddColumn(NYT::TColumnSchema().Name("setrace_url").Type(NYT::VT_STRING, /* required= */ false));

            const auto addAttributes = [](NYT::TRichYPath&& path) -> NYT::TRichYPath {
                return path.CompressionCodec("brotli_8")
                    .ErasureCodec(NYT::EErasureCodecAttr::EC_LRC_12_2_2_ATTR)
                    .OptimizeFor(NYT::EOptimizeForAttr::OF_SCAN_ATTR);
            };
            return operationSpec.AddInput<TWonderlog>(WonderlogsTable)
                .AddOutput<NYT::TNode>(addAttributes(NYT::TRichYPath(DialogsTable)).Schema(NImpl::DIALOGS_SCHEMA))
                .AddOutput<NYT::TNode>(
                    addAttributes(NYT::TRichYPath(BannedDialogsTable)).Schema(NImpl::DIALOGS_SCHEMA))
                .AddOutput<NYT::TNode>(NYT::TRichYPath(ErrorTable).Schema(errorSchema));
        }

        enum EOutIndices {
            Dialogs = 0,
            BannedDialogs = 1,
            Error = 2,
        };

    private:
        const TString& WonderlogsTable;
        const TString& DialogsTable;
        const TString& BannedDialogsTable;
        const TString& ErrorTable;
    };

    Y_SAVELOAD_JOB(BannedUsers, ProductionEnvironment);
    TDialogsMapper() = default;
    TDialogsMapper(TBannedUsers bannedUsers, TMaybe<TEnvironment> productionEnvironment)
        : BannedUsers(std::move(bannedUsers))
        , ProductionEnvironment(std::move(productionEnvironment)) {
    }

    void Do(TReader* reader, TWriter* writer) override {
        const auto logErrors = [writer](const TDialogsBuilder::TErrors& errors) {
            for (const auto& error : errors) {
                writer->AddRow(error, TInputOutputTables::EOutIndices::Error);
            }
        };

        for (auto& cursor : *reader) {
            TDialogsBuilder builder(ProductionEnvironment);
            const TWonderlog& wonderlog = cursor.GetRow();
            logErrors(builder.AddWonderlog(wonderlog));
            if (!builder.Valid()) {
                continue;
            }
            auto dialog = std::move(builder).Build();
            if (BannedUsers.Banned(wonderlog)) {
                writer->AddRow(dialog, TInputOutputTables::EOutIndices::BannedDialogs);
            } else {
                writer->AddRow(dialog, TInputOutputTables::EOutIndices::Dialogs);
            }
        }
    }

private:
    TBannedUsers BannedUsers;
    TMaybe<TEnvironment> ProductionEnvironment;
};

class TPrivateDialogsMapper : public NYT::IMapper<NYT::TTableReader<NYT::TNode>, NYT::TTableWriter<NYT::TNode>> {
public:
    class TInputOutputTables {
    public:
        TInputOutputTables(const TString& dialogsTable, const TString& outputTable, NYT::TNode schemaNode)
            : DialogsTable(dialogsTable)
            , OutputTable(outputTable) {
            for (auto& column : schemaNode.AsList()) {
                if (column.HasKey("sort_order")) {
                    column.AsMap().erase("sort_order");
                }
            }
            NYT::Deserialize(Schema, schemaNode);
        }
        NYT::TMapOperationSpec AddToOperationSpec(NYT::TMapOperationSpec&& operationSpec) {
            return operationSpec.AddInput<NYT::TNode>(DialogsTable)
                .AddOutput<NYT::TNode>(NYT::TRichYPath(OutputTable)
                                           .Schema(Schema)
                                           .CompressionCodec("brotli_8")
                                           .ErasureCodec(NYT::EErasureCodecAttr::EC_LRC_12_2_2_ATTR)
                                           .OptimizeFor(NYT::EOptimizeForAttr::OF_SCAN_ATTR));
        }

    private:
        const TString& DialogsTable;
        const TString& OutputTable;
        NYT::TTableSchema Schema;
    };

    Y_SAVELOAD_JOB(PrivateUsers);
    TPrivateDialogsMapper() = default;
    explicit TPrivateDialogsMapper(THashMap<TString, i64> privateUsers)
        : PrivateUsers(std::move(privateUsers)) {
    }

    void Do(TReader* reader, TWriter* writer) override {
        for (auto& cursor : *reader) {
            auto dialog = cursor.GetRow();
            const auto puid = [](const NYT::TNode& dialog) -> NYT::TNode {
                if (dialog.HasKey("puid")) {
                    return dialog["puid"];
                }
                if (dialog.HasKey("request") && dialog["request"].IsMap()) {
                    auto& request = dialog["request"];
                    if (request.HasKey("additional_options") && request["additional_options"].IsMap() &&
                        request["additional_options"].HasKey("puid")) {
                        return request["additional_options"]["puid"];
                    }
                }
                return NYT::TNode{""};
            }(dialog);
            const auto uuid = [](const NYT::TNode& dialog) -> NYT::TNode {
                if (dialog.HasKey("uuid")) {
                    return dialog["uuid"];
                }
                if (dialog.HasKey("request") && dialog["request"].IsMap()) {
                    auto& request = dialog["request"];
                    if (request.HasKey("uuid")) {
                        return request["uuid"];
                    }
                }
                return NYT::TNode{""};
            }(dialog);
            const auto deviceId = [](const NYT::TNode& dialog) -> NYT::TNode {
                if (dialog.HasKey("device_id")) {
                    return dialog["device_id"];
                }
                if (dialog.HasKey("request") && dialog["request"].IsMap()) {
                    auto& request = dialog["request"];
                    if (request.HasKey("device_id")) {
                        return request["device_id"];
                    }
                }
                return NYT::TNode{""};
            }(dialog);
            if (puid.IsString() && uuid.IsString() && deviceId.IsString()) {
                if (const auto* privateUntilTimeMs = PrivateUsers.FindPtr(puid.AsString())) {
                    const ui64 seed =
                        *privateUntilTimeMs + HashStringToUi64(puid.AsString()) + HashStringToUi64(uuid.AsString());
                    const auto ids = NImpl::TIds::Generate(seed);

                    NImpl::ChangeIds(dialog, ids.Puid, ids.Uuid, "uu/" + NormalizeUuid(ids.Uuid), ids.DeviceId);
                }
            }
            writer->AddRow(dialog);
        }
    }

private:
    THashMap<TString, i64> PrivateUsers;
};

void CensorDialogsTable(NYT::IClientPtr client, const TString& tmpDirectory, const TString& dialogsTable,
                        const THashMap<TString, i64>& privateUsers, const TString& outputTable) {
    auto tx = client->StartTransaction();

    CreateTable(tx, outputTable, /* expirationDate= */ {});

    const auto dialogsTmpTableUnsorted =
        CreateRandomTable(tx, tmpDirectory, "dialogs-unsorted", /* tempTable= */ true, /* ttl= */ TDuration::Days(2));
    const auto schema = tx->Get(dialogsTable + "/@schema");

    {
        NYT::TNode spec;
        spec["job_io"]["table_writer"]["max_row_weight"] = 32_MB;
        tx->Map(TPrivateDialogsMapper::TInputOutputTables{dialogsTable, dialogsTmpTableUnsorted, schema}
                    .AddToOperationSpec(NYT::TMapOperationSpec{})
                    .MapperSpec(NYT::TUserJobSpec{}.MemoryLimit(1_GB)),
                new TPrivateDialogsMapper{privateUsers}, NYT::TOperationOptions{}.Spec(spec));
    }

    auto outputTablePath = NYT::TRichYPath(outputTable)
                               .CompressionCodec("brotli_8")
                               .ErasureCodec(NYT::EErasureCodecAttr::EC_LRC_12_2_2_ATTR)
                               .OptimizeFor(NYT::EOptimizeForAttr::OF_SCAN_ATTR);

    const auto sortColumns = [](const NYT::TNode& schema) {
        TVector<TString> columns;
        for (const auto& column : schema.AsList()) {
            if (column.HasKey("sort_order")) {
                columns.push_back(column["name"].AsString());
            } else {
                break;
            }
        }
        return columns;
    }(schema);
    if (!sortColumns.empty()) {
        tx->Sort(
            NYT::TSortOperationSpec{}.AddInput(dialogsTmpTableUnsorted).Output(outputTablePath).SortBy(sortColumns));
    } else {
        tx->Move(dialogsTmpTableUnsorted, outputTable, NYT::TMoveOptions{}.Recursive(true).Force(true));
    }
    {
        const auto attributes = tx->Get(outputTablePath.Path_ + "/@");
        const TMergeAttributes mergeAttribute{
            attributes["compression_ratio"].AsDouble(),
            attributes.HasKey("data_weight") ? attributes["data_weight"].AsInt64() : TMaybe<ui64>{},
            attributes.HasKey("compressed_data_size") ? attributes["compressed_data_size"].AsInt64() : TMaybe<ui64>{},
            attributes["erasure_codec"].AsString()};

        const auto mergeMode = sortColumns.empty() ? NYT::EMergeMode::MM_UNORDERED : NYT::EMergeMode::MM_SORTED;
        NYT::TNode spec;
        spec["combine_chunks"] = true;
        spec["force_transform"] = true;
        spec["data_size_per_job"] = mergeAttribute.DataSizePerJob;
        spec["job_io"]["table_writer"]["desired_chunk_size"] = mergeAttribute.DesiredChunkSize;
        tx->Merge(NYT::TMergeOperationSpec{}.AddInput(outputTablePath).Output(outputTablePath).Mode(mergeMode),
                  NYT::TOperationOptions().Spec(spec));
    }
    tx->Commit();
}

REGISTER_MAPPER(TDialogsMapper)
REGISTER_MAPPER(TPrivateDialogsMapper)

} // namespace

void NAlice::NWonderlogs::MakeDialogs(NYT::IClientPtr client, const TString& tmpDirectory, const TString& wonderlogs,
                                      const TString& outputTable, const TString& bannedTable,
                                      const TString& errorTable, const TBannedUsers& bannedUsers,
                                      const TMaybe<TEnvironment>& productionEnvironment) {
    auto tx = client->StartTransaction();

    CreateTable(tx, outputTable, /* expirationDate= */ {});
    CreateTable(tx, bannedTable, /* expirationDate= */ {});
    const auto dialogsTmpTableUnsorted = CreateRandomTable(tx, tmpDirectory, "dialogs-unsorted");
    const auto bannedDialogsTmpTableUnsorted = CreateRandomTable(tx, tmpDirectory, "banned-dialogs-unsorted");
    CreateTable(tx, errorTable, TInstant::Now() + MONTH_TTL);

    tx->Map(TDialogsMapper::TInputOutputTables{wonderlogs, dialogsTmpTableUnsorted, bannedDialogsTmpTableUnsorted,
                                               errorTable}
                .AddToOperationSpec(NYT::TMapOperationSpec{})
                .Ordered(true)
                .MapperSpec(NYT::TUserJobSpec{}.MemoryLimit(1_GB).MemoryReserveFactor(14)),
            new TDialogsMapper(bannedUsers, productionEnvironment), NYT::TOperationOptions{});

    {
        NYT::TOperationTracker tracker;
        for (const auto& [unsortedDialogs, sortedDialogs] :
             {std::tie(dialogsTmpTableUnsorted, outputTable), std::tie(bannedDialogsTmpTableUnsorted, bannedTable)}) {
            auto outputTablePath = NYT::TRichYPath(sortedDialogs)
                                       .CompressionCodec("brotli_8")
                                       .ErasureCodec(NYT::EErasureCodecAttr::EC_LRC_12_2_2_ATTR)
                                       .OptimizeFor(NYT::EOptimizeForAttr::OF_SCAN_ATTR);

            tracker.AddOperation(tx->Sort(NYT::TSortOperationSpec{}
                                              .AddInput(unsortedDialogs)
                                              .Output(outputTablePath)
                                              .SortBy({"server_time_ms", "uuid", "sequence_number"}),
                                          NYT::TOperationOptions{}.Wait(false)));
        }
        tracker.WaitAllCompleted();
    }

    tx->Commit();
}

TMaybe<bool> NImpl::DoNotUseUserLogs(TMaybe<bool> doNotUseUserLogs, TMaybe<bool> prohibitedByRegion) {
    if ((doNotUseUserLogs && *doNotUseUserLogs) || (prohibitedByRegion && *prohibitedByRegion)) {
        return true;
    }
    return doNotUseUserLogs;
}

void NImpl::ChangeIds(NYT::TNode& dialog, const TString& newPuid, const TString& newUuid,
                      const TString& newNormalizedUuid, const TString& newDeviceId) {
    if (dialog.IsMap() && dialog.HasKey("puid")) {
        dialog["puid"] = newPuid;
    }
    if (dialog.IsMap() && dialog.HasKey("uuid")) {
        dialog["uuid"] = newNormalizedUuid;
    }
    if (dialog.IsMap() && dialog.HasKey("device_id")) {
        dialog["device_id"] = newDeviceId;
    }

    if (dialog.IsMap() && dialog.HasKey("request") && dialog["request"].IsMap()) {
        auto& request = dialog["request"];
        if (request.HasKey("uuid")) {
            request["uuid"] = newUuid;
        }
        if (request.HasKey("device_id")) {
            request["device_id"] = newDeviceId;
        }
        if (request.HasKey("additional_options") && request["additional_options"].IsMap() &&
            request["additional_options"].HasKey("puid")) {
            request["additional_options"]["puid"] = newPuid;
        }
    }
}

bool TBannedUsers::Banned(const TWonderlog& wonderlog) const {
    if (Ips.contains(wonderlog.GetDownloadingInfo().GetMegamind().GetClientIp())) {
        return true;
    }
    if (Ips.contains(wonderlog.GetDownloadingInfo().GetUniproxy().GetClientIp())) {
        return true;
    }
    if (Uuids.contains(wonderlog.GetUuid())) {
        return true;
    }
    if (DeviceIds.contains(wonderlog.GetSpeechkitRequest().GetApplication().GetDeviceId())) {
        return true;
    }
    if (DeviceIds.contains(wonderlog.GetClient().GetApplication().GetDeviceId())) {
        return true;
    }
    return false;
}

void NAlice::NWonderlogs::CensorDialogs(NYT::IClientPtr client, const TString& tmpDirectory,
                                        const TVector<TString>& dialogsTables, const TVector<TString>& outputTables,
                                        const TString& privateUsers, size_t threadCount) {
    auto threads = CreateThreadPool(threadCount);

    THashMap<TString, i64> privateUsersMap;
    {
        auto reader = client->CreateTableReader<TPrivateUser>(privateUsers);
        for (auto& cursor : *reader) {
            const auto& privateUser = cursor.GetRow();
            privateUsersMap[privateUser.GetPuid()] = privateUser.GetPrivateUntilTimeMs();
        }
    }

    TVector<NThreading::TFuture<void>> futures;
    {
        size_t count = dialogsTables.size();
        for (size_t i = 0; i < count; ++i) {
            futures.push_back(NThreading::Async(
                [client, &tmpDirectory, &dialogsTables, &outputTables, &privateUsersMap, i]() {
                    CensorDialogsTable(client, tmpDirectory, dialogsTables[i], privateUsersMap, outputTables[i]);
                },
                *threads));
        }
    }

    NThreading::NWait::WaitAllOrException(futures).Wait();
}

void NAlice::NWonderlogs::MakeDialogsMultiple(
    NYT::IClientPtr client, const TString& tmpDirectory, const TVector<TString>& wonderlogsTables,
    const TVector<TString>& outputTables, const TVector<TString>& bannedTables, const TVector<TString>& errorTables,
    size_t threadCount, const TBannedUsers& bannedUsers, const TMaybe<TEnvironment>& productionEnvironment) {
    auto threads = CreateThreadPool(threadCount);

    TVector<NThreading::TFuture<void>> futures;
    {
        size_t count = wonderlogsTables.size();
        for (size_t i = 0; i < count; ++i) {
            futures.push_back(NThreading::Async(
                [client, &tmpDirectory, &wonderlogsTables, &outputTables, &bannedTables, &errorTables, &bannedUsers,
                 &productionEnvironment, i]() {
                    MakeDialogs(client, tmpDirectory, wonderlogsTables[i], outputTables[i], bannedTables[i],
                                errorTables[i], bannedUsers, productionEnvironment);
                    {
                        auto tx = client->StartTransaction();
                        const auto attributes = tx->Get(outputTables[i] + "/@");
                        const TMergeAttributes mergeAttribute{
                            attributes["compression_ratio"].AsDouble(),
                            attributes.HasKey("data_weight") ? attributes["data_weight"].AsInt64() : TMaybe<ui64>{},
                            attributes.HasKey("compressed_data_size") ? attributes["compressed_data_size"].AsInt64()
                                                                      : TMaybe<ui64>{},
                            attributes["erasure_codec"].AsString()};

                        NYT::TNode spec;
                        spec["combine_chunks"] = true;
                        spec["force_transform"] = true;
                        spec["data_size_per_job"] = mergeAttribute.DataSizePerJob;
                        spec["job_io"]["table_writer"]["desired_chunk_size"] = mergeAttribute.DesiredChunkSize;
                        tx->Merge(NYT::TMergeOperationSpec{}
                                      .AddInput(outputTables[i])
                                      .Output(outputTables[i])
                                      .Mode(NYT::EMergeMode::MM_SORTED),
                                  NYT::TOperationOptions().Spec(spec));
                        tx->Commit();
                    }
                },
                *threads));
        }
    }

    NThreading::NWait::WaitAllOrException(futures).Wait();
}
