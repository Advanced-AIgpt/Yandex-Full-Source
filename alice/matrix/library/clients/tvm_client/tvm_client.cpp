#include "tvm_client.h"

#include <alice/matrix/library/logging/log_context.h>
#include <alice/matrix/library/metrics/metrics.h>


namespace NMatrix {

namespace {

static const TString DEFAULT_TVM_SECRET_ENVIRONMENT_VARIABLE = "TVM_SECRET";


class TTvmLogger : public NTvmAuth::ILogger {
public:
    TTvmLogger() = default;

    void Log(int level, const TString& message) override {
        TSourceMetrics metrics(NAME);
        TLogContext logContext(
            SpawnLogFrame(),
            nullptr
        );

        NEvClass::TMatrixTvmClientLogMessage::ELogSeverity logSeverity;
        if (NEvClass::TMatrixTvmClientLogMessage::ELogSeverity_IsValid(level)) {
            logSeverity = static_cast<NEvClass::TMatrixTvmClientLogMessage::ELogSeverity>(level);
        } else {
            logSeverity = NEvClass::TMatrixTvmClientLogMessage::EMERGENCY;
        }

        metrics.PushRate("log", NEvClass::TMatrixTvmClientLogMessage::ELogSeverity_Name(logSeverity));

        if (logSeverity <= NEvClass::TMatrixTvmClientLogMessage::WARNING) {
            logContext.LogEventErrorCombo<NEvClass::TMatrixTvmClientLogMessage>(logSeverity, message);
        } else {
            logContext.LogEventInfoCombo<NEvClass::TMatrixTvmClientLogMessage>(logSeverity, message);
        }
    }

private:
    static inline constexpr TStringBuf NAME = "tvm_logger";
};

TTvmClient::TConfig CreateTvmClientConfig(const TTvmClientSettings& config) {
    switch (config.GetModeCase()) {
        case TTvmClientSettings::kTvmApi: {
            const TTvmClientSettings::TTvmApiSettings& tvmApiConfig = config.GetTvmApi();

            NTvmAuth::NTvmApi::TClientSettings tvmConfig;

            tvmConfig.TvmHost = tvmApiConfig.GetHost();
            tvmConfig.TvmPort = tvmApiConfig.GetPort();

            tvmConfig.SelfTvmId = tvmApiConfig.GetSelfTvmId();

            tvmConfig.DiskCacheDir = tvmApiConfig.GetDiskCacheDir();

            for (const auto& tvmIdAlias : tvmApiConfig.GetFetchServiceTicketsFor()) {
                tvmConfig.FetchServiceTicketsForDstsWithAliases.insert({tvmIdAlias.GetAlias(), tvmIdAlias.GetTvmId()});
            }

            switch (tvmApiConfig.GetSecretCase()) {
                case TTvmClientSettings::TTvmApiSettings::kSecretEnvironmentVariable: {
                    tvmConfig.Secret = GetEnv(tvmApiConfig.GetSecretEnvironmentVariable());
                    break;
                }
                case TTvmClientSettings::TTvmApiSettings::kPlainTextSecret: {
                    // Special hack for tests
                    // Do not use in production
                    tvmConfig.Secret = tvmApiConfig.GetPlainTextSecret();
                    break;
                }
                case TTvmClientSettings::TTvmApiSettings::SECRET_NOT_SET: {
                    // Defaults in protobuf oneof don't work very well :(
                    tvmConfig.Secret = GetEnv(DEFAULT_TVM_SECRET_ENVIRONMENT_VARIABLE);
                    break;
                }
            }

            tvmConfig.CheckValid();

            return tvmConfig;
        }
        case TTvmClientSettings::kTvmTool: {
            const TTvmClientSettings::TTvmToolSettings& tvmToolConfig = config.GetTvmTool();

            NTvmAuth::NTvmTool::TClientSettings tvmConfig(tvmToolConfig.GetSelfAlias());
            tvmConfig.SetPort(tvmToolConfig.GetPort());

            // Special hack for tests
            // Do not use in production
            if (!tvmToolConfig.GetAuthToken().empty()) {
                tvmConfig.SetAuthToken(tvmToolConfig.GetAuthToken());
            }

            return tvmConfig;
        }
        case TTvmClientSettings::MODE_NOT_SET: {
            ythrow yexception() << "Tvm client mode not set";
        }
    }
}

} // namepsace


TTvmClient::TTvmClient(
    const TTvmClientSettings& config
)
    : Config_(CreateTvmClientConfig(config))
    , Initialized_(false)
    , TvmClientLogger_(MakeIntrusive<TTvmLogger>())
    , TvmClient_(nullptr)
{
    Initialize();
}

bool TTvmClient::EnsureInitializedAndReady() {
    return EnsureInitialized() && TvmClient_->GetStatus() == NTvmAuth::TClientStatus::Ok;
}

TExpected<TTvmClient::TServiceTicket, TString> TTvmClient::GetServiceTicketFor(const NTvmAuth::TClientSettings::TAlias& dstAlias) {
    if (!EnsureInitializedAndReady()) {
        static const TString error = "Client is not initialized";
        return error;
    }

    try {
        return TServiceTicket({
            .Ticket = TvmClient_->GetServiceTicketFor(dstAlias),
        });
    } catch (...) {
        return CurrentExceptionMessage();
    }
}

bool TTvmClient::EnsureInitialized() {
    return IsInitialized() || Initialize();
}

bool TTvmClient::IsInitialized() const {
    return Initialized_.load(std::memory_order_acquire);
}

bool TTvmClient::Initialize() {
    TSourceMetrics metrics(NAME);
    TLogContext logContext(
        SpawnLogFrame(),
        nullptr
    );

    logContext.LogEventInfoCombo<NEvClass::TMatrixTvmClientInitializationStart>();

    TGuard<TMutex> guard(InitializeMutex_);

    if (IsInitialized()) {
        metrics.PushRate("already_initialized");
        logContext.LogEventInfoCombo<NEvClass::TMatrixTvmClientAlreadyInitialized>();
        return true;
    }

    try {
        if (std::holds_alternative<NTvmAuth::NTvmApi::TClientSettings>(Config_)) {
            TvmClient_ = MakeHolder<NTvmAuth::TTvmClient>(std::get<NTvmAuth::NTvmApi::TClientSettings>(Config_), TvmClientLogger_);
        } else {
            TvmClient_ = MakeHolder<NTvmAuth::TTvmClient>(std::get<NTvmAuth::NTvmTool::TClientSettings>(Config_), TvmClientLogger_);
        }

        logContext.LogEventInfoCombo<NEvClass::TMatrixTvmClientInitializationSuccess>();
        Initialized_.store(true, std::memory_order_release);

        return true;
    } catch (...) {
        metrics.SetError("failed_to_initialize");
        logContext.LogEventErrorCombo<NEvClass::TMatrixTvmClientInitializationError>(CurrentExceptionMessage());

        return false;
    }
}

} // namespace NMatrix
