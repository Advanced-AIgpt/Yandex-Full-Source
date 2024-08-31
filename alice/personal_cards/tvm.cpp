#include "tvm.h"

#include <alice/bass/libs/logging/logger.h>

namespace NPersonalCards {

namespace {

class TTvmLogger: public NTvmAuth::ILogger {
public:
    TTvmLogger(const int level)
        : Level_(level)
    {}

    void Log(int level, const TString& msg) override {
        if (level > Level_) {
            return;
        }

        LOG(INFO) << "TvmClient: lvl=" << level << " msg: " << msg << Endl;
    }

private:
    const int Level_;
};

template <typename TSettings>
TAtomicSharedPtr<NTvmAuth::TTvmClient> CreateWithRetries(
    const TSettings& tvmSettings,
    TIntrusivePtr<TTvmLogger> logger,
    ui32 retriesCount
) {
    for (ui32 tryIdx = 0; tryIdx <= retriesCount; ++tryIdx) {
        try {
            return MakeAtomicShared<NTvmAuth::TTvmClient>(tvmSettings, logger);
        } catch (...) {
            if (tryIdx == retriesCount) {
                throw;
            } else {
                Sleep(TDuration::Seconds(1));
            }
        }
    }

    Y_UNREACHABLE();
}

} // namespace

TTvmClientPtr CreateTvmClient(const TTvmConfig& tvmConfig, bool withRetries) {
    auto logger = MakeIntrusive<TTvmLogger>(tvmConfig.GetLogLevel());
    const auto retriesCount = withRetries ? tvmConfig.GetCreationRetriesCount() : 0u;

    if (tvmConfig.HasAlias()) {
        NTvmAuth::NTvmTool::TClientSettings tvmSettings(tvmConfig.GetAlias());
        if (tvmConfig.HasHost()) {
            tvmSettings.SetHostname(tvmConfig.GetHost());
        }
        if (tvmConfig.HasPort()) {
            tvmSettings.SetPort(tvmConfig.GetPort());
        }

        return CreateWithRetries(tvmSettings, logger, retriesCount);
    } else if (tvmConfig.HasApi()){
        NTvmAuth::NTvmApi::TClientSettings tvmSettings;
        tvmSettings.SetSelfTvmId(tvmConfig.GetApi().GetSelfTvmId());
        tvmSettings.EnableServiceTicketChecking();
        tvmSettings.EnableUserTicketChecking(FromString<NTvmAuth::EBlackboxEnv>(tvmConfig.GetApi().GetBlackboxEnv()));

        if (tvmConfig.HasHost() && tvmConfig.HasPort()) {
            tvmSettings.SetTvmHostPort(TString(tvmConfig.GetHost()), tvmConfig.GetPort());
        }

        return CreateWithRetries(tvmSettings, logger, retriesCount);
    }

    throw yexception() << "Alias or tvm api settings must be provided.";
}

} // namespace NPersonalCards
