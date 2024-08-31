package ydb

import (
	"context"
	"io/ioutil"
	"path"
	"testing"
	"time"

	"github.com/gofrs/uuid"
	"github.com/stretchr/testify/assert"
	"github.com/stretchr/testify/require"

	"a.yandex-team.ru/alice/gamma/server/log"
	"a.yandex-team.ru/kikimr/public/sdk/go/ydb"
	"a.yandex-team.ru/kikimr/public/sdk/go/ydb/table"
)

const (
	endpointFile = "ydb_endpoint.txt"
	databaseFile = "ydb_database.txt"
)

func localDriverConfig(t *testing.T) *ydb.DriverConfig {
	config := &ydb.DriverConfig{}

	database, err := ioutil.ReadFile(databaseFile)
	require.NoError(t, err, "Can't read database file for local ydb adapter")

	config.Database = string(database)
	return config
}

func localAdapter(t *testing.T, ctx context.Context) (*ydb.DriverConfig, *Adapter) {
	config := localDriverConfig(t)
	adapter := &Adapter{Config: config}

	endpoint, err := ioutil.ReadFile(endpointFile)
	require.NoError(t, err, "Can't read endpoint file for local ydb adapter")

	if err := adapter.Dial(ctx, 1*time.Second, string(endpoint)); err != nil {
		assert.FailNowf(t, "Can't dial", "%v", err)
	}

	return config, adapter
}

func localTemplateConfig(t *testing.T, cfg *ydb.DriverConfig) TemplateConfig {
	randomUUID, err := uuid.NewV4()
	require.NoError(t, err)
	return TemplateConfig{
		TablePathPrefix:       cfg.Database,
		TablePath:             randomUUID.String(),
		TransactionsTablePath: randomUUID.String() + ".tx",
	}
}

func (session *Session) createTmpStoreTable(cfg TemplateConfig, ctx context.Context) (f func() error, err error) {
	err = session.session.CreateTable(ctx, path.Join(cfg.TablePathPrefix, cfg.TablePath),
		table.WithColumn("user_id", ydb.Optional(ydb.TypeString)),
		table.WithColumn("value", ydb.Optional(ydb.TypeString)),
		table.WithPrimaryKeyColumn("user_id"),
	)
	f = func() error {
		if err == nil {
			return session.session.DropTable(ctx, path.Join(cfg.TablePathPrefix, cfg.TablePath))
		}
		return nil
	}
	return f, err
}

func (session *Session) createTmpTransactionsTable(cfg TemplateConfig, ctx context.Context) (f func() error, err error) {
	err = session.session.CreateTable(ctx, path.Join(cfg.TablePathPrefix, cfg.TransactionsTablePath),
		table.WithColumn("session_id", ydb.Optional(ydb.TypeString)),
		table.WithColumn("user_id", ydb.Optional(ydb.TypeString)),
		table.WithColumn("response", ydb.Optional(ydb.TypeString)),
		table.WithColumn("message_id", ydb.Optional(ydb.TypeInt64)),
		table.WithColumn("timestamp", ydb.Optional(ydb.TypeInt64)),
		table.WithPrimaryKeyColumn("session_id", "user_id"),
	)
	f = func() error {
		if err == nil {
			return session.session.DropTable(ctx, path.Join(cfg.TablePathPrefix, cfg.TransactionsTablePath))
		}
		return nil
	}
	return f, err
}

func (session *Session) createTmpTables(cfg TemplateConfig, ctx context.Context) (func() error, error) {
	f1, err := session.createTmpStoreTable(cfg, ctx)
	if err != nil {
		return func() error { return nil }, err
	}
	f2, err := session.createTmpTransactionsTable(cfg, ctx)
	if err != nil {
		return f1, err
	}
	return func() (err error) {
		defer func() {
			err_ := f1()
			if err == nil {
				err = err_
			}
		}()
		defer func() {
			err_ := f2()
			if err == nil {
				err = err_
			}
		}()
		return
	}, nil
}

func TestWriteRead(t *testing.T) {
	ctx, f := context.WithTimeout(context.Background(), 30*time.Second)
	defer f()

	ydbConfig, adapter := localAdapter(t, ctx)

	unit := Item{
		Key: Key{
			SessionID: "",
			MessageID: 0,
		},
		Value: []byte("bar"),
	}

	session, err := adapter.CreateSession(ctx)
	if err != nil {
		assert.FailNowf(t, "Failed opening session", "%v", err)
	}
	defer func() {
		assert.NoError(t, adapter.CloseSession(context.Background(), session))
	}()

	cfg := localTemplateConfig(t, ydbConfig)

	cancel, err := session.createTmpTables(cfg, ctx)
	if err != nil {
		assert.FailNowf(t, "Failed creating tables", "%v", err)
	}
	defer func() {
		if err := cancel(); err != nil {
			assert.FailNowf(t, "Failed dropping tables", "%v", err)
		}
	}()

	err = session.Write(cfg, &unit, log.CreateLoggingContext(ctx))
	if err != nil {
		assert.FailNowf(t, "Failed write", "%v", err)
	}

	_, err = session.Read(cfg, &unit.Key, ctx)
	assert.EqualError(t, err, "already committed")

	unit.MessageID += 1
	value, err := session.Read(cfg, &unit.Key, ctx)
	if err != nil {
		assert.FailNowf(t, "Failed read", "%v", err)
	}

	assert.Equal(t, value.Value, unit.Value)
}
