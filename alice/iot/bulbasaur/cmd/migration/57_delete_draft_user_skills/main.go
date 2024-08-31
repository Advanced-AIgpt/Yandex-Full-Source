package main

import (
	"context"
	"fmt"
	"os"
	"path"
	"strings"
	"sync"
	"time"

	"a.yandex-team.ru/alice/iot/bulbasaur/db"
	"a.yandex-team.ru/alice/library/go/cli"
	"a.yandex-team.ru/kikimr/public/sdk/go/ydb"
	"a.yandex-team.ru/kikimr/public/sdk/go/ydb/table"
	"a.yandex-team.ru/library/go/core/log/zap"

	uberzap "go.uber.org/zap"
	"go.uber.org/zap/zapcore"
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

type UserSkill struct {
	huid    uint64
	userID  uint64
	skillID string
}

type MigrationDBClient struct {
	*db.DBClient
}

func (db *MigrationDBClient) streamDraftUserSkills(ctx context.Context) <-chan UserSkill {
	userSkillsChannel := make(chan UserSkill)

	go func() {
		defer close(userSkillsChannel)

		session, err := db.SessionPool.Get(ctx)
		if err != nil {
			logger.Fatalf("Can't get session: %v", err)
		}

		userSkillsPath := path.Join(db.Prefix, "UserSkills")
		logger.Infof("Reading user skills from %q", userSkillsPath)

		res, err := session.StreamReadTable(ctx, userSkillsPath,
			table.ReadColumn("huid"),
			table.ReadColumn("user_id"),
			table.ReadColumn("skill_id"),
		)
		if err != nil {
			logger.Fatalf("Failed to read table: %v", err)
		}
		defer func() {
			if err := res.Close(); err != nil {
				logger.Fatalf("Error while closing result set: %v", err)
			}
		}()

		var userSkillsRead, draftSkillsRead int
		for res.NextStreamSet(ctx) {
			for res.NextRow() {
				userSkillsRead++
				var userSkill UserSkill

				res.SeekItem("huid")
				userSkill.huid = res.OUint64()

				res.SeekItem("user_id")
				userSkill.userID = res.OUint64()

				res.SeekItem("skill_id")
				userSkill.skillID = string(res.OString())

				if err := res.Err(); err != nil {
					logger.Warnf("Error while reading result set from %s: %v", userSkillsPath, err)
				}

				if strings.Contains(userSkill.skillID, "-draft") {
					draftSkillsRead++
					userSkillsChannel <- userSkill
				}
			}
		}

		logger.Infof("Finished reading from %s. User skills read: %d, draft skills read: %d",
			userSkillsPath, userSkillsRead, draftSkillsRead)
	}()

	return userSkillsChannel
}

func (db *MigrationDBClient) deleteUserSkills(ctx context.Context, userSkills []UserSkill) error {
	query := fmt.Sprintf(`
		--!syntax_v1
		PRAGMA TablePathPrefix("%s");

		DECLARE $user_skills AS List<Struct<
			huid: Uint64,
			user_id: Uint64,
			skill_id: String
		>>;

		DELETE FROM
			UserSkills
		ON SELECT
			huid, user_id, skill_id
		FROM AS_TABLE($user_skills);
	`, db.Prefix)

	deleteParams := make([]ydb.Value, 0, len(userSkills))
	for _, userSkill := range userSkills {
		deleteParams = append(deleteParams, ydb.StructValue(
			ydb.StructFieldValue("huid", ydb.Uint64Value(userSkill.huid)),
			ydb.StructFieldValue("user_id", ydb.Uint64Value(userSkill.userID)),
			ydb.StructFieldValue("skill_id", ydb.StringValue([]byte(userSkill.skillID))),
		))
	}

	queryParams := table.NewQueryParameters(table.ValueParam("$user_skills", ydb.ListValue(deleteParams...)))

	return db.Write(ctx, query, queryParams)
}

func do(ctx context.Context, client *MigrationDBClient) error {
	batchSize := 1000
	draftUserSkillsChannel := client.streamDraftUserSkills(ctx)
	var batchNumber int
	var wg sync.WaitGroup
	var userSkillsDeleted int
	var mutex sync.Mutex

	for {
		batchStartTime := time.Now()
		userSkills := make([]UserSkill, 0, batchSize)
		batchNumber++

		for userSkill := range draftUserSkillsChannel {
			if len(userSkills) < batchSize {
				userSkills = append(userSkills, userSkill)
			}
			if len(userSkills) >= batchSize {
				break
			}
		}

		if len(userSkills) > 0 {
			wg.Add(1)
			go func(batchNumber int) {
				defer wg.Done()
				if err := client.deleteUserSkills(ctx, userSkills); err != nil {
					logger.Errorf("Error in batch %d: %v", batchNumber, err)
				} else {
					logger.Infof("Batch %d completed in %v. Deleted %d draft skills.", batchNumber, time.Since(batchStartTime), len(userSkills))
					mutex.Lock()
					userSkillsDeleted += len(userSkills)
					mutex.Unlock()
				}
			}(batchNumber)
		} else {
			break
		}
	}

	wg.Wait()

	logger.Infof("%d draft user skills deleted. Number of batches: %d\n", userSkillsDeleted, batchNumber-1)
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

	dbcli, err := db.NewClient(context.Background(), logger, endpoint, prefix,
		ydb.AuthTokenCredentials{AuthToken: token}, trace)
	if err != nil {
		panic(err.Error())
	}

	msg := fmt.Sprintf("Do you really want to delete draft user skills at `%s%s`", endpoint, prefix)
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
