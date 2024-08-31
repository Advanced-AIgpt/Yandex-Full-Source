package db

import (
	"context"

	"a.yandex-team.ru/alice/amelie/internal/model"
)

type DB interface {
	SessionStorage
}

type SessionStorage interface {
	Load(ctx context.Context, sessionID string) (model.Session, error)
	Save(ctx context.Context, session model.Session) error
	Count(ctx context.Context) (int64, error)
}
