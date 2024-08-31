#pragma once

#include "common/blackbox_api.h"
#include "common/data_sync_api.h"
#include "vins.h"

#include <library/cpp/neh/neh.h>
#include <util/generic/ptr.h>

namespace NBASS {

constexpr TStringBuf PERSONALIZATION_EXPERIMENT = "personalization";
constexpr TStringBuf MUSIC_PERSONALIZATION_EXPERIMENT = "music_personalization";
constexpr TStringBuf AUTO_INSERT_EXPERIMENT = "username_auto_insert";

class TUserAwareHandler : public IHandler {
public:
    struct TConfig {
        TDuration NameDelay = TDuration::Hours(3);
        double BiometryScoreThreshold = 0.9;
        double PersonalizationDropProbabilty = 0.66;
        TDuration PersonalizationAdditionalDataSyncTimeout = TDuration::MilliSeconds(50);

        // For music_personalization experiment, see DIALOG-4794 for details
        // Music name pronounce settings.
        // No more than count times per delayPeriod
        // At least (period - 1) music queries between pronunciations
        TDuration MusicNamePronounceDelayPeroid = TDuration::Hours(1);
        i64 MusicNamePronounceDelayCount = 3;
        i64 MusicNamePronouncePeriod = 5;
    };

    struct TDelegate {
        virtual ~TDelegate() = default;
        virtual TInstant GetTimestamp();
    };

    // Takes ownership of slaveHandler
    TUserAwareHandler(THolder<IHandler> slaveHandler, const TConfig& config, THolder<TDelegate> delegate,
                      THolder<TBlackBoxAPI> blackBoxAPI, THolder<TDataSyncAPI> dataSyncAPI);

    TResultValue Do(TRequestHandler& r) override; // IHandler
    TResultValue DoSetup(TSetupContext& ctx) override; // IHandler

private:
    TConfig Config;
    THolder<TDelegate> Delegate;
    THolder<IHandler> SlaveHandler;
    THolder<TBlackBoxAPI> BlackBoxAPI;
    THolder<TDataSyncAPI> DataSyncAPI;
};

}; // namespace NBASS
