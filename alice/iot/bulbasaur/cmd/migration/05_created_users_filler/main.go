package main

import (
	"context"
	"fmt"
	"os"

	"a.yandex-team.ru/kikimr/public/sdk/go/ydb"
	"a.yandex-team.ru/library/go/core/log/zap"

	uberzap "go.uber.org/zap"
	"go.uber.org/zap/zapcore"

	"a.yandex-team.ru/alice/iot/bulbasaur/db"
	"a.yandex-team.ru/alice/library/go/cli"
	"a.yandex-team.ru/kikimr/public/sdk/go/ydb/table"
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

func do(ctx context.Context, client *db.DBClient) (err error) {
	query := fmt.Sprintf(`
	--!syntax_v1
	PRAGMA TablePathPrefix("%s");
	$desired_by_devices = (
	SELECT
		huid,
		MIN(created) AS created
	FROM
		Devices
	WHERE
		created IS NOT NULL
	GROUP BY
		huid
	);

	$desired_by_scenarios = (
	SELECT
		huid,
		MIN(created) AS created
	FROM
		Scenarios
	WHERE
		created IS NOT NULL
	GROUP BY
		huid
	);

	$desired_by_rooms = (
	SELECT
		huid,
		MIN(created) AS created
	FROM
		Rooms
	WHERE
		created IS NOT NULL
	GROUP BY
		huid
	);

	$desired_by_groups = (
	SELECT
		huid,
		MIN(created) AS created
	FROM
		Groups
	WHERE
		created IS NOT NULL
	GROUP BY
		huid
	);

	$to_update = (SELECT
		u.hid AS hid,
		ListMin(AsList(d.created, g.created, r.created, s.created)) AS created
	FROM
		Users AS u
	LEFT JOIN $desired_by_devices AS d ON u.hid = d.huid
	LEFT JOIN $desired_by_groups AS g ON u.hid = g.huid
	LEFT JOIN $desired_by_rooms AS r ON u.hid = r.huid
	LEFT JOIN $desired_by_scenarios AS s ON u.hid = s.huid
	WHERE
		u.created IS NULL);

	UPDATE Users ON
		SELECT
			hid,
			created
		FROM
			$to_update;`, client.Prefix)

	params := table.NewQueryParameters()
	err = client.Write(ctx, query, params)
	if err != nil {
		return err
	}
	logger.Info("Done")
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

	//msg := fmt.Sprintf("You cannot perform this operation at `%s%s`", endpoint, prefix)
	//cli.Ban(`production|quasar`, prefix, msg, logger)

	msg := fmt.Sprintf("Do you really want to fill column created in table Users at `%s%s`", endpoint, prefix)
	c := cli.AskForConfirmation(msg, logger)
	if !c {
		logger.Info("Bye")
		os.Exit(0)
	}

	ctx := context.Background()

	if err := do(ctx, dbcli); err != nil {
		logger.Fatal(err.Error())
	}

}
