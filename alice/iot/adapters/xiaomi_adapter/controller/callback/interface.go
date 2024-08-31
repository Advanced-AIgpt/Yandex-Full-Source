package callback

import (
	"context"

	"a.yandex-team.ru/alice/iot/adapters/xiaomi_adapter/xiaomi/iotapi"
)

type IController interface {
	HandleUserEventCallback(ctx context.Context, userEventCallback iotapi.UserEventCallback) error
	HandlePropertiesChangedCallback(ctx context.Context, propertiesChangeCallback iotapi.PropertiesChangedCallback) error
	HandleEventOccurredCallback(ctx context.Context, eventOccurredCallback iotapi.EventOccurredCallback) error
}
