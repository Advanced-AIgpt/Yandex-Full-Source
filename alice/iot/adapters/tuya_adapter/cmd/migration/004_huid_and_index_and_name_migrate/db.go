package main

import (
	"context"
	"fmt"
	"path"
	"sync"

	"a.yandex-team.ru/alice/iot/adapters/tuya_adapter/db"
	"a.yandex-team.ru/kikimr/public/sdk/go/ydb"
	"a.yandex-team.ru/kikimr/public/sdk/go/ydb/table"
	"a.yandex-team.ru/library/go/core/xerrors"
)

func createUsersTable(ctx context.Context, client *db.DBClient) error {
	options := []table.CreateTableOption{
		table.WithColumn("hid", ydb.Optional(ydb.TypeUint64)),
		table.WithColumn("id", ydb.Optional(ydb.TypeUint64)),
		table.WithColumn("login", ydb.Optional(ydb.TypeString)),
		table.WithColumn("tuya_uid", ydb.Optional(ydb.TypeString)),
		table.WithPrimaryKeyColumn("hid", "id"),
		table.WithIndex("id_index",
			table.WithIndexType(table.GlobalIndex()),
			table.WithIndexColumns("id"),
		),
		table.WithIndex("tuya_uid_index",
			table.WithIndexType(table.GlobalIndex()),
			table.WithIndexColumns("tuya_uid"),
		),
		table.WithProfile(
			table.WithPartitioningPolicy(
				table.WithPartitioningPolicyUniformPartitions(4),
			),
		),
	}
	return table.Retry(ctx, client.SessionPool, table.OperationFunc(func(ctx context.Context, s *table.Session) error {
		return s.CreateTable(ctx, path.Join(client.Prefix, "Users"), options...)
	}))
}

type readUserRowResult struct {
	User OldUser
	err  error
}

type storeUserRowResults struct {
	Users HuidUsers
	err   error
}

func populateUsersTable(ctx context.Context, client *db.DBClient) (err error) {
	userChannel := make(chan readUserRowResult, 1000)
	storeChannel := make(chan storeUserRowResults, 5)
	go readOldUserRows(ctx, client, userChannel)
	var (
		wg          sync.WaitGroup
		batchNumber int
	)
	for {
		batchSize := 1000
		users := make(OldUsers, 0, batchSize)

		for readResult := range userChannel {
			user, err := readResult.User, readResult.err
			if err != nil {
				logger.Infof("Failed to read row from table %s, returning", client.Prefix)
				return err
			}
			if len(users) < batchSize {
				users = append(users, user)
			}
			if len(users) >= batchSize {
				break
			}
		}

		if len(users) > 0 {
			batchNumber++
			wg.Add(1)
			logger.Infof("Starting store of batch %d", batchNumber)
			go storeUserBatchInNewTable(ctx, client, users.toHuidUsers(), storeChannel, &wg)
		} else {
			logger.Infof("Processed all user rows, started storing %d batches", batchNumber)
			break
		}
	}

	go func() {
		wg.Wait()
		close(storeChannel)
	}()

	for storeResult := range storeChannel {
		if err := storeResult.err; err != nil {
			logger.Infof("Failed to store user row %v", err)
			return err
		}
	}
	return nil
}

func readOldUserRows(ctx context.Context, client *db.DBClient, userChannel chan<- readUserRowResult) {
	defer close(userChannel)
	defer func() {
		if r := recover(); r != nil {
			logger.Warnf("Panic while reading old user rows: %v", r)
			userChannel <- readUserRowResult{err: xerrors.Errorf("panic in reading old user rows")}
		}
	}()

	s, err := client.SessionPool.Get(ctx)
	if err != nil {
		logger.Warnf("Can't get session: %v", err)
		userChannel <- readUserRowResult{err: xerrors.Errorf("can't get sessinon to read table: %w", err)}
	}

	usersTablePath := path.Join(client.Prefix, "users")
	logger.Infof("Reading table from path %s", usersTablePath)
	res, err := s.StreamReadTable(ctx, usersTablePath,
		table.ReadColumn("id"),
		table.ReadColumn("login"),
		table.ReadColumn("tuya_uid"),
	)
	if err != nil {
		logger.Infof("Failed to read table: %v", err)
		userChannel <- readUserRowResult{err: err}
		return
	}
	defer func() {
		if err := res.Close(); err != nil {
			logger.Warnf("Error while closing result set: %v", err)
		}
	}()

	if !res.NextResultSet(ctx, "id", "login", "tuya_uid") {
		logger.Infof("Failed to read table %s: %v", usersTablePath, err)
		userChannel <- readUserRowResult{err: err}
		return
	}

	for res.NextRow() {
		var u OldUser
		if err = res.ScanWithDefaults(&u.ID, &u.Login, &u.TuyaUID); err != nil {
			logger.Infof("Error while reading user from %s: %v", usersTablePath, err)
			userChannel <- readUserRowResult{User: u, err: err}
			return
		}
		logger.Infof("Read user: %+v", u)
		userChannel <- readUserRowResult{User: u}
	}
	logger.Infof("Finished reading users from %s", usersTablePath)
}

func storeUserBatchInNewTable(ctx context.Context, client *db.DBClient, huidUsers HuidUsers, storeChannel chan<- storeUserRowResults, wg *sync.WaitGroup) {
	defer wg.Done()
	defer func() {
		if r := recover(); r != nil {
			logger.Warnf("panic while storing users batch: %v", r)
			storeChannel <- storeUserRowResults{err: xerrors.Errorf("panic in storing users batch: %v", r)}
		}
	}()

	logger.Infof("Storing new users batch, length: %d", len(huidUsers))

	query := fmt.Sprintf(`
		PRAGMA TablePathPrefix("%s");

		DECLARE $users AS List<Struct<
			hid: Uint64?,
			id: Uint64?,
			login: String?,
			tuya_uid: String?>>;

		UPSERT INTO
			Users (hid, id, login, tuya_uid)
		SELECT
			hid,
			id,
			login,
			tuya_uid
		FROM AS_TABLE($users);`, client.Prefix)

	params := table.NewQueryParameters(
		table.ValueParam("$users", huidUsers.ListValue()),
	)

	if err := client.Write(ctx, query, params); err != nil {
		logger.Infof("Failed to execute upsert query for users batch: %v", err)
		storeChannel <- storeUserRowResults{Users: huidUsers, err: err}
		return
	}
	storeChannel <- storeUserRowResults{Users: huidUsers}
	logger.Infof("Finished with users batch")
}
