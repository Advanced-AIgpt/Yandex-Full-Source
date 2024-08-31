package main

import (
	"context"
	"fmt"
	"path"
	"sync"
	"time"

	"a.yandex-team.ru/alice/iot/adapters/tuya_adapter/db"
	"a.yandex-team.ru/kikimr/public/sdk/go/ydb"
	"a.yandex-team.ru/kikimr/public/sdk/go/ydb/table"
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/xerrors"
)

type MigrationDBClient struct {
	*db.DBClient
}

type readUserRowResult struct {
	User User
	err  error
}

func (client *MigrationDBClient) moveFromUsersToGenericUsers(ctx context.Context) (err error) {
	batchNumber, batchSize := 0, 1000
	userChannel := client.readUserRows(ctx)

	var wg sync.WaitGroup
	for {
		startBatch := time.Now()
		batch := make(Users, 0, batchSize)

		for readResult := range userChannel {
			user, err := readResult.User, readResult.err
			if err != nil {
				logger.Infof("failed to read row from table %s, returning", client.Prefix)
				return err
			}
			if len(batch) < batchSize {
				batch = append(batch, user)
			}
			if len(batch) >= batchSize {
				break
			}
		}

		if len(batch) > 0 {
			batchNumber++
			wg.Add(1)
			go func(batchNumber int) {
				defer wg.Done()
				if err := client.storeUserBatchInNewTable(ctx, batch); err != nil {
					logger.Debugf("error in batch %d: %v", batchNumber, err)
				} else {
					logger.Debugf("processed batch %d %d values in %v", batchNumber, len(batch), time.Since(startBatch))
				}
			}(batchNumber)
		} else {
			break
		}
	}

	wg.Wait()
	logger.Infof("moving users done, finished on batch %d", batchNumber)
	return nil
}

func (client *MigrationDBClient) readUserRows(ctx context.Context) <-chan readUserRowResult {
	userChannel := make(chan readUserRowResult, 1000)
	go func() {
		defer close(userChannel)
		defer func() {
			if r := recover(); r != nil {
				logger.Warnf("panic while reading user rows: %v", r)
				userChannel <- readUserRowResult{err: xerrors.Errorf("panic in reading user rows")}
			}
		}()

		s, err := client.SessionPool.Get(ctx)
		if err != nil {
			logger.Warnf("can't get session: %v", err)
			userChannel <- readUserRowResult{err: xerrors.Errorf("can't get session to read table: %w", err)}
			return
		}

		usersTablePath := path.Join(client.Prefix, "Users")
		logger.Infof("reading table from path %s", usersTablePath)
		res, err := s.StreamReadTable(ctx, usersTablePath,
			table.ReadColumn("id"),
			table.ReadColumn("login"),
			table.ReadColumn("tuya_uid"),
		)
		if err != nil {
			logger.Infof("failed to read table: %v", err)
			userChannel <- readUserRowResult{err: err}
			return
		}
		defer func() {
			if err := res.Close(); err != nil {
				logger.Warnf("error while closing result set: %v", err)
			}
		}()

		usersCount := 0
		for res.NextStreamSet(ctx) {
			for res.NextRow() {
				var u User
				res.SeekItem("id")
				u.ID = res.OUint64()
				res.NextItem()
				u.Login = string(res.OString())
				res.NextItem()
				u.TuyaUID = string(res.OString())
				if err := res.Err(); err != nil {
					logger.Infof("error while reading user from %s: %v", usersTablePath, err)
					userChannel <- readUserRowResult{User: u, err: err}
					return
				}
				usersCount++
				logger.Info(fmt.Sprintf("read %d user", usersCount), log.Any("user", u))
				userChannel <- readUserRowResult{User: u}
			}
		}
		logger.Infof("finished reading %d users from %s", usersCount, usersTablePath)
	}()
	return userChannel
}

func (client *MigrationDBClient) storeUserBatchInNewTable(ctx context.Context, users Users) error {
	query := fmt.Sprintf(`
		--!syntax_v1
		PRAGMA TablePathPrefix("%s");

		DECLARE $users AS List<Struct<
			hid: Uint64,
			id: Uint64,
			login: String,
			tuya_uid: String,
			skill_id: String>>;

		UPSERT INTO
			GenericUsers(hid, id, skill_id, login, tuya_uid)
		SELECT
			hid, id, skill_id, login, tuya_uid
		FROM AS_TABLE($users);`, client.Prefix)

	ydbUsers := make([]ydb.Value, 0, len(users))
	for _, user := range users {
		ydbUsers = append(ydbUsers, user.toStructValue())
	}
	params := table.NewQueryParameters(
		table.ValueParam("$users", ydb.ListValue(ydbUsers...)),
	)

	if err := client.Write(ctx, query, params); err != nil {
		logger.Infof("failed to execute upsert query for users batch: %v", err)
		return err
	}
	return nil
}
