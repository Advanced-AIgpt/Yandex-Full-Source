package storage

import (
	"golang.org/x/xerrors"

	"a.yandex-team.ru/alice/gamma/server/log"
)

var (
	AlreadyCommittedError   = xerrors.New("already committed")
	InvalidTransactionError = xerrors.New("invalid transaction")
)

type Key struct {
	SkillID string
	UserID  string
}

type Item []byte

type TransactionID struct {
	SkillID   string
	UserID    string
	SessionID string
	MessageID int64
}

type Response []byte

type Transaction interface {
	GetID() TransactionID
	GetItem() Item
	GetResponse() Response
	SetResponse(Response)
	SetItem(Item)
	IsInvalid() bool
	Invalidate()
}

type basicTransaction struct {
	ID       TransactionID
	Item     Item
	Response Response
	invalid  bool
}

func (t *basicTransaction) GetID() TransactionID {
	return t.ID
}

func (t *basicTransaction) GetItem() Item {
	return t.Item
}

func (t *basicTransaction) SetItem(item Item) {
	t.Item = item
}

func (t *basicTransaction) IsInvalid() bool {
	return t.invalid
}

func (t *basicTransaction) Invalidate() {
	t.invalid = true
}

func (t *basicTransaction) GetResponse() Response {
	return t.Response
}

func (t *basicTransaction) SetResponse(response Response) {
	t.Response = response
}

type Storage interface {
	StartTransaction(ctx log.LoggingContext, skillID, sessionID, userID string, messageID int64) (Transaction, error)
	CommitTransaction(ctx log.LoggingContext, transaction Transaction) error
	RollbackTransaction(ctx log.LoggingContext, transaction Transaction) error
}

type Factory interface {
	CreateStorage() (Storage, error)
}
