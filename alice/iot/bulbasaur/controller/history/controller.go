package history

import (
	"context"

	historydb "a.yandex-team.ru/alice/iot/bulbasaur/controller/history/db"
	"a.yandex-team.ru/alice/iot/bulbasaur/model"
	"a.yandex-team.ru/alice/library/go/goroutines"
	"a.yandex-team.ru/alice/library/go/solomonapi"
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/log/ctxlog"
)

type Controller struct {
	db             historydb.DB
	logger         log.Logger
	solomonSender  solomonapi.Sender
	solomonFetcher solomonapi.Fetcher
	solomonShard   SolomonShard
}

func NewController(
	historyDB historydb.DB,
	logger log.Logger,
	solomonSender solomonapi.Sender,
	solomonFetcher solomonapi.Fetcher,
	solomonShard SolomonShard,
) *Controller {
	return &Controller{
		db:             historyDB,
		logger:         logger,
		solomonShard:   solomonShard,
		solomonSender:  solomonSender,
		solomonFetcher: solomonFetcher,
	}
}

func (c *Controller) PropertyHistory(ctx context.Context, user model.User, deviceID string, propertyType model.PropertyType, instance model.PropertyInstance, source model.RequestSource) (model.PropertyHistory, error) {
	history := model.PropertyHistory{
		Type:     propertyType,
		Instance: instance,
	}

	data, err := c.db.DevicePropertyHistory(ctx, user.ID, deviceID, propertyType, instance)
	if err != nil {
		return history, err
	}

	history.LogData = make([]model.PropertyLogData, 0, len(data))
	for _, d := range data {
		if d.Source != string(source) {
			continue
		}
		history.LogData = append(history.LogData, d)
	}

	return history, nil
}

func (c *Controller) StoreDeviceProperties(ctx context.Context, userID uint64, deviceProperties model.DevicePropertiesMap, source model.RequestSource) error {
	group := goroutines.Group{}
	group.Go(func() error {
		err := c.pushMetricsToSolomon(ctx, deviceProperties)
		if err != nil {
			ctxlog.Errorf(ctx, c.logger, "failed to push history data to solomon: %v", err)
		} else {
			ctxlog.Debugf(ctx, c.logger, "pushed device metrics to solomon")
		}
		return nil
	})

	group.Go(func() error {
		return c.db.StoreDeviceProperties(ctx, userID, deviceProperties, source)
	})
	return group.Wait()
}
