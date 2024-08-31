package sdk

import (
	"context"

	"a.yandex-team.ru/alice/iot/bulbasaur/model"
)

type Context interface {
	Context() context.Context
	Logger() DoubleLogger

	User() (model.User, bool)
	Origin() (model.Origin, bool)
	ClientDeviceID() string
}
