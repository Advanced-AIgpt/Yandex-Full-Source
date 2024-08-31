package bass

import (
	"context"

	"a.yandex-team.ru/alice/iot/bulbasaur/bass"
	"a.yandex-team.ru/alice/iot/bulbasaur/megamind"
	dtopush "a.yandex-team.ru/alice/iot/vulpix/dto/push"
)

type Controller struct {
	Bass bass.IBass
}

func (c *Controller) IoTBroadcastSuccessPush(ctx context.Context, userID uint64, deviceID string, frame dtopush.IoTBroadcastSuccess) error {
	analyticsData := bass.SemanticFrameAnalyticsData{
		ProductScenario: megamind.IoTProductScenarioName,
		Origin:          MMScenarioOrigin,
		Purpose:         "notify_about_broadcast_success",
	}
	return c.Bass.SendSemanticFramePush(ctx, userID, deviceID, frame.ToBassSemanticFrame(), analyticsData)
}

func (c *Controller) IoTBroadcastFailurePush(ctx context.Context, userID uint64, deviceID string, frame dtopush.IoTBroadcastFailure) error {
	analyticsData := bass.SemanticFrameAnalyticsData{
		ProductScenario: megamind.IoTProductScenarioName,
		Origin:          MMScenarioOrigin,
		Purpose:         "notify_about_broadcast_failure",
	}
	return c.Bass.SendSemanticFramePush(ctx, userID, deviceID, frame.ToBassSemanticFrame(), analyticsData)
}

func (c *Controller) IoTDiscoverySuccessPush(ctx context.Context, userID uint64, deviceID string, frame dtopush.IoTDiscoverySuccess) error {
	analyticsData := bass.SemanticFrameAnalyticsData{
		ProductScenario: megamind.IoTProductScenarioName,
		Origin:          MMScenarioOrigin,
		Purpose:         "notify_about_discovery_success",
	}
	return c.Bass.SendSemanticFramePush(ctx, userID, deviceID, frame.ToBassSemanticFrame(), analyticsData)
}
