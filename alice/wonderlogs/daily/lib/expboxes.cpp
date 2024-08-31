#include "expboxes.h"

#include "ttls.h"

#include <alice/wonderlogs/library/builders/dialogs.h>
#include <alice/wonderlogs/library/yt/utils.h>

#include <mapreduce/yt/library/operation_tracker/operation_tracker.h>

#include <util/generic/size_literals.h>

namespace {

const THashSet<TString> BANNED_AUTO_PHRASES = {"привет",        "пробки",        "погода", "появился интернет",
                                               "долго не было", "долго за рулем"};

const auto DIALOGS_SCHEMA =
    NYT::TTableSchema()
        .AddColumn(NYT::TColumnSchema()
                       .Name("uuid")
                       .Type(NYT::VT_STRING, /* required= */ true)
                       .SortOrder(NYT::ESortOrder::SO_ASCENDING))
        .AddColumn(NYT::TColumnSchema()
                       .Name("message_id")
                       .Type(NYT::VT_STRING, /* required= */ false)
                       .SortOrder(NYT::ESortOrder::SO_ASCENDING))
        .AddColumn(NYT::TColumnSchema().Name("server_time_ms").Type(NYT::VT_UINT64, /* required= */ true))
        .AddColumn(NYT::TColumnSchema().Name("sequence_number").Type(NYT::VT_UINT64, /* required= */ false))
        .AddColumn(NYT::TColumnSchema().Name("analytics_info").Type(NYT::VT_ANY, /* required= */ false))
        .AddColumn(NYT::TColumnSchema().Name("app_id").Type(NYT::VT_STRING, /* required= */ true))
        .AddColumn(NYT::TColumnSchema().Name("biometry_classification").Type(NYT::VT_ANY, /* required= */ false))
        .AddColumn(NYT::TColumnSchema().Name("biometry_scoring").Type(NYT::VT_ANY, /* required= */ false))
        .AddColumn(NYT::TColumnSchema().Name("callback_args").Type(NYT::VT_ANY, /* required= */ false))
        .AddColumn(NYT::TColumnSchema().Name("callback_name").Type(NYT::VT_STRING, /* required= */ false))
        .AddColumn(NYT::TColumnSchema().Name("client_time").Type(NYT::VT_UINT64, /* required= */ true))
        .AddColumn(NYT::TColumnSchema().Name("client_tz").Type(NYT::VT_STRING, /* required= */ true))
        .AddColumn(NYT::TColumnSchema().Name("contains_sensitive_data").Type(NYT::VT_BOOLEAN, /* required= */ true))
        .AddColumn(NYT::TColumnSchema().Name("device_id").Type(NYT::VT_STRING, /* required= */ false))
        .AddColumn(NYT::TColumnSchema().Name("device_revision").Type(NYT::VT_STRING, /* required= */ false))
        .AddColumn(NYT::TColumnSchema().Name("dialog_id").Type(NYT::VT_STRING, /* required= */ false))
        .AddColumn(NYT::TColumnSchema().Name("do_not_use_user_logs").Type(NYT::VT_BOOLEAN, /* required= */ true))
        .AddColumn(NYT::TColumnSchema().Name("enrollment_headers").Type(NYT::VT_ANY, /* required= */ false))
        .AddColumn(NYT::TColumnSchema().Name("environment").Type(NYT::VT_STRING, /* required= */ true))
        .AddColumn(NYT::TColumnSchema().Name("error").Type(NYT::VT_STRING, /* required= */ false))
        .AddColumn(NYT::TColumnSchema().Name("experiments").Type(NYT::VT_ANY, /* required= */ false))
        .AddColumn(NYT::TColumnSchema().Name("form").Type(NYT::VT_ANY, /* required= */ false))
        .AddColumn(NYT::TColumnSchema().Name("form_name").Type(NYT::VT_STRING, /* required= */ false))
        .AddColumn(NYT::TColumnSchema().Name("guest_data").Type(NYT::VT_ANY, /* required= */ false))
        .AddColumn(NYT::TColumnSchema().Name("lang").Type(NYT::VT_STRING, /* required= */ true))
        .AddColumn(NYT::TColumnSchema().Name("location_lat").Type(NYT::VT_DOUBLE, /* required= */ false))
        .AddColumn(NYT::TColumnSchema().Name("location_lon").Type(NYT::VT_DOUBLE, /* required= */ false))
        .AddColumn(NYT::TColumnSchema().Name("provider").Type(NYT::VT_STRING, /* required= */ true))
        .AddColumn(NYT::TColumnSchema().Name("puid").Type(NYT::VT_STRING, /* required= */ false))
        .AddColumn(NYT::TColumnSchema().Name("request").Type(NYT::VT_ANY, /* required= */ false))
        .AddColumn(NYT::TColumnSchema().Name("request_id").Type(NYT::VT_STRING, /* required= */ true))
        .AddColumn(NYT::TColumnSchema().Name("request_stat").Type(NYT::VT_ANY, /* required= */ false))
        .AddColumn(NYT::TColumnSchema().Name("response").Type(NYT::VT_ANY, /* required= */ false))
        .AddColumn(NYT::TColumnSchema().Name("response_id").Type(NYT::VT_STRING, /* required= */ true))
        .AddColumn(NYT::TColumnSchema().Name("server_time").Type(NYT::VT_UINT64, /* required= */ true))
        .AddColumn(NYT::TColumnSchema().Name("session_id").Type(NYT::VT_STRING, /* required= */ false))
        .AddColumn(NYT::TColumnSchema().Name("session_status").Type(NYT::VT_INT32, /* required= */ false))
        .AddColumn(NYT::TColumnSchema().Name("type").Type(NYT::VT_STRING, /* required= */ true))
        .AddColumn(NYT::TColumnSchema().Name("utterance_source").Type(NYT::VT_STRING, /* required= */ false))
        // TODO unicode
        .AddColumn(NYT::TColumnSchema().Name("utterance_text").Type(NYT::VT_STRING, /* required= */ false))
        .AddColumn(NYT::TColumnSchema().Name("trash_or_empty_request").Type(NYT::VT_BOOLEAN, /* required= */ false));

using namespace NAlice::NWonderlogs;

class TExpboxesDialogsMapper : public NYT::IMapper<NYT::TTableReader<TWonderlog>, NYT::TTableWriter<NYT::TNode>> {
public:
    class TInputOutputTables {
    public:
        TInputOutputTables(const TString& wonderlogsTable, const TString& dialogsTable, const TString& errorTable)
            : WonderlogsTable_(wonderlogsTable)
            , DialogsTable_(dialogsTable)
            , ErrorTable_(errorTable) {
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
            return operationSpec.AddInput<TWonderlog>(WonderlogsTable_)
                .AddOutput<NYT::TNode>(addAttributes(NYT::TRichYPath(DialogsTable_)).Schema(DIALOGS_SCHEMA))
                .AddOutput<NYT::TNode>(NYT::TRichYPath(ErrorTable_).Schema(errorSchema));
        }

        enum EOutIndices {
            Dialogs = 0,
            Error = 1,
        };

    private:
        const TString& WonderlogsTable_;
        const TString& DialogsTable_;
        const TString& ErrorTable_;
    };

    void Do(TReader* reader, TWriter* writer) override {
        const auto logErrors = [writer](const TDialogsBuilder::TErrors& errors) {
            for (const auto& error : errors) {
                writer->AddRow(error, TInputOutputTables::EOutIndices::Error);
            }
        };

        for (auto& cursor : *reader) {
            TDialogsBuilder builder(/* productionEnvironment= */ {});
            const TWonderlog& wonderlog = cursor.GetRow();
            logErrors(builder.AddWonderlog(wonderlog));
            if (!builder.Valid()) {
                continue;
            }
            auto dialog = std::move(builder).Build();
            bool bannedAutoRequest =
                wonderlog.GetSpeechkitRequest().GetApplication().GetAppId().find("auto") != TString::npos &&
                dialog.HasKey("utterance_text") && dialog["utterance_text"].IsString() &&
                BANNED_AUTO_PHRASES.contains(dialog["utterance_text"].AsString());
            if (!bannedAutoRequest) {
                writer->AddRow(std::move(dialog), TInputOutputTables::EOutIndices::Dialogs);
            }
        }
    }
};

REGISTER_MAPPER(TExpboxesDialogsMapper)

} // namespace

void NAlice::NWonderlogs::MakeExpboxes(NYT::IClientPtr client, const TString& /* tmpDirectory */,
                                       const TString& wonderlogs, const TString& outputTable,
                                       const TString& errorTable) {
    auto tx = client->StartTransaction();

    CreateTable(tx, outputTable, /* expirationDate= */ {});
    CreateTable(tx, errorTable, TInstant::Now() + MONTH_TTL);

    tx->Map(TExpboxesDialogsMapper::TInputOutputTables{wonderlogs, outputTable, errorTable}
                .AddToOperationSpec(NYT::TMapOperationSpec{})
                .Ordered(true)
                .MapperSpec(NYT::TUserJobSpec{}.MemoryLimit(1_GB).MemoryReserveFactor(14)),
            MakeIntrusive<TExpboxesDialogsMapper>(), NYT::TOperationOptions{});

    tx->Commit();
}
