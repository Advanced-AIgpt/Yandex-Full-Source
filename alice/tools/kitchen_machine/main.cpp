#include "main.h"

#include <alice/megamind/protos/scenarios/directives.pb.h>
#include <alice/tools/kitchen_machine/collapsed_log.pb.h>

#include <search/multiplexer/multiplexer.h>

#include <mapreduce/yt/client/init.h>
#include <mapreduce/yt/interface/client.h>
#include <mapreduce/yt/util/temp_table.h>

#include <library/cpp/cgiparam/cgiparam.h>
#include <library/cpp/getopt/last_getopt.h>
#include <library/cpp/getopt/modchooser.h>
#include <library/cpp/json/json_reader.h>
#include <library/cpp/json/json_writer.h>
#include <library/cpp/json/yson/json2yson.h>
#include <library/cpp/logger/global/global.h>
#include <library/cpp/protobuf/json/json2proto.h>
#include <library/cpp/yson/node/node_io.h>

#include <util/datetime/parser.h>
#include <util/digest/city.h>
#include <util/generic/vector.h>
#include <util/stream/output.h>
#include <util/string/split.h>
#include <util/system/env.h>

bool IsSmartSpeaker(TStringBuf message) {
    return message.Contains("aliced") || message.Contains("ru.yandex.quasar.app");
}

class TYTClientManager {
public:
    void Registger(NLastGetopt::TOpts& opts) {
        opts
            .AddLongOption("proxy", "YT proxy.")
            .Optional()
            .RequiredArgument("YT_PROXY")
            .StoreResult(&Proxy_)
            .DefaultValue("hahn");

        opts
            .AddLongOption("token", "YT token.")
            .Optional()
            .RequiredArgument("YT_TOKEN")
            .StoreResult(&Token_);
    }

    NYT::IClientPtr GetClient() {
        if (!Client_) {
            NYT::TCreateClientOptions clientOptions;
            if (!Token_.empty()) {
                clientOptions.Token(Token_);
            }
            Client_ = NYT::CreateClient(Proxy_, clientOptions);
        }
        return Client_;
    }

private:
    TString Proxy_;
    TString Token_;
    NYT::IClientPtr Client_;
};

NYT::IClientPtr CreateClient(const TString& proxy, const TString& token) {
    NYT::TCreateClientOptions clientOptions;
    if (!token.empty()) {
        clientOptions.Token(token);
    }
    return NYT::CreateClient(proxy, clientOptions);
}

bool ChooseMessageId(const NJson::TJsonValue& sessionLog, TString* messageId) {
    if (auto id = sessionLog.GetValueByPath("Directive.ForEvent"); id && id->IsString()) {
        *messageId = id->GetString();
        return true;
    }

    if (auto id = sessionLog.GetValueByPath("Event.event.header.messageId"); id && id->IsString()) {
        *messageId = id->GetString();
        return true;
    }

    if (auto id = sessionLog.GetValueByPath("Directive.directive.header.refMessageId"); id && id->IsString()) {
        *messageId = id->GetString();
        return true;
    }

    return false;
}

bool IsVoiceInput(const NJson::TJsonValue& sessionLog) {
    const auto* name = sessionLog.GetValueByPath("Event.event.header.name");
    return name && name->GetStringRobust() == "VoiceInput";
}

bool ParseTimestamp(const NJson::TJsonValue& sessionLog, i64* result) {
    auto timestampValue = sessionLog.GetValueByPath("Session.Timestamp");
    if (!timestampValue) {
        return false;
    }

    const auto& timestamp = timestampValue->GetStringRobust();

    TIso8601DateTimeParser parser;
    if (!parser.ParsePart(timestamp.data(), timestamp.size())) {
        return false;
    }

    *result = parser.GetResult(TInstant()).MilliSeconds();
    return true;
}

bool RetrieveEventTraits(const NJson::TJsonValue& sessionLog, TEventTraits* traits) {
    i64 timestamp = 0;
    if (!ParseTimestamp(sessionLog, &timestamp)) {
        return false;
    }
    traits->SetTimestamp(timestamp);

    traits->SetIsVoiceInput(IsVoiceInput(sessionLog));

    return true;
}

class TCollapseUniproxyLogMapper
    : public NYT::IMapper<NYT::TTableReader<NYT::TNode>, NYT::TTableWriter<NYT::TNode>> {
public:
    virtual void Do(NYT::TTableReader<NYT::TNode>* reader, NYT::TTableWriter<NYT::TNode>* writer) {
        NMultiplexer::TOrderedMultiplexer multiplexer(4);
        multiplexer.Run(TProducer(reader), TWorker(), TConsumer(writer));
    }

private:
    class TProducer {
    public:
        TProducer(NYT::TTableReader<NYT::TNode>* reader)
            : Reader_(reader)
            , Called_(false)
        { }

        TMaybe<TString> operator()() {
            while (Reader_->IsValid()) {
                if (Called_) {
                    Reader_->Next();
                }
                Called_ = true;

                if (!Reader_->IsValid()) {
                    break;
                }

                const auto& messageNode = Reader_->GetRow()["message"];
                if (!messageNode.IsString()) {
                    ERROR_LOG << "Incorrect message type, expected: " << NYT::TNode::String
                              << ", actual: " << messageNode.GetType() << Endl;
                    continue;
                }

                TMaybe<TString> result = messageNode.As<TString>();
                return result;
            }

            return Nothing();
        }

    private:
        NYT::TTableReader<NYT::TNode>* Reader_;
        bool Called_;
    };

    class TWorker {
    public:
        TMaybe<NYT::TNode> operator()(TString message) {
            if (!IsSmartSpeaker(message)) {
                return Nothing();
            }

            static const TStringBuf SessionLogPrefix = "SESSIONLOG: ";
            if (!message.StartsWith(SessionLogPrefix)) {
                // ERROR_LOG << "The session log doesn't start from the correct prefix." << Endl;
                //           << "Message: " << message << Endl;
                return Nothing();
            }

            NJson::TJsonValue sessionLog;
            try {
                NJson::ReadJsonTree(message.substr(SessionLogPrefix.size()), &sessionLog, /* throwOnError = */ true);
            } catch (...) {
                // ERROR_LOG << "Cannot parse the message json: " << CurrentExceptionMessage() << Endl;
                //           << "Message: " << message << Endl;
                return Nothing();
            }

            TCollapsedLog collapsedLog;
            if (!ChooseMessageId(sessionLog, collapsedLog.MutableMessageId())) {
                // ERROR_LOG << "Cannot find a message id in the message." << Endl
                //           << "JsonValue: " << NJson::WriteJson(sessionLog, /* formatOutput = */ false) << Endl;
                return Nothing();
            }

            auto* event = collapsedLog.AddEvents();
            RetrieveEventTraits(sessionLog, event->MutableTraits());
            event->SetSessionLog(NJson2Yson::SerializeJsonValueAsYson(sessionLog));

            NYT::TNode result;
            result
                ("MessageId", collapsedLog.GetMessageId())
                ("CollapsedLog", collapsedLog.SerializeAsString());

            return result;
        }
    };

    class TConsumer {
    public:
        TConsumer(NYT::TTableWriter<NYT::TNode>* writer)
            : Writer_(writer)
        { }

        void operator()(TMaybe<NYT::TNode> result) {
            if (result.Defined()) {
                Writer_->AddRow(*result);
            }
        }

    private:
        NYT::TTableWriter<NYT::TNode>* Writer_;
    };
};

void MergeCollapsedLog(TCollapsedLog* lft, TCollapsedLog* rgt) {
    Y_VERIFY(lft->GetMessageId().empty() || lft->GetMessageId() == rgt->GetMessageId());
    *lft->MutableMessageId() = std::move(*rgt->MutableMessageId());

    for (auto& event : *rgt->MutableEvents()) {
        *lft->AddEvents() = std::move(event);
    }
}

class TCollapseUniproxyLogReducer
    : public NYT::IReducer<NYT::TTableReader<NYT::TNode>, NYT::TTableWriter<NYT::TNode>> {
public:
    virtual void Do(NYT::TTableReader<NYT::TNode>* reader, NYT::TTableWriter<NYT::TNode>* writer) {
        TCollapsedLog result;

        for (; reader->IsValid(); reader->Next()) {
            const auto& row = reader->GetRow();

            TCollapsedLog collapsedLog;
            Y_VERIFY(collapsedLog.ParseFromString(row["CollapsedLog"].As<TString>()));

            MergeCollapsedLog(&result, &collapsedLog);
        }

        bool hasVoiceInput = false;
        for (const auto& event : result.GetEvents()) {
            if (event.GetTraits().GetIsVoiceInput()) {
                hasVoiceInput = true;
                break;
            }
        }

        if (!hasVoiceInput) {
            return;
        }

        Sort((*result.MutableEvents()).begin(), (*result.MutableEvents()).end(), [](const TEvent& a, const TEvent& b) {
            return a.GetTraits().GetTimestamp() < b.GetTraits().GetTimestamp();
        });

        writer->AddRow(NYT::TNode()
            ("MessageId", result.GetMessageId())
            ("CollapsedLog", result.SerializeAsString()));
    }
};

REGISTER_MAPPER(TCollapseUniproxyLogMapper);
REGISTER_REDUCER(TCollapseUniproxyLogReducer);

int CollapseUniproxyLog(int argc, const char* argv[]) {
    NLastGetopt::TOpts opts;
    opts.SetFreeArgsNum(0);

    TString proxy;
    opts
        .AddLongOption("proxy", "YT proxy.")
        .Optional()
        .RequiredArgument("YT_PROXY")
        .StoreResult(&proxy)
        .DefaultValue("hahn");

    TString token;
    opts
        .AddLongOption("token", "YT token.")
        .Optional()
        .RequiredArgument("YT_TOKEN")
        .StoreResult(&token);

    TString inputPath;
    opts
        .AddLongOption('i', "input-path", "Path to YT table.")
        .Required()
        .RequiredArgument("YT_PATH")
        .StoreResult(&inputPath);

    TString outputPath;
    opts
        .AddLongOption('o', "output-path", "Path to YT table.")
        .Required()
        .RequiredArgument("YT_PATH")
        .StoreResult(&outputPath);

    TString stderrPath;
    opts
        .AddLongOption("stderr", "Path to YT table.")
        .Optional()
        .RequiredArgument("YT_PATH")
        .StoreResult(&stderrPath);

    NLastGetopt::TOptsParseResult(&opts, argc, argv);

    auto client = CreateClient(proxy, token);
    NYT::TMapReduceOperationSpec operationSpec;

    operationSpec
        .AddOutput<NYT::TNode>("<sorted_by=[MessageId]>" + outputPath)
        .SortBy({"MessageId"})
        .ReduceBy({"MessageId"})
        .AddInput<NYT::TNode>(inputPath);

    if (!stderrPath.empty()) {
        operationSpec.StderrTablePath(stderrPath);
    }

    client->MapReduce(
        operationSpec,
        new TCollapseUniproxyLogMapper(),
        new TCollapseUniproxyLogReducer());

    return 0;
}

class ICollapsedLogWriter {
public:
    virtual bool Write(const TCollapsedLog& collapsedLog) = 0;
    virtual ~ICollapsedLogWriter() = default;
};

class TJsonCollapsedLogWriter : public ICollapsedLogWriter {
public:
    bool Write(const TCollapsedLog& collapsedLog) override {
        NJsonWriter::TBuf writer(NJsonWriter::HEM_UNSAFE, &Cout);
        writer.SetIndentSpaces(4);

        writer.BeginObject();
        writer.WriteKey("MessageId").WriteString(collapsedLog.GetMessageId());

        writer.WriteKey("Events").BeginList();
        for (const auto& event : collapsedLog.GetEvents()) {
            writer.BeginObject();
            writer.WriteKey("Timestamp").WriteLongLong(event.GetTraits().GetTimestamp());

            NJson::TJsonValue sessionLog;
            NJson2Yson::DeserializeYsonAsJsonValue(event.GetSessionLog(), &sessionLog, true);
            writer.WriteKey("SessionLog").WriteJsonValue(&sessionLog);

            writer.EndObject();
        }
        writer.EndList();

        writer.EndObject();

        return true;
    }
};

class TTsvWriter {
public:
    TTsvWriter& Write(const TString& str) {
        if (Writed_) {
            Builder_ << "\t";
        }
        Writed_ = true;
        Builder_ << str;

        return *this;
    }

    template <typename T>
    TTsvWriter& Write(const T& t) {
        return Write(ToString(t));
    }

    void FlushRow() {
        Builder_ << "\n";
        Writed_ = false;
    }

    void Flush() {
        Cout << Builder_;
        Builder_.clear();
    }

private:
    bool Writed_ = false;
    TStringBuilder Builder_;
};

TString GetType(const NJson::TJsonValue& sessionLog) {
    if (const auto* header = sessionLog.GetValueByPath("Directive.directive.header"); header) {
        return (*header)["namespace"].GetStringRobust() + "." + (*header)["name"].GetStringRobust();
    }


    if (const auto* type = sessionLog.GetValueByPath("Directive.type"); type) {
        return type->GetStringRobust();
    }

    if (const auto* eventHeader = sessionLog.GetValueByPath("Event.event.header"); eventHeader) {
        return (*eventHeader)["namespace"].GetStringRobust() + "." + (*eventHeader)["name"].GetStringRobust();
    }

    return TString("NULL");
}

struct TCollapsedLogTraits {
    bool IsRobot = false;
    i64 LastAsrResult = 0;
    i64 LastVinsResponse = 0;
    i64 AsrResultCount = 0;
    i64 VinsResponseCount = 0;
};

class IEventWriter {
public:
    virtual void Write(i64 timestamp, TTsvWriter& writer, const NJson::TJsonValue& sessionLog, TCollapsedLogTraits& traits) = 0;
    virtual ~IEventWriter() = default;
};

class TDefaultEventWriter : public IEventWriter {
public:
    void Write(i64 timestamp, TTsvWriter& writer, const NJson::TJsonValue& sessionLog, TCollapsedLogTraits& traits) override {
        Y_UNUSED(timestamp);
        Y_UNUSED(writer);
        Y_UNUSED(sessionLog);
        Y_UNUSED(traits);
    }
};

class TEventWriterFactory {
public:
    template <typename TWriter>
    void Register(const TString& name) {
        Callbacks_[name] = [] { return MakeHolder<TWriter>(); };
    }

    THolder<IEventWriter> Create(const TString& name) {
        if (!Callbacks_.contains(name)) {
            return MakeHolder<TDefaultEventWriter>();
        }
        return Callbacks_[name]();
    }

private:
    THashMap<TString, std::function<THolder<IEventWriter>()>> Callbacks_;
};

TEventWriterFactory* EventWriterFactory() {
    return Singleton<TEventWriterFactory>();
}

template <typename Type>
class TEventWriterRegistrator {
public:
    TEventWriterRegistrator(const TString& name) {
        EventWriterFactory()->Register<Type>(name);
    }
};

#define REGISTER(type, name) \
    namespace { \
        TEventWriterRegistrator<type> register_ ## type(name); \
    }

class TTtsGenerateTimingsEventWriter : public IEventWriter {
public:
    void Write(i64 timestamp, TTsvWriter& writer, const NJson::TJsonValue& sessionLog, TCollapsedLogTraits& traits) override {
        Y_UNUSED(timestamp);
        Y_UNUSED(traits);

        const auto* firstChunkTime = sessionLog.GetValueByPath("Directive.result.first_chunk_time");
        if (!firstChunkTime) {
            writer.Write("NULL");
        } else {
            writer.Write(firstChunkTime->GetStringRobust());
        }
    }
};

REGISTER(TTtsGenerateTimingsEventWriter, "TtsGenerateTimings");

class TAsrResultEventWriter : public IEventWriter {
public:
    void Write(i64 timestamp, TTsvWriter& writer, const NJson::TJsonValue& sessionLog, TCollapsedLogTraits& traits) override {
        traits.LastAsrResult = timestamp;
        traits.AsrResultCount++;

        TStringBuilder utterance;

        const auto* recognitionRaw = sessionLog.GetValueByPath("Directive.directive.payload.recognition");
        if (recognitionRaw) {
            const auto& recognition = recognitionRaw->GetArray();
            if (!recognition.empty()) {
                const auto& words = recognition[0]["words"].GetArray();
                for (const auto& word : words) {
                    if (!utterance.empty()) {
                        utterance << " ";
                    }
                    utterance << word["value"].GetStringRobust();
                }
            }
        }

        writer.Write("\"" + utterance + "\"");

        const auto* endOfUtt = sessionLog.GetValueByPath("Directive.directive.payload.endOfUtt");
        if (endOfUtt && endOfUtt->GetBooleanRobust()) {
            writer.Write("EoU");
        }
    }
};

REGISTER(TAsrResultEventWriter, "ASR.Result");

class TVinsVoiceInputEventWriter : public IEventWriter {
public:
    void Write(i64 timestamp, TTsvWriter& writer, const NJson::TJsonValue& sessionLog, TCollapsedLogTraits& traits) override {
        Y_UNUSED(timestamp);

        const auto* requestId = sessionLog.GetValueByPath("Event.event.payload.header.request_id");
        if (!requestId) {
            writer.Write("NULL");
        } else {
            const auto str = requestId->GetStringRobust();
            if (str.StartsWith("ffffffff-ffff-ffff")) {
                traits.IsRobot = true;
            }
            writer.Write(str);
        }
    }
};

REGISTER(TVinsVoiceInputEventWriter, "Vins.VoiceInput");

class TVinsVinsResponseEventWriter : public IEventWriter {
public:
    void Write(i64 timestamp, TTsvWriter& writer, const NJson::TJsonValue& sessionLog, TCollapsedLogTraits& traits) override {
        Y_UNUSED(timestamp);
        Y_UNUSED(traits);

        const auto* directives = sessionLog.GetValueByPath("Directive.directive.payload.response.directives");
        const auto* metas = sessionLog.GetValueByPath("Directive.directive.payload.response.meta");
        if (!directives || !metas) {
            return;
        }

        TStringBuilder metasResult;
        for (const auto& meta : metas->GetArray()) {
            if (!metasResult.empty()) {
                metasResult << ",";
            }
            metasResult << meta["intent"].GetStringRobust();
        }
        writer.Write(metasResult);

        TStringBuilder directivesResult;
        for (const auto& directive : directives->GetArray()) {
            if (!directivesResult.empty()) {
                directivesResult << ",";
            }

            TStringBuilder directiveName;
            if (directive["type"].IsDefined()) {
                directiveName << directive["type"].GetStringRobust() << ".";
            }
            directiveName << directive["name"].GetStringRobust();

            directivesResult << directiveName;
        }
        writer.Write(directivesResult);

        const auto* outputSpeech = sessionLog.GetValueByPath("Directive.directive.payload.voice_response.output_speech.text");
        if (outputSpeech) {
            const auto text = outputSpeech->GetStringRobust();
            writer.Write("\"" + text + "\"");
        }
    }
};

REGISTER(TVinsVinsResponseEventWriter, "Vins.VinsResponse");

class TVinsResponseEventWriter : public IEventWriter {
public:
    void Write(i64 timestamp, TTsvWriter& writer, const NJson::TJsonValue& sessionLog, TCollapsedLogTraits& traits) override {
        Y_UNUSED(writer);
        Y_UNUSED(sessionLog);
        Y_UNUSED(traits);

        traits.LastVinsResponse = timestamp;
        traits.VinsResponseCount++;
    }
};

REGISTER(TVinsResponseEventWriter, "VinsResponse");

class TUserFriendlyCollapsedLogWriter : public ICollapsedLogWriter {
public:
    bool Write(const TCollapsedLog& collapsedLog) override {
        TTsvWriter writer;
        TCollapsedLogTraits traits;

        TMaybe<i64> startTimestamp;
        TMaybe<i64> timestamp;

        writer.Write("===> " + collapsedLog.GetMessageId() + " <===");
        writer.FlushRow();

        for (const auto& event : collapsedLog.GetEvents()) {
            if (startTimestamp.Defined()) {
                writer.Write((event.GetTraits().GetTimestamp() - *startTimestamp) / 1000.);
            } else {
                writer.Write(0.);
                startTimestamp = event.GetTraits().GetTimestamp();
            }

            if (timestamp.Defined()) {
                writer.Write((event.GetTraits().GetTimestamp() - *timestamp) / 1000.);
            } else {
                writer.Write(0.);
            }
            timestamp = event.GetTraits().GetTimestamp();

            NJson::TJsonValue sessionLog;
            NJson2Yson::DeserializeYsonAsJsonValue(event.GetSessionLog(), &sessionLog);

            const auto type = GetType(sessionLog);
            writer.Write(type);

            auto* eventWriter = GetEventWriter(type);
            eventWriter->Write(event.GetTraits().GetTimestamp(), writer, sessionLog, traits);

            writer.FlushRow();
        }

        if (traits.IsRobot) {
            return false;
        }

        writer.Write("AsrResultCount=" + ToString(traits.AsrResultCount));
        writer.Write("VinsResponseCount=" + ToString(traits.VinsResponseCount));
        writer.FlushRow();

        writer.FlushRow();

        const auto delta = traits.LastVinsResponse - traits.LastAsrResult;
        //if (delta >= 0 && delta <= 50) {
        //if (delta >= 1880 && delta <= 1900) {
        //if (delta >= 1150 && delta <= 1200) {
        if (true || (delta >= 440 && delta <= 450)) {
            writer.Flush();
            return true;
        } else {
            return false;
        }
    }

private:
    IEventWriter* GetEventWriter(const TString& type) {
        if (!EventWriters_.contains(type)) {
            EventWriters_[type] = EventWriterFactory()->Create(type);
        }
        return EventWriters_[type].Get();
    }

    THashMap<TString, THolder<IEventWriter>> EventWriters_;
};

THolder<ICollapsedLogWriter> GetCollapsedLogWriter(EReadUniproxyLogFormat format) {
    switch (format) {
        case EReadUniproxyLogFormat::Json:
            return MakeHolder<TJsonCollapsedLogWriter>();
        case EReadUniproxyLogFormat::UserFriendly:
            return MakeHolder<TUserFriendlyCollapsedLogWriter>();
    }
}

TString BuildYPath(const TString& inputPath, const TString& messageId) {
    TStringBuilder builder;
    builder << inputPath;

    if (!messageId.empty()) {
        builder << "[\"" << messageId << "\"]";
    }

    return builder;
}

int ReadUniproxyLog(int argc, const char* argv[]) {
    NLastGetopt::TOpts opts;
    opts.SetFreeArgsNum(0);

    TString proxy;
    opts
        .AddLongOption("proxy", "YT proxy.")
        .Optional()
        .RequiredArgument("YT_PROXY")
        .StoreResult(&proxy)
        .DefaultValue("hahn");

    TString token;
    opts
        .AddLongOption("token", "YT token.")
        .Optional()
        .RequiredArgument("YT_TOKEN")
        .StoreResult(&token);

    TString inputPath;
    opts
        .AddLongOption('i', "input-path", "Path to YT table.")
        .Required()
        .RequiredArgument("YT_PATH")
        .StoreResult(&inputPath);

    size_t limit = 1000;
    opts
        .AddLongOption('n', "limit", "Row count limit.")
        .Optional()
        .RequiredArgument("SIZE")
        .DefaultValue(limit)
        .StoreResult(&limit);

    TString messageId;
    opts
        .AddLongOption("message-id", "Message id.")
        .Optional()
        .RequiredArgument("REQUEST_ID")
        .StoreResult(&messageId);

    EReadUniproxyLogFormat format = EReadUniproxyLogFormat::Json;
    opts
        .AddLongOption('f', "format", "Output format.")
        .Optional()
        .RequiredArgument("FORMAT")
        .DefaultValue(format)
        .StoreResult(&format);

    NLastGetopt::TOptsParseResult(&opts, argc, argv);

    auto client = CreateClient(proxy, token);
    auto reader = client->CreateTableReader<NYT::TNode>(BuildYPath(inputPath, messageId));
    auto writer = GetCollapsedLogWriter(format);

    for (size_t index = 0; index < limit && reader->IsValid(); reader->Next()) {
        const auto& row = reader->GetRow();

        TCollapsedLog collapsedLog;
        Y_VERIFY(collapsedLog.ParseFromString(row["CollapsedLog"].As<TString>()));

        if (writer->Write(collapsedLog)) {
            ++index;
        }
    }

    return 0;
}

bool IsAliceRequest(const NYT::TNode& row) {
    if (row["method"].As<TString>() == "POST") {
        return false;
    }
    
    if (!row["request"].As<TString>().StartsWith("/search/report_alice")) {
        return false;
    }

    return true;
}

TCgiParameters ParseCgiParameters(const TString& request) {
    TString path, rawCgiParams;
    Split(request, '?', path, rawCgiParams);
    TCgiParameters params(rawCgiParams);
    return params;
}

TString CanonizeAliceRequest(const TCgiParameters& params) {
    TVector<TString> cacheStrs;
    for (const auto& [key, value] : params) {
        if (key != "reqinfo" && key != "uuid") {
            cacheStrs.push_back(TStringBuilder() << key << "=" << value);
        }
    }

    Sort(cacheStrs);

    TStringBuilder cacheStr;
    for (const auto& str : cacheStrs) {
        if (!cacheStr.empty()) {
            cacheStr << '&';
        }
        cacheStr << str;
    }

    return cacheStr;
}

TMaybe<TString> GetProcessId(const TString& reqinfo) {
    TVector<TString> infos;
    Split(reqinfo, ";", infos);

    for (const auto& info : infos) {
        static const TString& PROCESS_PREFIX = "https://nirvana.yandex-team.ru/process/";
        if (info.StartsWith(PROCESS_PREFIX)) {
            return info.substr(PROCESS_PREFIX.size());
        }
    }

    return Nothing();
}

class TAliceRequestCacheHitHashMapper
    : public NYT::IMapper<NYT::TTableReader<NYT::TNode>, NYT::TTableWriter<NYT::TNode>> {
public:
    virtual void Do(NYT::TTableReader<NYT::TNode>* reader, NYT::TTableWriter<NYT::TNode>* writer) {
        for (; reader->IsValid(); reader->Next()) {
            const auto& row = reader->GetRow();
            if (!IsAliceRequest(row)) {
                continue;
            }

            const auto& request = row["request"].As<TString>();
            const auto params = ParseCgiParameters(request);
            if (!params.contains("reqinfo")) {
                continue;
            }

            const auto processId = GetProcessId(params.Get("reqinfo"));
            if (!processId.Defined()) {
                continue;
            }

            double requestTime = 0;
            if (!row["request_time"].IsString() || !TryFromString(row["request_time"].As<TString>(), requestTime)) {
                continue;
            }

            const auto canonizedRequest = CanonizeAliceRequest(params);

            writer->AddRow(NYT::TNode()
                ("ProcessId", *processId)
                ("RequestTime", requestTime)
                ("Hash", CityHash64(canonizedRequest)));
        }
    }
};

class TAliceRequestCacheHitHashReducer
    : public NYT::IReducer<NYT::TTableReader<NYT::TNode>, NYT::TTableWriter<NYT::TNode>> {
public:
    virtual void Do(NYT::TTableReader<NYT::TNode>* reader, NYT::TTableWriter<NYT::TNode>* writer) {
        TVector<TString> processIds;
        for (; reader->IsValid(); reader->Next()) {
            const auto& row = reader->GetRow();
            processIds.push_back(row["ProcessId"].As<TString>());
        }

        Y_VERIFY(!processIds.empty());

        TString leakyProcessId = processIds[0];
        SortUnique(processIds);

        writer->AddRow(NYT::TNode()
            ("ProcessId", leakyProcessId)
            ("LeakyCount", static_cast<ui64>(1))
            ("TotalCount", static_cast<ui64>(1)));

        for (const auto& processId : processIds) {
            if (processId != leakyProcessId) {
                writer->AddRow(NYT::TNode()
                    ("ProcessId", processId)
                    ("LeakyCount", static_cast<ui64>(0))
                    ("TotalCount", static_cast<ui64>(1)));
            }
        }
    };
};

REGISTER_MAPPER(TAliceRequestCacheHitHashMapper);
REGISTER_REDUCER(TAliceRequestCacheHitHashReducer);

class TAliceRequestCacheHitMapper
    : public NYT::IMapper<NYT::TTableReader<NYT::TNode>, NYT::TTableWriter<NYT::TNode>> {
public:
    virtual void Do(NYT::TTableReader<NYT::TNode>* reader, NYT::TTableWriter<NYT::TNode>* writer) {
        for (; reader->IsValid(); reader->Next()) {
            writer->AddRow(reader->GetRow());
        }
    }
};

class TAliceRequestCacheHitReducer
    : public NYT::IReducer<NYT::TTableReader<NYT::TNode>, NYT::TTableWriter<NYT::TNode>> {
public:
    virtual void Do(NYT::TTableReader<NYT::TNode>* reader, NYT::TTableWriter<NYT::TNode>* writer) {
        TString processId;
        ui64 leakyCount = 0;
        ui64 totalCount = 0;

        for (; reader->IsValid(); reader->Next()) {
            const auto& row = reader->GetRow();
            processId = row["ProcessId"].As<TString>();
            leakyCount += row["LeakyCount"].As<ui64>();
            totalCount += row["TotalCount"].As<ui64>();
        }

        writer->AddRow(NYT::TNode()
            ("ProcessId", processId)
            ("LeakyCount", leakyCount)
            ("TotalCount", totalCount)
            ("Ratio", static_cast<double>(1.0 * leakyCount / totalCount)));
    };
};

REGISTER_MAPPER(TAliceRequestCacheHitMapper);
REGISTER_REDUCER(TAliceRequestCacheHitReducer);

int ReadSearchAccessLog(int argc, const char* argv[]) {
    NLastGetopt::TOpts opts;
    opts.SetFreeArgsNum(0);

    TString proxy;
    opts
        .AddLongOption("proxy", "YT proxy.")
        .Optional()
        .RequiredArgument("YT_PROXY")
        .StoreResult(&proxy)
        .DefaultValue("hahn");

    TString token;
    opts
        .AddLongOption("token", "YT token.")
        .Optional()
        .RequiredArgument("YT_TOKEN")
        .StoreResult(&token);

    TString inputPath;
    opts
        .AddLongOption('i', "input-path", "Path to YT table.")
        .Required()
        .RequiredArgument("YT_PATH")
        .StoreResult(&inputPath);

    size_t limit = 1000;
    opts
        .AddLongOption('n', "limit", "Row count limit.")
        .Optional()
        .RequiredArgument("SIZE")
        .DefaultValue(limit)
        .StoreResult(&limit);

    NLastGetopt::TOptsParseResult(&opts, argc, argv);

    auto client = CreateClient(proxy, token);
    auto reader = client->CreateTableReader<NYT::TNode>(inputPath);

    for (size_t index = 0; index < limit && reader->IsValid(); reader->Next()) {
        auto row = reader->GetRow();
        
        if (!IsAliceRequest(row)) {
            continue;
        }

        const auto params = ParseCgiParameters(row["request"].As<TString>());
        row["canonized_request"] = CanonizeAliceRequest(params);

        row["request"] = NYT::TNode();
        for (const auto& [key, value] : params) {
            row["request"](key, value);
        }


        TVector<TString> headers;
        Split(row["headers"].As<TString>(), "~", headers);

        row["headers"] = NYT::TNode::CreateList();
        for (const auto& header : headers) {
            row["headers"].Add(header);
        }

        Cout << NYT::NodeToYsonString(row, NYson::EYsonFormat::Pretty) << Endl;

        ++index;
    }

    return 0;
}

int CalcAliceRequestCacheHit(int argc, const char* argv[]) {
    NLastGetopt::TOpts opts;
    opts.SetFreeArgsNum(0);

    TString proxy;
    opts
        .AddLongOption("proxy", "YT proxy.")
        .Optional()
        .RequiredArgument("YT_PROXY")
        .StoreResult(&proxy)
        .DefaultValue("hahn");

    TString token;
    opts
        .AddLongOption("token", "YT token.")
        .Optional()
        .RequiredArgument("YT_TOKEN")
        .StoreResult(&token);

    TVector<TString> inputs;
    opts
        .AddLongOption('i', "input-path", "Path to YT table.")
        .Required()
        .RequiredArgument("YT_PATH")
        .AppendTo(&inputs);

    TString outputPath;
    opts
        .AddLongOption('o', "output-path", "Path to YT table.")
        .Required()
        .RequiredArgument("YT_PATH")
        .StoreResult(&outputPath);

    NLastGetopt::TOptsParseResult(&opts, argc, argv);

    auto client = CreateClient(proxy, token);
    NYT::TTempTable tempTable(client);

    {
        NYT::TMapReduceOperationSpec operationSpec;

        operationSpec
            .AddOutput<NYT::TNode>(tempTable.Name())
            .SortBy({"Hash", "RequestTime", "ProcessId"})
            .ReduceBy({"Hash"});

        for (const auto& input : inputs) {
            operationSpec
                .AddInput<NYT::TNode>(input + "{method,request,request_time}");
        }

        client->MapReduce(
                operationSpec,
                new TAliceRequestCacheHitHashMapper(),
                new TAliceRequestCacheHitHashReducer());
    }

    {
        NYT::TMapReduceOperationSpec operationSpec;

        operationSpec
            .AddOutput<NYT::TNode>(outputPath)
            .SortBy({"ProcessId"})
            .ReduceBy({"ProcessId"})
            .AddInput<NYT::TNode>(tempTable.Name());

        client->MapReduce(
                operationSpec,
                new TAliceRequestCacheHitMapper(),
                new TAliceRequestCacheHitReducer(),
                new TAliceRequestCacheHitReducer());

        client->Sort(
            outputPath,
            outputPath,
            {"Ratio"});
    }

    return 0;
}

class TParamsStatCounterMapper
    : public NYT::IMapper<NYT::TTableReader<NYT::TNode>, NYT::TTableWriter<NYT::TNode>> {
public:
    virtual void Do(NYT::TTableReader<NYT::TNode>* reader, NYT::TTableWriter<NYT::TNode>* writer) {
        for (; reader->IsValid(); reader->Next()) {
            const auto& row = reader->GetRow();
            if (!IsAliceRequest(row)) {
                continue;
            }

            const auto& request = row["request"].As<TString>();
            const auto params = ParseCgiParameters(request);
            if (!params.contains("reqinfo") || params.Get("reqinfo").find("https://nirvana.yandex-team.ru/process") == std::string::npos) {
                continue;
            }

            for (const auto& [key, value] : params) {
                writer->AddRow(NYT::TNode()
                    ("Parameter", "request: " + key)
                    ("Value", value)
                    ("Count", static_cast<ui64>(1)));
            }

            TVector<TString> headers;
            Split(row["headers"].As<TString>(), "~", headers);
            SortUnique(headers);

            for (const auto& header : headers) {
                const auto pos = header.find(':');
                const auto key = header.substr(0, pos);
                const auto value = header.substr(pos + 1);

                writer->AddRow(NYT::TNode()
                    ("Parameter", "header: " + key)
                    ("Value", value)
                    ("Count", static_cast<ui64>(1)));
            }
        }
    }
};

class TParamsStatCounterReducer
    : public NYT::IReducer<NYT::TTableReader<NYT::TNode>, NYT::TTableWriter<NYT::TNode>> {
public:
    virtual void Do(NYT::TTableReader<NYT::TNode>* reader, NYT::TTableWriter<NYT::TNode>* writer) {
        TString parameter;
        TString value;
        ui64 count = 0;

        for (; reader->IsValid(); reader->Next()) {
            const auto& row = reader->GetRow();
            parameter = row["Parameter"].As<TString>();
            value = row["Value"].As<TString>();
            count += row["Count"].As<ui64>();
        }

        writer->AddRow(NYT::TNode()
            ("Parameter", parameter)
            ("Value", value)
            ("Count", count));
    };
};

REGISTER_MAPPER(TParamsStatCounterMapper);
REGISTER_REDUCER(TParamsStatCounterReducer);

class TParamsStatMapper
    : public NYT::IMapper<NYT::TTableReader<NYT::TNode>, NYT::TTableWriter<NYT::TNode>> {
public:
    virtual void Do(NYT::TTableReader<NYT::TNode>* reader, NYT::TTableWriter<NYT::TNode>* writer) {
        for (; reader->IsValid(); reader->Next()) {
            const auto& row = reader->GetRow();
            writer->AddRow(NYT::TNode()
                ("Parameter", row["Parameter"].As<TString>())
                ("UniqCount", static_cast<ui64>(1))
                ("TotalCount", row["Count"].As<ui64>())
                ("Ratio", static_cast<double>(1.0 / row["Count"].As<ui64>())));
        }
    }
};

class TParamsStatReducer
    : public NYT::IReducer<NYT::TTableReader<NYT::TNode>, NYT::TTableWriter<NYT::TNode>> {
public:
    virtual void Do(NYT::TTableReader<NYT::TNode>* reader, NYT::TTableWriter<NYT::TNode>* writer) {
        TString parameter;
        ui64 uniqCount = 0, totalCount = 0;

        for (; reader->IsValid(); reader->Next()) {
            const auto& row = reader->GetRow();
            parameter = row["Parameter"].As<TString>();
            uniqCount += row["UniqCount"].As<ui64>();
            totalCount += row["TotalCount"].As<ui64>();
        }

        writer->AddRow(NYT::TNode()
            ("Parameter", parameter)
            ("UniqCount", uniqCount)
            ("TotalCount", totalCount)
            ("Ratio", static_cast<double>(uniqCount * 1.0 / totalCount)));
    };
};

REGISTER_MAPPER(TParamsStatMapper);
REGISTER_REDUCER(TParamsStatReducer);

int CalcParamsStat(int argc, const char* argv[]) {
    NLastGetopt::TOpts opts;
    opts.SetFreeArgsNum(0);

    TString proxy;
    opts
        .AddLongOption("proxy", "YT proxy.")
        .Optional()
        .RequiredArgument("YT_PROXY")
        .StoreResult(&proxy)
        .DefaultValue("hahn");

    TString token;
    opts
        .AddLongOption("token", "YT token.")
        .Optional()
        .RequiredArgument("YT_TOKEN")
        .StoreResult(&token);

    TVector<TString> inputs;
    opts
        .AddLongOption('i', "input-path", "Path to YT table.")
        .Required()
        .RequiredArgument("YT_PATH")
        .AppendTo(&inputs);

    TString outputPath;
    opts
        .AddLongOption('o', "output-path", "Path to YT table.")
        .Required()
        .RequiredArgument("YT_PATH")
        .StoreResult(&outputPath);

    NLastGetopt::TOptsParseResult(&opts, argc, argv);

    auto client = CreateClient(proxy, token);
    NYT::TTempTable tempTable(client);

    {
        NYT::TMapReduceOperationSpec operationSpec;

        operationSpec
            .AddOutput<NYT::TNode>(tempTable.Name())
            .SortBy({"Parameter", "Value"})
            .ReduceBy({"Parameter", "Value"});

        for (const auto& input : inputs) {
            operationSpec
                .AddInput<NYT::TNode>(input + "{method,request,headers}");
        }

        client->MapReduce(
                operationSpec,
                new TParamsStatCounterMapper(),
                new TParamsStatCounterReducer(),
                new TParamsStatCounterReducer());
    }

    {
        NYT::TMapReduceOperationSpec operationSpec;

        operationSpec
            .AddOutput<NYT::TNode>(outputPath)
            .SortBy({"Parameter"})
            .ReduceBy({"Parameter"})
            .AddInput<NYT::TNode>(tempTable.Name());

        client->MapReduce(
                operationSpec,
                new TParamsStatMapper(),
                new TParamsStatReducer(),
                new TParamsStatReducer());
    }

    return 0;
}

int ReadWonderLog(int argc, const char* argv[]) {
    NLastGetopt::TOpts opts;
    opts.SetFreeArgsNum(0);

    TString proxy;
    opts
        .AddLongOption("proxy", "YT proxy.")
        .Optional()
        .RequiredArgument("YT_PROXY")
        .StoreResult(&proxy)
        .DefaultValue("hahn");

    TString token;
    opts
        .AddLongOption("token", "YT token.")
        .Optional()
        .RequiredArgument("YT_TOKEN")
        .StoreResult(&token);

    TString inputPath;
    opts
        .AddLongOption('i', "input-path", "Path to YT table.")
        .Required()
        .RequiredArgument("YT_PATH")
        .StoreResult(&inputPath);

    size_t limit = 1000;
    opts
        .AddLongOption('n', "limit", "Row count limit.")
        .Optional()
        .RequiredArgument("SIZE")
        .DefaultValue(limit)
        .StoreResult(&limit);

    NLastGetopt::TOptsParseResult(&opts, argc, argv);

    auto client = CreateClient(proxy, token);
    auto reader = client->CreateTableReader<NYT::TNode>(inputPath);

    for (size_t index = 0; index < limit && reader->IsValid(); reader->Next()) {
        auto row = reader->GetRow();
        Cout << NYT::NodeToYsonString(row, NYson::EYsonFormat::Pretty) << Endl;
        ++index;
    }

    return 0;
}

struct TWonderRow {
    TString UUId;
    TString AppId;
    NYT::TNode Directives;
    ui64 Timestamp = 0;
    THashSet<TString> Intents;
};

bool ParseWonderRow(const NYT::TNode& row, TWonderRow& result) {
    const auto& uuid = row["_uuid"];
    if (!uuid.IsString()) {
        ERROR_LOG << "_uuid is not a string." << Endl;
        return false;
    }
    result.UUId = uuid.As<TString>();

    const auto& request = row["speechkit_request"];
    if (!request.HasValue()) {
        ERROR_LOG << "There is no speechkit_request in the row." << Endl;
        return false;
    }

    const auto& app = request["application"];
    if (!app.HasValue()) {
        ERROR_LOG << "There is no application in the speechkit_request." << Endl;
        return false;
    }

    if (!TryFromString(app["timestamp"].As<TString>(), result.Timestamp)) {
        ERROR_LOG << "Cannot parse timestamp from the application object." << Endl;
        return false;
    }

    const auto& appId = app["app_id"];
    if (!appId.IsString()) {
        ERROR_LOG << "AppId is not a string." << Endl;
        return false;
    }
    result.AppId = appId.As<TString>();

    const auto& response = row["speechkit_response"];
    if (!response.HasValue()) {
        ERROR_LOG << "Tehere is no speechkit_response in the row." << Endl;
        return false;
    }

    const auto& responseResponse = response["response"];
    if (!responseResponse.HasValue()) {
        ERROR_LOG << "There is no response in the speechkit_response." << Endl;
        return false;
    }

    result.Directives = responseResponse["directives"];
    if (!result.Directives.IsList()) {
        ERROR_LOG << "There is no directives in the response." << Endl;
        return false;
    }

    const auto& analyticsInfo = response["megamind_analytics_info"]["analytics_info"];
    if (!analyticsInfo.IsList()) {
        ERROR_LOG << "AnalyticsInfo is not a list." << Endl;
        return false;
    }

    for (const auto& element : analyticsInfo.AsList()) {
        if (!element.IsList()) {
            continue;
        }

        for (const auto& value : element.AsList()) {
            if (!value.IsMap()) {
                continue;
            }

            const auto& scenarioAnalyticsInfo = value["scenario_analytics_info"];
            if (!scenarioAnalyticsInfo.IsMap()) {
                continue;
            }

            const auto& intent = scenarioAnalyticsInfo["intent"];
            if (!intent.IsString()) {
                ERROR_LOG << "Intent is not a string." << Endl;
                continue;
            }

            result.Intents.insert(intent.As<TString>());
        }
    }

    return true;
}

class TSequentialSoundSetLevelMapper
    : public NYT::IMapper<NYT::TTableReader<NYT::TNode>, NYT::TTableWriter<NYT::TNode>> {
public:
    virtual void Do(NYT::TTableReader<NYT::TNode>* reader, NYT::TTableWriter<NYT::TNode>* writer) {
        for (; reader->IsValid(); reader->Next()) {
            TWonderRow row;
            if (!ParseWonderRow(reader->GetRow(), row)) {
                continue;
            }

            if (row.UUId.StartsWith("deadbeef") || !IsSmartSpeaker(row.AppId)) {
                ERROR_LOG << "The request doesn't fit the constraints, uuid: " << row.UUId << ", AppId: " << row.AppId << Endl;
                continue;
            }

            NYT::TNode intents = NYT::TNode::CreateList();
            for (const auto& intent : row.Intents) {
                intents.Add(intent);
            }

            NYT::TNode result;
            result
                ("UserId", row.UUId)
                ("Timestamp", row.Timestamp)
                ("Intents", intents);

            writer->AddRow(result);
        }
    }
};

enum class ESoundSetLevel {
    Louder = 0,
    Quiter = 1,
};

class TSequentialSoundSetLevelReducer
    : public NYT::IReducer<NYT::TTableReader<NYT::TNode>, NYT::TTableWriter<NYT::TNode>> {
public:
    virtual void Do(NYT::TTableReader<NYT::TNode>* reader, NYT::TTableWriter<NYT::TNode>* writer) {
        TVector<std::pair<ui64, TMaybe<ESoundSetLevel>>> levels;

        for (; reader->IsValid(); reader->Next()) {
            const auto& row = reader->GetRow();

            TMaybe<ESoundSetLevel> soundSetLevel;
            for (const auto& intent : row["Intents"].AsList()) {
                if (intent == "personal_assistant.scenarios.sound_quiter") {
                    soundSetLevel = ESoundSetLevel::Quiter;
                } else if (intent == "personal_assistant.scenarios.sound_louder") {
                    soundSetLevel = ESoundSetLevel::Louder;
                }
            }

            levels.emplace_back(row["Timestamp"].As<ui64>(), soundSetLevel);
        }

        Sort(levels, [](const auto& a, const auto& b) {
            return a.first < b.first;
        });

        ui64 sequentialCount = 0;
        for (size_t i = 1; i < levels.size(); ++i) {
            if (!levels[i - 1].second.Defined() || !levels[i].second.Defined()) {
                continue;
            }
            if (levels[i - 1].first + 30 >= levels[i].first && *levels[i - 1].second == *levels[i].second) {
                ++sequentialCount;
            }
        }

        ui64 totalCount = 0;
        for (const auto& value : levels) {
            if (value.second.Defined()) {
                ++totalCount;
            }
        }

        writer->AddRow(NYT::TNode()
            ("Key", "Total")
            ("Value", totalCount));

        writer->AddRow(NYT::TNode()
            ("Key", "Sequential")
            ("Value", sequentialCount));
    };
};

REGISTER_MAPPER(TSequentialSoundSetLevelMapper);
REGISTER_REDUCER(TSequentialSoundSetLevelReducer);

class TSequentialSoundSetLevelFinalReducer
    : public NYT::IReducer<NYT::TTableReader<NYT::TNode>, NYT::TTableWriter<NYT::TNode>> {
public:
    virtual void Do(NYT::TTableReader<NYT::TNode>* reader, NYT::TTableWriter<NYT::TNode>* writer) {
        TString key;
        ui64 value = 0;

        for (; reader->IsValid(); reader->Next()) {
            const auto& row = reader->GetRow();
            key = row["Key"].As<TString>();
            value += row["Value"].As<ui64>();
        }

        writer->AddRow(NYT::TNode()
            ("Key", key)
            ("Value", value));
    };
};

REGISTER_REDUCER(TSequentialSoundSetLevelFinalReducer);

int CalcSequentialSoundSetLevel(int argc, const char* argv[]) {
    NLastGetopt::TOpts opts;
    opts.SetFreeArgsNum(0);

    TString proxy;
    opts
        .AddLongOption("proxy", "YT proxy.")
        .Optional()
        .RequiredArgument("YT_PROXY")
        .StoreResult(&proxy)
        .DefaultValue("hahn");

    TString token;
    opts
        .AddLongOption("token", "YT token.")
        .Optional()
        .RequiredArgument("YT_TOKEN")
        .StoreResult(&token);

    TVector<TString> inputs;
    opts
        .AddLongOption('i', "input-path", "Path to YT table.")
        .Required()
        .RequiredArgument("YT_PATH")
        .AppendTo(&inputs);

    TString outputPath;
    opts
        .AddLongOption('o', "output-path", "Path to YT table.")
        .Required()
        .RequiredArgument("YT_PATH")
        .StoreResult(&outputPath);

    TString stderrPath;
    opts
        .AddLongOption("stderr", "Path to YT table.")
        .Optional()
        .RequiredArgument("YT_PATH")
        .StoreResult(&stderrPath);

    TString tempTablePath;
    opts
        .AddLongOption("temp-table", "Path to YT temp table.")
        .Optional()
        .RequiredArgument("YT_PATH")
        .StoreResult(&tempTablePath);

    NLastGetopt::TOptsParseResult(&opts, argc, argv);

    auto client = CreateClient(proxy, token);

    THolder<NYT::TTempTable> tempTable;
    if (tempTablePath.empty()) {
        tempTable = MakeHolder<NYT::TTempTable>(client);
        tempTablePath = tempTable->Name();
    }

    {
        NYT::TMapReduceOperationSpec operationSpec;
        operationSpec
            .AddOutput<NYT::TNode>(tempTablePath)
            .SortBy({"UserId"})
            .ReduceBy({"UserId"});

        if (!stderrPath.empty()) {
            operationSpec.StderrTablePath(stderrPath);
        }

        for (const auto& input : inputs) {
            operationSpec.AddInput<NYT::TNode>(input);
        }

        client->MapReduce(
            operationSpec,
            new TSequentialSoundSetLevelMapper(),
            new TSequentialSoundSetLevelReducer());
    }

    client->Sort(tempTablePath, tempTablePath, {"Key"});

    {
        NYT::TReduceOperationSpec operationSpec;
        operationSpec
            .AddInput<NYT::TNode>(tempTablePath)
            .AddOutput<NYT::TNode>(outputPath)
            .SortBy({"Key"})
            .ReduceBy({"Key"});

        client->Reduce(
            operationSpec,
            new TSequentialSoundSetLevelFinalReducer());
    }

    return 0;
}

int main(int argc, const char* argv[]) {
    SetEnv("YT_LOG_LEVEL", GetEnv("YT_LOG_LEVEL", "INFO"));
    InitGlobalLog2Console(TLOG_DEBUG);
    NYT::Initialize(argc, argv);

    TModChooser modChooser;
    modChooser.AddMode(
        "collapse-uniproxy-log",
        CollapseUniproxyLog,
        "Collapse uniproxy log with messaage id.");
    modChooser.AddMode(
        "read-uniproxy-log",
        ReadUniproxyLog,
        "Read uniproxy log.");
    modChooser.AddMode(
        "read-search-access-log",
        ReadSearchAccessLog,
        "Read search access log.");
    modChooser.AddMode(
        "calc-params-stat",
        CalcParamsStat,
        "Calc params stat.");
    modChooser.AddMode(
        "calc-alice-request-cache-hit",
        CalcAliceRequestCacheHit,
        "Calc Alice request cache hit.");
    modChooser.AddMode(
        "read-wonder-log",
        ReadWonderLog,
        "Read Alice wonder log.");
    modChooser.AddMode(
        "calc-sequential-sound-set-level",
        CalcSequentialSoundSetLevel,
        "Calc sequential set sound level.");

    return modChooser.Run(argc, argv);
}
