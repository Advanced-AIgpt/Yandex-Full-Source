package main

import (
	"context"
	"path"
	"strconv"

	"golang.org/x/exp/slices"

	"a.yandex-team.ru/alice/iot/bulbasaur/model"
	"a.yandex-team.ru/alice/library/go/ydbclient"
	"a.yandex-team.ru/kikimr/public/sdk/go/ydb/table"
)

type DBDataSource struct {
	DB      *ydbclient.YDBClient
	Prefix  string
	SkillID string
}

func (ds *DBDataSource) StreamUsersID(ctx context.Context, usersCh chan string) {
	if slices.Contains(model.KnownInternalProviders, ds.SkillID) {
		// for internal provider - we can callback discovery for user that yet not exist in ExternalUsers table
		ds.streamUsersIDInternalProviders(ctx, usersCh)
	} else {
		ds.streamUsersID(ctx, usersCh)
	}
}

func (ds *DBDataSource) streamUsersIDInternalProviders(ctx context.Context, usersCh chan string) {
	s, err := ds.DB.SessionPool.Get(ctx)
	if err != nil {
		ds.DB.Logger.Warnf("Can't get session: %v", err)
		return
	}
	devicesTablePath := path.Join(ds.Prefix, "Devices")
	ds.DB.Logger.Infof("Reading Devices table from path %q", devicesTablePath)
	res, err := s.StreamReadTable(ctx, devicesTablePath,
		table.ReadColumn("user_id"),
		table.ReadColumn("skill_id"),
		table.ReadColumn("archived"),
	)
	if err != nil {
		ds.DB.Logger.Warnf("Failed to read ExternalUser table: %v", err)
		return
	}
	defer func() {
		if err := res.Close(); err != nil {
			ds.DB.Logger.Warnf("Error while closing result set: %v", err)
			return
		}
	}()

	usersMap := make(map[uint64]struct{})
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
				ds.DB.Logger.Warn(err.Error())
				return
			}
			if _, isKnown := usersMap[userID]; !archived && !isKnown && userSkillID == ds.SkillID {
				usersMap[userID] = struct{}{}
			}
		}
	}
	ds.DB.Logger.Warnf("Finished reading %d users from %s", len(usersMap), devicesTablePath)

	for user := range usersMap {
		usersCh <- strconv.FormatUint(user, 10)
	}
}

func (ds *DBDataSource) streamUsersID(ctx context.Context, usersCh chan string) {
	s, err := ds.DB.SessionPool.Get(ctx)
	if err != nil {
		ds.DB.Logger.Warnf("Can't get session: %v", err)
		return
	}
	externalUsersTablePath := path.Join(ds.Prefix, "ExternalUser")
	ds.DB.Logger.Infof("Reading ExternalUser table from path %q", externalUsersTablePath)
	res, err := s.StreamReadTable(ctx, externalUsersTablePath,
		table.ReadColumn("external_id"),
		table.ReadColumn("user_id"),
		table.ReadColumn("skill_id"),
	)
	if err != nil {
		ds.DB.Logger.Warnf("Failed to read ExternalUser table: %v", err)
		return
	}
	defer func() {
		if err := res.Close(); err != nil {
			ds.DB.Logger.Warnf("Error while closing result set: %v", err)
			return
		}
	}()

	externalUsersMap := make(map[string]struct{})
	for res.NextStreamSet(ctx) {
		for res.NextRow() {
			var (
				externalUserID string
				userSkillID    string
			)
			res.SeekItem("external_id")
			externalUserID = string(res.OString())
			res.SeekItem("skill_id")
			userSkillID = string(res.OString())
			if err := res.Err(); err != nil {
				ds.DB.Logger.Warn(err.Error())
				return
			}
			if _, isKnown := externalUsersMap[externalUserID]; !isKnown && userSkillID == ds.SkillID {
				externalUsersMap[externalUserID] = struct{}{}
			}
		}
	}
	ds.DB.Logger.Infof("Finished reading %d users from %s", len(externalUsersMap), externalUsersTablePath)

	for user := range externalUsersMap {
		usersCh <- user
	}
}
