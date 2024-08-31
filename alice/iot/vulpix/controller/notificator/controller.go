package notificator

import (
	"context"
	"strconv"

	"a.yandex-team.ru/alice/iot/bulbasaur/megamind"
	dtopush "a.yandex-team.ru/alice/iot/vulpix/dto/push"
	"a.yandex-team.ru/alice/library/go/notificator"
	"a.yandex-team.ru/alice/megamind/protos/common"
	protomatrix "a.yandex-team.ru/alice/protos/api/matrix"
	"a.yandex-team.ru/library/go/core/xerrors"
)

type Controller struct {
	Notificator notificator.IClient
}

func (c *Controller) IoTBroadcastSuccessPush(ctx context.Context, userID uint64, deviceID string, frame dtopush.IoTBroadcastSuccess) error {
	return c.simplePush(ctx, userID, deviceID, frame.ToTypedSemanticFrame())
}

func (c *Controller) IoTBroadcastFailurePush(ctx context.Context, userID uint64, deviceID string, frame dtopush.IoTBroadcastFailure) error {
	return c.simplePush(ctx, userID, deviceID, frame.ToTypedSemanticFrame())
}

func (c *Controller) IoTDiscoverySuccessPush(ctx context.Context, userID uint64, deviceID string, frame dtopush.IoTDiscoverySuccess) error {
	return c.simplePush(ctx, userID, deviceID, frame.ToTypedSemanticFrame())
}

func (c *Controller) simplePush(ctx context.Context, userID uint64, deviceID string, frame *common.TTypedSemanticFrame) error {
	tDelivery := &protomatrix.TDelivery{
		Puid:     strconv.FormatUint(userID, 10),
		DeviceId: deviceID,
		Ttl:      3, // seconds
		TRequestDirective: &protomatrix.TDelivery_SemanticFrameRequestData{
			SemanticFrameRequestData: &common.TSemanticFrameRequestData{
				TypedSemanticFrame: frame,
				Analytics: &common.TAnalyticsTrackingModule{
					ProductScenario: megamind.IoTProductScenarioName,
					Origin:          common.TAnalyticsTrackingModule_Scenario,
					Purpose:         "notify_about_broadcast_success",
					OriginInfo:      MMScenarioOriginInfo,
				},
			},
		},
	}
	response, err := c.Notificator.SendDeliveryPush(ctx, tDelivery)
	if err != nil {
		return xerrors.Errorf("failed to send request to notificator: %w", err)
	}
	if response.Code == protomatrix.TDeliveryResponse_NoLocations {
		return xerrors.Errorf("sent typed semantic frame to device %s but device is offline", deviceID)
	}
	return nil
}
