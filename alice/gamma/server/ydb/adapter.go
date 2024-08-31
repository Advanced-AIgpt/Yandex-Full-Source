package ydb

import (
	"bytes"
	"context"
	"net"
	"text/template"
	"time"

	"golang.org/x/xerrors"

	"a.yandex-team.ru/kikimr/public/sdk/go/ydb"
	"a.yandex-team.ru/kikimr/public/sdk/go/ydb/table"

	"a.yandex-team.ru/alice/gamma/server/log"
)

var AlreadyCommittedError = xerrors.New("already committed")

// todo: Make simple orm
type Key struct {
	SessionID string
	MessageID int64
	UserID    string
}

type Item struct {
	Key
	Value          []byte
	StoredResponse []byte
}

type TemplateConfig struct {
	TablePathPrefix       string
	TablePath             string
	TransactionsTablePath string
}

type Adapter struct {
	Config      *ydb.DriverConfig
	client      *table.Client
	sessionPool *table.SessionPool
}

func (adapter *Adapter) Dial(ctx context.Context, dialTimeout time.Duration, endpoint string) error {
	drv, err := (&ydb.Dialer{
		DriverConfig: adapter.Config,
		NetDial: func(ctx context.Context, addr string) (net.Conn, error) {
			var dialer net.Dialer
			log.Infof("Want to dial to %q", addr)
			return dialer.DialContext(ctx, "tcp", addr)
		},
		Timeout: dialTimeout,
	}).Dial(ctx, endpoint)
	if err != nil {
		return err
	}
	adapter.client = &table.Client{
		Driver: drv,
	}
	adapter.sessionPool = &table.SessionPool{
		IdleThreshold: time.Second,
		Builder:       adapter.client,
	}

	return nil
}

func (adapter *Adapter) Close() error {
	if err := adapter.sessionPool.Close(context.TODO()); err != nil {
		return err
	}
	return adapter.client.Driver.Close()
}

type Session struct {
	session *table.Session
}

func (adapter *Adapter) CreateSession(ctx context.Context) (*Session, error) {
	session, err := adapter.sessionPool.Get(ctx)

	if err != nil {
		return nil, err
	}
	return &Session{session}, nil
}

func (adapter *Adapter) CloseSession(ctx context.Context, session *Session) error {
	return adapter.sessionPool.Put(ctx, session.session)
}

var readTemplate = template.Must(template.New("").Parse(`
PRAGMA TablePathPrefix = "{{ .TablePathPrefix }}";

DECLARE $user_id AS String;
DECLARE $session_id AS String;

SELECT 
    message_id,
	response
FROM [{{ .TransactionsTablePath }}]
WHERE
    user_id == $user_id AND
    session_id == $session_id
;

SELECT 
    user_id,
    value 
FROM [{{ .TablePath }}]
WHERE
    user_id == $user_id
;
`))

func (session *Session) Read(cfg TemplateConfig, key *Key, ctx context.Context) (*Item, error) {
	var query bytes.Buffer
	if err := readTemplate.Execute(&query, cfg); err != nil {
		return nil, err
	}

	statement, err := session.session.Prepare(ctx, query.String())
	if err != nil {
		return nil, err
	}

	_, res, err := statement.Execute(
		ctx,
		table.TxControl(
			table.BeginTx(table.WithOnlineReadOnly()),
			table.CommitTx(),
		),
		table.NewQueryParameters(
			table.ValueParam("$user_id", ydb.StringValue([]byte(key.UserID))),
			table.ValueParam("$message_id", ydb.Int64Value(key.MessageID)),
			table.ValueParam("$session_id", ydb.StringValue([]byte(key.SessionID))),
		))

	if err != nil {
		return nil, err
	}

	if res.NextSet() {
		if res.NextRow() {
			res.SeekItem("message_id")
			if !res.IsNull() && res.OInt64() >= key.MessageID {
				res.SeekItem("response")
				return &Item{StoredResponse: res.OString()}, AlreadyCommittedError
			}
		}
	}

	result := new(Item)
	if res.NextSet() {
		if res.NextRow() {
			res.SeekItem("user_id")
			if !res.IsNull() {
				result.UserID = string(res.OString())
			}
			res.SeekItem("value")
			if !res.IsNull() {
				result.Value = res.OString()
			}
		}
	}

	if err := res.Err(); err != nil {
		return nil, err
	}

	return result, nil
}

var writeTemplate = template.Must(template.New("").Parse(`
PRAGMA TablePathPrefix = "{{ .TablePathPrefix }}";

DECLARE $user_id AS String;
DECLARE $session_id AS String;
DECLARE $message_id AS Int64;
DECLARE $timestamp AS Int64;
DECLARE $response AS String;
DECLARE $value AS String;

SELECT 
    message_id
FROM [{{ .TransactionsTablePath }}]
WHERE
    user_id == $user_id AND
    session_id == $session_id
;

UPSERT INTO [{{ .TablePath }}] (
    user_id,
    value
) VALUES (
    $user_id,
    $value
);

UPSERT INTO [{{ .TransactionsTablePath }}] (
    user_id,
    session_id,
    message_id,
	response,
    timestamp
) VALUES (
    $user_id,
    $session_id,
    $message_id,
	$response,
    $timestamp
);
`))

func (session *Session) Write(cfg TemplateConfig, value *Item, ctx log.LoggingContext) error {
	var query bytes.Buffer
	if err := writeTemplate.Execute(&query, cfg); err != nil {
		return err
	}

	statement, err := session.session.Prepare(ctx, query.String())
	if err != nil {
		return err
	}

	tx, res, err := statement.Execute(
		ctx,
		table.TxControl(
			table.BeginTx(table.WithSerializableReadWrite()),
		),
		table.NewQueryParameters(
			table.ValueParam("$user_id", ydb.StringValue([]byte(value.UserID))),
			table.ValueParam("$session_id", ydb.StringValue([]byte(value.SessionID))),
			table.ValueParam("$message_id", ydb.Int64Value(value.MessageID)),
			table.ValueParam("$response", ydb.StringValue(value.StoredResponse)),
			table.ValueParam("$timestamp", ydb.Int64Value(time.Now().Unix())),
			table.ValueParam("$value", ydb.StringValue(value.Value)),
		))

	defer func() {
		err := tx.Rollback(ctx)
		if err == nil {
			ctx.Logger.Debugf("Rolling Back %+v", value.Key)
		}
	}()

	if err != nil {
		return err
	}

	if res.NextSet() {
		if res.NextRow() {
			res.SeekItem("message_id")
			if !res.IsNull() && res.OInt64() >= value.MessageID {
				return AlreadyCommittedError
			}
		}
	}

	return tx.Commit(ctx)
}
