package model

import "fmt"

const (
	NotFound         = "NOT_FOUND"
	SessionNotFound  = "SESSION_NOT_FOUND"
	SessionCorrupted = "SESSION_CORRUPTED"
)

type DBError struct {
	InternalError error
}

func (err *DBError) Error() string {
	message := "DB ERROR"
	if err.InternalError != nil {
		message += fmt.Sprintf(": %s", err.InternalError.Error())
	}
	return message
}

// NotFoundError
type NotFoundError struct{}

func (err *NotFoundError) Error() string {
	return NotFound
}

// SessionNotFoundError
type SessionNotFoundError struct{}

func (err *SessionNotFoundError) Error() string {
	return SessionNotFound
}

// SessionCorruptedError
type SessionCorruptedError struct {
	InternalError error
}

func (err *SessionCorruptedError) Error() string {
	message := SessionCorrupted
	if err.InternalError != nil {
		message += fmt.Sprintf(": %s", err.InternalError.Error())
	}
	return message
}
