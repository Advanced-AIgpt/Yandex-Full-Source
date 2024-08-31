#include "json_wonderlogs.h"
#include "ttls.h"

#include <alice/wonderlogs/library/common/utils.h>
#include <alice/wonderlogs/library/yt/utils.h>
#include <alice/wonderlogs/protos/uniproxy_prepared.pb.h>
#include <alice/wonderlogs/protos/wonderlogs.pb.h>
#include <alice/wonderlogs/sdk/utils/speechkit_utils.h>

#include <alice/megamind/protos/speechkit/directives.pb.h>
#include <alice/megamind/protos/speechkit/response.pb.h>

#include <alice/library/json/json.h>

#include <library/cpp/json/yson/json2yson.h>
#include <library/cpp/yson/node/node_io.h>

#include <util/string/builder.h>

#include <utility>

using namespace NAlice::NWonderlogs;

namespace {

const TString ACTIVATION_TYPE_COL = "activation_type";
const TString ANALYTICS_INFO_COL = "analytics_info";
const TString CLIENT_TZ_COL = "client_tz";
const TString DIRECTIVES_COL = "directives";
const TString FIELDTABLE_COL = "fieldtable";
const TString MESSAGE_ID_COL = "message_id";
const TString REQUEST_COL = "request";
const TString REQUEST_ID_COL = "request_id";
const TString TRASH_OR_EMPTY_COL = "trash_or_empty";
const TString UNIPROXY_MDS_KEY_COL = "uniproxy_mds_key";
const TString UUID = "uuid";

class TBasketDataExtractor : public NYT::IMapper<NYT::TTableReader<TWonderlog>, NYT::TTableWriter<NYT::TNode>> {
public:
    Y_SAVELOAD_JOB(WonderlogsTables);
    TBasketDataExtractor() = default;
    explicit TBasketDataExtractor(TVector<TString> wonderlogsTables)
        : WonderlogsTables(std::move(wonderlogsTables)) {
    }

    class TInputOutputTables {
    public:
        TInputOutputTables(const TVector<TString>& wonderlogsTables, const TString& ysonWonderlogsTable,
                           const TString& errorTable)
            : WonderlogsTables(wonderlogsTables)
            , YsonWonderlogsTable(ysonWonderlogsTable)
            , ErrorTable(errorTable) {
        }
        NYT::TMapOperationSpec AddToOperationSpec(NYT::TMapOperationSpec&& operationSpec) {
            const auto dataSchema =
                NYT::TTableSchema()
                    .AddColumn(
                        NYT::TColumnSchema().Name(ACTIVATION_TYPE_COL).Type(NYT::VT_STRING, /* required= */ false))
                    .AddColumn(NYT::TColumnSchema().Name(ANALYTICS_INFO_COL).Type(NYT::VT_ANY, /* required= */ false))
                    .AddColumn(NYT::TColumnSchema().Name(CLIENT_TZ_COL).Type(NYT::VT_STRING, /* required= */ false))
                    .AddColumn(NYT::TColumnSchema().Name(DIRECTIVES_COL).Type(NYT::VT_ANY, /* required= */ false))
                    .AddColumn(NYT::TColumnSchema().Name(FIELDTABLE_COL).Type(NYT::VT_STRING, /* required= */ false))
                    .AddColumn(NYT::TColumnSchema().Name(MESSAGE_ID_COL).Type(NYT::VT_STRING, /* required= */ false))
                    .AddColumn(NYT::TColumnSchema().Name(REQUEST_COL).Type(NYT::VT_ANY, /* required= */ false))
                    .AddColumn(NYT::TColumnSchema().Name(REQUEST_ID_COL).Type(NYT::VT_STRING, /* required= */ false))
                    .AddColumn(
                        NYT::TColumnSchema().Name(TRASH_OR_EMPTY_COL).Type(NYT::VT_BOOLEAN, /* required= */ false))
                    .AddColumn(
                        NYT::TColumnSchema().Name(UNIPROXY_MDS_KEY_COL).Type(NYT::VT_STRING, /* required= */ false))
                    .AddColumn(NYT::TColumnSchema().Name(UUID).Type(NYT::VT_STRING, /* required= */ false));

            const auto errorSchema =
                NYT::TTableSchema()
                    .AddColumn(NYT::TColumnSchema().Name("process").Type(NYT::VT_STRING, /* required= */ false))
                    .AddColumn(NYT::TColumnSchema().Name("reason").Type(NYT::VT_STRING, /* required= */ false))
                    .AddColumn(NYT::TColumnSchema().Name("message").Type(NYT::VT_STRING, /* required= */ false))
                    .AddColumn(NYT::TColumnSchema().Name("uuid").Type(NYT::VT_STRING, /* required= */ false))
                    .AddColumn(NYT::TColumnSchema().Name("message_id").Type(NYT::VT_STRING, /* required= */ false))
                    .AddColumn(
                        NYT::TColumnSchema().Name("megamind_request_id").Type(NYT::VT_STRING, /* required= */ false));
            for (const auto& wonderlogsTable : WonderlogsTables) {
                operationSpec.AddInput<TWonderlog>(wonderlogsTable);
            }
            return operationSpec.AddOutput<NYT::TNode>(NYT::TRichYPath(YsonWonderlogsTable).Schema(dataSchema))
                .AddOutput<NYT::TNode>(NYT::TRichYPath(ErrorTable).Schema(errorSchema));
        }

        enum EOutIndices {
            YsonWonderlogs = 0,
            Error = 1,
        };

    private:
        const TVector<TString>& WonderlogsTables;
        const TString& YsonWonderlogsTable;
        const TString& ErrorTable;
    };

    void Do(TReader* reader, TWriter* writer) override {
        for (auto& cursor : *reader) {
            TWonderlog wonderlog = cursor.GetRow();

            if (!wonderlog.HasSpeechkitRequest()) {
                continue;
            }

            {
                auto app = wonderlog.GetClient().GetApplication();
                app.MergeFrom(wonderlog.GetSpeechkitRequest().GetApplication());
                *wonderlog.MutableSpeechkitRequest()->MutableApplication() = app;
                if (!wonderlog.GetSpeechkitRequest()
                         .GetRequest()
                         .GetAdditionalOptions()
                         .GetBassOptions()
                         .HasClientIP() &&
                    wonderlog.GetDownloadingInfo().GetUniproxy().HasClientIp()) {
                    wonderlog.MutableSpeechkitRequest()
                        ->MutableRequest()
                        ->MutableAdditionalOptions()
                        ->MutableBassOptions()
                        ->SetClientIP(wonderlog.GetDownloadingInfo().GetUniproxy().GetClientIp());
                }
            }

            const auto logError = [&wonderlog, writer](const TString& reason, const TString& message) {
                NYT::TNode error;
                error["process"] = "P_YSON_WONDERLOGS_MAPPER";
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

            NYT::TNode data;

            data[FIELDTABLE_COL] = WonderlogsTables[cursor.GetTableIndex()];
            data[UUID] = wonderlog.GetUuid();
            data[MESSAGE_ID_COL] = wonderlog.GetMessageId();
            data[REQUEST_ID_COL] = wonderlog.GetMegamindRequestId();
            data[UNIPROXY_MDS_KEY_COL] = NYT::TNode::CreateEntity();
            if (wonderlog.GetAsr().GetVoiceByUniproxy().HasMds()) {
                data[UNIPROXY_MDS_KEY_COL] = wonderlog.GetAsr().GetVoiceByUniproxy().GetMds();
            }
            data[ANALYTICS_INFO_COL] = NYT::TNode::CreateEntity();
            if (wonderlog.GetSpeechkitResponse().HasMegamindAnalyticsInfo()) {
                data[ANALYTICS_INFO_COL] = NodeFromProto(wonderlog.GetSpeechkitResponse().GetMegamindAnalyticsInfo());
            }
            data[CLIENT_TZ_COL] = NYT::TNode::CreateEntity();
            if (wonderlog.GetSpeechkitRequest().GetApplication().HasTimezone()) {
                data[CLIENT_TZ_COL] = wonderlog.GetSpeechkitRequest().GetApplication().GetTimezone();
            }

            data[TRASH_OR_EMPTY_COL] = NYT::TNode::CreateEntity();
            if (wonderlog.HasAsr() && wonderlog.GetAsr().HasTrashOrEmpty()) {
                data[TRASH_OR_EMPTY_COL] = wonderlog.GetAsr().GetTrashOrEmpty();
            }
            const auto& skRequest = wonderlog.GetSpeechkitRequest();
            try {
                TVinsLikeRequest vinsLikeRequestStruct(skRequest);
                data[REQUEST_COL] =
                    NYT::NodeFromYsonString(NJson2Yson::SerializeJsonValueAsYson(vinsLikeRequestStruct.DumpJson()));
            } catch (...) {
                logError("R_FAILED_CREATE_VINS_LIKE_REQUEST", ToString(skRequest));
                continue;
            }

            data[DIRECTIVES_COL] = NYT::TNode::CreateList();
            for (const auto& directive : wonderlog.GetSpeechkitResponse().GetResponse().GetDirectives()) {
                data[DIRECTIVES_COL].AsList().push_back(NodeFromProto(directive));
            }

            data[ACTIVATION_TYPE_COL] = NYT::TNode::CreateEntity();
            if (wonderlog.GetSpeechkitRequest().GetRequest().HasActivationType()) {
                data[ACTIVATION_TYPE_COL] = wonderlog.GetSpeechkitRequest().GetRequest().GetActivationType();
            }

            writer->AddRow(data, TInputOutputTables::EOutIndices::YsonWonderlogs);
        }
    }

private:
    TVector<TString> WonderlogsTables;
};

REGISTER_MAPPER(TBasketDataExtractor)

} // namespace

namespace NAlice::NWonderlogs {

void ExtractYsonDataFromWonderlogs(NYT::IClientPtr client, const TVector<TString>& wonderlogs,
                                   const TString& outputTable, const TString& errorTable) {
    CreateTable(client, outputTable, /* expirationDate= */ {});
    CreateTable(client, errorTable, TInstant::Now() + MONTH_TTL);

    client->Map(TBasketDataExtractor::TInputOutputTables{wonderlogs, outputTable, errorTable}.AddToOperationSpec(
                    NYT::TMapOperationSpec()),
                new TBasketDataExtractor(wonderlogs), NYT::TOperationOptions{}.InferOutputSchema(true));
}

} // namespace NAlice::NWonderlogs
