package db

import (
	"context"
	"time"

	"a.yandex-team.ru/alice/iot/bulbasaur/model"
	quasarmetrics "a.yandex-team.ru/alice/library/go/metrics"
	"a.yandex-team.ru/library/go/core/metrics"
)

type ClientWithMetrics struct {
	dbClient *Client

	devicePropertyHistory quasarmetrics.YDBSignals
	storeDeviceProperties quasarmetrics.YDBSignals

	registry metrics.Registry
	policy   quasarmetrics.BucketsGenerationPolicy
}

func NewMetricsClientWithDB(client *Client, registry metrics.Registry, policy quasarmetrics.BucketsGenerationPolicy) *ClientWithMetrics {
	return &ClientWithMetrics{
		dbClient:              client,
		devicePropertyHistory: quasarmetrics.NewYDBSignals("devicePropertyHistory", registry, policy),
		storeDeviceProperties: quasarmetrics.NewYDBSignals("storeDeviceProperties", registry, policy),
		registry:              registry,
		policy:                policy,
	}
}

func (db *ClientWithMetrics) IsDatabaseError(err error) bool {
	return db.dbClient.IsDatabaseError(err)
}

func (db *ClientWithMetrics) DevicePropertyHistory(ctx context.Context, userID uint64, deviceID string, propertyType model.PropertyType, instance model.PropertyInstance) (_ []model.PropertyLogData, err error) {
	start := time.Now()
	defer func() { db.recordMetric(ctx, start, db.devicePropertyHistory, err) }()
	return db.dbClient.DevicePropertyHistory(ctx, userID, deviceID, propertyType, instance)
}

func (db *ClientWithMetrics) StoreDeviceProperties(ctx context.Context, userID uint64, deviceProperties model.DevicePropertiesMap, source model.RequestSource) (err error) {
	start := time.Now()
	defer func() { db.recordMetric(ctx, start, db.storeDeviceProperties, err) }()
	return db.dbClient.StoreDeviceProperties(ctx, userID, deviceProperties, source)
}

func (db *ClientWithMetrics) recordMetric(ctx context.Context, start time.Time, signal quasarmetrics.YDBSignals, err error) {
	if !db.dbClient.HasTransaction(ctx) {
		signal.RecordDurationSince(start)
		signal.RecordMetrics(err)
	}
}
