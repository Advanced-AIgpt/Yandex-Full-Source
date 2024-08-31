#include "parser.h"

#include <robot/library/fork_subscriber/fork_subscriber.h>
#include <alice/rtlog/common/service_instance_repository.h>
#include <alice/rtlog/common/eventlog/formatting.h>
#include <alice/rtlog/protos/rtlog.ev.pb.h>
#include <alice/library/go/setrace/protos/log.pb.h>

#include <google/protobuf/compiler/importer.h>
#include <google/protobuf/io/zero_copy_stream_impl_lite.h>
#include <google/protobuf/dynamic_message.h>
#include <google/protobuf/io/zero_copy_stream_impl.h>
#include <ydb/public/sdk/cpp/client/ydb_driver/driver.h>
#include <ydb/public/sdk/cpp/client/ydb_table/table.h>
#include <library/cpp/cache/cache.h>
#include <library/cpp/eventlog/evdecoder.h>
#include <library/cpp/eventlog/events_extension.h>
#include <library/cpp/eventlog/logparser.h>
#include <library/cpp/eventlog/proto/events_extension.pb.h>
#include <library/cpp/framing/unpacker.h>
#include <library/cpp/json/json_writer.h>
#include <library/cpp/protobuf/util/pb_io.h>
#include <library/cpp/string_utils/base64/base64.h>

#include <util/charset/utf8.h>
#include <util/folder/path.h>
#include <util/generic/cast.h>
#include <util/string/builder.h>
#include <util/string/cast.h>
#include <util/string/printf.h>
#include <util/stream/file.h>
#include <util/stream/mem.h>
#include <util/system/file.h>

using namespace google::protobuf;
using namespace google::protobuf::compiler;
using namespace google::protobuf::io;
using namespace NRTLogEvents;
using namespace NKikimr::NUdf;
using namespace NProtoBuf;
using namespace NRobot;

namespace {
    using namespace NRTLog;

    void NoopEventSerializer(const Message* event, IOutputStream& output, EFieldOutputFlags flags) {
        Y_UNUSED(event);
        Y_UNUSED(output);
        Y_UNUSED(flags);
    }

    class TProtoSourceTree final: public SourceTree {
    public:
        explicit TProtoSourceTree(const TFsPath& directoryPath)
                : DirectoryPath(directoryPath)
        {
        }

        ZeroCopyInputStream* Open(const string& filename) override {
            const TFsPath filePath = DirectoryPath / filename;
            Y_VERIFY(filePath.Exists(), "file [%s] not found", filePath.GetPath().c_str());
            TFileHandle fileHandle(filePath.GetPath(), OpenExisting | RdOnly);
            FileInputStream* result = new FileInputStream(fileHandle.Release());
            result->SetCloseOnDelete(true);
            return result;
        }

    private:
        const TFsPath& DirectoryPath;
    };

    class TProtoErrorCollector final: public MultiFileErrorCollector {
    public:
        void AddError(const string& filename, int line, int column, const string& message) override {
            Y_FAIL("proto error, file name [%s], line [%d], column [%d], message [%s]",
                   filename.c_str(), line, column, message.c_str());
        }
    };

    class TProtoBundle {
    public:
        explicit TProtoBundle(const TString& directoryPath)
                : DirectoryPath(directoryPath)
                , SourceTree(DirectoryPath)
                , ErrorCollector()
                , Importer(&SourceTree, &ErrorCollector)
                , Factory(Importer.pool())
                , Files()
        {
            TVector<TFsPath> files;
            DirectoryPath.List(files);
            for (const TFsPath& f: files) {
                if (!f.IsDirectory()) {
                    const TFsPath relativePath = f.RelativePath(DirectoryPath);
                    Files.push_back(Importer.Import(relativePath.GetPath()));
                }
            }
        }

        const DescriptorPool& GetPool() {
            return *Importer.pool();
        }

        MessageFactory& GetFactory() {
            return Factory;
        }

        const TVector<const FileDescriptor*>& GetFiles() const {
            return Files;
        }

    private:
        TFsPath DirectoryPath;
        TProtoSourceTree SourceTree;
        TProtoErrorCollector ErrorCollector;
        Importer Importer;
        DynamicMessageFactory Factory;
        TVector<const FileDescriptor*> Files;
    };

    class TDynamicEventsRegistry {
    public:
        explicit TDynamicEventsRegistry(const TString& protoDirectory)
                : ProtoBundle(protoDirectory)
                , EventFactory()
                , ProtobufEventFactory(&EventFactory)
        {
            RegisterFile(*ActivationStarted::descriptor()->file(), *MessageFactory::generated_factory());
            for (const auto& f: ProtoBundle.GetFiles()) {
                RegisterFile(*f, ProtoBundle.GetFactory());
            }
        }

        IEventFactory& GetEventFactory() {
            return ProtobufEventFactory;
        }

    private:
        void RegisterFile(const FileDescriptor& file, MessageFactory& messageFactory) {
            for (int i = 0; i < file.message_type_count(); ++i) {
                auto descriptor = file.message_type(i);
                EventFactory.RegisterEvent(descriptor->options().GetExtension(message_id),
                                           messageFactory.GetPrototype(descriptor),
                                           NoopEventSerializer);
            }
        }

    private:
        TProtoBundle ProtoBundle;
        TEventFactory EventFactory;
        TProtobufEventFactory ProtobufEventFactory;
    };

    class TInstanceIdResolver {
    public:
        explicit TInstanceIdResolver(TYdbSettings ydbSettings)
                : YdbSettings(std::move(ydbSettings))
                , Repository(nullptr)
                , Cache(2500)
                , ForkSubscriber(TForkObserver()
                                         .WithPrepareAction([this] {
                                             Repository.Reset();
                                         }))
        {
        }

        ui64 Resolve(const TInstanceDescriptor& instanceDescriptor) {
            const auto cacheKey = std::make_tuple(instanceDescriptor.GetServiceName(), instanceDescriptor.GetHostName());
            const auto it = Cache.Find(cacheKey);
            if (it != Cache.End()) {
                return it.Value();
            }
            if (!Repository) {
                Repository = MakeHolder<TServiceInstanceRepository>(YdbSettings);
            }
            const ui64 result = ResolveOrDie(instanceDescriptor);
            Cache.Insert(cacheKey, result);
            return result;
        }

    private:
        ui64 ResolveOrDie(const TInstanceDescriptor& instanceDescriptor) {
            try {
                return Repository->Resolve(instanceDescriptor);
            }
            catch (const yexception& e) {
                Y_FAIL("failed to resolve instance id, %s", e.what());
            }
        }

        TYdbSettings YdbSettings;
        THolder<TServiceInstanceRepository> Repository;
        TLRUCache<std::tuple<TString, TString>, ui64> Cache;
        TForkSubscriber ForkSubscriber;
    };

    class TScenarioLogParser final: public IScenarioLogParser {
    public:
        explicit TScenarioLogParser(TYdbSettings ydbSettings, const TString& protoDirectory,
                               const TScenarioLogParserCounters& counters)
                : InstanceIdResolver(MakeHolder<TInstanceIdResolver>(std::move(ydbSettings)))
                , DynamicEventsRegistry(protoDirectory.empty()
                                        ? nullptr
                                        : MakeHolder<TDynamicEventsRegistry>(protoDirectory))
                , Counters(counters)
        {
        }

        TVector<TRecord> Parse(const TStringBuf& chunk) const override {
            TVector<TScenarioLog> scenarioLogs = ParseProtoseq(chunk);
            TVector<TRecord> records;
            records.reserve(scenarioLogs.size());
            for (size_t i = 0; i < scenarioLogs.size(); ++i) {
                const auto& scenarioLog = scenarioLogs[i];
                records.emplace_back();
                {
                    auto& item = *records.back().MutableEventItem();
                    item.SetReqTimestamp(-static_cast<i64>(scenarioLog.GetReqTimestamp()));
                    item.SetReqId(scenarioLog.GetReqId());
                    item.SetActivationId(scenarioLog.GetActivationId());
                    try {
                        item.SetInstanceId(InstanceIdResolver->Resolve(scenarioLog.GetInstanceDescriptor()));
                    } catch (const yexception& e) {
                        Cerr << "Dropping frame, reason: " << e.what() << Endl;
                        continue;
                    }
                    item.SetServiceName(scenarioLog.GetInstanceDescriptor().GetServiceName());
                    item.SetFrameId(scenarioLog.GetFrameId());
                    item.SetEventIndex(scenarioLog.GetEventIndex());
                    if (scenarioLog.HasActivationStarted()) {
                        item.SetEventType("ActivationStarted");
                        item.SetEvent(FormatAsJson(scenarioLog.GetActivationStarted()));
                    } else if (scenarioLog.HasActivationFinished()) {
                        item.SetEventType("ActivationFinished");
                        item.SetEvent(FormatAsJson(scenarioLog.GetActivationFinished()));
                    } else if (scenarioLog.HasCreateRequestContext()) {
                        item.SetEventType("CreateRequestContext");
                        item.SetEvent(FormatAsJson(scenarioLog.GetCreateRequestContext()));
                    } else if (scenarioLog.HasChildActivationStarted()) {
                        item.SetEventType("ChildActivationStarted");
                        item.SetEvent(FormatAsJson(scenarioLog.GetChildActivationStarted()));
                    } else if (scenarioLog.HasChildActivationFinished()) {
                        item.SetEventType("ChildActivationFinished");
                        item.SetEvent(FormatAsJson(scenarioLog.GetChildActivationFinished()));
                    } else if (scenarioLog.HasLogEvent()) {
                        item.SetEventType("LogEvent");
                        item.SetEvent(FormatAsJson(scenarioLog.GetLogEvent()));
                    } else if (scenarioLog.HasSearchRequest()) {
                        item.SetEventType("SearchRequest");
                        item.SetEvent(FormatAsJson(scenarioLog.GetSearchRequest()));
                    }
                    item.SetTimestamp(scenarioLog.GetTimestamp());
                }
            }
            return records;
        }

        TString FormatAsJson(const Message& message) const {
            const auto formattedMessage = FormatRTLogEvent(message);
            if (formattedMessage.StrippedBytesCount > 0) {
                Counters.StrippedMessagesCount.Inc();
                Counters.StrippedBytesCount.Add(formattedMessage.StrippedBytesCount);
            }

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

        TVector<TScenarioLog> ParseProtoseq(const TStringBuf& chunk) const {
            NFraming::TUnpacker protoseqUnpacker(chunk);
            TVector<TScenarioLog> scenarioLogs;
            TStringBuf currentChunk;
            TStringBuf skip;
            while(protoseqUnpacker.NextFrame(currentChunk, skip)) {
                scenarioLogs.emplace_back();
                auto &sLog = scenarioLogs.back();
                if (!sLog.ParseFromString(TString(currentChunk))) {
                    Cerr << "error parsing protoseq, failed to parse frame, "
                    "skip rest of frames in batch" << Endl;
                    break;
                }
            }
            return scenarioLogs;
        }
    private:
        THolder<TInstanceIdResolver> InstanceIdResolver;
        THolder<TDynamicEventsRegistry> DynamicEventsRegistry;
        mutable TScenarioLogParserCounters Counters;
    };
}

namespace NRTLog {
    THolder<IScenarioLogParser> MakeScenarioLogParser(TYdbSettings ydbSettings, const TString& protoDirectory,
                                            const TScenarioLogParserCounters& counters) {
        return MakeHolder<TScenarioLogParser>(std::move(ydbSettings), protoDirectory, counters);
    }
}
