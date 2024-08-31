package executors

import (
	"context"
	"math/rand"
	"time"

	"a.yandex-team.ru/alice/iot/adapters/xiaomi_adapter/db"
	"a.yandex-team.ru/alice/iot/adapters/xiaomi_adapter/xiaomi/iotapi"
	xmodel "a.yandex-team.ru/alice/iot/adapters/xiaomi_adapter/xiaomi/model"
	"a.yandex-team.ru/alice/iot/adapters/xiaomi_adapter/xiaomi/token"
	bulbasaur "a.yandex-team.ru/alice/iot/bulbasaur/errors"
	"a.yandex-team.ru/alice/library/go/queue"
	"a.yandex-team.ru/alice/library/go/socialism"
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/log/ctxlog"
	"a.yandex-team.ru/library/go/core/metrics"
	"a.yandex-team.ru/library/go/core/metrics/solomon"
	"a.yandex-team.ru/library/go/core/xerrors"
)

type TaskExecutor struct {
	logger               log.Logger
	tokenReceiver        token.Receiver
	database             db.DB
	apiClients           iotapi.APIClients
	tokenNotFoundCounter metrics.Counter
}

func (t TaskExecutor) getUserAndToken(ctx context.Context, externalUserID string) (*xmodel.ExternalUser, string, error) {
	user, err := t.database.SelectExternalUser(ctx, externalUserID)
	if err != nil {
		return nil, "", err
	}
	if user == nil {
		return nil, "", nil
	}
	xiaomiToken, err := t.tokenReceiver.GetToken(ctx, user.UserIDs)
	if err != nil {
		return nil, "", err
	}
	return user, xiaomiToken, nil
}

func NewTaskExecutor(
	logger log.Logger,
	tokenReceiver token.Receiver,
	database db.DB,
	apiClient iotapi.APIClients,
	registry metrics.Registry,
) TaskExecutor {
	tokenNotFoundCounter := registry.Counter("token_not_found_errors")
	solomon.Rated(tokenNotFoundCounter)

	return TaskExecutor{
		apiClients:           apiClient,
		logger:               logger,
		tokenReceiver:        tokenReceiver,
		database:             database,
		tokenNotFoundCounter: tokenNotFoundCounter,
	}
}

// returns duration in period [1:00:00 - 23:59:59]
func randomResubmitTime() time.Duration {
	hours := time.Duration(rand.Intn(23) + 1) // [1..23]
	minutes := time.Duration(rand.Intn(60))   // [0..59]
	seconds := time.Duration(rand.Intn(60))   // [0..59]
	return (hours+48)*time.Hour + minutes*time.Minute + seconds*time.Second
}

func (t TaskExecutor) ExecutePropertySubscriptionTask(ctx context.Context, externalUserID string, taskPayload xmodel.PropertySubscriptionTaskPayload, extraInfo queue.ExtraInfo) (handleErr error) {
	user, xiaomiToken, err := t.getUserAndToken(ctx, externalUserID)
	if err != nil {
		taskErr := xerrors.Errorf("can't get base executor data: %w", err)
		ctxlog.Warnf(ctx, t.logger, "can't get base executor data: %v", err)
		if extraInfo.RetryLeft == 0 {
			// stop retrying TOKEN_NOT_FOUND errors - 404 from socialism
			if isTokenNotFoundError(err) {
				t.tokenNotFoundCounter.Inc()
				ctxlog.Info(ctx, t.logger,
					"token not found, stop subscription task",
					log.String("external_user_id", externalUserID),
					log.String("task_name", xmodel.PropertySubscriptionTaskName),
				)
				return nil // no retry
			}

			return queue.NewFailAndResubmitTaskError(10*time.Minute, taskErr)
		}
		return taskErr // this will retry
	}
	if user == nil {
		ctxlog.Info(ctx, t.logger,
			"owner of subscription task not found",
			log.String("external_user_id", externalUserID),
			log.String("task_name", xmodel.PropertySubscriptionTaskName),
		)
		return nil // this will not retry
	}
	ctxlog.Infof(ctx, t.logger,
		"executing device property subscription task for user with id '%s' (property id '%s')",
		externalUserID, taskPayload.PropertyID)

	customData := iotapi.PropertiesChangedCustomData{
		SubscriptionKey: user.SubscriptionKey,
		UserID:          externalUserID,
		DeviceID:        taskPayload.DeviceID,
		Type:            taskPayload.XiaomiType,
		Region:          iotapi.Region(taskPayload.Region),
		IsSplit:         taskPayload.IsSplit,
	}

	apiClient := t.apiClients.GetAPIClient(iotapi.Region(taskPayload.Region))

	propertyID := taskPayload.PropertyID
	if err := apiClient.SubscribeToPropertyChanges(ctx, xiaomiToken, propertyID, customData); err != nil {
		var deviceNotFoundError iotapi.DeviceNotFoundError
		if xerrors.As(err, &deviceNotFoundError) {
			ctxlog.Infof(ctx, t.logger, "device %s is not found for property %s", deviceNotFoundError.DeviceID, propertyID)
			return nil // this will not retry
		}

		taskErr := xerrors.Errorf("unable to call subscriptions api: %w", err)
		ctxlog.Warnf(ctx, t.logger, "unable to call subscriptions api: %v (retries left: %d)", err, extraInfo.RetryLeft)

		if extraInfo.RetryLeft == 0 {
			var featureNotOnlineError iotapi.FeatureNotOnlineError
			if xerrors.As(err, &featureNotOnlineError) {
				// temporary solution to stop spawning these tasks
				return queue.NewFailAndResubmitTaskError(24*time.Hour, taskErr)
			}

			return queue.NewFailAndResubmitTaskError(10*time.Minute, taskErr)
		}
		return taskErr // this will retry
	}

	ctxlog.Info(ctx, t.logger, "subscription is successful")
	return queue.NewDoneAndResubmitTaskError(randomResubmitTime()) // this will randomly resubmit in future
}

func (t TaskExecutor) ExecuteDeviceEventSubscriptionTask(ctx context.Context, externalUserID string, taskPayload xmodel.EventSubscriptionTaskPayload, extraInfo queue.ExtraInfo) error {
	user, xiaomiToken, err := t.getUserAndToken(ctx, externalUserID)
	if err != nil {
		taskErr := xerrors.Errorf("can't get base executor data: %w", err)
		ctxlog.Warnf(ctx, t.logger, "can't get base executor data: %v", err)

		if extraInfo.RetryLeft == 0 {
			// stop retrying TOKEN_NOT_FOUND errors - 404 from socialism
			if isTokenNotFoundError(err) {
				t.tokenNotFoundCounter.Inc()
				ctxlog.Info(ctx, t.logger,
					"token not found, stop subscription task",
					log.String("external_user_id", externalUserID),
					log.String("task_name", xmodel.EventSubscriptionTaskName),
				)
				return nil // no retry
			}

			return queue.NewFailAndResubmitTaskError(10*time.Minute, taskErr)
		}
		return taskErr // this will retry
	}
	if user == nil {
		ctxlog.Info(ctx, t.logger, "owner of subscription task not found",
			log.String("external_user_id", externalUserID),
			log.String("task_name", xmodel.EventSubscriptionTaskName),
		)
		return nil // this will not retry
	}
	ctxlog.Infof(ctx, t.logger,
		"executing device events subscription task for user with id '%s' (event id '%s')",
		externalUserID, taskPayload.EventID)

	customData := iotapi.EventOccurredCustomData{
		SubscriptionKey: user.SubscriptionKey,
		UserID:          externalUserID,
		DeviceID:        taskPayload.DeviceID,
		Type:            taskPayload.XiaomiType,
		Region:          iotapi.Region(taskPayload.Region),
		IsSplit:         taskPayload.IsSplit,
	}
	apiClient := t.apiClients.GetAPIClient(iotapi.Region(taskPayload.Region))

	eventID := taskPayload.EventID
	if err := apiClient.SubscribeToDeviceEvents(ctx, xiaomiToken, eventID, customData); err != nil {
		var deviceNotFoundError iotapi.DeviceNotFoundError
		if xerrors.As(err, &deviceNotFoundError) {
			ctxlog.Infof(ctx, t.logger, "device %s is not found for event %s", deviceNotFoundError.DeviceID, eventID)
			return nil // this will not retry
		}

		taskErr := xerrors.Errorf("unable to call subscriptions api: %w", err)
		ctxlog.Warnf(ctx, t.logger, "unable to call subscriptions api: %v (retries left: %d)", err, extraInfo.RetryLeft)

		if extraInfo.RetryLeft == 0 {
			var featureNotOnlineError iotapi.FeatureNotOnlineError
			if xerrors.As(err, &featureNotOnlineError) {
				// temporary solution to stop spawning these tasks
				return queue.NewFailAndResubmitTaskError(24*time.Hour, taskErr)
			}

			return queue.NewFailAndResubmitTaskError(10*time.Minute, taskErr)
		}
		return taskErr // this will retry
	}

	ctxlog.Info(ctx, t.logger, "subscription is successful")
	return queue.NewDoneAndResubmitTaskError(randomResubmitTime()) // this will randomly resubmit in future
}

func (t TaskExecutor) ExecuteUserEventSubscriptionTask(ctx context.Context, externalUserID string, _ struct{}, extraInfo queue.ExtraInfo) error {
	user, xiaomiToken, err := t.getUserAndToken(ctx, externalUserID)
	if err != nil {
		taskErr := xerrors.Errorf("can't get base executor data: %w", err)
		ctxlog.Warnf(ctx, t.logger, "can't get base executor data: %v", err)

		if extraInfo.RetryLeft == 0 {
			// stop retrying TOKEN_NOT_FOUND errors - 404 from socialism
			if isTokenNotFoundError(err) {
				t.tokenNotFoundCounter.Inc()
				ctxlog.Info(ctx, t.logger,
					"token not found, stop subscription task",
					log.String("external_user_id", externalUserID),
					log.String("task_name", xmodel.UserEventSubscriptionTaskName),
				)
				return nil // no retry
			}
			return queue.NewFailAndResubmitTaskError(10*time.Minute, taskErr)
		}
		return taskErr // this will retry
	}
	if user == nil {
		ctxlog.Info(ctx, t.logger, "owner of subscription task not found",
			log.String("external_user_id", externalUserID),
			log.String("task_name", xmodel.UserEventSubscriptionTaskName),
		)
		return nil // this will not retry
	}
	ctxlog.Infof(ctx, t.logger, "executing user events subscription task for user with id '%s'", externalUserID)

	for region, apiClient := range t.apiClients.Clients {
		customData := iotapi.UserEventCustomData{
			SubscriptionKey: user.SubscriptionKey,
			UserID:          externalUserID,
			Region:          region,
		}
		if err := apiClient.SubscribeToUserEvents(ctx, xiaomiToken, customData); err != nil {
			taskErr := xerrors.Errorf("unable to call subscriptions api: %w", err)
			ctxlog.Warnf(ctx, t.logger, "unable to call subscriptions api: %v (retries left: %d)", err, extraInfo.RetryLeft)

			if extraInfo.RetryLeft == 0 {
				return queue.NewFailAndResubmitTaskError(10*time.Minute, taskErr)
			}
			return taskErr // this will retry
		}
	}

	ctxlog.Info(ctx, t.logger, "subscription is successful")
	return queue.NewDoneAndResubmitTaskError(randomResubmitTime()) // this will randomly resubmit in future
}

func isTokenNotFoundError(err error) bool {
	var errorList bulbasaur.Errors
	if xerrors.As(err, &errorList) {
		// check if all errors contain TOKEN_NOT_FOUND error
		return errorList.All(&socialism.TokenNotFoundError{})
	}

	return xerrors.Is(err, &socialism.TokenNotFoundError{})
}
