package settings

import (
	"context"

	"a.yandex-team.ru/library/go/core/xerrors"
)

type ctxKey int

const (
	settingsContextKey ctxKey = 0
)

func ContextWithSettings(ctx context.Context, settings UserSettings) context.Context {
	return context.WithValue(ctx, settingsContextKey, settings)
}

func SettingsFromContext(ctx context.Context) (UserSettings, error) {
	if ctxSettings, ok := ctx.Value(settingsContextKey).(UserSettings); ok {
		return ctxSettings, nil
	}
	return UserSettings{}, xerrors.New("context does not contain UserSettings")
}
