package session

import (
	"errors"
)

var ErrSessionNotFound = errors.New("session not found")

type Storage interface {
	Load(chatID int64) (*Session, error)
	Save(session *Session) error
	Count() (int64, error)
}
