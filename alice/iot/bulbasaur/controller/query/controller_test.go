package query

import (
	"context"
	"testing"

	"github.com/stretchr/testify/assert"

	"a.yandex-team.ru/alice/iot/bulbasaur/controller/history"
	"a.yandex-team.ru/alice/iot/bulbasaur/controller/notificator"
	"a.yandex-team.ru/alice/iot/bulbasaur/controller/updates"
	"a.yandex-team.ru/alice/iot/bulbasaur/db"
	"a.yandex-team.ru/alice/iot/bulbasaur/model"
	"a.yandex-team.ru/alice/iot/bulbasaur/provider"
	xtesting "a.yandex-team.ru/alice/iot/bulbasaur/testing"
	"a.yandex-team.ru/alice/library/go/requestid"
	"a.yandex-team.ru/alice/library/go/timestamp"
	"a.yandex-team.ru/alice/library/go/xiva"
	xmetrics "a.yandex-team.ru/library/go/core/metrics/mock"
)

func TestUpdateYandexIODevicesStatus(t *testing.T) {
	ctx := timestamp.ContextWithTimestamper(context.Background(), timestamp.NewMockTimestamper().WithCurrentTimestamp(12345))
	alice := model.User{ID: 42, Login: "alice"}
	yandexIOLamp := model.Device{
		ID:         "lamp-id",
		ExternalID: "lamp-external-id",
		SkillID:    model.YANDEXIO,
		Status:     model.OnlineDeviceStatus,
		CustomData: map[string]string{"parent_endpoint_id": "some-speaker-id"},
	}

	logger := xtesting.NopLogger()
	dbClient := &db.DBClientMock{}
	notificatorController := notificator.NewMock()
	notificatorController.OnlineDeviceIDsResponses["online-request-id"] = []string{"some-speaker-id"}
	notificatorController.OnlineDeviceIDsResponses["offline-request-id"] = []string{}
	pf := &provider.Factory{
		Notificator:     notificatorController,
		Logger:          logger,
		SignalsRegistry: provider.NewSignalsRegistry(xmetrics.NewRegistry(xmetrics.NewRegistryOpts())),
	}
	s := NewController(logger, dbClient, pf, updates.NewController(logger, xiva.NewMockClient(), dbClient, notificatorController), history.NewMock(), notificatorController)
	t.Run("online status in response", func(t *testing.T) {
		requestDevices := model.Devices{yandexIOLamp}
		onlineCtx := requestid.WithRequestID(ctx, "online-request-id")
		actualResultDevices, actualDeviceStatusMap, err := s.UpdateDevicesState(onlineCtx, requestDevices, model.NewOrigin(ctx, model.SearchAppSurfaceParameters{}, alice))
		assert.NoError(t, err)

		assert.Equal(t, model.OnlineDeviceStatus, actualResultDevices[0].Status)
		assert.Equal(t, model.OnlineDeviceStatus, actualDeviceStatusMap[yandexIOLamp.ID])
		assert.Equal(t, timestamp.PastTimestamp(12345), actualResultDevices[0].StatusUpdated)
	})

	t.Run("offline status in response", func(t *testing.T) {
		requestDevices := model.Devices{yandexIOLamp}
		onlineCtx := requestid.WithRequestID(ctx, "offline-request-id")
		actualResultDevices, actualDeviceStatusMap, err := s.UpdateDevicesState(onlineCtx, requestDevices, model.NewOrigin(ctx, model.SearchAppSurfaceParameters{}, alice))
		assert.NoError(t, err)

		assert.Equal(t, model.OfflineDeviceStatus, actualResultDevices[0].Status)
		assert.Equal(t, model.OfflineDeviceStatus, actualDeviceStatusMap[yandexIOLamp.ID])
		assert.Equal(t, timestamp.PastTimestamp(12345), actualResultDevices[0].StatusUpdated)
	})

	t.Run("offline status in device", func(t *testing.T) {
		yandexIOLamp.Status = model.OfflineDeviceStatus
		requestDevices := model.Devices{yandexIOLamp}
		actualResultDevices, actualDeviceStatusMap, err := s.UpdateDevicesState(ctx, requestDevices, model.NewOrigin(ctx, model.SearchAppSurfaceParameters{}, alice))
		assert.NoError(t, err)

		assert.Equal(t, model.OfflineDeviceStatus, actualResultDevices[0].Status)
		assert.Equal(t, model.OfflineDeviceStatus, actualDeviceStatusMap[yandexIOLamp.ID])
		assert.Equal(t, timestamp.PastTimestamp(12345), actualResultDevices[0].StatusUpdated)
	})
}
