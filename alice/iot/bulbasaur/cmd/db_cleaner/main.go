package main

import (
	"context"
	"fmt"
	"os"
	"path"
	"strconv"
	"sync"

	"a.yandex-team.ru/library/go/core/log/zap"

	"a.yandex-team.ru/alice/iot/bulbasaur/db"
	"a.yandex-team.ru/alice/library/go/cli"
	"a.yandex-team.ru/kikimr/public/sdk/go/ydb"
	"a.yandex-team.ru/kikimr/public/sdk/go/ydb/table"
	uberzap "go.uber.org/zap"
	"go.uber.org/zap/zapcore"
)

var logger *zap.Logger

type MigrationDBClient struct {
	*db.DBClient
}

func (db *MigrationDBClient) deleteRecords(ctx context.Context, huid uint64) error {
	for _, tableName := range [...]string{
		"DeviceGroups",
		"Devices",
		//		"ExternalUsers",
		"Groups",
		"Rooms",
		"Scenarios",
		//		"StationOwners",
		"UserSkills",
	} {
		query := fmt.Sprintf(`
			--!syntax_v1
			PRAGMA TablePathPrefix("%s");
			DECLARE $huid AS Uint64;
			DELETE FROM
				%s
			WHERE
				huid == $huid`, db.Prefix, tableName)
		params := table.NewQueryParameters(
			table.ValueParam("$huid", ydb.Uint64Value(huid)),
		)
		logger.Infof("cleaning <%s> records for user: %d", tableName, huid)
		if err := db.Write(ctx, query, params); err != nil {
			return err
		}
	}

	for _, tableName := range []string{
		"Users",
	} {
		query := fmt.Sprintf(`
			--!syntax_v1
			PRAGMA TablePathPrefix("%s");
			DECLARE $huid AS Uint64;
			DELETE FROM
				%s
			WHERE
				hid == $huid`, db.Prefix, tableName)
		params := table.NewQueryParameters(
			table.ValueParam("$huid", ydb.Uint64Value(huid)),
		)
		logger.Infof("cleaning <%s> records for user: %d", tableName, huid)
		if err := db.Write(ctx, query, params); err != nil {
			return err
		}
	}

	return nil
}

func (db *MigrationDBClient) streamUserRecords(ctx context.Context) <-chan uint64 {
	huidsChannel := make(chan uint64)

	go func() {
		defer close(huidsChannel)

		s, err := db.SessionPool.Get(ctx)
		if err != nil {
			logger.Fatalf("Can't get session: %v", err)
		}

		usersTablePath := path.Join(db.Prefix, "Users")
		logger.Infof("Reading Users table from path %q", usersTablePath)

		res, err := s.StreamReadTable(ctx, usersTablePath,
			table.ReadColumn("id"),
			table.ReadColumn("hid"),
			table.ReadColumn("login"),
		)
		if err != nil {
			logger.Fatalf("Failed to read table: %v", err)
		}
		defer func() {
			if err := res.Close(); err != nil {
				logger.Fatalf("Error while closing result set: %v", err)
			}
		}()

		var usersCount int
		for res.NextStreamSet(ctx) {
			for res.NextRow() {

				//id
				res.SeekItem("id")
				id := res.OUint64()
				if id == 0 {
					continue // skip users without id
				}

				//hid
				res.NextItem()
				hid := res.OUint64()
				if hid == 0 {
					continue // skip users without hid
				}

				//login
				res.NextItem()
				login := string(res.OString())

				//generated users has constraint of `login = id`
				if login != strconv.FormatUint(id, 10) {
					continue
				}

				usersCount++
				huidsChannel <- hid
			}
		}

		logger.Infof("finished reading %d records from %s", usersCount, usersTablePath)
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

func do(ctx context.Context, client *MigrationDBClient) error {
	var maxWorkers = 50
	var wg sync.WaitGroup
	var records = client.streamUserRecords(ctx)
	for i := 0; i < maxWorkers; i++ {
		wg.Add(1)
		go func() {
			defer wg.Done()
			for record := range records {
				if err := client.deleteRecords(ctx, record); err != nil {
					logger.Fatalf("failed to delete records: %v", err)
				}
			}
		}()
	}
	wg.Wait()
	logger.Info("done")
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

	_, trace := os.LookupEnv("YDB_DEBUG")

	dbcli, err := db.NewClient(context.Background(), logger, endpoint, prefix, ydb.AuthTokenCredentials{AuthToken: token}, trace)
	if err != nil {
		panic(err.Error())
	}

	msg := fmt.Sprintf("Do you really want to CLEAN DATABASE at `%s%s`", endpoint, prefix)
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
