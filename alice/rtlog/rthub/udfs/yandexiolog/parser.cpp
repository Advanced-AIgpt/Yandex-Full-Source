#include "parser.h"

#include <alice/rtlog/protos/rtlog.ev.pb.h>

#include <infra/proto_logger/api/api.pb.h>

#include <library/cpp/json/json_writer.h>
#include <library/cpp/protobuf/json/proto2json.h>
#include <library/cpp/protobuf/util/pb_io.h>

#include <util/string/split.h>

#include <optional>

namespace {
    const TString YANDEXIO = "YandexIo";
    const TString ACTIVATION_ID_SUFFIX = "-device";

    using namespace NRTLog;

    using TYandexioLog = NProtoLogger::NApi::TReqWriteMetrics2LogQuasar;

    class TYandexioLogParser final: public IYandexioLogParser {
    public:
        explicit TYandexioLogParser(const TYandexioLogParserCounters& counters)
            : Counters(counters)
            , NextEventIndex(1)
        {
            jsonOutputConfig.SetFormatOutput(true);
        }

        TVector<TRecord> Parse(const TStringBuf& chunk) const override {
            const TVector<TString> messages = StringSplitter(chunk).Split('\n').SkipEmpty();

            TDeque<TRecord> records;
            // records.reserve(messages.size());
            for (const auto& message : messages) {
                TYandexioLog protoMessage;
                if (NProtoBuf::TryParseFromBase64String(message, protoMessage)) {
                    std::optional<TRecord> parsedRecord = ParseYandexioLogMessage(protoMessage);
                    if (parsedRecord) {
                        auto& item = *parsedRecord->MutableEventItem();
                        item.SetEventIndex(NextEventIndex++);
                        records.push_back(*parsedRecord);
                        if (item.GetEventType() == "ysk_voicedialog_start_voice_input" ||
                            item.GetEventType() == "ysk_voicedialog_start_music_input" ||
                            item.GetEventType() == "ysk_voicedialog_start_vins_request")
                        {
                            item.SetEventType("ActivationStarted");
                            // This EventIndex is always zero because it should always go first
                            //
                            // There shouln't be any collisions because there shouldn't be more than one
                            // ysk_voicedialog_start_voice_input for each message_id(ReqId)
                            item.SetEventIndex(0);
                            NRTLogEvents::ActivationStarted activationStarted;
                            activationStarted.SetSession(false);
                            activationStarted.SetContinue(false);
                            activationStarted.SetReqTimestamp(item.GetReqTimestamp());
                            activationStarted.SetActivationId(item.GetReqId() + ACTIVATION_ID_SUFFIX);
                            activationStarted.MutableInstanceDescriptor()->SetHostName(YANDEXIO);
                            activationStarted.MutableInstanceDescriptor()->SetServiceName(YANDEXIO);
                            item.SetEvent(FormatAsJson(activationStarted));
                            records.push_front(*parsedRecord);

                            item.SetEventType("ChildActivationStarted");
                            item.SetEventIndex(NextEventIndex++);
                            NRTLogEvents::ChildActivationStarted childActivationStarted;
                            childActivationStarted.SetDescription("uniproxy");
                            childActivationStarted.SetActivationId(item.GetReqId());
                            item.SetEvent(FormatAsJson(childActivationStarted));
                            records.push_back(*parsedRecord);
                        }
                    }
                } else {
                    Cerr << "Cannot parse message: " << message << Endl;
                    Counters.ErrorsCount.Inc();
                }
            }
            if (NextEventIndex > 1000) {
                NextEventIndex = 1;
            }

            TVector<TRecord> result;
            result.reserve(records.size());
            std::move(records.begin(), records.end(), std::back_inserter(result));

            return result;
        }

    private:
        std::optional<TRecord> ParseYandexioLogMessage(const TYandexioLog& message) const {
            // InstanceId is supposed to be removed, just use constant here.
            constexpr ui64 INSTANCE_ID = 12345678;

            // TYandexioLog uses proto3 syntax, so we don't have HasSetraceReqId
            // and use empty value as a filter.
            TString setraceReqId = message.GetSetraceClientReqId();
            if (setraceReqId.empty()) {
                setraceReqId = message.GetSetraceReqId();
            }

            if (!setraceReqId.empty()) {
                TRecord record;
                auto& item = *record.MutableEventItem();

                item.SetReqId(setraceReqId);
                item.SetReqTimestamp(message.GetTimestamp() * 1000000);
                item.SetTimestamp(message.GetTimestamp() * 1000000);
                item.SetFrameId(0);
                item.SetActivationId(setraceReqId + ACTIVATION_ID_SUFFIX);
                item.SetInstanceId(INSTANCE_ID);
                item.SetEventType(message.GetMetricName());
                item.SetServiceName(YANDEXIO);
                item.SetEvent(FormatAsJson(message));

                return record;
            }

            return std::nullopt;
        }

    private:
        TString FormatAsJson(const NProtoBuf::Message& message) const {
            TStringStream stream;
            NJson::TJsonWriter writer{&stream, jsonOutputConfig};
            NProtobufJson::Proto2Json(message, writer);
            writer.Flush();

            return stream.Str();
        }

        NJson::TJsonWriterConfig jsonOutputConfig;
        mutable TYandexioLogParserCounters Counters;
        mutable ui32 NextEventIndex;
    };
}

namespace NRTLog {
    THolder<IYandexioLogParser> MakeYandexioLogParser(const TYandexioLogParserCounters& counters) {
        return MakeHolder<TYandexioLogParser>(counters);
    }
}
