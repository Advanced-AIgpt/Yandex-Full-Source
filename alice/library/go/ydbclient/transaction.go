package ydbclient

import (
	"context"

	"a.yandex-team.ru/kikimr/public/sdk/go/ydb/table"
	"a.yandex-team.ru/library/go/core/log/ctxlog"
	"a.yandex-team.ru/library/go/core/xerrors"
)

type TransactionType int

const (
	SerializableReadWrite TransactionType = iota
	OnlineReadOnly
	StaleReadOnly
)

type txTypeKey struct{}

type txCtxKey struct {
	YDBPrefix string
}

type txContext struct {
	tc *table.TransactionControl
	s  *table.Session
	tx *table.Transaction
}

type TransactionFunc func(ctx context.Context, s *table.Session, txControl *table.TransactionControl) (*table.Transaction, error)

func (ydbClient *YDBClient) ContextWithTransaction(ctx context.Context, tx *txContext) context.Context {
	return context.WithValue(ctx, txCtxKey{YDBPrefix: ydbClient.Prefix}, tx)
}

func (ydbClient *YDBClient) HasTransaction(ctx context.Context) bool {
	return ydbClient.getTransactionContext(ctx) != nil
}

func (ydbClient *YDBClient) ContextWithoutTransaction(ctx context.Context) context.Context {
	if ydbClient.getTransactionContext(ctx) == nil {
		return ctx
	}
	return context.WithValue(ctx, txCtxKey{YDBPrefix: ydbClient.Prefix}, nil)
}

func (ydbClient *YDBClient) ContextWithTransactionType(ctx context.Context, transactionType TransactionType) context.Context {
	return context.WithValue(ctx, txTypeKey{}, transactionType)
}

func (ydbClient *YDBClient) getTransactionContext(ctx context.Context) *txContext {
	val, ok := ctx.Value(txCtxKey{YDBPrefix: ydbClient.Prefix}).(*txContext)
	if ok {
		return val
	}
	return nil
}

func (ydbClient *YDBClient) CallInTx(ctx context.Context, defaultTransactionType TransactionType, f TransactionFunc) (err error) {
	transactionType, ok := ctx.Value(txTypeKey{}).(TransactionType)
	if !ok {
		transactionType = defaultTransactionType
	}

	if txCtx := ydbClient.getTransactionContext(ctx); txCtx == nil {
		return ydbClient.RetryTransaction(ctx, transactionType, f)
	} else {
		var txControl *table.TransactionControl
		if txCtx.tx == nil {
			txControl = txCtx.tc
		} else {
			txControl = table.TxControl(table.WithTx(txCtx.tx))
		}
		var fTx *table.Transaction
		fTx, err = f(ctx, txCtx.s, txControl)
		if fTx != nil {
			txCtx.tx = fTx
		}
		return err
	}
}

func (ydbClient *YDBClient) RetryTransaction(ctx context.Context, transactionType TransactionType, f TransactionFunc) error {
	if ydbClient.getTransactionContext(ctx) != nil {
		return xerrors.Errorf("fail to open nested transaction")
	}

	return ydbClient.Retry(ctx, func(ctx context.Context, s *table.Session) (err error) {
		ctx, cancel := context.WithCancel(ctx)
		defer cancel()

		var txOptions []table.TxControlOption
		var explicitCommit bool
		switch transactionType {
		case SerializableReadWrite:
			txOptions = []table.TxControlOption{table.BeginTx(table.WithSerializableReadWrite())}
			explicitCommit = true
		case OnlineReadOnly:
			txOptions = []table.TxControlOption{table.BeginTx(table.WithOnlineReadOnly()), table.CommitTx()}
			explicitCommit = false
		case StaleReadOnly:
			txOptions = []table.TxControlOption{table.BeginTx(table.WithStaleReadOnly()), table.CommitTx()}
			explicitCommit = false
		default:
			return xerrors.Errorf("unknown transaction type: %v", transactionType)
		}

		tc := table.TxControl(txOptions...)
		var tx *table.Transaction

		defer func() {
			if tx != nil && explicitCommit {
				err = ydbClient.CloseTransaction(ctx, tx, err)
			}
		}()

		txCtx := &txContext{tc, s, nil}
		ctx = ydbClient.ContextWithTransaction(ctx, txCtx)
		tx, err = f(ctx, s, tc)
		if tx == nil {
			tx = txCtx.tx
		}
		return err
	})
}

func (ydbClient *YDBClient) CloseTransaction(ctx context.Context, tx *table.Transaction, err error) error {
	if err != nil {
		ctxlog.Warnf(ctx, ydbClient.Logger, "rolling back transaction due to err: %v", err)
		if e := tx.Rollback(ctx); e != nil {
			ctxlog.Warnf(ctx, ydbClient.Logger, "failed to roll back transaction due to err: %v", e)
		}
	} else {
		if _, e := tx.CommitTx(ctx); e != nil {
			ctxlog.Warnf(ctx, ydbClient.Logger, "failed to commit transaction due to err: %v", e)
			return e
		}
	}

	return err
}
