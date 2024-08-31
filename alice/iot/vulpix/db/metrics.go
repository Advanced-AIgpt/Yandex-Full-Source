package db

import (
	"context"
	"time"

	bmodel "a.yandex-team.ru/alice/iot/bulbasaur/model"
	"a.yandex-team.ru/alice/iot/vulpix/model"
	quasarmetrics "a.yandex-team.ru/alice/library/go/metrics"
	"a.yandex-team.ru/alice/library/go/ydbclient"
	"a.yandex-team.ru/kikimr/public/sdk/go/ydb"
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/metrics"
	"a.yandex-team.ru/library/go/core/xerrors"
)

type ClientWithMetrics struct {
	dbClient IClient

	selectDeviceState          quasarmetrics.YDBSignals
	storeDeviceState           quasarmetrics.YDBSignals
	selectConnectingDeviceType quasarmetrics.YDBSignals
	storeConnectingDeviceType  quasarmetrics.YDBSignals
}

func NewMetricsClient(ctx context.Context, logger log.Logger, endpoint, prefix string, credentials ydb.Credentials, trace bool, registry metrics.Registry, policy quasarmetrics.BucketsGenerationPolicy, options ...ydbclient.Options) (*ClientWithMetrics, error) {
	ydbClient, err := ydbclient.NewYDBClient(ctx, logger, endpoint, prefix, credentials, trace, options...)
	if err != nil {
		return nil, xerrors.Errorf("unable to create ydbClient: %w", err)
	}
	dbClient := NewClientWithYDBClient(ydbClient)
	return NewMetricsClientWithDB(dbClient, registry, policy), nil
}

func NewMetricsClientWithDB(client IClient, registry metrics.Registry, policy quasarmetrics.BucketsGenerationPolicy) *ClientWithMetrics {
	return &ClientWithMetrics{
		dbClient:                   client,
		selectDeviceState:          quasarmetrics.NewYDBSignals("selectDeviceState", registry, policy),
		storeDeviceState:           quasarmetrics.NewYDBSignals("storeDeviceState", registry, policy),
		selectConnectingDeviceType: quasarmetrics.NewYDBSignals("selectConnectingDeviceType", registry, policy),
		storeConnectingDeviceType:  quasarmetrics.NewYDBSignals("storeConnectingDeviceType", registry, policy),
	}
}

func (db *ClientWithMetrics) SelectDeviceState(ctx context.Context, userID uint64, speakerID string) (model.DeviceState, error) {
	start := time.Now()
	defer db.selectDeviceState.RecordDurationSince(start)

	state, err := db.dbClient.SelectDeviceState(ctx, userID, speakerID)

	db.selectDeviceState.RecordMetrics(err)
	return state, err
}

func (db *ClientWithMetrics) StoreDeviceState(ctx context.Context, userID uint64, speakerID string, state model.DeviceState) error {
	start := time.Now()
	defer db.storeDeviceState.RecordDurationSince(start)

	err := db.dbClient.StoreDeviceState(ctx, userID, speakerID, state)

	db.storeDeviceState.RecordMetrics(err)
	return err
}

func (db *ClientWithMetrics) SelectConnectingDeviceType(ctx context.Context, userID uint64, speakerID string) (bmodel.DeviceType, error) {
	start := time.Now()
	defer db.selectConnectingDeviceType.RecordDurationSince(start)

	dt, err := db.dbClient.SelectConnectingDeviceType(ctx, userID, speakerID)

	db.selectConnectingDeviceType.RecordMetrics(err)
	return dt, err
}

func (db *ClientWithMetrics) StoreConnectingDeviceType(ctx context.Context, userID uint64, speakerID string, deviceType bmodel.DeviceType) error {
	start := time.Now()
	defer db.storeConnectingDeviceType.RecordDurationSince(start)

	err := db.dbClient.StoreConnectingDeviceType(ctx, userID, speakerID, deviceType)

	db.storeConnectingDeviceType.RecordMetrics(err)
	return err
}
