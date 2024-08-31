package notificator

import (
	"context"
	"strconv"

	"golang.org/x/exp/slices"

	"a.yandex-team.ru/alice/library/go/notificator"
	"a.yandex-team.ru/alice/megamind/protos/common"
	matrixpb "a.yandex-team.ru/alice/protos/api/matrix"
	notificatorpb "a.yandex-team.ru/alice/protos/api/notificator"
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/xerrors"
)

type Controller struct {
	notificator notificator.IClient
	logger      log.Logger
}

func NewController(notificator notificator.IClient, logger log.Logger) *Controller {
	return &Controller{
		notificator: notificator,
		logger:      logger,
	}
}

func (c *Controller) SendTypedSemanticFrame(ctx context.Context, userID uint64, deviceID string, frame TSF, options ...Option) error {
	tDelivery := &matrixpb.TDelivery{
		Puid:     strconv.FormatUint(userID, 10),
		DeviceId: deviceID,
		Ttl:      DefaultPushTTL, // seconds
		TRequestDirective: &matrixpb.TDelivery_SemanticFrameRequestData{
			SemanticFrameRequestData: &common.TSemanticFrameRequestData{
				TypedSemanticFrame: frame.ToTypedSemanticFrame(),
				Analytics: &common.TAnalyticsTrackingModule{
					ProductScenario: MMScenarioProductName,
					Origin:          common.TAnalyticsTrackingModule_Scenario,
					Purpose:         MMScenarioPurpose,
					OriginInfo:      MMScenarioOriginInfo,
				},
			},
		},
	}

	for _, option := range options {
		option(tDelivery)
	}

	response, err := c.notificator.SendDeliveryPush(ctx, tDelivery)
	if err != nil {
		return xerrors.Errorf("failed to send request to notificator: %w", err)
	}

	switch status := response.GetSubwayRequestStatus().GetStatus(); status {
	case matrixpb.TDeliveryResponse_TSubwayRequestStatus_OK:
		return nil
	case matrixpb.TDeliveryResponse_TSubwayRequestStatus_LOCATION_DISCOVERY_ERROR,
		matrixpb.TDeliveryResponse_TSubwayRequestStatus_LOCATION_NOT_FOUND,
		matrixpb.TDeliveryResponse_TSubwayRequestStatus_OUTDATED_LOCATION:
		errorMessage := response.GetSubwayRequestStatus().GetErrorMessage()
		return xerrors.Errorf("failed to send typed semantic frame to device %s: %s :%w", deviceID, errorMessage, DeviceOfflineError)
	default:
		errorMessage := response.GetSubwayRequestStatus().GetErrorMessage()
		return xerrors.Errorf("failed to send typed semantic frame to device %s: status %s, message %s", deviceID, status.String(), errorMessage)
	}
}

func (c *Controller) SendSpeechkitDirective(ctx context.Context, userID uint64, deviceID string, directive SpeechkitDirective) error {
	rawSpeechKitDirective, err := marshalSpeechkitDirective(directive)
	if err != nil {
		return xerrors.Errorf("failed to marshal sk directive: %w", err)
	}
	deliveryPush := &matrixpb.TDelivery{
		Puid:     strconv.FormatUint(userID, 10),
		DeviceId: deviceID,
		Ttl:      DefaultPushTTL, // seconds
		TRequestDirective: &matrixpb.TDelivery_SpeechKitDirective{
			SpeechKitDirective: rawSpeechKitDirective,
		},
	}
	response, err := c.notificator.SendDeliveryPush(ctx, deliveryPush)
	if err != nil {
		return xerrors.Errorf("failed to send sk directive to notificator: %w", err)
	}

	switch status := response.GetSubwayRequestStatus().GetStatus(); status {
	case matrixpb.TDeliveryResponse_TSubwayRequestStatus_OK:
		return nil
	case matrixpb.TDeliveryResponse_TSubwayRequestStatus_LOCATION_DISCOVERY_ERROR,
		matrixpb.TDeliveryResponse_TSubwayRequestStatus_LOCATION_NOT_FOUND,
		matrixpb.TDeliveryResponse_TSubwayRequestStatus_OUTDATED_LOCATION:
		errorMessage := response.GetSubwayRequestStatus().GetErrorMessage()
		return xerrors.Errorf("failed to send speechkit directive to device %s: %s :%w", deviceID, errorMessage, DeviceOfflineError)
	default:
		errorMessage := response.GetSubwayRequestStatus().GetErrorMessage()
		return xerrors.Errorf("failed to send speechkit directive to device %s: status %s, message %s", deviceID, status.String(), errorMessage)
	}
}

func (c *Controller) IsDeviceOnline(ctx context.Context, userID uint64, deviceID string) bool {
	onlineDeviceIDs, err := c.OnlineDeviceIDs(ctx, userID)
	if err != nil {
		return false
	}
	return slices.Contains(onlineDeviceIDs, deviceID)
}

func (c *Controller) OnlineDeviceIDs(ctx context.Context, userID uint64) ([]string, error) {
	// ZION-151 might offer a better way
	result := make([]string, 0)
	request := &notificatorpb.TGetDevicesRequest{Puid: strconv.FormatUint(userID, 10)}
	response, err := c.notificator.GetDevices(ctx, request)
	if err != nil {
		return nil, err
	}
	for _, matrixDevice := range response.GetDevices() {
		result = append(result, matrixDevice.GetDeviceId())
	}
	return result, nil
}
