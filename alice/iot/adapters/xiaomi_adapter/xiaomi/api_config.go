package xiaomi

import (
	"a.yandex-team.ru/alice/iot/adapters/xiaomi_adapter/xiaomi/iotapi"
	"a.yandex-team.ru/alice/iot/adapters/xiaomi_adapter/xiaomi/miotspec"
	"a.yandex-team.ru/alice/iot/adapters/xiaomi_adapter/xiaomi/userapi"
	"a.yandex-team.ru/alice/library/go/zora"
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/metrics"
)

type APIConfig struct {
	IOTAPIClients  iotapi.APIClients
	UserAPIClient  userapi.APIClient
	MIOTSpecClient miotspec.APIClient
}

func NewAPIConfig(logger log.Logger, registry metrics.Registry, zoraClient *zora.Client, appID, callbackURL string) APIConfig {
	var config APIConfig

	apiRegistry := registry.WithPrefix("api")

	chinaIotAPIClient := iotapi.NewClientWithMetrics(logger, appID, iotapi.ChinaRegion, callbackURL, apiRegistry, zoraClient)
	ruIotAPIClient := iotapi.NewClientWithMetrics(logger, appID, iotapi.RussiaRegion, callbackURL, apiRegistry, zoraClient)

	config.IOTAPIClients = iotapi.APIClients{
		DefaultRegion: iotapi.ChinaRegion,
		Clients: map[iotapi.Region]iotapi.APIClient{
			iotapi.ChinaRegion:  chinaIotAPIClient,
			iotapi.RussiaRegion: ruIotAPIClient,
		},
	}

	userAPIClient := userapi.NewClientWithMetrics(logger, appID, apiRegistry, zoraClient)
	config.UserAPIClient = userAPIClient

	miotSpecClient := miotspec.NewClientWithMetrics(logger, apiRegistry, zoraClient)
	config.MIOTSpecClient = miotSpecClient
	return config
}
