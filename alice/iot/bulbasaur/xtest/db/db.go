package xtestdb

import (
	"context"
	"testing"

	"a.yandex-team.ru/alice/iot/bulbasaur/db"
	xtestlogs "a.yandex-team.ru/alice/iot/bulbasaur/xtest/logs"
)

type DB struct {
	ctx    context.Context
	t      *testing.T
	logger *xtestlogs.Logger
	client *db.DBClient
}

func NewDB(ctx context.Context, t *testing.T, logger *xtestlogs.Logger, client *db.DBClient) *DB {
	return &DB{
		ctx,
		t,
		logger,
		client,
	}
}

func (f *DB) DBClient() *db.DBClient {
	return f.client
}
