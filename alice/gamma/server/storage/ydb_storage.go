package storage

import (
	"context"
	"path"
	"time"

	"golang.org/x/xerrors"

	ydbDriver "a.yandex-team.ru/kikimr/public/sdk/go/ydb"

	"a.yandex-team.ru/alice/gamma/server/log"
	"a.yandex-team.ru/alice/gamma/server/ydb"
)

var WrongTxTypeError = xerrors.New("wrong transaction type")

type YdbStorage struct {
	TablePathPrefix string
	RequestTimeout  time.Duration

	adapter *ydb.Adapter
}

type YdbStorageTransaction struct {
	basicTransaction
	dbSession *ydb.Session
	cfg       ydb.TemplateConfig
}

func (storage *YdbStorage) init(ctx context.Context, dialTimeout time.Duration, endpoint string) error {
	return storage.adapter.Dial(ctx, dialTimeout, endpoint)
}

func (storage *YdbStorage) Finalize() error {
	if err := storage.adapter.Close(); err != nil {
		return xerrors.Errorf("ydb error: %w", err)
	}
	return nil
}

func (storage *YdbStorage) getTableConfig(skillID string) ydb.TemplateConfig {
	return ydb.TemplateConfig{
		TablePathPrefix:       path.Join(storage.TablePathPrefix, skillID),
		TablePath:             "storage",
		TransactionsTablePath: "transactions",
	}
}

func fromStorageItem(item *ydb.Item) (Item, error) {
	return Item(item.Value), nil
}

func toStorageItem(id TransactionID, item Item, response Response) (ydb.Item, error) {
	return ydb.Item{
		Key: ydb.Key{
			UserID:    id.UserID,
			SessionID: id.SessionID,
			MessageID: id.MessageID,
		},
		Value:          item,
		StoredResponse: response,
	}, nil
}

func (storage *YdbStorage) StartTransaction(ctx log.LoggingContext, skillID, sessionID, userID string, messageID int64) (tx Transaction, err error) {
	var transaction YdbStorageTransaction
	transaction.ID = TransactionID{
		SkillID:   skillID,
		UserID:    userID,
		SessionID: sessionID,
		MessageID: messageID,
	}
	transaction.dbSession, err = storage.adapter.CreateSession(ctx)

	if err != nil {
		return nil, xerrors.Errorf("ydb error: %w", err)
	}

	defer func() {
		if err != nil {
			transaction.Invalidate()
			if err := storage.adapter.CloseSession(context.TODO(), transaction.dbSession); err != nil {
				ctx.Logger.Errorf("Failed to close transaction: %+v", err)
			}
		}
	}()

	transaction.cfg = storage.getTableConfig(skillID)

	readTime := time.Now()

	item, err := transaction.dbSession.Read(
		transaction.cfg,
		&ydb.Key{
			SessionID: sessionID,
			MessageID: messageID,
			UserID:    userID,
		},
		ctx,
	)

	ctx.Logger.Infof("Reading from ydb took %s", time.Since(readTime))

	if err != nil {
		if xerrors.Is(err, ydb.AlreadyCommittedError) {
			transaction.Response = item.StoredResponse
			return &transaction, AlreadyCommittedError
		}
		return nil, xerrors.Errorf("ydb error: %w", err)
	}

	if item != nil && len(item.Value) > 0 {
		transaction.Item, err = fromStorageItem(item)
	} else {
		transaction.Item = Item{}
	}

	if err != nil {
		return nil, err
	}

	return &transaction, nil
}

func (storage *YdbStorage) CommitTransaction(ctx log.LoggingContext, transaction Transaction) (err error) {
	if transaction.IsInvalid() {
		return InvalidTransactionError
	}
	tx, ok := transaction.(*YdbStorageTransaction)
	if !ok {
		return WrongTxTypeError
	}

	defer func() {
		err_ := storage.adapter.CloseSession(context.TODO(), tx.dbSession)
		if err == nil {
			tx.Invalidate()
			if err_ != nil {
				err = xerrors.Errorf("ydb error: %w", err_)
			}
		}
	}()

	item, err := toStorageItem(tx.ID, tx.Item, tx.Response)

	if err != nil {
		return xerrors.Errorf("ydb error: %w", err)
	}

	writeTime := time.Now()

	err = tx.dbSession.Write(
		tx.cfg,
		&item,
		ctx,
	)

	ctx.Logger.Infof("Writing to ydb took %s", time.Since(writeTime))

	if err != nil {
		if xerrors.Is(err, ydb.AlreadyCommittedError) {
			return AlreadyCommittedError
		}
		return xerrors.Errorf("ydb error: %w", err)
	}
	return nil
}

func (storage *YdbStorage) RollbackTransaction(ctx log.LoggingContext, transaction Transaction) error {
	if transaction.IsInvalid() {
		return InvalidTransactionError
	}
	tx, ok := transaction.(*YdbStorageTransaction)
	if !ok {
		return WrongTxTypeError
	}
	tx.Invalidate()
	if err := storage.adapter.CloseSession(context.TODO(), tx.dbSession); err != nil {
		return xerrors.Errorf("ydb error: %w", err)
	}
	return nil
}

type YdbStorageFactory struct {
	Endpoint        string
	DialTimeout     time.Duration
	TablePathPrefix string
	Context         context.Context
	Config          *ydbDriver.DriverConfig
}

func (factory *YdbStorageFactory) CreateStorage() (Storage, error) {
	storage := YdbStorage{
		TablePathPrefix: factory.TablePathPrefix,
		adapter: &ydb.Adapter{
			Config: factory.Config,
		},
	}

	if err := storage.init(factory.Context, factory.DialTimeout, factory.Endpoint); err != nil {
		return nil, xerrors.Errorf("ydb error: %w", err)
	}
	return &storage, nil
}
