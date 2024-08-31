#include "client.h"

#include "common.h"

#include <alice/rtlog/protos/rtlog.ev.pb.h>
#ifdef RTLOG_AGENT
#   include <robot/rthub/misc/shmem_queue.h>
#endif

#include <logbroker/unified_agent/client/cpp/logger/backend.h>

#include <robot/library/fork_subscriber/fork_subscriber.h>

#include <library/cpp/logger/backend.h>
#include <library/cpp/logger/composite.h>
#include <library/cpp/logger/file.h>
#include <library/cpp/logger/log.h>

#include <util/generic/guid.h>
#include <util/string/builder.h>
#include <util/string/split.h>
#include <util/string/printf.h>
#include <util/system/event.h>
#include <util/system/file.h>
#include <util/system/fstat.h>
#include <util/system/getpid.h>
#include <util/system/hostname.h>
#include <util/system/rwlock.h>
#include <util/system/spinlock.h>
#include <util/thread/pool.h>

#include <thread>

using namespace NRTLogEvents;

namespace NRTLog {
    struct TClientStatsInternal {
        std::atomic<ui64> ActiveLoggersCount{0};
        std::atomic<ui64> EventsCount{0};
        std::atomic<ui64> PendingBytesCount{0};
        std::atomic<ui64> WrittenFramesCount{0};
        std::atomic<ui64> WrittenBytesCount{0};
        std::atomic<ui64> ErrorsCount{0};

        void IncrementErrorsCount() {
            ErrorsCount.fetch_add(1, std::memory_order_relaxed);
        }
    };

    namespace {
        TString ToLogString(TInstant instant) {
            return Sprintf("%s,%03d",
                           TInstant::Now().FormatLocalTime("%Y-%m-%d %H:%M:%S").c_str(),
                           instant.MilliSecondsOfSecond());
        }

        void LogCurrentError(TClientStatsInternal& stats) {
            stats.IncrementErrorsCount();
            try {
                const auto message = Sprintf("rtlog ERROR %s: %s\n",
                                             ToLogString(TInstant::Now()).c_str(),
                                             CurrentExceptionMessage().c_str());
                Cerr.Write(message);
                Cerr.Flush();
            } catch(...) {
            }
        }

    #ifdef RTLOG_AGENT
        class TSharedMemoryQueueLogBackend final: public TLogBackend {
        public:
            explicit TSharedMemoryQueueLogBackend(TString name)
                : Queue(NRTHub::OpenSharedMemoryQueue(std::move(name)))
            {
            }

            ~TSharedMemoryQueueLogBackend() override = default;

            void WriteData(const TLogRecord& rec) override {
                NRTHub::TEnqueueScope scope(*Queue, rec.Len);
                memcpy(scope.Transaction->GetBytes(), rec.Data, rec.Len);
            }

            void ReopenLog() override {
                Y_FAIL("not implemented");
            }

        private:
            THolder<NRTHub::ISharedMemoryQueue> Queue;
        };
    #endif

        class TWatchedFileLogBackend final: public TLogBackend {
        public:
            explicit TWatchedFileLogBackend(TString path, TMaybe<TDuration> fileStatCheckPeriod, TClientStatsInternal& stats)
                : FilePath(std::move(path))
                , FileStatCheckPeriod(fileStatCheckPeriod)
                , Lock()
                , File(Nothing())
                , FileStat(Nothing())
                , NextFileStatCheckTime()
                , Stats(stats)
            {
                OpenFile();
            }

            ~TWatchedFileLogBackend() override = default;

            void WriteData(const TLogRecord& rec) override {
                auto guard = TReadGuard(Lock);
                if (NextFileStatCheckTime.Defined()) {
                    const auto now = TInstant::Now();
                    if (NextFileStatCheckTime <= now) {
                        auto unguardRead = Unguard(guard);
                        auto guardWrite = TWriteGuard(Lock);
                        if (NextFileStatCheckTime <= now) {
                            bool needReopen;
                            if (File) {
                                const TFileStat newStat(File->GetName());
                                needReopen = newStat.INode != FileStat->INode;
                            } else {
                                needReopen = true;
                            }
                            if (needReopen) {
                                OpenFile();
                            }
                        }
                    }
                }
                if (File) {
                    File->Write(rec.Data, rec.Len);
                    Stats.WrittenFramesCount.fetch_add(1, std::memory_order_relaxed);
                    Stats.WrittenBytesCount.fetch_add(rec.Len, std::memory_order_relaxed);
                } else {
                    Stats.IncrementErrorsCount();
                }
            }

            void ReopenLog() override {
                auto guard = TWriteGuard(Lock);
                OpenFile();
            }

        private:
            void OpenFile() {
                //Lock must be held for writing

                try {
                    File = TFile::ForAppend(FilePath);
                    FileStat = TFileStat(*File);
                } catch(...) {
                    LogCurrentError(Stats);
                    File = Nothing();
                    FileStat = Nothing();
                }
                if (FileStatCheckPeriod.Defined()) {
                    NextFileStatCheckTime = TInstant::Now() + *FileStatCheckPeriod;
                }
            }

        private:
            TString FilePath;
            TMaybe<TDuration> FileStatCheckPeriod;
            TRWMutex Lock;
            TMaybe<TFile> File;
            TMaybe<TFileStat> FileStat;
            TMaybe<TInstant> NextFileStatCheckTime;
            TClientStatsInternal& Stats;
        };


        THolder<TLogBackend> CreateBackend(TString fileName, const TClientOptions& options,
                                           TClientStatsInternal& stats, const TString& unifiedAgentUri,
                                           TMaybe<TLog>& unifiedAgentLog) {
#ifdef RTLOG_AGENT
            static const auto shmPrefix = TString("shm:");
            if (fileName.StartsWith(shmPrefix)) {
                return MakeHolder<TSharedMemoryQueueLogBackend>(fileName.substr(shmPrefix.size()));
            }
    #endif
            auto compositeLogBackend = MakeHolder<TCompositeLogBackend>();
            if (!unifiedAgentUri.empty()) {
                NUnifiedAgent::TClientParameters unifiedAgentParams{unifiedAgentUri};
                if (unifiedAgentLog.Defined()) {
                    unifiedAgentLog->SetDefaultPriority(ELogPriority::TLOG_INFO);
                    unifiedAgentParams.SetLog(*unifiedAgentLog);
                }
                auto recordConverter = NUnifiedAgent::MakeDefaultRecordConverter(false);

                compositeLogBackend->AddLogBackend(NUnifiedAgent::MakeLogBackend(unifiedAgentParams,
                                                                                 /* sessionParameters= */{},
                                                                                 std::move(recordConverter)));
            }
            if (!fileName.empty()) {
                compositeLogBackend->AddLogBackend(MakeHolder<TWatchedFileLogBackend>(std::move(fileName),
                                                                                      options.FileStatCheckPeriod,
                                                                                      stats));
            }
            return compositeLogBackend;
        }

        TMaybe<TLog> CreateUnifiedAgentLog(const TString& unifiedAgentLogFile) {
            if (unifiedAgentLogFile.empty()) {
                return {};
            }
            return CreateLogBackend(unifiedAgentLogFile, ELogPriority::TLOG_INFO, true);
        }
    } // namespace

    TToken TToken::Parse(TStringBuf token, TEventTimestamp timestamp) {
        TToken result;
        const auto items = StringSplitter(token).Split('$').ToList<TString>();
        ui64 reqTimestamp = 0;
        if (items.size() == 3 && TryFromString<ui64>(items[0], reqTimestamp) && !items[1].empty() && !items[2].empty()) {
            result.ReqTimestamp = reqTimestamp;
            result.ReqId = items[1];
            result.ActivationId = items[2];
        } else if (items.size() == 1 && !items[0].empty()) {
            result.ReqTimestamp = timestamp;
            result.ReqId = items[0];
            result.ActivationId = CreateGuidAsString();
        } else {
            result = New(timestamp);
        }
        return result;
    }

    TToken TToken::New(TEventTimestamp timestamp) {
        TToken result;
        result.ReqTimestamp = timestamp;
        result.ReqId = CreateGuidAsString();
        result.ActivationId = CreateGuidAsString();
        return result;
    }

    TString TToken::Serialize() const {
        return Sprintf("%lu$%s$%s", ReqTimestamp, ReqId.c_str(), ActivationId.c_str());
    }

    TFrameBuffer::TFrameBuffer(const TToken& token)
        : StartTimestamp((TEventTimestamp)-1)
        , EndTimestamp(0)
        , Buf()
        , MetaFlags({
            {TString{RTLOG_REQID}, token.ReqId},
            {TString{RTLOG_ACTIVATION_ID}, token.ActivationId},
            {TString{RTLOG_REQ_TS}, ToString(token.ReqTimestamp)}
        })
    {
    }

    size_t TFrameBuffer::WriteEvent(TEventTimestamp ts, TEventClass eventId, const TBytesOrMessage& body) {
        //Lock must be held

        Y_VERIFY(body.Message || body.Bytes);
        const ui32 bodySize = ui32(body.Message ? body.Message->ByteSize() : body.Bytes->size());
        const ui32 totalSize = sizeof(bodySize) + sizeof(ts) + sizeof(eventId) + bodySize;
        const size_t before = Buf.Size();
        Buf.Advance(totalSize);
        char* target = Buf.Data() + before;
        memcpy(target, &totalSize, sizeof(totalSize));
        target += sizeof(totalSize);
        memcpy(target, &ts, sizeof(ts));
        target += sizeof(ts);
        memcpy(target, &eventId, sizeof(eventId));
        target += sizeof(eventId);
        if (body.Message) {
            Y_PROTOBUF_SUPPRESS_NODISCARD body.Message->SerializeToArray(target, bodySize);
        } else {
            memcpy(target, body.Bytes->data(), body.Bytes->size());
        }
        if (ts < StartTimestamp) {
            StartTimestamp = ts;
        }
        if (ts > EndTimestamp) {
            EndTimestamp = ts;
        }
        return totalSize;
    }

    void TFrameBuffer::AddMetaFlag(TString&& key, TString&& value) {
        MetaFlags.emplace_back(std::move(key), std::move(value));
    }

    class TClient::TImpl: public TAtomicRefCount<TClient::TImpl> {
    public:
        explicit TImpl(const TString& fileName,
                       const TString& serviceName,
                       const TClientOptions& options,
                       TEventLogFormat eventLogFormat,
                       const TString& unifiedAgentUri,
                       const TString& unifiedAgentLogFile)
            : Stats()
            , Options(options)
            , UnifiedAgentLog(CreateUnifiedAgentLog(unifiedAgentLogFile))
            , EventLog(TLog(CreateBackend(fileName, Options, Stats, unifiedAgentUri, UnifiedAgentLog)), NEvClass::Factory()->CurrentFormat(), eventLogFormat)
            , InstanceDescriptor()
            , InstanceId(Nothing())
            , ActiveLoggers()
            , MtpQueue(nullptr)
            , FlushThread(nullptr)
            , StopEvent()
            , BackgroundActivitiesLock()
            , LoggersLock()
            , ForkSubscriber(NRobot::TForkObserver()
                .WithPrepareAction([this] () {
                    BackgroundActivitiesLock.Acquire();
                    StopBackgroundActivities();
                })
                .WithParentAction([this] () {
                    BackgroundActivitiesLock.Release();
                })
                .WithChildAction([this] () {
                    BackgroundActivitiesLock.Release();
                }))
        {
            InstanceDescriptor.SetServiceName(serviceName);
            if (options.Hostname) {
                InstanceDescriptor.SetHostName(options.Hostname.GetRef());
            } else {
                InstanceDescriptor.SetHostName(FQDNHostName());
            }
        }

        ~TImpl() {
            with_lock(BackgroundActivitiesLock) {
                StopBackgroundActivities();
            }
        }

        TRequestLoggerPtr CreateRequestLogger(TStringBuf token, bool session, TEventTimestamp timestamp) {
            auto result = std::make_shared<TRequestLogger>(this, Stats, token, session, timestamp, Options.FlushSize);
            if (Options.FlushPeriod) {
                with_lock(LoggersLock) {
                    ActiveLoggers.push_back(result);
                }
                with_lock(BackgroundActivitiesLock) {
                    if (!FlushThread) {
                        StopEvent.Reset();
                        FlushThread = MakeHolder<std::thread>(&TClient::TImpl::PeriodicFlush, this);
                    }
                }
            }
            Stats.ActiveLoggersCount.fetch_add(1, std::memory_order_relaxed);
            return result;
        }

        void Reopen() {
            EventLog.ReopenLog();
            if (UnifiedAgentLog.Defined()) {
                UnifiedAgentLog->ReopenLog();
            }
        }

        void WriteFrame(TFrameBuffer&& frame) {
            auto writeFrame = [this, frame = std::move(frame)] () mutable {
                const auto size = frame.Buf.Size();
                try {
                    EventLog.WriteFrame(frame.Buf, frame.StartTimestamp, frame.EndTimestamp, nullptr, frame.MetaFlags);
                } catch(...) {
                    LogCurrentError(Stats);
                }
                Stats.PendingBytesCount.fetch_sub(size, std::memory_order_relaxed);
            };

            if (!Options.Async) {
                writeFrame();
                return;
            }
            with_lock(BackgroundActivitiesLock) {
                if (!MtpQueue) {
                    MtpQueue = MakeHolder<TThreadPool>(TThreadPool::TParams().SetBlocking(true).SetCatching(false));
                    MtpQueue->Start(1);
                }
                Y_VERIFY(MtpQueue->AddFunc(std::move(writeFrame)));
            }
        }

        TClientStats GetStats() const {
            TClientStats result;
            result.ActiveLoggersCount = Stats.ActiveLoggersCount.load(std::memory_order_relaxed);
            result.EventsCount = Stats.EventsCount.load(std::memory_order_relaxed);
            result.PendingBytesCount = Stats.PendingBytesCount.load(std::memory_order_relaxed);
            result.WrittenFramesCount = Stats.WrittenFramesCount.load(std::memory_order_relaxed);
            result.WrittenBytesCount = Stats.WrittenBytesCount.load(std::memory_order_relaxed);
            result.ErrorsCount = Stats.ErrorsCount.load(std::memory_order_relaxed);
            return result;
        }

        void FillInstanceInfo(ActivationStarted& ev) {
            if (InstanceId.Defined()) {
                ev.SetInstanceId(*InstanceId);
            } else {
                *ev.MutableInstanceDescriptor() = InstanceDescriptor;
            }
        }

    private:
        void PeriodicFlush() {
            while (!StopEvent.WaitT(*Options.FlushPeriod / 2)) {
                TVector<TRequestLoggerPtr> aliveLoggers;
                with_lock(LoggersLock) {
                    for (size_t i = 0; i < ActiveLoggers.size(); ) {
                        if (auto requestLogger = ActiveLoggers[i].lock()) {
                            aliveLoggers.push_back(std::move(requestLogger));
                            ++i;
                        } else {
                            if (i != ActiveLoggers.size() - 1) {
                                ActiveLoggers[i] = std::move(ActiveLoggers.back());
                            }
                            ActiveLoggers.pop_back();
                        }
                    }
                }
                const auto deadline = TInstant::Now() - *Options.FlushPeriod;
                for (auto& l: aliveLoggers) {
                    l->Flush(deadline);
                }
            }
        }

        void StopBackgroundActivities() {
            //BackgroundActivitiesLock must be held

            if (MtpQueue) {
                MtpQueue->Stop();
                MtpQueue.Reset();
            }
            if (FlushThread) {
                StopEvent.Signal();
                FlushThread->join();
                FlushThread.Reset();
            }
        }

    private:
        TClientStatsInternal Stats;
        TClientOptions Options;
        TMaybe<TLog> UnifiedAgentLog;
        TEventLog EventLog;
        NRTLogEvents::TInstanceDescriptor InstanceDescriptor;
        TMaybe<ui64> InstanceId;
        TVector<std::weak_ptr<TRequestLogger>> ActiveLoggers;
        THolder<TThreadPool> MtpQueue;
        THolder<std::thread> FlushThread;
        TManualEvent StopEvent;
        TAdaptiveLock BackgroundActivitiesLock;
        TAdaptiveLock LoggersLock;
        NRobot::TForkSubscriber ForkSubscriber;
    };

    TRequestLogger::TRequestLogger(TIntrusivePtr<TClient::TImpl> client, TClientStatsInternal& stats,
                                   TStringBuf token, bool session, TEventTimestamp timestamp,
                                   TMaybe<ui64> flushSize)
        : Client(std::move(client))
        , Stats(stats)
        , Token(TToken::Parse(token, timestamp))
        , ActiveFrame(Token)
        , Finished(false)
        , Session(session)
        , FlushSize(flushSize)
        , Lock()
    {
        with_lock(Lock) {
            EnsureFrameStarted(false, timestamp);
        }
    }

    TRequestLogger::~TRequestLogger() {
        if (!Finished) {
            EnsureFrameStarted(true, Now().MicroSeconds());
            Client->WriteFrame(std::move(ActiveFrame));
        }
        Stats.ActiveLoggersCount.fetch_sub(1, std::memory_order_relaxed);
    }

    void TRequestLogger::DoLogEvent(TEventTimestamp timestamp, TEventClass eventId, const TBytesOrMessage& body) {
        with_lock(Lock) {
            EnsureFrameStarted(true, timestamp);
            WriteEvent(timestamp, eventId, body);
        }
    }

    void TRequestLogger::Flush(TInstant deadline) {
        TFrameBuffer frameToWrite{Token};
        with_lock(Lock) {
            const bool shouldFlush =
                (!ActiveFrame.Buf.Empty() && TInstant::MicroSeconds(ActiveFrame.StartTimestamp) < deadline) ||
                (FlushSize.Defined() && ActiveFrame.Buf.size() > *FlushSize);
            if (!shouldFlush) {
                return;
            }
            DoSwap(frameToWrite, ActiveFrame);
        }
        Client->WriteFrame(std::move(frameToWrite));
    }

    void TRequestLogger::EnsureFrameStarted(bool isContinue, TEventTimestamp timestamp) {
        //Lock must be held

        if (ActiveFrame.Buf.Empty()) {
            ActivationStarted ev;
            Client->FillInstanceInfo(ev);
            ev.SetReqTimestamp(Token.ReqTimestamp);
            ev.SetReqId(Token.ReqId);
            ev.SetActivationId(Token.ActivationId);
            ev.SetPid(GetPID());
            ev.SetSession(Session);
            ev.SetContinue(isContinue);
            ev.SetContinueId(ContinueId);
            WriteEvent(timestamp, ActivationStarted::ID, {nullptr, &ev});
            ActiveFrame.AddMetaFlag(TString{RTLOG_CONTINUE_ID}, ToString(ContinueId));
            ++ContinueId;
        }
    }

    void TRequestLogger::LogEvent(const NRTLogEvents::TCreateRequestContext& requestContext) {
        TString indexKey{RTLOG_INDEX_KEYS};
        TStringBuilder indexValue{};
        bool isFirst = true;
        for (const auto& [_, value] : requestContext.GetFields()) {
            if (!isFirst) {
                indexValue << ',';
            }
            isFirst = false;
            indexValue << value;
        }

        with_lock(Lock) {
            EnsureFrameStarted(true, Now().MicroSeconds());
            ActiveFrame.AddMetaFlag(std::move(indexKey), std::move(indexValue));
        }
        DoLogEvent(Now().MicroSeconds(), requestContext.ID, {nullptr, &requestContext});
    }

    TString TRequestLogger::LogChildActivationStarted(const TString& newReqid, const TString& newActivationId, const TString& description) {
        TEventTimestamp timestamp = Now().MicroSeconds();
        const auto token = TToken{timestamp, newReqid, newActivationId};
        return LogChildActivationStarted(timestamp, /* newReqid = */ true, token, description);
    }

    TString TRequestLogger::LogChildActivationStarted(TEventTimestamp timestamp, bool newReqid, const TString& description) {
        const auto token = newReqid ? TToken::New(timestamp) : TToken{Token.ReqTimestamp, Token.ReqId, CreateGuidAsString()};
        return LogChildActivationStarted(timestamp, newReqid, token, description);
    }
    TString TRequestLogger::LogChildActivationStarted(TEventTimestamp timestamp, bool newReqid, const TToken& token, const TString& description) {
        ChildActivationStarted ev;
        if (newReqid) {
            ev.SetReqTimestamp(token.ReqTimestamp);
            ev.SetReqId(token.ReqId);
        }
        ev.SetActivationId(token.ActivationId);
        ev.SetDescription(description);
        LogEvent(timestamp, ev);
        return token.Serialize();
    }

    void TRequestLogger::LogChildActivationFinished(TEventTimestamp timestamp, TStringBuf token, bool ok, TStringBuf errorMsg) {
        const auto t = TToken::Parse(token, timestamp);
        LogEvent(timestamp, ChildActivationFinished(t.ActivationId, ok, TString(errorMsg)));
    }

    void TRequestLogger::Finish(TEventTimestamp timestamp) {
        const auto ev = ActivationFinished();
        TFrameBuffer frameToWrite{Token};
        with_lock(Lock) {
            EnsureFrameStarted(true, timestamp);
            WriteEvent(timestamp, ActivationFinished::ID, {nullptr, &ev});
            DoSwap(frameToWrite, ActiveFrame);
            Finished = true;
        }
        Client->WriteFrame(std::move(frameToWrite));
    }

    void TRequestLogger::WriteEvent(TEventTimestamp ts, TEventClass eventId, const TBytesOrMessage& body) {
        const auto writtenBytes = ActiveFrame.WriteEvent(ts, eventId, body);
        Stats.EventsCount.fetch_add(1, std::memory_order_relaxed);
        Stats.PendingBytesCount.fetch_add(writtenBytes, std::memory_order_relaxed);
    }

    TClient::TClient(const TString& fileName,
                     const TString& serviceName,
                     const TClientOptions& options,
                     TEventLogFormat eventLogFormat,
                     const TString unifiedAgentUri,
                     const TString unifiedAgentLogFile)
        : Impl(MakeIntrusive<TImpl>(fileName, serviceName, options, eventLogFormat, unifiedAgentUri, unifiedAgentLogFile))
    {
    }

    TRequestLoggerPtr TClient::CreateRequestLogger(TStringBuf token, bool session, TEventTimestamp timestamp) {
        return Impl->CreateRequestLogger(token, session, timestamp);
    }

    void TClient::Reopen() {
        Impl->Reopen();
    }

    TClientStats TClient::GetStats() const {
        return Impl->GetStats();
    }

    TClient::~TClient() = default;
}
