#pragma once

#include <alice/bass/libs/metrics/place.h>

#include <util/generic/ptr.h>
#include <util/generic/strbuf.h>
#include <util/generic/string.h>
#include <util/thread/pool.h>

#include <functional>

#include "fwd.h"

class IEventLog;
class TConfig;
class TAvatarsMap;
class IScheduler;
class ISourcesRegistryDelegate;
class TSourcesRegistry;

namespace NYdb::NTable {
class TTableClient;
} // namespace NYdb::NTable

namespace NHttpFetcher {
class TFetcher;
} // namespace NHttpFetcher

namespace NRTLog {
class TClient;
} // namespace NRTLog

namespace NVideoCommon {
class TIviGenres;
} // namespace NVideoCommon

namespace NGeobase {
class TLookup;
} // namespace NGeobase

namespace NBASS {

class IHandler;
class TContinuableHandler;
class TContinuationParserRegistry;

using THandlerFactory = std::function<THolder<IHandler>()>;
using TContinuableHandlerFactory = std::function<THolder<TContinuableHandler>()>;

class TFormsDb;

class ITVM2TicketCache;
class TYdbConfig;

namespace NMusic {
class TStationsData;
} // namespace NMusic

namespace NNews {
class TNewsData;
} // namespace NNews

class IGlobalContext : public TThrRefBase {
public:
    struct TSecrets {
        const TString GoogleAPIsKey;
        const TString KinopoiskToken;
        const TString TVClientId;
        const TString TVUid;
        const TString NavigatorKey;
        const TString TVM2Secret;
        const TString StaticMapRouterKey;
    };

public:
    template <typename T, typename... TArgs>
    static TGlobalContextPtr MakePtr(TArgs&&... args) {
        return MakeIntrusive<T>(std::forward<TArgs>(args)...);
    }

public:
    ~IGlobalContext() override = default;

    virtual const TConfig& Config() const = 0;
    virtual IEventLog& EventLog() = 0;
    virtual NRTLog::TClient& RTLogClient() = 0;

    virtual const TSecrets& Secrets() const = 0;
    virtual const TAvatarsMap& AvatarsMap() const = 0;

    virtual NMetrics::ICountersPlace& Counters() = 0;
    virtual IScheduler& Scheduler() = 0;

    virtual const ISourcesRegistryDelegate& SourcesRegistryDelegate() const = 0;
    virtual const TSourcesRegistry& Sources() const = 0;

    virtual const THandlerFactory* ActionHandler(TStringBuf action) const = 0;
    virtual const THandlerFactory* FormHandler(TStringBuf action) const = 0;
    virtual const TContinuableHandlerFactory* ContinuableHandler(TStringBuf form) const = 0;
    virtual TDuration ActionSLA(TStringBuf action) const = 0;
    virtual TDuration FormSLA(TStringBuf action) const = 0;

    virtual ::NVideoCommon::TIviGenres& IviGenres() = 0;

    virtual const TFormsDb& FormsDb() const = 0;

    virtual NYdb::NTable::TTableClient& YdbClient() = 0;
    virtual TYdbConfig& YdbConfig() = 0;

    virtual IThreadPool& MessageBusThreadPool() = 0;
    virtual IThreadPool& MarketThreadPool() = 0;
    virtual IThreadPool& AviaThreadPool() = 0;

    virtual ITVM2TicketCache* TVM2TicketCache() = 0;
    virtual bool IsTVM2Expired() = 0;

    virtual TContinuationParserRegistry& ContinuationRegistry() = 0;

    virtual const NGeobase::TLookup& GeobaseLookup() const = 0;

    virtual const NMusic::TStationsData& RadioStations() const = 0;
    virtual const NNews::TNewsData& NewsData() const = 0;

    void InitHandlers();

protected:
    virtual void DoInitHandlers() = 0;

private:
    bool HandlersInitialized = false;
};

} // namespace NBASS
