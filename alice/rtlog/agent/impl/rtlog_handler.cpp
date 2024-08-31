#include "rtlog_handler.h"

#include <alice/rtlog/agent/protos/rtlog_agent.pb.h>
#include <alice/rtlog/protos/rtlog.ev.pb.h>
#include <alice/rtlog/common/eventlog/formatting.h>
#include <alice/rtlog/common/eventlog/helpers.h>

#include <robot/rthub/misc/common.h>
#include <library/cpp/eventlog/common.h>
#include <library/cpp/eventlog/logparser.h>
#include <library/cpp/json/json_writer.h>

#include <util/digest/city.h>
#include <util/generic/maybe.h>
#include <util/stream/input.h>
#include <util/stream/mem.h>
#include <util/string/printf.h>

namespace NRTLogAgent {
    namespace {
        using namespace NRTLogEvents;
        using namespace NJson;
        using namespace google::protobuf;
        using namespace NRTLog;

        class TRTLogHandler: public IRTLogHandler {
        public:
            THolder<Message> ParseForIndex(const char* data, const TLogItemLocation& location) override {
                auto result = MakeHolder<TFrameInfo>();
                ForEachEvent(data, location.Size, [&](ui32 eventIndex, const auto& e, const auto&) {
                    if (e->Class == ActivationStarted::ID) {
                        const auto& activationStarted = Cast<ActivationStarted>(e.Get());
                        result->SetReqId(activationStarted.GetReqId());
                        result->SetReqTimestamp(activationStarted.GetReqTimestamp());
                        *result->MutableInstance() = activationStarted.GetInstanceDescriptor();
                    } else if (e->Class == NRTLogEvents::CreateRequestContext::ID) {
                        const auto& ev = Cast<CreateRequestContext>(e.Get());
                        for(const auto& item: ev.GetFields()) {
                            AddIndexKey(item.second, eventIndex, *result);
                        };
                    } else if (e->Class == NRTLogEvents::ChildActivationStarted::ID) {
                        const auto& ev = Cast<ChildActivationStarted>(e.Get());
                        if (ev.HasReqId()) {
                            AddIndexKey(ev.GetReqId(), eventIndex, *result);
                        }
                    }
                    return true;
                });
                if (!result->HasReqId()) {
                    ythrow yexception() << "ActivationStarted event not found";
                }
                result->SetSegmentTimestamp(location.SegmentTimestamp);
                result->SetOffset(location.Offset);
                result->SetSize(location.Size);
                return result;
            }

            TString ParseForQuery(const TBuffer& data, const TMaybe<TFieldSpec>& keySpec, bool pretty) override {
                TJsonValue result;
                result.SetType(JSON_MAP);

                TJsonValue events;
                events.SetType(JSON_ARRAY);

                bool fieldSpecMismatch = false;
                bool activationFound = false;

                ForEachEvent(data.Data(), data.Size(), [&](ui32 eventIndex, const auto& e, const auto& frame) {
                    if (keySpec && keySpec->EventIndex == eventIndex) {
                        if (e->Class != NRTLogEvents::CreateRequestContext::ID) {
                            fieldSpecMismatch = true;
                            return false;
                        }
                        const auto& ev = Cast<CreateRequestContext>(e.Get());
                        auto it = ev.GetFields().find(keySpec->Key);
                        if (it == ev.GetFields().end() || it->second != keySpec->Value) {
                            fieldSpecMismatch = true;
                            return false;
                        }
                    }
                    if (e->Class == ActivationStarted::ID) {
                        if (activationFound) {
                            ythrow yexception() << "multiple ActivationStarted events found";
                        }
                        activationFound = true;
                        const auto& activationStarted = Cast<ActivationStarted>(e.Get());
                        result["ReqTimestamp"] = TInstant::MicroSeconds(activationStarted.GetReqTimestamp()).ToStringUpToSeconds();
                        result["ReqId"] = activationStarted.GetReqId();
                        result["ActivationId"] = activationStarted.GetActivationId();
                        result["FrameId"] = frame.FrameId();
                        result["Session"] = activationStarted.GetSession();
                        result["Continue"] = activationStarted.GetContinue();
                    }
                    {
                        TJsonValue value;
                        value.SetType(JSON_MAP);
                        value["Timestamp"] = TInstant::MicroSeconds(e->Timestamp).ToStringUpToSeconds();
                        value["EventType"] = e->GetName();
                        value["Event"] = NRTLog::FormatRTLogEvent(*e->GetProto()).Value;

                        events.AppendValue(std::move(value));
                    }
                    return true;
                });

                if (!activationFound) {
                    ythrow yexception() << "ActivationStarted event not found";
                }
                if (fieldSpecMismatch) {
                    result["FieldSpecMismatch"] = true;
                } else {
                    result["Events"] = std::move(events);
                }

                {
                    NJson::TJsonWriterConfig config;
                    config
                        .SetFormatOutput(pretty);

                    TString resultJson;
                    {
                        TStringOutput output(resultJson);
                        NJson::WriteJson(&output, &result, config);
                    }
                    return resultJson;
                }
            }

        private:
            void AddIndexKey(const TString& key, ui32 eventIndex, TFrameInfo& target) {
                auto& indexKey = *target.MutableIndexKeys()->Add();
                indexKey.SetKey(CityHash64(key));
                indexKey.SetEventIndex(eventIndex);
            }

            template <typename TCallback>
            void ForEachEvent(const char* data, size_t size, TCallback callback) {
                TMemoryInput input(data, size);
                auto frame = FindNextFrame(&input, NEvClass::Factory());
                if (!frame) {
                    ythrow yexception() << "frame not found";
                }
                TFrameDecoder decoder(*frame, nullptr, false, true);
                ui32 eventIndex = 0;
                while (true) {
                    auto e = *decoder;
                    if (!e || e->Class == TEndOfFrameEvent::EventClass) {
                        break;
                    }
                    if (!callback(eventIndex, e, *frame)) {
                        break;
                    }
                    ++eventIndex;
                    if (!decoder.Next()) {
                        break;
                    }
                }
            }
        };
    }

    THolder<IRTLogHandler> MakeRTLogHandler() {
        return MakeHolder<TRTLogHandler>();
    }
}

using namespace NRTLogAgent;

template <>
void Out<TLogItemLocation>(IOutputStream& os, const TLogItemLocation& l) {
    os << Sprintf("(%u, %u, %u)", l.SegmentTimestamp, l.Offset, l.Size);
}
