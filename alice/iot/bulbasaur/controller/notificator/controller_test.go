package notificator_test

import (
	"context"
	"testing"

	"github.com/stretchr/testify/assert"

	"a.yandex-team.ru/alice/iot/bulbasaur/controller/notificator"
	libnotificator "a.yandex-team.ru/alice/library/go/notificator"
	"a.yandex-team.ru/alice/megamind/protos/common"
	matrixpb "a.yandex-team.ru/alice/protos/api/matrix"
	notificatorpb "a.yandex-team.ru/alice/protos/api/notificator"
	"a.yandex-team.ru/library/go/core/log/nop"
	"a.yandex-team.ru/library/go/core/xerrors"
)

type dummyFrame struct{}

func (d dummyFrame) ToTypedSemanticFrame() *common.TTypedSemanticFrame {
	return nil
}

func TestController(t *testing.T) {
	clientMock := &libnotificator.Mock{
		DeviceIDToDeliveryResponse: map[string]*matrixpb.TDeliveryResponse{
			"online-speaker": {
				SubwayRequestStatus: &matrixpb.TDeliveryResponse_TSubwayRequestStatus{
					Status:       matrixpb.TDeliveryResponse_TSubwayRequestStatus_OK,
					ErrorMessage: "",
				},
			},
			"offline-speaker": {
				SubwayRequestStatus: &matrixpb.TDeliveryResponse_TSubwayRequestStatus{
					Status:       matrixpb.TDeliveryResponse_TSubwayRequestStatus_LOCATION_NOT_FOUND,
					ErrorMessage: "ba ba black sheep",
				},
			},
		},
		UserIDToGetDevicesResponse: map[string]*notificatorpb.TGetDevicesResponse{
			"12345": {
				Devices: []*notificatorpb.TGetDevicesResponse_TDevice{
					{DeviceId: "online-speaker"},
				},
			},
		},
	}
	ctx := context.Background()
	controller := notificator.NewController(clientMock, &nop.Logger{})

	assert.True(t, controller.SendTypedSemanticFrame(ctx, 12345, "online-speaker", dummyFrame{}) == nil)
	assert.True(t, xerrors.Is(controller.SendTypedSemanticFrame(ctx, 12345, "offline-speaker", dummyFrame{}), notificator.DeviceOfflineError))

	assert.True(t, controller.IsDeviceOnline(ctx, 12345, "online-speaker"))
	assert.False(t, controller.IsDeviceOnline(ctx, 12345, "offline-speaker"))
}
