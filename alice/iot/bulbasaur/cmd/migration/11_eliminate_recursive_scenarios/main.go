package main

import (
	"context"
	"encoding/json"
	"fmt"
	"os"
	"path"

	"a.yandex-team.ru/kikimr/public/sdk/go/ydb"
	"a.yandex-team.ru/library/go/core/log/zap"

	"a.yandex-team.ru/alice/iot/bulbasaur/db"
	"a.yandex-team.ru/alice/iot/bulbasaur/model"
	"a.yandex-team.ru/alice/library/go/cli"
	"a.yandex-team.ru/kikimr/public/sdk/go/ydb/table"
	uberzap "go.uber.org/zap"
	"go.uber.org/zap/zapcore"
	"golang.org/x/xerrors"
)

var logger *zap.Logger

type userScenario struct {
	Scenario model.Scenario
	User     model.User
}

type MigrationDBClient struct {
	*db.DBClient
}

func (db *MigrationDBClient) changeRecord(_ context.Context, _ userScenario) (err error) {
	return nil
}

func (db *MigrationDBClient) streamRecords(ctx context.Context) <-chan userScenario {
	scenariosChannel := make(chan userScenario)

	go func() {
		defer close(scenariosChannel)

		s, err := db.SessionPool.Get(ctx)
		if err != nil {
			logger.Fatalf("Can't get session: %v", err)
		}

		scenariosTablePath := path.Join(db.Prefix, "Scenarios")
		logger.Infof("Reading Scenarios table from path %q", scenariosTablePath)

		res, err := s.StreamReadTable(ctx, scenariosTablePath,
			table.ReadColumn("archived"),
			table.ReadColumn("user_id"),
			table.ReadColumn("external_actions"),
			table.ReadColumn("id"),
			table.ReadColumn("name"),
			table.ReadColumn("icon"),
			table.ReadColumn("devices"),
		)
		if err != nil {
			logger.Fatalf("Failed to read table: %v", err)
		}
		defer func() {
			if err := res.Close(); err != nil {
				logger.Fatalf("Error while closing result set: %v", err)
			}
		}()

		var scenariosCount int
		for res.NextStreamSet(ctx) {
			for res.NextRow() {
				var us userScenario

				//archived
				res.SeekItem("archived")
				archived := res.OBool()
				if archived {
					continue
				}

				//user
				res.NextItem()
				us.User.ID = res.OUint64()
				if us.User.ID == 0 {
					continue
				}

				res.NextItem()
				us.Scenario.ID = string(res.OString())
				res.NextItem()
				us.Scenario.Name = model.ScenarioName(res.OString())
				res.NextItem()
				us.Scenario.Icon = model.ScenarioIcon(res.OString())

				//devices
				res.NextItem()
				var devices []model.ScenarioDevice
				if err := json.Unmarshal([]byte(res.OJSON()), &devices); err != nil {
					logger.Fatalf("failed to parse `devices` field for scenario %s: %v", us.Scenario.ID, err)
				}
				us.Scenario.Devices = devices

				if res.Err() != nil {
					logger.Fatalf("failed to parse YDB response row: %v", res.Err())
				}

				scenariosCount++
				scenariosChannel <- us
			}
		}

		logger.Infof("finished reading %d records from %s", scenariosCount, scenariosTablePath)
	}()

	return scenariosChannel
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
	for userScenario := range client.streamRecords(ctx) {
		if err := client.changeRecord(ctx, userScenario); err != nil {
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

	msg := fmt.Sprintf("Do you really want to elimintate recursive ExternalActions from user Scenarios at `%s%s`", endpoint, prefix)
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
