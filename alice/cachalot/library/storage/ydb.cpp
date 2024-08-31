#include <alice/cachalot/library/storage/ydb.h>


namespace NCachalot {


TYdbContext::TYdbContext(const NCachalot::TYdbSettings& settings)
    : Settings(settings)
{
    InitDriver();
}


TYdbContext::~TYdbContext()
{ }


TString TYdbContext::GetAuthToken() {
    return TString(getenv("YDB_TOKEN"));
}

const NCachalot::TYdbSettings& TYdbContext::GetSettings() const {
    return Settings;
}


void TYdbContext::InitDriver() {
    NYdb::TDriverConfig config;

    config
        .SetEndpoint(Settings.Endpoint())
        .SetDatabase(Settings.Database())
        .SetAuthToken(GetAuthToken())
        .SetClientThreadsNum(Settings.ClientThreads())
        .SetNetworkThreadsNum(Settings.NetworkThreads())
        .SetBalancingPolicy(NYdb::EBalancingPolicy::UsePreferableLocation)
        .SetDiscoveryMode(NYdb::EDiscoveryMode::Async)
    ;

    NYdb::NTable::TSessionPoolSettings poolSettings;
    if (Settings.HasMaxActiveSessions()) {
        poolSettings.MaxActiveSessions(Settings.MaxActiveSessions());
    }
    if (Settings.HasMinPoolSize()) {
        poolSettings.MinPoolSize(Settings.MinPoolSize());
    }
    auto clientSettings = NYdb::NTable::TClientSettings()
        .SessionPoolSettings(poolSettings)
        // disabling useless client-side cache. https://clubs.at.yandex-team.ru/ydb/590
        .UseQueryCache(false)
    ;

    try {
        Cerr << TInstant::Now().ToString() << "> CREATING YDB DRIVER FOR " << Settings.Database() << "..." << Endl;
        Driver = MakeHolder<NYdb::TDriver>(config);

        Cerr << TInstant::Now().ToString() << "> CREATING YDB TABLE CLIENT FOR " << Settings.Database() << "..." << Endl;
        Client = MakeHolder<NYdb::NTable::TTableClient>(*Driver.Get(), clientSettings);
        Cerr << TInstant::Now().ToString() << "> YDB INIT DONE" << Endl;
    } catch (const std::exception& ex) {
        Cerr << TInstant::Now().ToString() << "> failed to create YDB client for " << Settings.Database() << ": " << ex.what() << Endl;
    }
}

}   // namespace NCachalot
