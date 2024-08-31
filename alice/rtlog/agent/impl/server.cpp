#include <alice/rtlog/agent/impl/counters.h>
#include <alice/rtlog/agent/impl/message_sender.h>
#include <alice/rtlog/agent/impl/rtlog_handler.h>
#include <alice/rtlog/agent/impl/server.h>
#include <alice/rtlog/agent/protos/config.pb.h>

#include <robot/rthub/misc/common.h>
#include <robot/rthub/misc/shmem_queue.h>
#include <library/cpp/monlib/service/monservice.h>
#include <library/cpp/monlib/dynamic_counters/page.h>

#include <util/folder/iterator.h>
#include <util/generic/deque.h>
#include <util/string/split.h>
#include <util/string/printf.h>
#include <util/string/subst.h>
#include <util/string/type.h>
#include <util/system/file.h>
#include <util/system/fs.h>

#include <thread>

namespace NRTLogAgent {
    using namespace NRTHub;
    using namespace NMonitoring;
    using namespace NThreading;
    using namespace google::protobuf;

    namespace {
        class TLimiter {
        public:
            TLimiter(const TLimitConfig& config, std::function<void()> purge,
                     TDeprecatedCounter& contentionCounter, TDeprecatedCounter& itemsCounter, TDeprecatedCounter& sizeCounter)
                : Config(config)
                , Purge(purge)
                , ItemsCount(0)
                , ItemsSizeInBytes(0)
                , ContentionCounter(contentionCounter)
                , ItemsCounter(itemsCounter)
                , SizeCounter(sizeCounter)
            {
            }

            void Increment(ui32 count, ui64 size) {
                Y_VERIFY(!Config.HasItemsCount() || count < Config.GetItemsCount(),
                         "requested count [%u] exceeds total limit [%u]",
                         count, Config.GetItemsCount());
                Y_VERIFY(!Config.HasItemsSizeInBytes() || size < Config.GetItemsSizeInBytes(),
                         "requested size [%lu] exceeds total limit [%lu]",
                         size, Config.GetItemsSizeInBytes());
                with_lock (SpinLock) {
                    ui32 newItemsCount;
                    ui64 newItemsSizeInBytes;
                    while (true) {
                        newItemsCount = ItemsCount + count;
                        newItemsSizeInBytes = ItemsSizeInBytes + size;
                        if (!Violated(newItemsCount, newItemsSizeInBytes)) {
                            break;
                        }
                        ++ContentionCounter;
                        auto u = Unguard(SpinLock);
                        Sleep(TDuration::MilliSeconds(10));
                        Purge();
                    }
                    ItemsCount = newItemsCount;
                    ItemsSizeInBytes = newItemsSizeInBytes;
                    ItemsCounter = newItemsCount;
                    SizeCounter = newItemsSizeInBytes;
                }
            }

            void Decrement(ui32 count, ui64 size) {
                with_lock (SpinLock) {
                    Y_VERIFY(ItemsCount >= count);
                    Y_VERIFY(ItemsSizeInBytes >= size);
                    ItemsCount -= count;
                    ItemsSizeInBytes -= size;
                    ItemsCounter -= count;
                    SizeCounter -= size;
                }
            }

        private:
            inline bool Violated(ui32 newItemsCount, ui64 newItemsSizeInBytes) const {
                return Config.HasItemsCount() && newItemsCount > Config.GetItemsCount() ||
                       Config.HasItemsSizeInBytes() && newItemsSizeInBytes > Config.GetItemsSizeInBytes();
            }

            TLimitConfig Config;
            std::function<void()> Purge;
            ui32 ItemsCount;
            ui64 ItemsSizeInBytes;
            TDeprecatedCounter& ContentionCounter;
            TDeprecatedCounter& ItemsCounter;
            TDeprecatedCounter& SizeCounter;
            TSpinLock SpinLock;
        };

        class TServer;

        class TQueryContentPage final: public IMonPage {
        public:
            TQueryContentPage(const TString& path, TServer& server)
                : NMonitoring::IMonPage(path)
                , Server(server)
            {
            }

            void Output(IMonHttpRequest& request) override;

        private:
            TServer& Server;
        };

        class TServer final: public TNonCopyable, public IServer {
        public:
            explicit TServer(const TConfig& config)
                : Config(config)
                , Counters(MakeIntrusive<TRTLogAgentCounters>())
                , Queue(MakeSharedMemoryQueue(Config.GetAppEndpoint().GetMaxItems(),
                                              Config.GetAppEndpoint().GetQueueName()))
                , MonService(nullptr)
                , Segments()
                , SegmentsByTimestamp()
                , TotalSegmentsSize(0)
                , SendInflightLimiter(Config.GetMessageSender().GetInflightLimit(),
                                      [this]() { PurgeSendInflightMessages(false); },
                                      Counters->MessageSenderCounters.OutputContention,
                                      Counters->MessageSenderCounters.InflightOutputMessagesCount,
                                      Counters->MessageSenderCounters.InflightOutputBytesCount)
                , SendInflightMessages()
                , LogHandler(MakeRTLogHandler())
                , MessageSender(MakeLogbrokerMessageSender(Config.GetMessageSender().GetQueue()))
                , WorkerThread()
                , Lock()
                , SendInflightLock()
            {
                Y_VERIFY(config.GetLocalStorage().GetSegmentSizeInBytes() <= Config.GetLocalStorage().GetAvailableSpaceInBytes(),
                         "SegmentSizeInBytes [%u] is greater than AvailableSpaceInBytes [%lu]",
                         config.GetLocalStorage().GetSegmentSizeInBytes(), Config.GetLocalStorage().GetAvailableSpaceInBytes());
                Y_VERIFY(Config.GetAppEndpoint().GetMaxItemSizeInBytes() <= config.GetLocalStorage().GetSegmentSizeInBytes(),
                         "MaxItemSizeInBytes [%u] is greater than SegmentSizeInBytes [%u]",
                         Config.GetAppEndpoint().GetMaxItemSizeInBytes(), config.GetLocalStorage().GetSegmentSizeInBytes());

                TFsPath directoryPath(Config.GetLocalStorage().GetDirectory());
                if (directoryPath.Exists()) {
                    Y_VERIFY(directoryPath.IsDirectory(), "path [%s] is not a directory", directoryPath.RealLocation().c_str());
                } else {
                    directoryPath.MkDir();
                }
                TDirIterator dir(directoryPath, TDirIterator::TOptions(FTS_LOGICAL));
                for (auto file = dir.begin(), end = dir.end(); file != end; ++file) {
                    if (file->fts_level == FTS_ROOTLEVEL) {
                        continue;
                    }
                    TFsPath fsPath(file->fts_path);
                    auto name = fsPath.GetName();
                    SubstGlobal(name, '_', ':');
                    TInstant timestamp;
                    if (!TInstant::TryParseIso8601(name, timestamp)) {
                        continue;
                    }
                    RegisterSegment(fsPath.RealLocation(), timestamp, false);
                }
                if (!Segments.empty()) {
                    Sort(Segments, [](const auto& s1, const auto& s2) {
                        return s1->GetTimestamp() < s2->GetTimestamp();
                    });
                    Segments.back()->MakeWritable();
                }
                Queue->MakeReadonly();
            }

            void Start() override {
                THttpServerOptions httpServerOptions(Config.GetPublicEndpoint().GetPort());
                httpServerOptions.SetThreads(5);
                httpServerOptions.SetMaxConnections(100);
                httpServerOptions.SetMaxQueueSize(100);
                httpServerOptions.EnableKeepAlive(false);
                MonService = MakeHolder<TMonService2>(httpServerOptions, "RTLogAgent");
                MonService->Register(new TDynamicCountersPage("rtlog-agent", "Counters", Counters,
                                                              [this]() { UpdateCounters(); }));
                MonService->Register(new TQueryContentPage("query-content", *this));
                MonService->Start();

                WorkerThread = std::thread(&TServer::Run, this);
            }

            void Stop() override {
                Queue->Stop();
                WorkerThread.join();
                PurgeSendInflightMessages(true);
                MonService->Stop();
            }

            TMaybe<TString> QueryContent(const TLogItemLocation& location, const TMaybe<TFieldSpec>& keySpec, bool pretty) noexcept {
                TIntrusivePtr<TSegmentInfo> segment;
                with_lock(Lock) {
                    const auto it = SegmentsByTimestamp.find(TInstant::Seconds(location.SegmentTimestamp));
                    if (it == SegmentsByTimestamp.end()) {
                        return Nothing();
                    }
                    segment = it->second;
                }
                try {
                    auto content = segment->Read(location.Offset, location.Size);
                    if (!content.Defined()) {
                        return Nothing();
                    }
                    return LogHandler->ParseForQuery(*content, keySpec, pretty);
                } catch (...) {
                    ++Counters->ReadErrorsCount;
                    Log(ELogType::Error,
                        Sprintf("TServer::ReadContent location [%s], error [%s]",
                                ToString(location).c_str(), CurrentExceptionMessage().c_str()));
                    return Nothing();
                }
            }

        private:
            class TSegmentInfo: public TAtomicRefCount<TSegmentInfo> {
            public:
                explicit TSegmentInfo(TString filePath, TInstant timestamp, bool createFile)
                    : FilePath(std::move(filePath))
                    , Size()
                    , Timestamp(timestamp)
                    , FileForReading(MakeHolder<TFile>(FilePath, (createFile ? CreateNew : OpenExisting) | RdOnly))
                    , FileForWriting(nullptr)
                    , Lock()
                {
                    const i64 fileSize = FileForReading->GetLength();
                    Y_VERIFY(fileSize >= 0 && fileSize <= std::numeric_limits<ui32>::max(),
                             "unexpected file size [%ld]", fileSize);
                    Size = static_cast<ui32>(fileSize);
                }

                void RemoveFile() {
                    with_lock(Lock) {
                        FileForWriting.Destroy();
                        FileForReading.Destroy();
                        NFs::Remove(FilePath);
                    }
                }

                ui32 Write(const char* data, size_t size) {
                    Y_VERIFY(FileForWriting);
                    FileForWriting->Write(data, size);
                    const auto result = Size;
                    Size += size;
                    return result;
                }

                void MakeWritable() {
                    Y_VERIFY(!FileForWriting);
                    FileForWriting = MakeHolder<TFile>(FilePath, OpenExisting | WrOnly | ForAppend);
                }

                void MakeNonWritable() {
                    Y_VERIFY(FileForWriting);
                    FileForWriting.Destroy();
                }

                TMaybe<TBuffer> Read(ui32 offset, ui32 size) {
                    with_lock(Lock) {
                        if (!FileForReading) {
                            return Nothing();
                        }
                        FileForReading->Seek(offset, sSet);
                        TBuffer buffer;
                        buffer.Resize(size);
                        const auto readBytes = FileForReading->Read(buffer.Data(), size);
                        if (readBytes != size) {
                            return Nothing();
                        }
                        return TMaybe<TBuffer>(std::move(buffer));
                    }
                }

                ui32 GetSize() const noexcept {
                    return Size;
                }

                TInstant GetTimestamp() const noexcept {
                    return Timestamp;
                }

            private:
                TString FilePath;
                ui32 Size;
                TInstant Timestamp;
                THolder<TFile> FileForReading;
                THolder<TFile> FileForWriting;
                TMutex Lock;
            };

            struct TSendInflightMessage {
                TFuture<void> Future;
                size_t Size;
            };

        private:
            void Run() noexcept {
                while (true) {
                    TDequeueScope dequeueScope(*Queue);
                    if (!dequeueScope.Transaction) {
                        break;
                    }
                    const auto transactionSize = dequeueScope.Transaction->GetSize();
                    if (transactionSize > Config.GetAppEndpoint().GetMaxItemSizeInBytes()) {
                        ++Counters->MaxItemSizeViolationsCount;
                        Log(ELogType::Error,
                            Sprintf("transaction size [%lu] is greater than MaxItemSizeInBytes [%u]",
                                     transactionSize, Config.GetAppEndpoint().GetMaxItemSizeInBytes()));
                        continue;
                    }
                    while (TotalSegmentsSize + transactionSize > Config.GetLocalStorage().GetAvailableSpaceInBytes()) {
                        RemoveOldestSegment();
                    }
                    if (Segments.empty() || Segments.back()->GetSize() + transactionSize > Config.GetLocalStorage().GetSegmentSizeInBytes()) {
                        CreateNewSegment();
                    }
                    auto& segment = *Segments.back();
                    const auto offset = segment.Write(dequeueScope.Transaction->GetBytes(), transactionSize);
                    TotalSegmentsSize += transactionSize;
                    ++Counters->WrittenItemsCount;
                    Counters->WrittenBytesCount += transactionSize;

                    TLogItemLocation location{static_cast<ui32>(segment.GetTimestamp().Seconds()),
                                              offset,
                                              static_cast<ui32>(transactionSize)};
                    THolder<Message> indexData;
                    try {
                        indexData = LogHandler->ParseForIndex(dequeueScope.Transaction->GetBytes(), location);
                        Y_VERIFY(indexData);
                    } catch(...) {
                        ++Counters->ParseFailuresCount;
                        Log(ELogType::Error,
                            Sprintf("ParseIndexData failed, location [%s], error [%s]",
                                    ToString(location).c_str(), CurrentExceptionMessage().c_str()));
                        continue;
                    }

                    const auto messageSize = static_cast<size_t>(indexData->ByteSize());
                    //todo избавиться от синхронного торможения тут, слать из файла позже, если не успеваем
                    PurgeSendInflightMessages(false);
                    SendInflightLimiter.Increment(1, messageSize);
                    ++Counters->MessageSenderCounters.OutputMessagesCount;
                    Counters->MessageSenderCounters.OutputBytesCount += messageSize;
                    auto sendFuture = MessageSender->Send(*indexData);
                    SendInflightMessages.push_back({std::move(sendFuture), messageSize});
                }
            }

            void PurgeSendInflightMessages(bool wait) {
                with_lock(SendInflightLock) {
                    while (!SendInflightMessages.empty()) {
                        const auto& f = SendInflightMessages.front();
                        if (!f.Future.HasValue()) {
                            if (!wait) {
                                break;
                            }
                            auto future = f.Future;
                            auto u = Unguard(SendInflightLock);
                            future.Wait();
                            Y_VERIFY(!future.HasException());
                            continue;
                        }
                        SendInflightLimiter.Decrement(1, f.Size);
                        SendInflightMessages.pop_front();
                    }
                }
            }

            void UpdateCounters() {
                PurgeSendInflightMessages(false);
                with_lock(Lock) {
                    Counters->DataBytes = TotalSegmentsSize;
                    Counters->DataSeconds = Segments.empty()
                                            ? 0
                                            : (TInstant::Now() - Segments.front()->GetTimestamp()).Seconds();
                    Counters->DataSegments = Segments.size();
                }
                ConvertQueueStatsToCounters(Queue->GetStats(), Counters->QueueCounters);
            }

            static void ConvertQueueStatsToCounters(const TSharedMemoryQueueStats& stats, TSharedMemoryQueueCounters& target) {
                target.InflightItemsCount = stats.InflightItemsCount;
                target.ItemsInQueueCount = stats.ItemsInQueueCount;
                target.BlockedEnqueueTransactionsCount = stats.BlockedEnqueueTransactionsCount;
                target.BlockedDequeueTransactionsCount = stats.BlockedDequeueTransactionsCount;
                target.PendingEnqueueTransactionsCount = stats.PendingEnqueueTransactionsCount;
                target.PendingDequeueTransactionsCount = stats.PendingDequeueTransactionsCount;
                target.EnqueueWaitCount = stats.EnqueueWaitCount;
                target.DequeueWaitCount = stats.DequeueWaitCount;
                target.AllocatedPages = stats.AllocatedPages;
                target.AllocatedBytes = stats.AllocatedBytes;
                target.UsedPages = stats.UsedPages;
                target.UsedBytes = stats.UsedBytes;
            }

            void RegisterSegment(TString filePath, TInstant timestamp, bool createFile) {
                with_lock(Lock) {
                    Segments.push_back(MakeIntrusive<TSegmentInfo>(std::move(filePath), timestamp, createFile));
                    SegmentsByTimestamp.insert({timestamp, Segments.back().Get()});
                }
                TotalSegmentsSize += Segments.back()->GetSize();
            }

            void RemoveOldestSegment() {
                TIntrusivePtr<TSegmentInfo> segment;
                with_lock(Lock) {
                    segment = Segments.front();
                    Segments.pop_front();
                    Y_VERIFY(SegmentsByTimestamp.erase(segment->GetTimestamp()) == 1);
                }
                segment->RemoveFile();
                TotalSegmentsSize -= segment->GetSize();
            }

            void CreateNewSegment() {
                TInstant timestamp;
                TString filePath;
                while (true) {
                    timestamp = TInstant::Now();
                    auto fileName = timestamp.ToStringUpToSeconds();
                    SubstGlobal(fileName, ':', '_');
                    filePath = JoinFsPaths(Config.GetLocalStorage().GetDirectory(), fileName);
                    if (!NFs::Exists(filePath)) {
                        break;
                    }
                    Log(ELogType::Error, Sprintf("file [%s] already exists, sleeping", filePath.c_str()));
                    Sleep(TDuration::Seconds(1));
                }
                if (!Segments.empty()) {
                    Segments.back()->MakeNonWritable();
                }
                RegisterSegment(std::move(filePath), timestamp, true);
                Segments.back()->MakeWritable();
            }

        private:
            const TConfig& Config;
            TIntrusivePtr<TRTLogAgentCounters> Counters;
            THolder<ISharedMemoryQueue> Queue;
            THolder<TMonService2> MonService;
            TDeque<TIntrusivePtr<TSegmentInfo>> Segments;
            THashMap<TInstant, TSegmentInfo*> SegmentsByTimestamp;
            std::atomic<ui64> TotalSegmentsSize;
            TLimiter SendInflightLimiter;
            TDeque<TSendInflightMessage> SendInflightMessages;
            THolder<IRTLogHandler> LogHandler;
            THolder<IMessageSender> MessageSender;
            std::thread WorkerThread;
            TAdaptiveLock Lock;
            TAdaptiveLock SendInflightLock;
        };

        ui32 GetRequiredUI32(const IMonHttpRequest& request, const TString& name) {
            const auto& result = request.GetParams().Get(name);
            if (result.empty()) {
                throw yexception() << "missed required parameter [" << name << "]";
            }
            return FromString<ui32>(result);
        }

        void TQueryContentPage::Output(IMonHttpRequest& request) {
            auto& out = request.Output();

            TLogItemLocation location;
            TMaybe<TFieldSpec> fieldSpec;
            try {
                location.SegmentTimestamp = GetRequiredUI32(request, "segment-timestamp");
                location.Offset = GetRequiredUI32(request, "offset");
                location.Size = GetRequiredUI32(request, "size");

                const auto& key = request.GetParams().Get("field-key");
                const auto& value = request.GetParams().Get("field-value");
                const auto& eventIndexStr = request.GetParams().Get("field-event-index");
                if (!key.empty() && !value.empty() && !eventIndexStr.empty()) {
                    fieldSpec.ConstructInPlace();
                    fieldSpec->Key = key;
                    fieldSpec->Value = value;
                    fieldSpec->EventIndex = FromString<ui32>(eventIndexStr);
                }
            } catch (...) {
                Log(ELogType::Info, Sprintf("TQueryContentPage::Output error %s", CurrentExceptionMessage().c_str()));
                out << "HTTP/1.1 400 Bad request\r\nConnection: Close\r\n\r\n";
                out.Flush();
                return;
            }

            const auto& pretty = request.GetParams().Get("pretty");
            const auto& result = Server.QueryContent(location, fieldSpec, IsTrue(pretty));
            if (!result.Defined()) {
                out << HTTPNOTFOUND;
                out.Flush();
                return;
            }
            out << HTTPOKJSON << *result;
            out.Flush();
        }
    }

    TIntrusivePtr<IServer> MakeServer(const TConfig& config) {
        return MakeIntrusive<TServer>(config);
    }
}
