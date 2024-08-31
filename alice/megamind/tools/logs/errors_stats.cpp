#include "errors_stats.h"

#include <alice/megamind/tools/logs/protos/logs.pb.h>
#include <alice/megamind/tools/logs/util.h>

#include <alice/library/yt/protos/logs.pb.h>
#include <alice/library/yt/util.h>

#include <mapreduce/yt/interface/client.h>

using namespace NYT;

namespace NMegamindLog {

namespace {

class TMegamindLogErrorsMapper : public IMapper<TTableReader<TMegamindLogRow>, TTableWriter<TErrorsInfoRow>> {
public:
    void Do(TReader* reader, TWriter* writer) override {
        for (; reader->IsValid(); reader->Next()) {
            const auto& row = reader->GetRow();

            if (IsRealReqId(row.GetReqId())) {
                if (const auto response = TryParseMegamindResponse(row.GetMessage())) {
                    TVector<TErrorsInfoRow> infoRows = ParseErrorsInfoRows(*response);
                    for (const auto& infoRow : infoRows) {
                        writer->AddRow(infoRow);
                    }
                }
            }
        }
    }

private:
    TMaybe<NJson::TJsonValue> TryParseMegamindResponse(TStringBuf message) {
        NJson::TJsonValue value;
        if (NJson::ReadJsonTree(message, &value)) {
           return value;
        }
        return Nothing();
    }

    TVector<TErrorsInfoRow> ParseErrorsInfoRows(const NJson::TJsonValue& megamindResponse) {
        TVector<TErrorsInfoRow> errors;

        if (megamindResponse["response"].Has("meta")) {
            const auto& metas = megamindResponse["response"]["meta"].GetArray();
            for (const auto& meta : metas) {
                if (meta.Has("error_type")) {
                    TErrorsInfoRow row;

                    row.SetReqId(megamindResponse["header"]["request_id"].GetString());
                    row.SetErrorType(meta["error_type"].GetString());
                    row.SetType(meta["type"].GetString());
                    row.SetMessage(meta["message"].GetString());

                    if (const auto scenarioName = TryParseMetaMessageScenarioName(row.GetMessage())) {
                        row.SetScenarioName(std::move(*scenarioName));
                    }

                    errors.emplace_back(std::move(row));
                }
            }
        }

        return errors;
    }

    TMaybe<TString> TryParseMetaMessageScenarioName(TStringBuf message) {
        if (const auto messageJson = TryParseMetaMessage(message)) {
            if (messageJson->Has("scenario_name")) {
                return (*messageJson)["scenario_name"].GetString();
            }
        }
        return Nothing();
    }

    TMaybe<NJson::TJsonValue> TryParseMetaMessage(TStringBuf message) {
        NJson::TJsonValue value;
        if (NJson::ReadJsonTree(message, &value)) {
            return value;
        }
        return Nothing();
    }
};
REGISTER_MAPPER(TMegamindLogErrorsMapper);

} // namespace

void TErrorsStatsTablePreparer::Prepare(NYT::IClientPtr client, const NYT::TYPath& megamindLog, const NYT::TYPath& to) const {
    NAliceYT::CreateTableWithSchema<TErrorsInfoRow>(client, to, /* force = */ true);
    {
        TMapOperationSpec spec;
        spec.AddInput<TMegamindLogRow>(megamindLog)
            .AddOutput<TErrorsInfoRow>(to);
        client->Map(spec, new TMegamindLogErrorsMapper);
    }
    {
        TSortOperationSpec spec;
        spec.SortBy({"ReqId"})
            .AddInput(to)
            .Output(to);
        client->Sort(spec);
    }
}

} //namespace NMegamindLog

