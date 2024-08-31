#include <library/cpp/testing/unittest/registar.h>
#include <alice/cachalot/library/config/load.h>
#include <alice/cachalot/library/main_class.h>

using namespace NCachalot;

template<class TStorage>
void ValidateTTLMax(TStorage storage) {
	Y_ASSERT(storage.TimeToLiveSeconds() <= storage.MaxTimeToLiveSeconds());
}

Y_UNIT_TEST_SUITE(CachalotConfigTest) {

Y_UNIT_TEST(TestTTLDefautLessThanMax) {
	for (auto& cfg : NCachalot::ExecuteConfigOptions) {
		auto bin = "cachalot";
		TApplicationSettings appSettings = NCachalot::NConfig::LoadApplicationConfig(1, &bin, cfg.GetDefaultConfigResorce());
		{
			auto ydb = appSettings.YabioContext().Storage().YdbClient();
			Y_ASSERT(ydb.TimeToLiveSeconds() < ydb.MaxTimeToLiveSeconds());
		}
		{ // ActivationServiceSettings
		    ValidateTTLMax(appSettings.Activation().Ydb());
		}
		{ // CacheServiceSettings
			for (auto &[str, settings] : appSettings.Cache().Storages()) {
				ValidateTTLMax(settings.Ydb());
				ValidateTTLMax(settings.Imdb());	
			}
		}
		{ // GDPRServiceSettings
			 auto dir = appSettings.GDPR();
			 ValidateTTLMax(dir.Ydb());
			 ValidateTTLMax(dir.OldYdb());
			 ValidateTTLMax(dir.NewYdb());
		}
		{ // MegamindSessionServiceSettings
			auto dir = appSettings.MegamindSession().Storage();
			ValidateTTLMax(dir.Ydb());
			ValidateTTLMax(dir.Imdb());
		}
		{ // TakeoutServiceSettings
			ValidateTTLMax(appSettings.Takeout().Ydb());
		}
		{ // VinsContextServiceSettings
			ValidateTTLMax(appSettings.VinsContext().Ydb());
		}
		{ // YabioContextServiceSettings
			ValidateTTLMax(appSettings.YabioContext().Storage().YdbClient());
		}
	}
}

}
