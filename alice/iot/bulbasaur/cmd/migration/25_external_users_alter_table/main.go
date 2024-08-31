package main

import (
	"context"
	"fmt"
	"os"
	"path"

	uberzap "go.uber.org/zap"
	"go.uber.org/zap/zapcore"
	"golang.org/x/exp/slices"
	"golang.org/x/xerrors"

	"a.yandex-team.ru/alice/iot/bulbasaur/db"
	"a.yandex-team.ru/alice/iot/bulbasaur/model"
	"a.yandex-team.ru/alice/library/go/cli"
	"a.yandex-team.ru/kikimr/public/sdk/go/ydb"
	"a.yandex-team.ru/kikimr/public/sdk/go/ydb/table"
	"a.yandex-team.ru/library/go/core/log/zap"
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

type ExternalUser struct {
	HskID          uint64
	SkillID        string
	UserID         uint64
	ExternalUserID string
}

func (eu ExternalUser) ToYDBValue() ydb.Value {
	return ydb.StructValue(
		ydb.StructFieldValue("hskid", ydb.Uint64Value(eu.HskID)),
		ydb.StructFieldValue("skill_id", ydb.StringValue([]byte(eu.SkillID))),
		ydb.StructFieldValue("user_id", ydb.Uint64Value(eu.UserID)),
		ydb.StructFieldValue("external_id", ydb.StringValue([]byte(eu.ExternalUserID))),
	)
}

type MigrationDBClient struct {
	*db.DBClient
}

func (db *MigrationDBClient) StreamUsers(ctx context.Context) ([]ExternalUser, error) {
	var users []ExternalUser
	s, err := db.SessionPool.Get(ctx)
	if err != nil {
		return users, xerrors.Errorf("Can't get session: %w", err)
	}

	eUsersTablePath := path.Join(db.Prefix, "ExternalUsers")
	logger.Infof("Reading ExternalUsers table from path %q", eUsersTablePath)

	res, err := s.StreamReadTable(ctx, eUsersTablePath,
		table.ReadColumn("hskid"),
		table.ReadColumn("skill_id"),
		table.ReadColumn("user_id"),
		table.ReadColumn("external_id"),
	)
	if err != nil {
		return users, xerrors.Errorf("Failed to read table: %w", err)
	}
	defer func() {
		if err := res.Close(); err != nil {
			logger.Fatalf("Error while closing result set: %v", err)
		}
	}()

	var count, usefullCount int
	for res.NextStreamSet(ctx) {
		for res.NextRow() {
			count++

			var eUser ExternalUser
			res.SeekItem("hskid")
			eUser.HskID = res.OUint64()

			res.SeekItem("skill_id")
			eUser.SkillID = string(res.OString())

			res.SeekItem("user_id")
			eUser.UserID = res.OUint64()

			res.SeekItem("external_id")
			eUser.ExternalUserID = string(res.OString())

			if err := res.Err(); err != nil {
				return users, xerrors.Errorf("Error occurred while reading %s: %w", eUsersTablePath, err)
			}

			if !slices.Contains(model.KnownInternalProviders, eUser.SkillID) {
				usefullCount++
				users = append(users, eUser)
			}
		}
	}

	logger.Infof("Finished reading %d users from %s, will migrate %d", count, eUsersTablePath, usefullCount)
	return users, nil
}

func (db *MigrationDBClient) UpsertUsers(ctx context.Context, users []ExternalUser) error {
	logger.Infof("Upserting %d users to ExternalUser table", len(users))
	query := fmt.Sprintf(`
		--!syntax_v1
		PRAGMA TablePathPrefix("%s");

		DECLARE $values AS List<Struct<
			hskid: Uint64,
			skill_id: String,
			user_id: Uint64,
			external_id: String
		>>;

		UPSERT INTO
			ExternalUser (hskid, skill_id, user_id, external_id)
		SELECT
			hskid, skill_id, user_id, external_id
		FROM AS_TABLE($values);
	`, db.Prefix)

	values := make([]ydb.Value, 0, len(users))
	for _, u := range users {
		ydbValue := u.ToYDBValue()
		values = append(values, ydbValue)
	}
	params := table.NewQueryParameters(
		table.ValueParam("$values", ydb.ListValue(values...)),
	)

	if err := db.Write(ctx, query, params); err != nil {
		return xerrors.Errorf("failed to upsert users: %w", err)
	}

	return nil
}

func do(ctx context.Context, client *MigrationDBClient) error {
	var users []ExternalUser
	users, err := client.StreamUsers(ctx)
	if err != nil {
		return err
	}

	//upload into db by batches of `size`
	size := 50000
	var j int
	for i := 0; i < len(users); i += size {
		j += size
		if j > len(users) {
			j = len(users)
		}

		if err := client.UpsertUsers(ctx, users[i:j]); err != nil {
			return err
		}
	}

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

	msg := fmt.Sprintf("start migration from ExternalUsers to ExternalUser at `%s%s`?", endpoint, prefix)
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
