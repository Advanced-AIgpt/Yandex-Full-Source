package ydbclient

import (
	"context"
	"strings"
	"time"

	"a.yandex-team.ru/kikimr/public/sdk/go/ydb"
	"a.yandex-team.ru/kikimr/public/sdk/go/ydb/table"
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/log/ctxlog"
	"a.yandex-team.ru/library/go/core/xerrors"
)

const TvmID = 2002490

type YDBClient struct {
	Prefix      string
	SessionPool *table.SessionPool
	Driver      *ydb.Driver
	TableClient *table.Client
	Logger      log.Logger

	retryer *table.Retryer
}

type Options struct {
	PreferLocalDC               bool
	BalancingMethod             string
	TableTracer                 *table.ClientTrace
	SessionPoolKeepAliveMinSize int
	SessionPoolSizeLimit        int
}

func NewYDBClient(ctx context.Context, logger log.Logger, endpoint, prefix string, credentials ydb.Credentials, trace bool, options ...Options) (*YDBClient, error) {
	logger.Info("initializing YDB client")

	var opt Options
	switch len(options) {
	case 0: // pass
	case 1:
		opt = options[0]
	default:
		return nil, xerrors.Errorf("failed to create ydb client: too many options received: %v", len(options))
	}

	var balancingMethod ydb.BalancingMethod
	switch strings.ToUpper(opt.BalancingMethod) {
	case "", "P2C":
		balancingMethod = ydb.BalancingP2C
	case "RANDOM_CHOICE":
		balancingMethod = ydb.BalancingRandomChoice
	case "ROUND_ROBIN":
		balancingMethod = ydb.BalancingRoundRobin
	default:
		return nil, xerrors.Errorf("unexpected ydb balancing method: %q", opt.BalancingMethod)
	}

	config := &ydb.DriverConfig{
		Credentials:          credentials,
		Database:             prefix,
		BalancingMethod:      balancingMethod,
		PreferLocalEndpoints: opt.PreferLocalDC,
		RequestTimeout:       time.Second * 5,
	}
	if trace {
		var dtrace ydb.DriverTrace
		Stub(&dtrace, func(name string, args ...interface{}) {
			logger.Infof("[ydb_driver] %s: %+v", name, ClearContext(args))
		})
		config.Trace = dtrace
	}

	driver, err := ydb.Dial(ctx, endpoint, config)
	if err != nil {
		logger.Errorf("cannot initialize YDB driver: %v", err)
		return nil, xerrors.Errorf("cannot initialize YDB driver: %w", err)
	}

	ydbClient := &YDBClient{Logger: logger}
	ydbClient.Prefix = prefix
	ydbClient.Driver = &driver
	ydbClient.TableClient = &table.Client{Driver: driver}
	ydbClient.SessionPool = &table.SessionPool{
		KeepAliveMinSize: 200,
		SizeLimit:        400,
		Builder:          ydbClient.TableClient,
	}
	ydbClient.retryer = &table.Retryer{
		SessionProvider: ydbClient.SessionPool,
		MaxRetries:      ydb.DefaultMaxRetries,
		RetryChecker:    ydb.DefaultRetryChecker,
		FastBackoff:     ydb.DefaultFastBackoff,
		SlowBackoff:     ydb.DefaultSlowBackoff,
	}

	if opt.SessionPoolKeepAliveMinSize != 0 {
		ydbClient.SessionPool.KeepAliveMinSize = opt.SessionPoolKeepAliveMinSize
	}
	if opt.SessionPoolSizeLimit != 0 {
		ydbClient.SessionPool.SizeLimit = opt.SessionPoolSizeLimit
	}
	if opt.TableTracer != nil {
		ydbClient.TableClient.Trace = *opt.TableTracer
	}

	logger.Info("YDB client was successfully initialized")
	return ydbClient, nil
}

func (ydbClient *YDBClient) Read(ctx context.Context, query string, params *table.QueryParameters) (*table.Result, error) {
	var res *table.Result
	readFunc := func(ctx context.Context, s *table.Session, tc *table.TransactionControl) (tx *table.Transaction, err error) {
		stmt, err := s.Prepare(ctx, query)
		if err != nil {
			return nil, err
		}

		tx, res, err = stmt.Execute(ctx, tc, params)
		return tx, err
	}

	err := ydbClient.CallInTx(ctx, OnlineReadOnly, readFunc)
	if err != nil {
		return nil, xerrors.Errorf("database read request has failed: %w", err)
	}

	if err := res.Err(); err != nil {
		return nil, err
	}

	return res, nil
}

func (ydbClient *YDBClient) Write(ctx context.Context, query string, params *table.QueryParameters) error {
	writeFunc := func(ctx context.Context, s *table.Session, tc *table.TransactionControl) (tx *table.Transaction, err error) {
		stmt, err := s.Prepare(ctx, query)
		if err != nil {
			return nil, err
		}

		tx, _, err = stmt.Execute(ctx, tc, params)
		return tx, err
	}
	err := ydbClient.CallInTx(ctx, SerializableReadWrite, writeFunc)
	if err != nil {
		return xerrors.Errorf("database write request has failed: %w", err)
	}

	return nil
}

type operation struct {
	retryCount    int
	maxRetryCount int

	logger log.Logger
	do     func(ctx context.Context, s *table.Session) (err error)
}

func (op *operation) Do(ctx context.Context, s *table.Session) error {
	if op.retryCount > 0 {
		ctxlog.Infof(ctx, op.logger, "Retrying ydb operation, %d/%d", op.retryCount, op.maxRetryCount)
	}
	err := op.do(ctx, s)
	op.retryCount++
	return err
}

func (ydbClient *YDBClient) operation(do func(ctx context.Context, s *table.Session) (err error)) *operation {
	return &operation{logger: ydbClient.Logger, do: do, maxRetryCount: ydbClient.retryer.MaxRetries}
}

func (ydbClient *YDBClient) Retry(ctx context.Context, do func(ctx context.Context, s *table.Session) (err error)) error {
	return ydbClient.retryer.Do(ctx, ydbClient.operation(do))
}

func (ydbClient *YDBClient) RollbackTransaction(ctx context.Context, tx *table.Transaction, err error) {
	if tx != nil {
		ctxlog.Warnf(ctx, ydbClient.Logger, "rolling back transaction due to err: %v", err)
		if e := tx.Rollback(ctx); e != nil {
			ctxlog.Warnf(ctx, ydbClient.Logger, "failed to roll back transaction due to err: %v", e)
		}
		// no error returned because nothing could be done
	}
}

func (ydbClient *YDBClient) IsDatabaseError(err error) bool {
	trErr := &ydb.TransportError{}
	opErr := &ydb.OpError{}
	return xerrors.As(err, &trErr) || xerrors.As(err, &opErr)
}

func (ydbClient *YDBClient) Transaction(ctx context.Context, _ string, f func(ctx context.Context) error) error {
	return ydbClient.RetryTransaction(ctx, SerializableReadWrite, func(ctx context.Context, session *table.Session, tc *table.TransactionControl) (tx *table.Transaction, err error) {
		return nil, f(ctx)
	})
}

func (ydbClient *YDBClient) WithoutTransaction(ctx context.Context) context.Context {
	return ydbClient.ContextWithoutTransaction(ctx)
}
