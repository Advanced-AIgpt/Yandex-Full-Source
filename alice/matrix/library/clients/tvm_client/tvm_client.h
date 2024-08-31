#pragma once

#include <alice/matrix/library/config/config.pb.h>

#include <infra/libs/outcome/result.h>

#include <library/cpp/tvmauth/client/facade.h>

#include <util/system/mutex.h>


namespace NMatrix {

class TTvmClient : public TMoveOnly {
public:
    using TConfig = std::variant<NTvmAuth::NTvmTool::TClientSettings, NTvmAuth::NTvmApi::TClientSettings>;

    struct TServiceTicket {
        TString Ticket;
    };

public:
    explicit TTvmClient(
        const TTvmClientSettings& config
    );

    bool EnsureInitializedAndReady();

    TExpected<TServiceTicket, TString> GetServiceTicketFor(const NTvmAuth::TClientSettings::TAlias& dstAlias);

private:
    bool EnsureInitialized();
    bool IsInitialized() const;
    bool Initialize();

public:
    static inline constexpr TStringBuf USER_TICKET_HEADER_NAME = "X-Ya-User-Ticket";
    static inline constexpr TStringBuf SERVICE_TICKET_HEADER_NAME = "X-Ya-Service-Ticket";

private:
    static inline constexpr TStringBuf NAME = "tvm_client";

    TConfig Config_;
    std::atomic<bool> Initialized_;
    TMutex InitializeMutex_;
    NTvmAuth::TLoggerPtr TvmClientLogger_;
    THolder<NTvmAuth::TTvmClient> TvmClient_;
};

} // namespace NMatrix
