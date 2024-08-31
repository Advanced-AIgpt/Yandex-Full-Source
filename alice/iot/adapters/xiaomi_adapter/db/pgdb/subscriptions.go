package pgdb

import (
	"context"

	xmodel "a.yandex-team.ru/alice/iot/adapters/xiaomi_adapter/xiaomi/model"
	"a.yandex-team.ru/alice/library/go/queue"
	"a.yandex-team.ru/alice/library/go/timestamp"
	"a.yandex-team.ru/library/go/core/xerrors"
)

func (c *Client) StoreUserSubscriptions(ctx context.Context, externalUserID string) error {
	timestamper, err := timestamp.TimestamperFromContext(ctx)
	if err != nil {
		return xerrors.Errorf("unable to get timestamper: %w", err)
	}
	currentTime := timestamper.CurrentTimestamp().AsTime()
	task, err := queue.NewTask(xmodel.UserEventSubscriptionTaskName, externalUserID, struct{}{}, currentTime)
	if err != nil {
		return xerrors.Errorf("unable to create task: %w", err)
	}
	task.SetMergeKey(externalUserID)
	return c.queue.SubmitTasks(ctx, []queue.Task{task})
}

func (c *Client) StoreDeviceSubscriptions(ctx context.Context, externalUserID string, device xmodel.Device, propertyIDs, eventIDs []string) error {
	tasks := make([]queue.Task, 0, len(propertyIDs)+len(eventIDs)+1)
	timestamper, err := timestamp.TimestamperFromContext(ctx)
	if err != nil {
		return xerrors.Errorf("unable to get timestamper: %w", err)
	}
	currentTime := timestamper.CurrentTimestamp().AsTime()

	// add device status subscription task
	dssTaskPayload := xmodel.DeviceStatusSubscriptionTaskPayload{
		DeviceID: device.GetDeviceID(), // important to use method here
	}
	dssTask, err := queue.NewTask(xmodel.DeviceStatusSubscriptionTaskName, externalUserID, dssTaskPayload, currentTime)
	if err != nil {
		return xerrors.Errorf("unable to create device status subscription task: %w", err)
	}
	dssTask.SetMergeKey(device.GetDeviceID())
	tasks = append(tasks, dssTask)

	// add property subscription task
	for _, propertyID := range propertyIDs {
		psTaskPayload := xmodel.PropertySubscriptionTaskPayload{
			DeviceID:   device.DID,
			PropertyID: propertyID,
			IsSplit:    device.IsSplit,
			XiaomiType: device.Type,
			Region:     device.Region,
		}
		psTask, err := queue.NewTask(xmodel.PropertySubscriptionTaskName, externalUserID, psTaskPayload, currentTime)
		if err != nil {
			return xerrors.Errorf("unable to create property subscription task: %w", err)
		}
		psTask.SetMergeKey(propertyID)
		tasks = append(tasks, psTask)
	}

	// add event subscription task
	for _, eventID := range eventIDs {
		esTaskPayload := xmodel.EventSubscriptionTaskPayload{
			DeviceID:   device.DID,
			EventID:    eventID,
			IsSplit:    device.IsSplit,
			XiaomiType: device.Type,
			Region:     device.Region,
		}
		esTask, err := queue.NewTask(xmodel.EventSubscriptionTaskName, externalUserID, esTaskPayload, currentTime)
		if err != nil {
			return xerrors.Errorf("unable to create event subscription task: %w", err)
		}
		esTask.SetMergeKey(eventID)
		tasks = append(tasks, esTask)
	}
	return c.queue.SubmitTasks(ctx, tasks)
}
