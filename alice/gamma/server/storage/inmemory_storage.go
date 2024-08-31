package storage

import (
	"sync"

	"a.yandex-team.ru/alice/gamma/server/log"
)

type InMemoryStorage struct {
	storage map[Key]Item

	transactions map[TransactionID]Response
	storageMutex *sync.Mutex
}

func (storage *InMemoryStorage) StartTransaction(ctx log.LoggingContext, skillID, sessionID, userID string, messageID int64) (Transaction, error) {
	tx := basicTransaction{
		ID: TransactionID{
			SkillID:   skillID,
			SessionID: sessionID,
			UserID:    userID,
			MessageID: messageID,
		},
	}

	storage.storageMutex.Lock()
	defer storage.storageMutex.Unlock()

	if response, ok := storage.transactions[tx.ID]; ok {
		tx.Response = response
		return &tx, AlreadyCommittedError
	}

	var ok bool

	tx.Item, ok = storage.storage[Key{SkillID: skillID, UserID: userID}]
	if !ok {
		tx.Item = Item{}
	}

	return &tx, nil
}

func (storage *InMemoryStorage) CommitTransaction(ctx log.LoggingContext, transaction Transaction) error {
	if transaction.IsInvalid() {
		return InvalidTransactionError
	}
	id := transaction.GetID()

	storage.storageMutex.Lock()
	defer storage.storageMutex.Unlock()

	if _, ok := storage.transactions[id]; ok {
		return nil
	}

	storage.storage[Key{SkillID: id.SkillID, UserID: id.UserID}] = transaction.GetItem()
	storage.transactions[id] = transaction.GetResponse()
	transaction.Invalidate()

	return nil
}

func (storage *InMemoryStorage) RollbackTransaction(ctx log.LoggingContext, transaction Transaction) error {
	if transaction.IsInvalid() {
		return InvalidTransactionError
	}
	transaction.Invalidate()
	return nil
}

type InMemoryStorageFactory struct{}

func (*InMemoryStorageFactory) CreateStorage() (Storage, error) {
	return &InMemoryStorage{
		make(map[Key]Item),
		make(map[TransactionID]Response),
		&sync.Mutex{},
	}, nil
}
