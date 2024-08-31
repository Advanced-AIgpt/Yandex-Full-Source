package main

import (
	"context"
	"fmt"
	"os"
	"strconv"

	"a.yandex-team.ru/alice/iot/bulbasaur/db"
	"a.yandex-team.ru/alice/library/go/cli"
	"a.yandex-team.ru/alice/library/go/tools"
	"a.yandex-team.ru/kikimr/public/sdk/go/ydb"
	"a.yandex-team.ru/kikimr/public/sdk/go/ydb/table"
	"a.yandex-team.ru/library/go/core/log/zap"
	uzap "go.uber.org/zap"
	"go.uber.org/zap/zapcore"
)

var logger *zap.Logger
var currentUserID uint64
var limit uint64

type MigrationDBClient struct {
	*db.DBClient
}

type dbObject struct {
	huid uint64
	id   string
}

func (db *MigrationDBClient) getObjectsChunk(ctx context.Context, lastDBObject *dbObject) ([]dbObject, bool, error) {
	query := fmt.Sprintf(`
		--!syntax_v1
		PRAGMA TablePathPrefix("%s");

		DECLARE $limit AS Uint64;
		DECLARE $huid AS Uint64;
		DECLARE $lastId AS String;

		SELECT huid, id FROM Devices
		WHERE huid == $huid AND id > $lastId AND archived == true
		ORDER BY huid, id LIMIT $limit;
	`, db.Prefix)

	var lastDBObjectID string

	if lastDBObject != nil {
		lastDBObjectID = lastDBObject.id
	}

	params := table.NewQueryParameters(
		table.ValueParam("$limit", ydb.Uint64Value(limit)),
		table.ValueParam("$huid", ydb.Uint64Value(tools.Huidify(currentUserID))),
		table.ValueParam("$lastId", ydb.StringValue([]byte(lastDBObjectID))),
	)

	dbObjects := make([]dbObject, 0)

	res, err := db.Read(ctx, query, params)
	if err != nil {
		return dbObjects, false, err
	}

	if !res.NextSet() {
		return dbObjects, false, nil
	}

	for res.NextRow() {
		var o dbObject

		res.NextItem()
		o.huid = res.OUint64()

		res.NextItem()
		o.id = string(res.OString())

		dbObjects = append(dbObjects, o)
	}

	hasMoreData := uint64(res.RowCount()) == limit
	return dbObjects, hasMoreData, nil
}

func (db *MigrationDBClient) deleteArchived(ctx context.Context, dbObjects []dbObject) error {
	if len(dbObjects) == 0 {
		return nil
	}

	query := fmt.Sprintf(`
		--!syntax_v1
		PRAGMA TablePathPrefix("%s");

		DECLARE $objects AS List<Struct<
			huid: Uint64,
			id: String
		>>;

		DELETE FROM
			Devices
		ON SELECT
			huid, id
		FROM AS_TABLE($objects);
	`, db.Prefix)

	deleteParams := make([]ydb.Value, 0, len(dbObjects))
	for _, o := range dbObjects {
		logger.Infof("Prepare to delete object with values (huid: %d, id: %s)", o.huid, o.id)

		deleteParams = append(deleteParams, ydb.StructValue(
			ydb.StructFieldValue("huid", ydb.Uint64Value(o.huid)),
			ydb.StructFieldValue("id", ydb.StringValue([]byte(o.id))),
		))
	}

	params := table.NewQueryParameters(
		table.ValueParam("$objects", ydb.ListValue(deleteParams...)),
	)

	return db.Write(ctx, query, params)
}

func initLogging() (*zap.Logger, func()) {
	encoderConfig := uzap.NewDevelopmentEncoderConfig()
	encoderConfig.EncodeLevel = zapcore.CapitalColorLevelEncoder
	encoderConfig.EncodeCaller = zapcore.ShortCallerEncoder
	encoder := zapcore.NewConsoleEncoder(encoderConfig)

	core := zapcore.NewCore(encoder, zapcore.AddSync(os.Stderr), uzap.DebugLevel)
	stop := func() {
		_ = core.Sync()
	}
	logger = zap.NewWithCore(core, uzap.AddStacktrace(uzap.FatalLevel), uzap.AddCaller())

	return logger, stop
}

func do(ctx context.Context, client *MigrationDBClient) error {
	hasMoreData := true
	deletedCount := 0
	var lastDBObject *dbObject

	for hasMoreData {
		chunk, moreData, err := client.getObjectsChunk(ctx, lastDBObject)
		if err != nil {
			return err
		}
		hasMoreData = moreData
		if len(chunk) > 0 {
			lastDBObject = &chunk[len(chunk)-1]
		}
		if err := client.deleteArchived(ctx, chunk); err != nil {
			return err
		}
		deletedCount += len(chunk)
	}

	logger.Info("Devices cleanup done\n")
	logger.Infof("Devices deleted count: %d\n", deletedCount)
	return nil
}

func main() {
	var stop func()
	logger, stop = initLogging()
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

	stringifiedUserID := os.Getenv("USER_ID")
	if len(stringifiedUserID) == 0 {
		panic("USER_ID env is not set")
	}
	var err error
	currentUserID, err = strconv.ParseUint(stringifiedUserID, 10, 64)
	if err != nil {
		panic(err.Error())
	}

	stringifiedLimit := os.Getenv("ROWS_LIMIT")
	if len(stringifiedLimit) == 0 {
		panic("ROWS_LIMIT env is not set")
	}
	limit, err = strconv.ParseUint(stringifiedLimit, 10, 64)
	if err != nil {
		panic(err.Error())
	}

	_, trace := os.LookupEnv("YDB_DEBUG")

	dbcli, err := db.NewClient(context.Background(), logger, endpoint, prefix, ydb.AuthTokenCredentials{AuthToken: token}, trace)
	if err != nil {
		panic(err.Error())
	}

	msg := fmt.Sprintf("Do you really want to cleanup archived Devices for user_id %d at `%s%s`", currentUserID, endpoint, prefix)
	c := cli.AskForConfirmation(msg, logger)
	if !c {
		logger.Info("Bye")
		os.Exit(0)
	}

	ctx := context.Background()

	if err := do(ctx, &MigrationDBClient{dbcli}); err != nil {
		logger.Fatal(err.Error())
	}
}
