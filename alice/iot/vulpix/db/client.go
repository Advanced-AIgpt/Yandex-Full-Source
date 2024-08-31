package db

import (
	"context"

	"a.yandex-team.ru/alice/library/go/ydbclient"
	"a.yandex-team.ru/kikimr/public/sdk/go/ydb"
	"a.yandex-team.ru/kikimr/public/sdk/go/ydb/table"
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/log/ctxlog"
	"a.yandex-team.ru/library/go/core/xerrors"
)

type Client struct {
	Prefix string
	*ydbclient.YDBClient
}

func NewClient(ctx context.Context, logger log.Logger, endpoint, prefix string, credentials ydb.Credentials, trace bool, options ...ydbclient.Options) (*Client, error) {
	ydbClient, err := ydbclient.NewYDBClient(ctx, logger, endpoint, prefix, credentials, trace, options...)
	if err != nil {
		return nil, xerrors.Errorf("unable to create ydbClient: %w", err)
	}
	return NewClientWithYDBClient(ydbClient), nil
}

func NewClientWithYDBClient(ydbClient *ydbclient.YDBClient) *Client {
	return &Client{
		Prefix:    ydbClient.Prefix,
		YDBClient: ydbClient,
	}
}

func (db *Client) closeTransaction(ctx context.Context, tx *table.Transaction, err error) error {
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
