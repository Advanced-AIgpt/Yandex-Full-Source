#include "json_wonderlogs.h"

#include "ttls.h"

#include <alice/wonderlogs/library/common/utils.h>
#include <alice/wonderlogs/library/yt/utils.h>
#include <alice/wonderlogs/protos/wonderlogs.pb.h>

#include <google/protobuf/util/json_util.h>

using namespace NAlice::NWonderlogs;

namespace {

class TJsonWonderlogsMapper : public NYT::IMapper<NYT::TTableReader<TWonderlog>, NYT::TTableWriter<NYT::TNode>> {
public:
    class TInputOutputTables {
    public:
        TInputOutputTables(const TString& wonderlogsTable, const TString& jsonWonderlogsTable,
                           const TString& errorTable)
            : WonderlogsTable(wonderlogsTable)
            , JsonWonderlogsTable(jsonWonderlogsTable)
            , ErrorTable(errorTable) {
        }
        NYT::TMapOperationSpec AddToOperationSpec(NYT::TMapOperationSpec&& operationSpec) {
            const auto jsonWonderlogsSchema =
                NYT::TTableSchema()
                    .AddColumn(NYT::TColumnSchema().Name("json").Type(NYT::VT_STRING, /* required= */ false))
                    .AddColumn(NYT::TColumnSchema().Name("uuid").Type(NYT::VT_STRING, /* required= */ false))
                    .AddColumn(NYT::TColumnSchema().Name("message_id").Type(NYT::VT_STRING, /* required= */ false))
                    .AddColumn(
                        NYT::TColumnSchema().Name("megamind_request_id").Type(NYT::VT_STRING, /* required= */ false));

            const auto errorSchema =
                NYT::TTableSchema()
                    .AddColumn(NYT::TColumnSchema().Name("process").Type(NYT::VT_STRING, /* required= */ false))
                    .AddColumn(NYT::TColumnSchema().Name("reason").Type(NYT::VT_STRING, /* required= */ false))
                    .AddColumn(NYT::TColumnSchema().Name("message").Type(NYT::VT_STRING, /* required= */ false))
                    .AddColumn(NYT::TColumnSchema().Name("uuid").Type(NYT::VT_STRING, /* required= */ false))
                    .AddColumn(NYT::TColumnSchema().Name("message_id").Type(NYT::VT_STRING, /* required= */ false))
                    .AddColumn(
                        NYT::TColumnSchema().Name("megamind_request_id").Type(NYT::VT_STRING, /* required= */ false));

            return operationSpec.AddInput<TWonderlog>(WonderlogsTable)
                .AddOutput<NYT::TNode>(NYT::TRichYPath(JsonWonderlogsTable).Schema(jsonWonderlogsSchema))
                .AddOutput<NYT::TNode>(NYT::TRichYPath(ErrorTable).Schema(errorSchema));
        }

        enum EOutIndices {
            JsonWonderlogs = 0,
            Error = 1,
        };

    private:
        const TString& WonderlogsTable;
        const TString& JsonWonderlogsTable;
        const TString& ErrorTable;
    };

    void Do(TReader* reader, TWriter* writer) override {
        for (auto& cursor : *reader) {
            TWonderlog wonderlog = cursor.GetRow();
            const auto logError = [&wonderlog, writer](const TString& reason, const TString& message) {
                NYT::TNode error;
                error["process"] = "P_JSON_WONDERLOGS_MAPPER";
                error["reason"] = reason;
                error["message"] = message;

                if (wonderlog.HasUuid()) {
                    error["uuid"] = wonderlog.GetUuid();
                }
                if (wonderlog.HasMessageId()) {
                    error["message_id"] = wonderlog.GetMessageId();
                }
                if (wonderlog.HasMegamindRequestId()) {
                    error["megamind_request_id"] = wonderlog.GetMessageId();
                }
                if (auto setraceUrl = TryGenerateSetraceUrl(
                        {wonderlog.HasMessageId() ? wonderlog.GetMessageId() : TMaybe<TString>{},
                         wonderlog.HasMegamindRequestId() ? wonderlog.GetMegamindRequestId() : TMaybe<TString>{},
                         wonderlog.HasUuid() ? wonderlog.GetUuid() : TMaybe<TString>{}})) {
                    error["setrace_url"] = *setraceUrl;
                }
                writer->AddRow(error, TInputOutputTables::EOutIndices::Error);
            };

            NYT::TNode jsonWonderlog;
            jsonWonderlog["uuid"] = wonderlog.GetUuid();
            jsonWonderlog["message_id"] = wonderlog.GetMessageId();
            jsonWonderlog["megamind_request_id"] = wonderlog.GetMegamindRequestId();
            TString json;
            const auto status = google::protobuf::util::MessageToJsonString(wonderlog, &json);
            if (!status.ok()) {
                logError("R_FAILED_CONVERT_JSON_TO_PROTO", wonderlog.DebugString());
            }
            Y_ENSURE(status.ok());
            jsonWonderlog["json"] = json;

            writer->AddRow(jsonWonderlog, TInputOutputTables::EOutIndices::JsonWonderlogs);
        }
    }
};

REGISTER_MAPPER(TJsonWonderlogsMapper)

} // namespace

void NAlice::NWonderlogs::MakeJsonWonderlogs(NYT::IClientPtr client, const TString& /* tmpDirectory */,
                                             const TString& wonderlogs, const TString& outputTable,
                                             const TString& errorTable) {
    CreateTable(client, outputTable, /* expirationDate= */ {});
    CreateTable(client, errorTable, TInstant::Now() + MONTH_TTL);

    client->Map(TJsonWonderlogsMapper::TInputOutputTables{wonderlogs, outputTable, errorTable}.AddToOperationSpec(
                    NYT::TMapOperationSpec()),
                new TJsonWonderlogsMapper, NYT::TOperationOptions{}.InferOutputSchema(true));
}
