package main

import (
	"context"
	"path"

	"a.yandex-team.ru/alice/iot/bulbasaur/db"
	"a.yandex-team.ru/kikimr/public/sdk/go/ydb/table"
)

// plan
// stream all xiaomi users
// check in M regions
// inside checker there is K goroutines limit
// after getting devices we want to get supported with types, supported categories with no capabilities and unsupported categories

type DBClient struct {
	*db.DBClient
}

func (db *DBClient) GetSkillUserIDs(ctx context.Context, skillID string) []uint64 {
	var usersIDs []uint64

	usersMap := make(map[uint64]struct{})
	s, err := db.SessionPool.Get(ctx)
	if err != nil {
		db.Logger.Warnf("Can't get session: %v", err)
	}

	devicesTablePath := path.Join(db.Prefix, "Devices")
	db.Logger.Infof("Reading UserSkills table from path %q", devicesTablePath)

	res, err := s.StreamReadTable(ctx, devicesTablePath,
		table.ReadColumn("user_id"),
		table.ReadColumn("skill_id"),
		table.ReadColumn("archived"),
	)
	if err != nil {
		db.Logger.Warnf("Failed to read table: %v", err)
	}

	defer func() {
		if err := res.Close(); err != nil {
			db.Logger.Warnf("Error while closing regionInfo set: %v", err)
			return
		}
	}()

	var skillUsersCount int
	for res.NextStreamSet(ctx) {
		for res.NextRow() {
			var (
				userID      uint64
				userSkillID string
				archived    bool
			)

			res.SeekItem("user_id")
			userID = res.OUint64()
			res.SeekItem("skill_id")
			userSkillID = string(res.OString())
			res.SeekItem("archived")
			archived = res.OBool()
			if err := res.Err(); err != nil {
				db.Logger.Warnf("%v", err)
				break
			}
			if _, isKnown := usersMap[userID]; !isKnown && !archived && userSkillID == skillID {
				usersMap[userID] = struct{}{}
				skillUsersCount++
				usersIDs = append(usersIDs, userID)
			}
		}
	}

	db.Logger.Infof("Finished reading %d users from %s", skillUsersCount, devicesTablePath)

	return usersIDs
}
