#pragma once

#include <alice/bass/forms/context/context.h>
#include <alice/bass/libs/globalctx/globalctx.h>

#include <alice/library/unittest/ut_helpers.h>

#include <ydb/public/sdk/cpp/client/ydb_driver/driver.h>

#include <library/cpp/scheme/fwd.h>
#include <library/cpp/testing/unittest/env.h>
#include <library/cpp/testing/unittest/registar.h>

#include <util/generic/maybe.h>
#include <util/generic/strbuf.h>

namespace NBASS {
class TYdbConfig;
} // namespace NBASS

namespace NTestingHelpers {

inline const TString DEFAULT_CONFIG = TStringBuilder()
                                      << ArcadiaSourceRoot() << "/alice/bass/configs/localhost_config.json";

/** Implementation for IGlobalContext for test purposes.
 * Beware it is not thread safe!
 * All the instances shares the following states: geodb
 */
class TTestGlobalContext : public NBASS::IGlobalContext {
public:
    explicit TTestGlobalContext(TStringBuf pathToConfig = DEFAULT_CONFIG);
    TTestGlobalContext(TConfig::TOnJsonPatch configPatcher, TStringBuf pathToConfig = DEFAULT_CONFIG);

    ~TTestGlobalContext();

    const TConfig& Config() const override;
    IEventLog& EventLog() override;
    NRTLog::TClient& RTLogClient() override;

    const TSecrets& Secrets() const override;
    const TAvatarsMap& AvatarsMap() const override;

    const ISourcesRegistryDelegate& SourcesRegistryDelegate() const override;
    const TSourcesRegistry& Sources() const override;

    NBASS::NMetrics::ICountersPlace& Counters() override;
    IScheduler& Scheduler() override;

    const NBASS::THandlerFactory* ActionHandler(TStringBuf) const override {
        return nullptr;
    }

    const NBASS::THandlerFactory* FormHandler(TStringBuf) const override {
        return nullptr;
    }

    const NBASS::TContinuableHandlerFactory* ContinuableHandler(TStringBuf) const override {
        return nullptr;
    }

    TDuration ActionSLA(TStringBuf /* action */) const override {
        return {};
    }

    TDuration FormSLA(TStringBuf /* action */) const override {
        return {};
    }

    NVideoCommon::TIviGenres& IviGenres() override;

    const NBASS::TFormsDb& FormsDb() const override;

    const NBASS::NMusic::TStationsData& RadioStations() const override;
    const NBASS::NNews::TNewsData& NewsData() const override;

    NBASS::TYdbConfig& YdbConfig() override;
    NYdb::NTable::TTableClient& YdbClient() override;

    void DoInitHandlers() override {
    }

    NBASS::ITVM2TicketCache* TVM2TicketCache() override;
    bool IsTVM2Expired() override;

    IThreadPool& MessageBusThreadPool() override;
    IThreadPool& MarketThreadPool() override;
    IThreadPool& AviaThreadPool() override;

    NBASS::TContinuationParserRegistry& ContinuationRegistry() override;

    const NGeobase::TLookup& GeobaseLookup() const override;


private:
    class TImpl;
    mutable THolder<TImpl> Impl_;
};

class TRequestJson {
public:
    TRequestJson();
    explicit TRequestJson(TStringBuf jsonString);

    TRequestJson& SetClient(TStringBuf client);
    TRequestJson& SetExpFlag(TStringBuf expFlag, TStringBuf value = {});

    /** Setting a new form is a complete overwrite.
     */
    TRequestJson& SetForm(TStringBuf formName);

    TRequestJson& SetUID(ui64 uid);


    explicit operator NSc::TValue() const {
        return Request;
    }

    explicit operator NSc::TValue() {
        return Request;
    }

public:
    NSc::TValue Request;
};

class TBassContextFixture : public NUnitTest::TBaseFixture {
public:
public:
    TRequestJson ConstructDefaultRequestJson() const;

    TRequestJson CreateDefaultRequestJson() const;
    NBASS::TContext::TPtr MakeContext(const NSc::TValue& request);
    NBASS::TContext::TPtr MakeContext(TStringBuf request);

    // Creates context from json with auth header. Fails in case of error.
    NBASS::TContext::TPtr MakeAuthorizedContext(const NSc::TValue& request);
    NBASS::TContext::TPtr MakeAuthorizedContext(TStringBuf request);

    NBASS::TGlobalContextPtr GlobalCtx();

    NYdb::TDriver& LocalYdb();
    TConfig::TYdbScheme LocalYdbConfig();

private:
    NBASS::TGlobalContextPtr GlobalCtx_;
    TMaybe<NYdb::TDriver> LocalDriver_;
    TMaybe<NSc::TValue> LocalYdbConfig_;

protected:
    virtual NBASS::TGlobalContextPtr MakeGlobalCtx() {
        return NBASS::IGlobalContext::MakePtr<TTestGlobalContext>();
    }
};

void CheckResponse(const NBASS::TContext& ctx, const NSc::TValue& expected);

// Creates context from json. Fails in case of error.
NBASS::TContext::TPtr MakeContext(const NSc::TValue& request, bool shouldValidateRequest = true,
                                  const TMaybe<TString>& userTicket = Nothing());
NBASS::TContext::TPtr MakeContext(TStringBuf request, bool shouldValidateRequest = true,
                                  const TMaybe<TString>& userTicket = Nothing());

// Creates context from json with auth header. Fails in case of error.
NBASS::TContext::TPtr MakeAuthorizedContext(const NSc::TValue& request);
NBASS::TContext::TPtr MakeAuthorizedContext(TStringBuf request);

NBASS::TContext::TPtr
CreateVideoContextWithAgeRestriction(NBASS::EContentRestrictionLevel restriction,
                                     std::function<NBASS::TContext::TPtr(const NSc::TValue&)> contextCreator =
                                         [](const NSc::TValue& request) { return MakeContext(request); });

inline bool IsAttentionInContext(NBASS::TContext& ctx, TStringBuf attentionType) {
    return ctx.HasAttention(attentionType);
}

} // namespace NTestingHelpers
