package pgdb

import (
	"context"
	"database/sql"

	"a.yandex-team.ru/alice/library/go/pgclient"
	"a.yandex-team.ru/alice/library/go/queue"
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/log/ctxlog"
	"github.com/gofrs/uuid"
)

type Client struct {
	logger        log.Logger
	uuidGenerator uuid.Generator
	queue         *queue.Queue
	pgClient      *pgclient.PGClient
}

func NewClient(logger log.Logger, pgClient *pgclient.PGClient, queue *queue.Queue, generator uuid.Generator) *Client {
	return &Client{
		logger:        logger,
		uuidGenerator: generator,
		pgClient:      pgClient,
		queue:         queue,
	}
}

func (c *Client) closeTransaction(ctx context.Context, tx *sql.Tx, err error) error {
	if err != nil {
		ctxlog.Warnf(ctx, c.logger, "rolling back transaction due to err: %v", err)
		if e := tx.Rollback(); e != nil {
			ctxlog.Warnf(ctx, c.logger, "failed to roll back transaction due to err: %v", e)
		}
	} else {
		if e := tx.Commit(); e != nil {
			ctxlog.Warnf(ctx, c.logger, "failed to commit transaction due to err: %v", e)
			return e
		}
	}

	return err
}
