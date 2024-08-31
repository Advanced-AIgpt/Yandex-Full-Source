package pgclient

import (
	"database/sql"
	"time"
)

type Option func(*sql.DB)

// pool setting per DB host
func MaxIdlePoolConns(conns int) Option {
	return func(db *sql.DB) {
		db.SetMaxIdleConns(conns)
	}
}

// pool setting per DB host
func MaxOpenPoolConns(conns int) Option {
	return func(db *sql.DB) {
		db.SetMaxOpenConns(conns)
	}
}

// pool setting per DB host
func PoolConnMaxLifetime(lifetime time.Duration) Option {
	return func(db *sql.DB) {
		db.SetConnMaxLifetime(lifetime)
	}
}

var defaultOptions = []Option{MaxOpenPoolConns(10)}
