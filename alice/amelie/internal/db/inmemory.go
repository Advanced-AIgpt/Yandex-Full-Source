package db

import (
	"context"

	"a.yandex-team.ru/alice/amelie/internal/model"
)

type InMemoryDB struct {
	coll map[string]model.Session
}

func NewInMemoryDB() *InMemoryDB {
	return &InMemoryDB{
		coll: map[string]model.Session{},
	}
}

func (db *InMemoryDB) Load(_ context.Context, sessionID string) (model.Session, error) {
	if session, ok := db.coll[sessionID]; ok {
		return session, nil
	}
	return model.Session{}, &model.SessionNotFoundError{}
}

func (db *InMemoryDB) Save(_ context.Context, session model.Session) error {
	db.coll[session.ID] = session
	return nil
}

func (db *InMemoryDB) Count(_ context.Context) (int64, error) {
	return int64(len(db.coll)), nil
}
