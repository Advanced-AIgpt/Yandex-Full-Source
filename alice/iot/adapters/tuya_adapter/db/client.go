package db

import (
	"a.yandex-team.ru/alice/library/go/timestamp"
	"a.yandex-team.ru/kikimr/public/sdk/go/ydb"
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/log/ctxlog"
	"a.yandex-team.ru/library/go/core/xerrors"

	"context"

	"a.yandex-team.ru/alice/library/go/ydbclient"
	"a.yandex-team.ru/kikimr/public/sdk/go/ydb/table"
)

type DBClient struct {
	Prefix      string
	timestamper timestamp.ITimestamper
	*ydbclient.YDBClient
}

func NewClient(ctx context.Context, logger log.Logger, endpoint, prefix string, credentials ydb.Credentials, trace bool, options ...ydbclient.Options) (*DBClient, error) {
	ydbClient, err := ydbclient.NewYDBClient(ctx, logger, endpoint, prefix, credentials, trace, options...)
	if err != nil {
		return nil, xerrors.Errorf("unable to create ydbClient: %w", err)
	}
	return NewClientWithYDBClient(ydbClient), nil
}

func NewClientWithYDBClient(ydbClient *ydbclient.YDBClient) *DBClient {
	return &DBClient{
		Prefix:      ydbClient.Prefix,
		timestamper: timestamp.NewTimestamper(),
		YDBClient:   ydbClient,
	}
}

func (db *DBClient) closeTransaction(ctx context.Context, tx *table.Transaction, err error) error {
	if err != nil {
		ctxlog.Warnf(ctx, db.Logger, "rolling back transaction due to err: %v", err)
		if e := tx.Rollback(ctx); e != nil {
			ctxlog.Warnf(ctx, db.Logger, "failed to roll back transaction due to err: %v", e)
		}
	} else {
		if _, e := tx.CommitTx(ctx); e != nil {
			ctxlog.Warnf(ctx, db.Logger, "failed to commit transaction due to err: %v", e)
			return e
		}
	}

	return err
}

func (db *DBClient) CurrentTimestamp() timestamp.PastTimestamp {
	return db.timestamper.CurrentTimestamp()
}