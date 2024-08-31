package pgclient

import (
	"context"
	"database/sql"
	"errors"
	"fmt"
	"time"

	"github.com/jackc/pgx/v4"
	"github.com/jackc/pgx/v4/stdlib"
	"golang.yandex/hasql"
	"golang.yandex/hasql/checkers"

	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/log/ctxlog"
	"a.yandex-team.ru/library/go/core/xerrors"
)

type PGClient struct {
	cluster *hasql.Cluster
}

var (
	ErrMasterIsUnavailable = errors.New("postgres: master is unavailable")
	ErrNodeIsUnavailable   = errors.New("postgres: node is unavailable")
)

func NewPGClient(hosts []string, port int, dbName, user, password string, logger log.Logger, poolOptions ...Option) (*PGClient, error) {
	var nodes []hasql.Node
	for _, host := range hosts {
		connString := fmt.Sprintf("host=%s port=%d user=%s password=%s dbname=%s sslmode=verify-full", host, port, user, password, dbName)
		connConfig, err := pgx.ParseConfig(connString)
		if err != nil {
			return nil, err
		}

		// workaround for https://github.com/jackc/pgx/issues/602
		connConfig.BuildStatementCache = nil
		connConfig.PreferSimpleProtocol = true

		db := stdlib.OpenDB(*connConfig)
		for _, opt := range append(defaultOptions, poolOptions...) {
			opt(db)
		}

		nodes = append(nodes, hasql.NewNode(host, db))
	}
	opts := []hasql.ClusterOption{
		hasql.WithUpdateInterval(2 * time.Second),
		hasql.WithNodePicker(hasql.PickNodeRoundRobin()),
		hasql.WithTracer(newHASQLTracer(logger)),
	}
	c, err := hasql.NewCluster(nodes, checkers.PostgreSQL, opts...)
	if err != nil {
		return nil, err
	}

	ctx, cancel := context.WithTimeout(context.Background(), 30*time.Second)
	defer cancel()
	if _, err := c.WaitForPrimary(ctx); err != nil {
		return nil, err
	}

	return &PGClient{c}, nil
}

func newHASQLTracer(logger log.Logger) hasql.Tracer {
	return hasql.Tracer{
		UpdateNodes: func() {
			ctxlog.Info(context.Background(), logger, "started updating nodes")
		},
		UpdatedNodes: func(nodes hasql.AliveNodes) {
			ctxlog.Infof(context.Background(), logger, "finished updating nodes: %+v\n", nodes)
		},
		NodeDead: func(node hasql.Node, err error) {
			ctxlog.Infof(context.Background(), logger, "node %q is dead: %v", node, err)
		},
		NodeAlive: func(node hasql.Node) {
			ctxlog.Infof(context.Background(), logger, "node %q is alive", node)
		},
		NotifiedWaiters: func() {
			ctxlog.Infof(context.Background(), logger, "notified all waiters")
		},
	}
}

func (c *PGClient) GetMaster() (*sql.DB, error) {
	if node := c.cluster.Primary(); node != nil {
		return node.DB(), nil
	}
	return nil, ErrMasterIsUnavailable
}

// GetSecondaryPreferred returns secondary if possible and master otherwise
func (c *PGClient) GetSecondaryPreferred() (*sql.DB, error) {
	if node := c.cluster.StandbyPreferred(); node != nil {
		return node.DB(), nil
	}
	return nil, ErrNodeIsUnavailable
}

func (c *PGClient) ExecuteInTransaction(ctx context.Context, node hasql.NodeStateCriteria, processFunc func(context.Context, *sql.Tx) error) error {
	if node := c.cluster.Node(node); node != nil {
		db := node.DB()

		tx, err := db.BeginTx(ctx, nil)
		if err != nil {
			return err
		}

		err = processFunc(ctx, tx)

		if err != nil {
			rollbackErr := tx.Rollback()
			if rollbackErr != nil {
				return xerrors.Errorf("rollback fail: %v; initial error: %v", rollbackErr, err)
			}
			return err
		}
		if err := tx.Commit(); err != nil {
			return err
		}

		return nil
	}

	return ErrNodeIsUnavailable
}
