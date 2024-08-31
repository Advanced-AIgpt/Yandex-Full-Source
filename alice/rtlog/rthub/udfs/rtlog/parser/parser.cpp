#include "parser.h"

#include <alice/rtlog/rthub/udfs/rtlog/util/util.h>

#include <alice/rtlog/common/eventlog/formatting.h>
#include <alice/rtlog/protos/rtlog.ev.pb.h>

#include <apphost/lib/event_log/decl/events.ev.pb.h>

#include <search/begemot/status/events.ev.pb.h>

#include <library/cpp/eventlog/events_extension.h>
#include <library/cpp/eventlog/logparser.h>
#include <library/cpp/eventlog/proto/events_extension.pb.h>
#include <library/cpp/json/json_writer.h>
#include <library/cpp/string_utils/base64/base64.h>

#include <util/charset/utf8.h>
#include <util/string/builder.h>
#include <util/string/printf.h>
#include <util/string/split.h>

#include <util/stream/file.h>


using namespace NRTLogEvents;

namespace NRTLog {

namespace {

ui64 APP_HOST_INSTANCE_ID = 9999;
ui64 BEGEMOT_INSTANCE_ID = 9998;

// There are more of them, but only these we need to treat deffently
const std::array<TEventClass, 9> APPHOST_EVENT_CLASSES = {{
    NAppHost::TReqID::ID,
    NAppHost::TSourceAttempt::ID,
    NAppHost::TSourceError::ID,
    NAppHost::TSourceSuccess::ID,
    NAppHost::TAppHostInstallation::ID,
    NAppHost::THttpAdapterInstallation::ID,
    NAppHost::TFullHttpRequest::ID,
    NAppHost::TSubhostRequestParams::ID,
    NAppHost::TInputDump::ID
}};

// https://a.yandex-team.ru/arc/trunk/arcadia/search/begemot/status/events.ev?blame=true&rev=r8268697#L8
constexpr TEventClass BEGEMOT_REQ_ID = NBg::NEv::THttpRequestId::ID;

// reqid should look like 8-8-8-8 or 8-4-4-4-12
// 8-8-8-8 is typical for usual client requests through uniproxy and those created on backends
// 8-4-4-4-12 is typical for downloader requests (uuid4)
const std::array<TVector<size_t>, 2> APPROVED_REQID_FORMATS = {{
    {8, 8, 8, 8},
    {8, 4, 4, 4, 12}
}};
const std::array<TString, 3> HEADERS_FOR_OBFUSCATION = {{
    "X-Ya-Service-Ticket",
    "X-Ya-User-Ticket",
    "X-OAuth-Token"
}};

const TString APP_HOST_ACTIVATION_ID_POSTFIX = "-app-host";
const TString VINS_CALLBACK_PREFIX = "callback:";
const TString VINS_REQUEST_PREFIX = "vins:";
const TString MEGAMIND_SETRACE_LABEL = "megamind/http";

struct TEventInfo {
    ui64 Timestamp;
    TString EventType;
    TString Event;
    TString EventBinary;
};


bool IsApphostEvent(const TEvent& event) {
    for (const auto eventClass : APPHOST_EVENT_CLASSES) {
        if (event.Class == eventClass) {
            return true;
        }
    }
    return false;
}

bool IsBegemotEvent(const TEvent& event) {
    return event.Class == BEGEMOT_REQ_ID;
}

bool CheckReqIdCorrectness(const TString& reqid) {
    TVector<TString> parts = StringSplitter(reqid).Split('-');

    TVector<size_t> partSizes;
    for (const auto& part : parts) {
        partSizes.push_back(part.size());
    }

    for (const auto& approvedFromat : APPROVED_REQID_FORMATS) {
        if (approvedFromat == partSizes) {
            return true;
        }
    }
    return false;
}

TString FormatAsJson(const Message& message) {
    const auto formattedMessage = NRTLog::FormatRTLogEvent(message);

    {
        NJson::TJsonWriterConfig config;
        config.SetFormatOutput(true);

        TString reformatted;
        {
            TStringOutput output(reformatted);
            NJson::WriteJson(&output, &formattedMessage.Value, config);
        }
        return reformatted;
    }
}

bool FillApphostEventInfo(TEventInfo& eventInfo,
                            const TEvent& event,
                            TMaybe<NRTLogEvents::ActivationStarted>& activationStarted,
                            THashMap<std::pair<ui64, TString>, ui64>& handleIdToRUID,
                            bool& isHttpAdapter,
                            bool& isAppHost,
                            TMaybe<ui64>& subGraphRuid)
{
    eventInfo.Timestamp = event.Timestamp;
    if (event.Class == NAppHost::TReqID::ID) {
        TString token = VerifyDynamicCast<const NAppHost::TReqID*>(event.GetProto())->GetID();
        const auto items = StringSplitter(token).Split('$').ToList<TString>();
        ui64 reqTimestamp = 0;
        if (items.size() != 3 || !TryFromString<ui64>(items[0], reqTimestamp) || items[1].empty() || items[2].empty()) {
            return false;
        }
        if (!activationStarted.Defined()) {
            activationStarted.ConstructInPlace();
        }
        activationStarted->SetReqTimestamp(reqTimestamp);
        activationStarted->SetReqId(items[1]);
        activationStarted->SetActivationId(items[2]);
        activationStarted->SetSession(false);
        activationStarted->SetContinue(false);
        if (!activationStarted->HasInstanceDescriptor()) {
            activationStarted->SetInstanceId(APP_HOST_INSTANCE_ID);
        }
    } else if (event.Class == NAppHost::TSourceAttempt::ID) {
        NRTLogEvents::ChildActivationStarted childActivationStarted;
        NAppHost::TSourceAttempt sourceAttempt = *VerifyDynamicCast<const NAppHost::TSourceAttempt*>(event.GetProto());
        if (!activationStarted.Defined()) {
            // Theoretically this should never happen
            return false;
        }
        TString activationId;
        if (isHttpAdapter) {
            activationId = TStringBuilder{} << activationStarted->GetActivationId() << APP_HOST_ACTIVATION_ID_POSTFIX;
        } else {
            activationId = TStringBuilder{} << activationStarted->GetActivationId() << '-' << sourceAttempt.GetRuid();
        }
        childActivationStarted.SetActivationId(activationId);
        childActivationStarted.SetDescription(sourceAttempt.GetSource());
        handleIdToRUID.emplace(std::pair<ui64, TString>{sourceAttempt.GetHandleId(), sourceAttempt.GetSource()}, sourceAttempt.GetRuid());
        eventInfo.EventType = "ChildActivationStarted";
        eventInfo.Event = FormatAsJson(childActivationStarted);
    } else if (event.Class == NAppHost::TSourceSuccess::ID || event.Class == NAppHost::TSourceError::ID) {
        NRTLogEvents::ChildActivationFinished childActivationFinished;
        std::pair<ui64, TString> ruidKey;
        if (event.Class == NAppHost::TSourceSuccess::ID) {
            NAppHost::TSourceSuccess sourceSuccess = *VerifyDynamicCast<const NAppHost::TSourceSuccess*>(event.GetProto());
            ruidKey = {sourceSuccess.GetHandleId(), sourceSuccess.GetSource()};
            childActivationFinished.SetOk(true);
        } else {
            NAppHost::TSourceError sourceError = *VerifyDynamicCast<const NAppHost::TSourceError*>(event.GetProto());
            ruidKey = {sourceError.GetHandleId(), sourceError.GetSource()};
            childActivationFinished.SetOk(false);
            childActivationFinished.SetErrorMessage(sourceError.GetMessage());
        }
        if (!activationStarted.Defined()) {
            // Theoretically this should never happen
            return false;
        }
        TString activationId;
        if (isHttpAdapter) {
            activationId = TStringBuilder{} << activationStarted->GetActivationId() << APP_HOST_ACTIVATION_ID_POSTFIX;
        } else {
            if (auto* ptr = handleIdToRUID.FindPtr(ruidKey); ptr != nullptr) {
                activationId = TStringBuilder{} << activationStarted->GetActivationId() << '-' << *ptr;
            } else {
                return false;
            }
        }
        childActivationFinished.SetActivationId(std::move(activationId));
        eventInfo.EventType = "ChildActivationFinished";
        eventInfo.Event = FormatAsJson(childActivationFinished);
    } else if (event.Class == NAppHost::TAppHostInstallation::ID) {
        isAppHost = true;
        if (!activationStarted.Defined()) {
            activationStarted.ConstructInPlace();
        }
        NAppHost::TAppHostInstallation appHostInstallation = *VerifyDynamicCast<const NAppHost::TAppHostInstallation*>(event.GetProto());
        auto* instanceDescriptor = activationStarted->MutableInstanceDescriptor();
        instanceDescriptor->SetServiceName("apphost-" + appHostInstallation.GetName());
        instanceDescriptor->SetHostName(appHostInstallation.GetFQDNHostName());
    } else if (event.Class == NAppHost::THttpAdapterInstallation::ID) {
        isHttpAdapter = true;
        if (!activationStarted.Defined()) {
            activationStarted.ConstructInPlace();
        }
        NAppHost::THttpAdapterInstallation httpAdapterInstallation = *VerifyDynamicCast<const NAppHost::THttpAdapterInstallation*>(event.GetProto());
        auto* instanceDescriptor = activationStarted->MutableInstanceDescriptor();
        instanceDescriptor->SetServiceName("http_adapter-" + httpAdapterInstallation.GetName());
        instanceDescriptor->SetHostName(httpAdapterInstallation.GetFQDNHostName());

        activationStarted->ClearInstanceId();
    } else if (event.Class == NAppHost::TFullHttpRequest::ID) {
        isHttpAdapter = true;
    } else if (event.Class == NAppHost::TSubhostRequestParams::ID) {
        NAppHost::TSubhostRequestParams subhostRequestParams = *VerifyDynamicCast<const NAppHost::TSubhostRequestParams*>(event.GetProto());
        subGraphRuid = subhostRequestParams.GetRuid();
    }
    if (!IsUtf(eventInfo.Event)) {
        eventInfo.Event = TStringBuilder() << "{\"BadUtf8\": true, \"Base64\": \""
                                            << Base64Encode(eventInfo.Event) << "\"}";
    }

    return true;
}

bool FillBegemotEventInfo(TEventInfo& eventInfo,
                            const TEvent& event,
                            TMaybe<NRTLogEvents::ActivationStarted>& activationStarted)
{
    eventInfo.Timestamp = event.Timestamp;
    const auto* proto = VerifyDynamicCast<const NBg::NEv::THttpRequestId*>(event.GetProto());
    TString token = proto->Getreqid();
    const auto items = StringSplitter(token).Split('$').ToList<TString>();
    ui64 reqTimestamp = 0;
    if (items.size() != 3 || !TryFromString<ui64>(items[0], reqTimestamp) || items[1].empty() || items[2].empty()) {
        return false;
    }
    activationStarted.ConstructInPlace();
    activationStarted->SetReqTimestamp(reqTimestamp);
    activationStarted->SetReqId(items[1]);
    activationStarted->SetActivationId(TStringBuilder{} << items[2] << "-" << proto->Getruid());
    activationStarted->SetSession(false);
    activationStarted->SetContinue(false);
    activationStarted->SetInstanceId(BEGEMOT_INSTANCE_ID);
    activationStarted->MutableInstanceDescriptor()->SetServiceName("begemot");
    activationStarted->MutableInstanceDescriptor()->SetHostName(proto->Getaddress());
    eventInfo.Event = FormatAsJson(*activationStarted);
    eventInfo.EventType = "ActivationStarted";
    return true;
}

TMaybe<TSpecialEventItem> HandleSpecaialEvent(const TEvent& event,
                                                const NRTLogEvents::ActivationStarted& activationStarted)
{
    if ((activationStarted.GetInstanceDescriptor().GetServiceName() == "uniproxy" || activationStarted.GetInstanceDescriptor().GetServiceName() == "cuttlefish") &&
        event.Class == NRTLogEvents::ChildActivationStarted::ID)
    {
        const auto& ev = *VerifyDynamicCast<const ChildActivationStarted*>(event.GetProto());
        for (const auto& prefix : {VINS_CALLBACK_PREFIX, VINS_REQUEST_PREFIX, MEGAMIND_SETRACE_LABEL}) {
            if (ev.GetDescription().StartsWith(prefix)) {
                TSpecialEventItem specialEvent;

                specialEvent.SetReqId(activationStarted.GetReqId());
                specialEvent.SetActivationId(activationStarted.GetActivationId());
                specialEvent.SetReqTimestamp(-static_cast<i64>(activationStarted.GetReqTimestamp()));

                specialEvent.SetEventType("VinsRequestUtterance");
                specialEvent.SetEvent(ev.GetDescription());
                specialEvent.SetTimestamp(event.Timestamp);
                return specialEvent;
            }
        }
    }

    return Nothing();
}

TMaybe<TErrorEventItem> HandleErrorEvent(const TConstEventPtr& event,
                                        const NRTLogEvents::ActivationStarted& activationStarted)
{
    if (event->Class == LogEvent::ID) {
        const TLogEvent* ev = VerifyDynamicCast<const TLogEvent*>(event->GetProto());

        if (ev->GetSeverity() == RTLOG_SEVERITY_ERROR &&
            NAlice::NeedToSend(ev->GetMessage(), ev->GetBacktrace()))
        {
            TErrorEventItem errorEvent;

            errorEvent.SetLevel("error");
            errorEvent.SetProject("alice");
            errorEvent.SetMessage(ev->GetMessage());
            errorEvent.SetTimestamp(event->Timestamp / 1000);
            errorEvent.SetReqid(activationStarted.GetReqId());
            errorEvent.SetService(activationStarted.GetInstanceDescriptor().GetServiceName());

            const auto& fields = ev->GetFields();

            if (const TString* pos = MapFindPtr(fields, "pos"); pos != nullptr) {
                errorEvent.SetFile(NAlice::FileFromPos(*pos));
                errorEvent.SetLine(NAlice::LineFromPos(*pos));
            } else if (const TString* name = MapFindPtr(fields, "name"); name != nullptr) {
                // Vins writes python-path instead of file
                errorEvent.SetFile(*name);
                if (const TString* lineNo = MapFindPtr(fields, "lineno"); lineNo != nullptr) {
                    ui64 result = 0;
                    if (TryFromString(*lineNo, result)) {
                        errorEvent.SetLine(result);
                    }
                }
            }

            const TString& host = activationStarted.GetInstanceDescriptor().GetHostName();
            errorEvent.SetHost(host);
            errorEvent.SetDc(NAlice::DataCenterFromHost(host));

            if (ev->GetBacktrace()) {
                const auto& backtrace = ev->GetBacktrace();
                errorEvent.SetStack(backtrace);
                errorEvent.SetLanguage(NAlice::GetLanguage(backtrace));
            }

            if (const TString* env = MapFindPtr(fields, "env"); env != nullptr) {
                errorEvent.SetEnv(*env);
            } else if (const TString* nannyServiceId = MapFindPtr(fields, "nanny_service_id");
                nannyServiceId != nullptr)
            {
                const auto environment = NAlice::GetEnv(*nannyServiceId);
                if (environment.Defined()) {
                    errorEvent.SetEnv(environment.GetRef());
                }
            }

            return errorEvent;
        }
    }
    return Nothing();
}



TString ObfuscateTFullHttpRequest(const TEvent& event) {
    NAppHost::TFullHttpRequest httpRequest = *VerifyDynamicCast<const NAppHost::TFullHttpRequest*>(event.GetProto());
    for (auto& header : *httpRequest.MutableHeaders()) {
        for (const auto& prefix : HEADERS_FOR_OBFUSCATION) {
            if (AsciiEqualsIgnoreCase(prefix, header.substr(0, prefix.size()))) {
                header = TStringBuilder{} << prefix << ": [obfuscated]";
            }
        }
    }
    return FormatAsJson(httpRequest);
}

} // namespace anonymous

TEventsParser::TEventsParser(const TEventsParserCounters& counters)
    : Counters(counters)
{
}


size_t GetStartFrameOffset(const TFrame& frame) {
    return frame.Address() - COMPRESSED_LOG_FRAME_SYNC_DATA.size();
}

TVector<NRTLog::TRecord> TEventsParser::Parse(const TStringBuf& chunk, bool saveBinaryFrame) const {
    TVector<NRTLog::TRecord> records;
    {
        auto* eventFactory = NEvClass::Factory();

        TMemoryInput input(chunk);
        TMemoryInput inputCopy;
        if (saveBinaryFrame) {
            inputCopy = TMemoryInput{chunk};
        }

        TFrameStreamer frameStreamer(input, eventFactory);
        ui32 prefixLength = 0;
        while (frameStreamer.Avail()) {
            bool doneNextFrame = false;
            const auto& frame = *frameStreamer;
            ui32 frameId = frame.FrameId();

            TFrameDecoder decoder(frame, nullptr, false, true);

            TDeque<TEventInfo> events;
            TVector<TString> keys;
            TDeque<TSpecialEventItem> specialEvents;
            TDeque<TErrorEventItem> errorEvents;
            TMaybe<NRTLogEvents::ActivationStarted> activationStarted;
            THashMap<std::pair<ui64, TString>, ui64> handleIdToRUID;
            bool isHttpAdapter = false;
            bool isAppHost = false;
            TMaybe<ui64> subGraphRuid;

            while (true) {
                TConstEventPtr e;
                try {
                    e = *decoder;
                }
                catch (...) {
                    Counters.ErrorsCount.Inc();
                    const TString message = Sprintf("error parsing event"
                                                    ", rest of the frame will be skipped"
                                                    ", error message: %s",
                                                    CurrentExceptionMessage().c_str());
                    Cerr << message << Endl;
                    break;
                }
                if (!e || e->Class == TEndOfFrameEvent::EventClass) {
                    break;
                }

                // Temporary fix for SETRACE-499
                if (activationStarted.Defined() && e->Class == ChildActivationStarted::ID) {
                    ChildActivationStarted childActivation = *VerifyDynamicCast<const ChildActivationStarted*>(e->GetProto());
                    if (childActivation.GetActivationId() == activationStarted->GetActivationId()) {
                        if (!decoder.Next()) {
                            break;
                        }
                        continue;
                    }
                }

                if (IsApphostEvent(*e)) {
                    events.emplace_back();
                    TEventInfo& eventInfo = events.back();
                    if (!FillApphostEventInfo(eventInfo, *e, activationStarted, handleIdToRUID, isHttpAdapter, isAppHost, subGraphRuid)) {
                        break;
                    }
                    if (eventInfo.Event.empty()) {
                        events.pop_back();
                    }
                }

                if (IsBegemotEvent(*e)) {
                    TEventInfo& eventInfo = events.emplace_back();
                    if (!FillBegemotEventInfo(eventInfo, *e, activationStarted)) {
                        break;
                    }

                    if (eventInfo.Event.empty()) {
                        events.pop_back();
                    }
                }

                events.emplace_back();
                TEventInfo& eventInfo = events.back();
                eventInfo.Timestamp = e->Timestamp;
                eventInfo.EventType = e->GetName();
                if (e->Class == NAppHost::TTvmTicket::ID) {
                    eventInfo.Event = "{\"Message\": \"This is an obfuscated TTvmTicket\"}";
                } else if (e->Class == NAppHost::TFullHttpRequest::ID) {
                    eventInfo.Event = ObfuscateTFullHttpRequest(*e);
                } else if (e->Class == NAppHost::TInputDump::ID) {
                    eventInfo.Event = "{\"Message\": \"We do not write TInputDump because it is too big\"}";
                } else {
                    eventInfo.Event = FormatAsJson(*e->GetProto());
                    if (!IsUtf(eventInfo.Event)) {
                        Counters.BadUtf8BytesCount.Add(eventInfo.Event.size());
                        Counters.BadUtf8MessagesCount.Inc();
                        eventInfo.Event = TStringBuilder() << "{\"BadUtf8\": true, \"Base64\": \""
                                                        << Base64Encode(eventInfo.Event) << "\"}";
                    }
                }

                if (e->Class == ActivationStarted::ID) {
                    activationStarted = *VerifyDynamicCast<const ActivationStarted*>(e->GetProto());
                    if (activationStarted->HasReqId()) {
                        keys.push_back(activationStarted->GetReqId());
                    }
                } else if (e->Class == NRTLogEvents::CreateRequestContext::ID) {
                    const auto& ev = *VerifyDynamicCast<const CreateRequestContext*>(e->GetProto());
                    for(const auto& item: ev.GetFields()) {
                        if (!item.second.empty()) {
                            keys.push_back(item.second);
                        }
                    }
                } else if (e->Class == NRTLogEvents::ChildActivationStarted::ID) {
                    const auto& ev = *VerifyDynamicCast<const ChildActivationStarted*>(e->GetProto());
                    if (ev.HasReqId()) {
                        keys.push_back(ev.GetReqId());
                    }
                }

                if (activationStarted.Defined()) {
                    if (const auto specialEvent = HandleSpecaialEvent(*e, *activationStarted); specialEvent.Defined()) {
                        specialEvents.push_back(*specialEvent);
                    }
                }

                if (activationStarted.Defined()) {
                    const auto errorEvent = HandleErrorEvent(e, *activationStarted);
                    if (errorEvent.Defined()) {
                        errorEvents.push_back(*errorEvent);
                    }
                }

                if (!decoder.Next()) {
                    break;
                }
            }

            // This condition ensures that we only put "random" reqids in YDB
            // The main reason is to not let through reqids that start with timestamp
            if (activationStarted.Defined() && !CheckReqIdCorrectness(activationStarted->GetReqId())) {
                Counters.BadReqIdCount.Inc();
                events.clear();
                activationStarted.Clear();
                if (!doneNextFrame) {
                    frameStreamer.Next();
                }
                continue;
            }

            if (activationStarted.Defined() && (isHttpAdapter || isAppHost)) {
                TEventInfo appHostActivationStarted;
                if (isAppHost && activationStarted->HasInstanceDescriptor()) {
                    if (subGraphRuid.Defined()) {
                        activationStarted->SetActivationId(TStringBuilder{} << activationStarted->GetActivationId() << '-' << *subGraphRuid);
                    } else if (activationStarted->GetInstanceDescriptor().GetServiceName() != "apphost-VOICE") {
                        activationStarted->SetActivationId(TStringBuilder{} << activationStarted->GetActivationId() << APP_HOST_ACTIVATION_ID_POSTFIX);
                    }
                }

                appHostActivationStarted.Timestamp = events.front().Timestamp;
                appHostActivationStarted.EventType = "ActivationStarted";
                appHostActivationStarted.Event = FormatAsJson(*activationStarted);
                events.push_front(appHostActivationStarted);
            }

            if (saveBinaryFrame && !events.empty()) {
                size_t frameEnd = !frameStreamer.Next() ? chunk.size() : GetStartFrameOffset(*frameStreamer);
                doneNextFrame = true;
                if (frameEnd <= chunk.size()) {
                    TEventInfo binaryEvent;
                    binaryEvent.Timestamp = events.front().Timestamp;
                    binaryEvent.EventType = "BinaryFrame";
                    binaryEvent.Event = "{}";

                    binaryEvent.EventBinary = TStringBuf{chunk.begin() + prefixLength, frameEnd - prefixLength};
                    prefixLength = frameEnd;

                    events.clear();
                    events.push_front(binaryEvent);
                }
            }

            if (activationStarted.Defined()) {
                ui64 instanceId = 123;
                size_t newSize = records.size() + keys.size() + specialEvents.size() + errorEvents.size();
                records.reserve(newSize);
                for (size_t i = 0; i < events.size(); ++i) {
                    const auto& eventInfo = events[i];
                    records.emplace_back();
                    {
                        auto& item = *records.back().MutableEventItem();
                        item.SetReqTimestamp(-static_cast<i64>(activationStarted->GetReqTimestamp()));
                        item.SetReqId(activationStarted->GetReqId());
                        item.SetActivationId(activationStarted->GetActivationId());
                        if (activationStarted->HasInstanceDescriptor()) {
                            item.SetServiceName(activationStarted->GetInstanceDescriptor().GetServiceName());
                        } else {
                            item.SetServiceName("");
                        }
                        item.SetInstanceId(instanceId);
                        item.SetFrameId(frameId);
                        item.SetEventIndex(i);
                        item.SetTimestamp(eventInfo.Timestamp);
                        item.SetEventType(std::move(eventInfo.EventType));
                        item.SetEvent(std::move(eventInfo.Event));
                        if (item.GetEvent().empty()) {
                            item.SetEvent("{}");
                        }
                        item.SetEventBinary(std::move(eventInfo.EventBinary));
                    }
                    for (const auto& specialEvent : specialEvents) {
                        records.emplace_back();
                        *records.back().MutableSpecialEventItem() = specialEvent;
                    }
                    for (const auto& errorEvent : errorEvents) {
                        records.emplace_back();
                        *records.back().MutableErrorEventItem() = errorEvent;
                    }
                    for (const auto& k: keys) {
                        records.emplace_back();
                        auto& item = *records.back().MutableEventIndexItem();
                        item.SetKey(k);
                        item.SetReqTimestamp(-static_cast<i64>(activationStarted->GetReqTimestamp()));
                        item.SetReqId(activationStarted->GetReqId());
                        item.SetActivationId(activationStarted->GetActivationId());
                        item.SetFrameId(frameId);
                        item.SetEventIndex(i);
                    }
                }
            }
            if (!doneNextFrame) {
                frameStreamer.Next();
            }
        }
    }
    return records;
}

} // namespace NRTLog
