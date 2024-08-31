#pragma once

#include "fwd.h"

#include "common.h"

#include <alice/rtlog/protos/rtlog.ev.pb.h>

#include <library/cpp/eventlog/eventlog.h>
#include <library/cpp/eventlog/eventlog_int.h>
#include <library/cpp/logger/record.h>

#include <util/datetime/base.h>
#include <util/generic/buffer.h>
#include <util/generic/maybe.h>
#include <util/generic/ptr.h>
#include <util/generic/string.h>

namespace NRTLog {
    struct TBytesOrMessage {
        const TString* Bytes;
        const NProtoBuf::Message* Message;
    };

    struct TToken {
        ui64 ReqTimestamp;
        TString ReqId;
        TString ActivationId;

        static TToken Parse(TStringBuf token, TEventTimestamp timestamp);

        static TToken New(TEventTimestamp timestamp);

        TString Serialize() const;
    };

    struct TFrameBuffer {
        explicit TFrameBuffer(const TToken& token);

        size_t WriteEvent(TEventTimestamp ts, TEventClass eventId, const TBytesOrMessage& body);

        void AddMetaFlag(TString&& key, TString&& value);

        TEventTimestamp StartTimestamp;
        TEventTimestamp EndTimestamp;
        TBuffer Buf;
        TLogRecord::TMetaFlags MetaFlags;
    };

    struct TClientOptions {
        bool Async = false;
        TMaybe<TDuration> FlushPeriod = Nothing();
        TMaybe<TDuration> FileStatCheckPeriod = Nothing();
        TMaybe<ui64> FlushSize = 1024 * 1024;
        TMaybe<TString> Hostname = Nothing();
    };

    struct TClientStats {
        ui64 ActiveLoggersCount;
        ui64 EventsCount;
        ui64 PendingBytesCount;
        ui64 WrittenFramesCount;
        ui64 WrittenBytesCount;
        ui64 ErrorsCount;
    };

    class TClient {
    public:
        explicit TClient(const TString& fileName,
                         const TString& serviceName,
                         const TClientOptions& options = {},
                         TEventLogFormat eventLogFormat = COMPRESSED_LOG_FORMAT_V4,
                         const TString unifiedAgentUri = {},
                         const TString unifiedAgentLogFile = {});
        ~TClient();

        TRequestLoggerPtr CreateRequestLogger(TStringBuf token = {},
                                              bool session = false,
                                              TEventTimestamp timestamp = Now().MicroSeconds());

        void Reopen();

        TClientStats GetStats() const;

    public:
        class TImpl;

    private:
        TIntrusivePtr<TImpl> Impl;
    };

    struct TClientStatsInternal;

    class TRequestLogger {
    public:
        explicit TRequestLogger(TIntrusivePtr<TClient::TImpl> client, TClientStatsInternal& stats,
                                TStringBuf token, bool session, TEventTimestamp timestamp,
                                TMaybe<ui64> flushSize);

        ~TRequestLogger();

        void LogEvent(const NRTLogEvents::TCreateRequestContext& requestContext);

        template <class T>
        inline void LogEvent(const T& ev) {
            DoLogEvent(Now().MicroSeconds(), ev.ID, {nullptr, &ev});
        }

        template <class T>
        inline void LogEvent(TEventTimestamp timestamp, const T& ev) {
            DoLogEvent(timestamp, ev.ID, {nullptr, &ev});
        }

        inline void LogEvent(TEventClass eventId, const TString& bytes) {
            DoLogEvent(Now().MicroSeconds(), eventId, {&bytes, nullptr});
        }

        inline void LogEvent(TEventTimestamp timestamp, TEventClass eventId, const TString& bytes) {
            DoLogEvent(timestamp, eventId, {&bytes, nullptr});
        }

        inline TString LogChildActivationStarted(bool newReqid, const TString& description) {
            return LogChildActivationStarted(Now().MicroSeconds(), newReqid, description);
        }

        // Sets explicit ReqId for ChildActivation, use with caution
        TString LogChildActivationStarted(const TString& newReqid, const TString& newActivationId, const TString& description);

        TString LogChildActivationStarted(TEventTimestamp timestamp, bool newReqid, const TString& description);

        TString LogChildActivationStarted(TEventTimestamp timestamp, bool newReqid, const TToken& token, const TString& description);

        void LogChildActivationFinished(TStringBuf token, bool ok, TStringBuf errorMsg = "") {
            LogChildActivationFinished(Now().MicroSeconds(), token, ok, errorMsg);
        }

        void LogChildActivationFinished(TEventTimestamp timestamp, TStringBuf token, bool ok, TStringBuf errorMsg = "");

        TString GetToken() const {
            return Token.Serialize();
        }

        inline void Finish() {
            Finish(Now().MicroSeconds());
        }

        void Finish(TEventTimestamp timestamp);

    private:
        friend class TClient::TImpl;

        void DoLogEvent(TEventTimestamp timestamp, TEventClass eventId, const TBytesOrMessage& body);

        void Flush(TInstant deadline);

        void EnsureFrameStarted(bool isContinue, TEventTimestamp timestamp);

        inline void WriteEvent(TEventTimestamp ts, TEventClass eventId, const TBytesOrMessage& body);

    private:
        TIntrusivePtr<TClient::TImpl> Client;
        TClientStatsInternal& Stats;
        TToken Token;
        TFrameBuffer ActiveFrame;
        ui32 ContinueId = 0;
        bool Finished;
        bool Session;
        TMaybe<ui64> FlushSize;
        TMutex Lock;
    };
} // namespace NRTLog
