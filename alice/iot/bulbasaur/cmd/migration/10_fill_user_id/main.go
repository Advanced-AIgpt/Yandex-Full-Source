package main

import (
	"context"
	"fmt"
	"os"
	"path"

	"a.yandex-team.ru/library/go/core/log/zap"

	"a.yandex-team.ru/kikimr/public/sdk/go/ydb"
	uberzap "go.uber.org/zap"
	"go.uber.org/zap/zapcore"
	"golang.org/x/xerrors"

	"a.yandex-team.ru/alice/iot/bulbasaur/db"
	"a.yandex-team.ru/alice/library/go/cli"
	"a.yandex-team.ru/kikimr/public/sdk/go/ydb/table"
)

var logger *zap.Logger

type MigrationDBClient struct {
	*db.DBClient
}

func (db *MigrationDBClient) getUserID(ctx context.Context, huid uint64) (uint64, error) {
	var uid uint64
	query := fmt.Sprintf(`
			--!syntax_v1
			PRAGMA TablePathPrefix("%s");
			DECLARE $huid AS uint64;

			SELECT
				id
			FROM
				Users
			WHERE
				hid == $huid`, db.Prefix)
	params := table.NewQueryParameters(
		table.ValueParam("$huid", ydb.Uint64Value(huid)),
	)

	selectFunc := func(ctx context.Context, s *table.Session) (err error) {
		readTx := table.TxControl(
			table.BeginTx(table.WithOnlineReadOnly()),
			table.CommitTx(),
		)

		stmt, err := s.Prepare(ctx, query)
		if err != nil {
			return err
		}

		_, res, err := stmt.Execute(ctx, readTx, params)
		if err != nil {
			return err
		}

		for res.NextSet() {
			for res.NextRow() {
				res.SeekItem("id")
				uid = res.OUint64()
			}
		}

		return nil
	}

	if err := table.Retry(ctx, db.SessionPool, table.OperationFunc(selectFunc)); err != nil {
		return uid, xerrors.Errorf("database request has failed: %w", err)
	}

	return uid, nil
}

func (db *MigrationDBClient) updateRecord(ctx context.Context, huid uint64, tableName string) (err error) {
	var uid uint64
	if uid, err = db.getUserID(ctx, huid); err != nil {
		return xerrors.Errorf(fmt.Sprintf("cannot get uid for huid: <%d>", huid), err)
	} else if uid == 0 {
		logger.Warnf("cannot set user_id at <%s>, huid: <%d>", tableName, huid)
		return nil
	}

	logger.Infof("updating record at <%s> set user_id: <%d> where huid: <%d>", tableName, uid, huid)
	updateQuery := fmt.Sprintf(`
			--!syntax_v1
			PRAGMA TablePathPrefix("%s");
			DECLARE $huid AS Uint64;
			DECLARE $uid AS Uint64;

			UPDATE
				%s
			SET
				user_id = $uid
			WHERE
				huid == $huid`, db.Prefix, tableName)
	params := table.NewQueryParameters(
		table.ValueParam("$huid", ydb.Uint64Value(huid)),
		table.ValueParam("$uid", ydb.Uint64Value(uid)),
	)

	if err := db.Write(ctx, updateQuery, params); err != nil {
		return xerrors.Errorf("failed to update record with huid <%d>: %w", huid, err)
	}

	return nil
}

func (db *MigrationDBClient) streamRecords(ctx context.Context, tableName string) <-chan uint64 {
	huidsChannel := make(chan uint64)

	go func() {
		defer close(huidsChannel)

		s, err := db.SessionPool.Get(ctx)
		if err != nil {
			logger.Fatalf("Can't get session: %v", err)
		}

		recordsTablePath := path.Join(db.Prefix, tableName)
		logger.Infof("Reading records from path %q", recordsTablePath)

		res, err := s.StreamReadTable(ctx, recordsTablePath,
			table.ReadColumn("user_id"),
			table.ReadColumn("huid"),
		)
		if err != nil {
			logger.Fatalf("Failed to read table: %v", err)
		}
		defer func() {
			if err := res.Close(); err != nil {
				logger.Fatalf("Error while closing result set: %v", err)
			}
		}()

		var count int
		for res.NextStreamSet(ctx) {
			for res.NextRow() {
				//userID
				res.SeekItem("user_id")
				userID := res.OUint64()
				if userID != 0 {
					continue
				}

				//huid
				res.NextItem()
				huid := res.OUint64()

				count++
				huidsChannel <- huid
			}
		}

		logger.Infof("finished reading %d records from %s", count, recordsTablePath)
	}()

	return huidsChannel
}

func initLogging() (logger *zap.Logger, stop func()) {
	encoderConfig := uberzap.NewDevelopmentEncoderConfig()
	encoderConfig.EncodeLevel = zapcore.CapitalColorLevelEncoder
	encoderConfig.EncodeCaller = zapcore.ShortCallerEncoder
	encoder := zapcore.NewConsoleEncoder(encoderConfig)

	core := zapcore.NewCore(encoder, zapcore.AddSync(os.Stderr), uberzap.DebugLevel)
	stop = func() {
		_ = core.Sync()
	}
	logger = zap.NewWithCore(core, uberzap.AddStacktrace(uberzap.FatalLevel), uberzap.AddCaller())

	return
}

func do(ctx context.Context, client *MigrationDBClient, tableName string) error {
	for record := range client.streamRecords(ctx, tableName) {
		if err := client.updateRecord(ctx, record, tableName); err != nil {
			return xerrors.Errorf("failed to update record: %w", err)
		}
	}
	logger.Info("done")
	return nil
}

func main() {
	logger, stop := initLogging()
	defer stop()

	endpoint := os.Getenv("YDB_ENDPOINT")
	if len(endpoint) == 0 {
		panic("YDB_ENDPOINT env is not set")
	}

	prefix := os.Getenv("YDB_PREFIX")
	if len(prefix) == 0 {
		panic("YDB_PREFIX env is not set")
	}

	token := os.Getenv("YDB_TOKEN")
	if len(token) == 0 {
		panic("YDB_TOKEN env is not set")
	}

	_, trace := os.LookupEnv("YDB_DEBUG")

	dbcli, err := db.NewClient(context.Background(), logger, endpoint, prefix, ydb.AuthTokenCredentials{AuthToken: token}, trace)
	if err != nil {
		panic(err.Error())
	}

	msg := fmt.Sprintf("do you really want to update fill in empty user_id at `%s%s`", endpoint, prefix)
	c := cli.AskForConfirmation(msg, logger)
	if !c {
		logger.Info("Bye")
		os.Exit(0)
	}

	ctx := context.Background()

	TableName := "Scenarios"
	if err := do(ctx, &MigrationDBClient{dbcli}, TableName); err != nil {
		logger.Fatal(err.Error())
	}
}
