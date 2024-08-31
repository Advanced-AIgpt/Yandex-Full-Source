#pragma once

#include <alice/cuttlefish/library/proto_configs/rtlog.cfgproto.pb.h>
#include <alice/rtlog/client/client.h>
#include <apphost/api/service/cpp/service.h>

#include <library/cpp/neh/http_common.h>

#include <util/generic/maybe.h>
#include <util/generic/singleton.h>

namespace NAlice::NCuttlefish {
    class TRtLogClient {
    public:
        void Init(const NAliceServiceConfig::TRtLog&);

        NRTLog::TRequestLoggerPtr CreateRequestLogger(const TString& token, bool session=false) {
            if (RTLogClient_) {
                return RTLogClient_->CreateRequestLogger(token, session);
            } else {
                return nullptr;
            }
        }

        inline void Reopen() {
            if (RTLogClient_) {
                RTLogClient_->Reopen();
            }
        }

        static TRtLogClient& Instance() {
            // priority MUST be more than arcadia/robot/library/fork_subscriber/fork_subscriber.cpp
            // 1000000 > 65536 (default priority)
            return *SingletonWithPriority<TRtLogClient, 1000000>();
        }

    private:
        NRTLog::TClientOptions ClientOptions_;
        THolder<NRTLog::TClient> RTLogClient_;
    };

    class TRtLogInputRequest {
    public:
        struct TOptions {
            bool WriteInfoToRtLog = true;
        };

    public:
        void SetToken(const TString& rtLogToken) {
            RtLogToken_ = rtLogToken;
        }
        void Init(TRtLogClient& client, bool session=false) {
            RtLogger_ = client.CreateRequestLogger(RtLogToken_, session);
            OnRtLogInit();
        }
        virtual void OnRtLogInit() {
        }

        void RtLogInfoEvent(const TString&, NRTLog::TRequestLogger* logger=nullptr);
        void RtLogErrorEvent(const TString&, NRTLog::TRequestLogger* logger=nullptr);

    protected:
        NRTLog::TRequestLogger* RtLogPtr() {  // WARNING: can return nullptr !!
            return RtLogger_.get();
        }
        NRTLog::TRequestLoggerPtr LoggerPtr() {  // WARNING: can return empty ptr !!
            return RtLogger_;
        }

        TString RtLogToken_; // own rtlog token (empty if not receive it from user/client)
        NRTLog::TRequestLoggerPtr RtLogger_;
        TOptions Options;
    };

    // helper for keep rtlog (active state) for output request
    class TRTLogActivation {
    public:
        TRTLogActivation() = default;
        TRTLogActivation(NRTLog::TRequestLoggerPtr logger, const TString& description, bool newRequest=true)
            : Token_(logger->LogChildActivationStarted(newRequest, description))
            , Logger_(logger)
        {
        }
        TRTLogActivation(NRTLog::TRequestLoggerPtr logger, const TString& description, const TString& newReqId)
            : Token_(logger->LogChildActivationStarted(newReqId, newReqId, description))
            , Logger_(logger)
        {
        }

        TRTLogActivation(const TRTLogActivation&) = delete;
        TRTLogActivation(TRTLogActivation&&) = default;
        TRTLogActivation& operator=(const TRTLogActivation&) = delete;
        TRTLogActivation& operator=(TRTLogActivation&&) = default;

        ~TRTLogActivation() {
            Finish();
        }

        void Finish(bool ok = true, const TString& errorText = {}) noexcept {
            try {
                if (Logger_) {
                    Logger_->LogChildActivationFinished(Token_, ok, errorText);
                    Logger_.reset();
                    Token_.clear();
                }
            } catch (...) {
            }
        }

        inline operator bool() const noexcept {
            return bool(Logger_);
        }

        inline const TString& Token() const noexcept {
            return Token_;
        }

        NRTLog::TRequestLoggerPtr LoggerPtr() const noexcept {
            return Logger_;
        }

        NRTLog::TRequestLogger* Logger() const noexcept {
            return Logger_.get();
        }

    private:
        TString Token_;
        NRTLog::TRequestLoggerPtr Logger_;
    };

    void SaveRtLogEvent(NRTLog::TRequestLogger*, const TString& message, NRTLogEvents::ESeverity severity);
    inline void SaveRtLogInfoEvent(NRTLog::TRequestLogger* logger, const TString& message) {
        SaveRtLogEvent(logger, message, NRTLogEvents::RTLOG_SEVERITY_INFO);
    }
    inline void SaveRtLogErrorEvent(NRTLog::TRequestLogger* logger, const TString& message) {
        SaveRtLogEvent(logger, message, NRTLogEvents::RTLOG_SEVERITY_ERROR);
    }

    inline void SaveRtLogInfoEvent(const TRTLogActivation& activation, const TString& message) {
        SaveRtLogInfoEvent(activation.Logger(), message);
    }
    inline void SaveRtLogInfoEvent(const NRTLog::TRequestLoggerPtr& logger, const TString& message) {
        SaveRtLogInfoEvent(logger.get(), message);
    }
    inline void SaveRtLogErrorEvent(const TRTLogActivation& activation, const TString& message) {
        SaveRtLogErrorEvent(activation.Logger(), message);
    }
    inline void SaveRtLogErrorEvent(const NRTLog::TRequestLoggerPtr& logger, const TString& message) {
        SaveRtLogErrorEvent(logger.get(), message);
    }

    TMaybe<TString> TryGetRtLogTokenFromAppHostContext(const NAppHost::IServiceContext& ahContext);
    TString GetRtLogTokenFromTypedAppHostServiceContext(const NAppHost::ITypedServiceContext& typedServiceContext);
    TMaybe<TString> TryGetRtLogTokenFromHttpRequestHeaders(const THttpHeaders& httpHeaders);
}
