package storage

import "a.yandex-team.ru/alice/gamma/server/log"

func CreateStorage(factory Factory) (*ProxyStorage, error) {
	storage, err := factory.CreateStorage()
	if err != nil {
		return nil, err
	}
	return &ProxyStorage{storage: storage}, nil
}

type ProxyStorage struct {
	storage Storage
}

type ProxyStorageTransaction struct {
	basicTransaction
}

func (storage *ProxyStorage) StartTransaction(ctx log.LoggingContext, skillID, sessionID, userID string,
	messageID int64, state []byte) (Transaction, error) {
	if state == nil {
		return storage.storage.StartTransaction(ctx, skillID, sessionID, userID, messageID)
	}

	return &ProxyStorageTransaction{basicTransaction: basicTransaction{Item: state}}, nil
}

func (storage *ProxyStorage) CommitTransaction(ctx log.LoggingContext, transaction Transaction) (err error) {
	switch transaction.(type) {
	case *YdbStorageTransaction:
		return storage.storage.CommitTransaction(ctx, transaction)
	}

	return nil
}

func (storage *ProxyStorage) RollbackTransaction(ctx log.LoggingContext, transaction Transaction) error {
	switch transaction.(type) {
	case *YdbStorageTransaction:
		return storage.storage.RollbackTransaction(ctx, transaction)
	}

	return nil
}
