package main

import (
	"bufio"
	"context"
	"fmt"
	"os"
	"strings"

	"a.yandex-team.ru/library/go/core/log/ctxlog"
	"a.yandex-team.ru/library/go/core/log/zap"

	uberzap "go.uber.org/zap"
	"go.uber.org/zap/zapcore"

	"a.yandex-team.ru/alice/iot/bulbasaur/db"
	"a.yandex-team.ru/alice/library/go/tools"
	"a.yandex-team.ru/kikimr/public/sdk/go/ydb"
	"a.yandex-team.ru/kikimr/public/sdk/go/ydb/table"
	"a.yandex-team.ru/library/go/core/xerrors"
)

var logger *zap.Logger

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

	LIMIT := int64(100)
	TABLE := "Rooms0627" //FIXME: user real table name

	c := askForConfirmation(fmt.Sprintf("Do you really want to perform migrations at `%s%s/%s`", endpoint, prefix, TABLE))
	if c {
		ctx := context.Background()
		if err := migrate(ctx, dbcli, TABLE, LIMIT); err != nil {
			logger.Error(err.Error())
			os.Exit(1)
		}
	} else {
		logger.Info("Bye")
	}
}

type changeInfo struct {
	uid uint64
	id  string
}

func migrate(ctx context.Context, db *db.DBClient, tableName string, limit int64) error {
	s, err := db.SessionPool.Get(ctx)
	if err != nil {
		return err
	}

	fetched := true
	processed := int64(0)
	for fetched && (limit == -1 || processed < limit) {
		readTx := table.TxControl(
			table.BeginTx(table.WithOnlineReadOnly()),
			table.CommitTx(),
		)
		records, err := getRecords(ctx, db, s, readTx, tableName)
		if err != nil {
			return err
		}
		fetched = len(records) > 0

		for _, record := range records {
			writeTx := table.TxControl(
				table.BeginTx(table.WithSerializableReadWrite()),
				table.CommitTx(),
			)
			if err := setRecordHuid(ctx, db, s, writeTx, tableName, record); err != nil {
				return err
			}
			processed = processed + 1
		}
	}

	return nil
}

func setRecordHuid(ctx context.Context, db *db.DBClient, s *table.Session, tc *table.TransactionControl, tableName string, changeInfo changeInfo) error {
	logger.Infof("Update <%s> set huid=%d where uid=%d and id=%s",
		tableName, tools.Huidify(changeInfo.uid), changeInfo.uid, changeInfo.id)

	query := fmt.Sprintf(`
		--!syntax_v1
		PRAGMA TablePathPrefix("%s");
		DECLARE $uid AS Uint64;
		DECLARE $huid AS Uint64;
		DECLARE $id AS String;

		UPDATE %s
		SET huid = $huid
		WHERE user_id = $uid AND
		id = $id AND
		huid is null`, db.Prefix, tableName)
	params := table.NewQueryParameters(
		table.ValueParam("$uid", ydb.Uint64Value(changeInfo.uid)),
		table.ValueParam("$huid", ydb.Uint64Value(tools.Huidify(changeInfo.uid))),
		table.ValueParam("$id", ydb.StringValue([]byte(changeInfo.id))),
	)

	preparedQuery, err := s.Prepare(ctx, query)
	if err != nil {
		return err
	}

	_, _, err = preparedQuery.Execute(ctx, tc, params)
	if err != nil {
		return err
	}

	logger.Info("Done")
	return nil
}

func getRecords(ctx context.Context, db *db.DBClient, s *table.Session, tc *table.TransactionControl, tableName string) ([]changeInfo, error) {
	records := make([]changeInfo, 0)
	logger.Info("fetching new batch")
	query := fmt.Sprintf(`
		--!syntax_v1
		PRAGMA TablePathPrefix("%s");

		SELECT user_id, id
		FROM %s
		WHERE huid is null
		AND user_id is not null
		LIMIT 10`, db.Prefix, tableName)
	params := table.NewQueryParameters()

	preparedQuery, err := s.Prepare(ctx, query)
	if err != nil {
		return nil, err
	}

	_, res, err := preparedQuery.Execute(ctx, tc, params)
	if err != nil {
		return nil, err
	}

	for res.NextSet() {
		for res.NextRow() {
			record := changeInfo{}
			res.SeekItem("user_id")
			record.uid = uint64(res.OUint64())
			res.NextItem()
			record.id = string(res.OString())

			if res.Err() != nil {
				ctxlog.Errorf(ctx, db.Logger, "failed to parse YDB response row: %v", res.Err())
				return nil, xerrors.Errorf("failed to parse YDB response row: %w", res.Err())
			}
			records = append(records, record)
		}
	}

	logger.Info("fetching Done")
	return records, nil
}

func askForConfirmation(s string) bool {
	reader := bufio.NewReader(os.Stdin)
	logger.Infof("%s [y/n]: ", s)
	response, err := reader.ReadString('\n')
	if err != nil {
		logger.Fatal(err.Error())
	}

	response = strings.ToLower(strings.TrimSpace(response))
	if strings.ToLower(response) == "y" || strings.ToLower(response) == "yes" {
		return true
	}
	return false
}
