package pgdb

import (
	"context"
	"time"

	xmodel "a.yandex-team.ru/alice/iot/adapters/xiaomi_adapter/xiaomi/model"
	quasarmetrics "a.yandex-team.ru/alice/library/go/metrics"
	"a.yandex-team.ru/library/go/core/metrics"
)

type ClientWithMetrics struct {
	client *Client

	registry metrics.Registry

	selectExternalUserSignals       quasarmetrics.PGDBSignals
	storeExternalUserSignals        quasarmetrics.PGDBSignals
	deleteExternalUserSignals       quasarmetrics.PGDBSignals
	storeUserSubscriptionsSignals   quasarmetrics.PGDBSignals
	storeDeviceSubscriptionsSignals quasarmetrics.PGDBSignals
}

func NewClientWithMetrics(client *Client, registry metrics.Registry, policy quasarmetrics.BucketsGenerationPolicy) *ClientWithMetrics {
	clientWithMetrics := &ClientWithMetrics{
		client:                          client,
		registry:                        registry,
		selectExternalUserSignals:       quasarmetrics.NewPGDBSignals("selectExternalUser", registry, policy),
		storeExternalUserSignals:        quasarmetrics.NewPGDBSignals("storeExternalUser", registry, policy),
		deleteExternalUserSignals:       quasarmetrics.NewPGDBSignals("deleteExternalUser", registry, policy),
		storeUserSubscriptionsSignals:   quasarmetrics.NewPGDBSignals("storeUserSubscriptions", registry, policy),
		storeDeviceSubscriptionsSignals: quasarmetrics.NewPGDBSignals("storeDeviceSubscriptions", registry, policy),
	}
	return clientWithMetrics
}

func (c *ClientWithMetrics) SelectExternalUser(ctx context.Context, externalUserID string) (*xmodel.ExternalUser, error) {
	start := time.Now()
	defer c.selectExternalUserSignals.RecordDurationSince(start)

	externalUser, err := c.client.SelectExternalUser(ctx, externalUserID)

	c.selectExternalUserSignals.RecordMetrics(err)
	return externalUser, err
}

func (c *ClientWithMetrics) StoreExternalUser(ctx context.Context, externalUserID string, userID uint64) error {
	start := time.Now()
	defer c.storeExternalUserSignals.RecordDurationSince(start)

	err := c.client.StoreExternalUser(ctx, externalUserID, userID)

	c.storeExternalUserSignals.RecordMetrics(err)
	return err
}

func (c *ClientWithMetrics) DeleteExternalUser(ctx context.Context, externalUserID string, userIDToDelete uint64) error {
	start := time.Now()
	defer c.deleteExternalUserSignals.RecordDurationSince(start)

	err := c.client.DeleteExternalUser(ctx, externalUserID, userIDToDelete)

	c.deleteExternalUserSignals.RecordMetrics(err)
	return err
}

func (c *ClientWithMetrics) StoreUserSubscriptions(ctx context.Context, externalUserID string) error {
	start := time.Now()
	defer c.storeUserSubscriptionsSignals.RecordDurationSince(start)

	err := c.client.StoreUserSubscriptions(ctx, externalUserID)

	c.storeUserSubscriptionsSignals.RecordMetrics(err)
	return err
}

func (c *ClientWithMetrics) StoreDeviceSubscriptions(ctx context.Context, externalUserID string, device xmodel.Device, propertyIDs, eventIDs []string) error {
	start := time.Now()
	defer c.storeDeviceSubscriptionsSignals.RecordDurationSince(start)

	err := c.client.StoreDeviceSubscriptions(ctx, externalUserID, device, propertyIDs, eventIDs)

	c.storeDeviceSubscriptionsSignals.RecordMetrics(err)
	return err
}
