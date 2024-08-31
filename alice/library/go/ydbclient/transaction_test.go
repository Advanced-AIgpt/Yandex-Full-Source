package ydbclient

import (
	"context"
	"fmt"
	"path"
	"sync"

	"a.yandex-team.ru/kikimr/public/sdk/go/ydb"
	"a.yandex-team.ru/kikimr/public/sdk/go/ydb/table"
)

func (s *DBClientSuite) TestCallInTransaction() {
	s.Run("Transaction_OK", func() {
		err := s.ydbClient.RetryTransaction(s.context, SerializableReadWrite, func(ctx context.Context, session *table.Session, tc *table.TransactionControl) (tx *table.Transaction, err error) {
			tx, _, err = session.Execute(ctx, tc, "SELECT 1", nil)
			return tx, err
		})
		s.NoError(err)
	})

	s.Run("Transaction_Error", func() {
		err := s.ydbClient.RetryTransaction(s.context, SerializableReadWrite, func(ctx context.Context, session *table.Session, tc *table.TransactionControl) (tx *table.Transaction, err error) {
			tx, _, err = session.Execute(ctx, tc, "SELECT asd_unknown_field", nil)
			return tx, err
		})
		s.Error(err)
	})

	s.Run("Transaction_Invalidate_OK", func() {
		// test make lock invalidations for first try of transaction
		// and no problem with second try
		// then check about second transaction ok and was exactly two tries

		tablePath := path.Join(s.prefix, "invalidation_test_table")
		err := table.Retry(s.context, s.ydbClient.SessionPool, table.OperationFunc(func(ctx context.Context, session *table.Session) error {
			return session.CreateTable(ctx, tablePath,
				table.WithColumn("name", ydb.Optional(ydb.TypeString)),
				table.WithColumn("val", ydb.Optional(ydb.TypeString)),
				table.WithPrimaryKeyColumn("name"),
			)
		}))
		s.NoError(err)

		defer func() {
			err = table.Retry(s.context, s.ydbClient.SessionPool, table.OperationFunc(func(ctx context.Context, session *table.Session) error {
				return session.DropTable(ctx, tablePath)
			}))
			s.NoError(err)
		}()

		calledTimes := 0

		var writeInMiddleStart sync.Mutex
		writeInMiddleStart.Lock()

		var writeInMiddleFinished sync.Mutex
		writeInMiddleFinished.Lock()

		go func() {
			writeInMiddleStart.Lock()            // wait to start
			defer writeInMiddleFinished.Unlock() // signal about finished

			err := s.ydbClient.RetryTransaction(s.context, SerializableReadWrite, func(ctx context.Context, session *table.Session, tc *table.TransactionControl) (tx *table.Transaction, err error) {
				s.T().Log("Middle upsert started")
				query := fmt.Sprintf("PRAGMA TablePathPrefix(\"%s\"); UPSERT INTO invalidation_test_table (name, val) VALUES ('1', 'test')", s.prefix)
				tx, _, err = session.Execute(ctx, tc, query, nil)
				s.NoError(err)
				return tx, err
			})
			s.NoError(err)
			s.T().Log("Middle upsert finished")
		}()

		err = s.ydbClient.RetryTransaction(s.context, SerializableReadWrite, func(ctx context.Context, session *table.Session, tc *table.TransactionControl) (tx *table.Transaction, err error) {
			calledTimes++
			s.T().Logf("Start select, %v", calledTimes)

			query := fmt.Sprintf("PRAGMA TablePathPrefix(\"%s\"); SELECT * FROM invalidation_test_table", s.prefix)
			tx, res, err := session.Execute(ctx, tc, query, nil)
			s.NoError(err)
			for res.NextRow() {
				// pass
			}
			_ = res.Close()
			if calledTimes == 1 {
				writeInMiddleStart.Unlock()

				writeInMiddleFinished.Lock() // wait
			}
			s.T().Log("Start upsert")
			query = fmt.Sprintf("PRAGMA TablePathPrefix(\"%s\"); UPSERT INTO invalidation_test_table (name, val) VALUES ('1', 'test')", s.prefix)
			_, err = tx.Execute(ctx, query, nil)
			s.NoError(err) // error will on commit transaction. Then transaction must be retried.
			s.NotNil(tx)
			return tx, err
		})
		s.NoError(err)
		s.Equal(2, calledTimes)
	})

	s.Run("Transaction_Context_Cancelled", func() {
		var testCtx context.Context
		err := s.ydbClient.RetryTransaction(s.context, SerializableReadWrite, func(ctx context.Context, session *table.Session, tc *table.TransactionControl) (tx *table.Transaction, err error) {
			testCtx = ctx
			return nil, nil
		})
		s.NoError(err)
		s.NotNil(testCtx)
		s.Error(testCtx.Err())
	})
}

func (s *DBClientSuite) TestContextWithoutTransaction() {
	ctx := context.Background()

	s.Nil(s.ydbClient.getTransactionContext(ctx))
	s.Nil(s.ydbClient.getTransactionContext(s.ydbClient.ContextWithoutTransaction(ctx)))

	trCtx := s.ydbClient.ContextWithTransaction(ctx, &txContext{})
	s.NotNil(s.ydbClient.getTransactionContext(trCtx))
	s.Nil(s.ydbClient.getTransactionContext(s.ydbClient.ContextWithoutTransaction(trCtx)))
}
