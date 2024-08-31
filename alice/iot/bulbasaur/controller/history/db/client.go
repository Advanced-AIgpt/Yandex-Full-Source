package db

import (
	"context"

	"a.yandex-team.ru/kikimr/public/sdk/go/ydb"
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/xerrors"

	"a.yandex-team.ru/alice/library/go/ydbclient"
)

type Client struct {
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
	return &Client{YDBClient: ydbClient}
}
