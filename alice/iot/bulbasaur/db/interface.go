package db

import (
	"context"

	"a.yandex-team.ru/alice/iot/bulbasaur/controller/experiments"
	"a.yandex-team.ru/alice/iot/bulbasaur/model"
	"a.yandex-team.ru/alice/library/go/timestamp"
	"a.yandex-team.ru/alice/library/go/ydbclient"
)

type TransactionType = ydbclient.TransactionType

const (
	SerializableReadWrite = ydbclient.SerializableReadWrite
	OnlineReadOnly        = ydbclient.OnlineReadOnly
	StaleReadOnly         = ydbclient.StaleReadOnly
)

type DB interface {
	Transaction(ctx context.Context, name string, f func(ctx context.Context) error) error

	SelectUserDevice(ctx context.Context, userID uint64, deviceID string) (model.Device, error)
	StoreUserDevice(ctx context.Context, user model.User, device model.Device) (model.Device, model.StoreResult, error)
	SelectUserDeviceSimple(ctx context.Context, userID uint64, deviceID string) (model.Device, error)
	SelectUserDevices(ctx context.Context, userID uint64) (model.Devices, error)
	SelectUserDevicesSimple(ctx context.Context, userID uint64) (model.Devices, error)
	SelectUserGroupDevices(ctx context.Context, userID uint64, groupID string) ([]model.Device, error)
	SelectUserHouseholdDevices(ctx context.Context, userID uint64, householdID string) (model.Devices, error)
	SelectUserProviderDevices(ctx context.Context, userID uint64, skillID string) ([]model.Device, error)
	SelectUserProviderDevicesSimple(ctx context.Context, userID uint64, skillID string) (model.Devices, error)
	SelectUserProviderArchivedDevicesSimple(ctx context.Context, userID uint64, skillID string) (map[string]model.Device, error)
	DeleteUserDevices(ctx context.Context, userID uint64, deviceIDList []string) error
	UpdateUserDeviceName(ctx context.Context, userID uint64, deviceID string, name string) error
	UpdateUserDeviceNameAndAliases(ctx context.Context, userID uint64, deviceID string, name string, aliases []string) error
	UpdateUserDeviceRoom(ctx context.Context, userID uint64, deviceID string, roomID string) error
	UpdateUserDeviceGroups(ctx context.Context, user model.User, deviceID string, groupsIDs []string) error
	UpdateUserDeviceType(ctx context.Context, userID uint64, deviceID string, newDeviceType model.DeviceType) error
	StoreUserDeviceConfigs(ctx context.Context, userID uint64, deviceConfigs model.DeviceConfigs) error

	CreateUserRoom(ctx context.Context, user model.User, room model.Room) (string, error)
	SelectUserRoom(ctx context.Context, userID uint64, roomID string) (model.Room, error)
	UpdateUserRoomName(ctx context.Context, user model.User, roomID string, name string) error
	UpdateUserRoomNameAndDevices(ctx context.Context, user model.User, room model.Room) error
	SelectUserRooms(ctx context.Context, userID uint64) ([]model.Room, error)
	SelectUserRoomDevices(ctx context.Context, userID uint64, roomID string) ([]model.Device, error)
	DeleteUserRoom(ctx context.Context, userID uint64, roomID string) error

	CreateUserGroup(ctx context.Context, user model.User, group model.Group) (string, error)
	SelectUserGroup(ctx context.Context, userID uint64, groupID string) (model.Group, error)
	SelectUserGroups(ctx context.Context, userID uint64) ([]model.Group, error)
	UpdateUserGroupName(context.Context, model.User, string, string) error
	UpdateUserGroupNameAndAliases(ctx context.Context, user model.User, groupID string, name string, aliases []string) error
	UpdateUserGroupNameAndDevices(ctx context.Context, user model.User, group model.Group) error
	DeleteUserGroup(ctx context.Context, userID uint64, groupID string) error

	SelectUser(ctx context.Context, userID uint64) (model.User, error)
	StoreUser(ctx context.Context, user model.User) error
	StoreUserWithHousehold(ctx context.Context, user model.User, household model.Household) (string, error)

	SelectAllExperiments(ctx context.Context) (experiments.Experiments, error)

	// SelectUserInfo explicitly works out of transaction and reads stale userInfo data
	SelectUserInfo(ctx context.Context, userID uint64) (model.UserInfo, error)
	SelectFavorites(ctx context.Context, user model.User) (model.Favorites, error)

	DeleteUserNetwork(ctx context.Context, userID uint64, networkID string) error
	SelectUserNetwork(ctx context.Context, userID uint64, SSID string) (model.Network, error)
	SelectUserNetworks(ctx context.Context, userID uint64) (model.Networks, error)
	StoreUserNetwork(ctx context.Context, userID uint64, network model.Network) error

	CheckUserSkillExist(ctx context.Context, userID uint64, skillID string) (bool, error)
	DeleteUserSkill(ctx context.Context, userID uint64, skillID string) error
	SelectUserSkills(ctx context.Context, userID uint64) ([]string, error)
	StoreUserSkill(ctx context.Context, userID uint64, skillID string) error

	DeleteExternalUser(ctx context.Context, skillID string, user model.User) error
	SelectExternalUsers(ctx context.Context, externalID, skillID string) ([]model.User, error)
	StoreExternalUser(ctx context.Context, externalID string, skillID string, user model.User) error

	CreateUserHousehold(ctx context.Context, userID uint64, household model.Household) (newHouseholdID string, err error)
	DeleteUserHousehold(ctx context.Context, userID uint64, householdID string) error
	SelectCurrentHousehold(ctx context.Context, userID uint64) (model.Household, error)
	SelectUserHousehold(ctx context.Context, userID uint64, ID string) (model.Household, error)
	SelectUserHouseholds(ctx context.Context, userID uint64) (model.Households, error)
	SetCurrentHouseholdForUser(ctx context.Context, userID uint64, householdID string) error
	UpdateUserHousehold(ctx context.Context, userID uint64, household model.Household) error

	MoveUserDevicesToHousehold(ctx context.Context, user model.User, deviceIDs []string, householdID string) error
	SelectUserHouseholdDevicesSimple(ctx context.Context, userID uint64, householdID string) (model.Devices, error)
	SelectUserHouseholdRooms(ctx context.Context, userID uint64, householdID string) (model.Rooms, error)
	SelectUserHouseholdGroups(ctx context.Context, userID uint64, householdID string) (model.Groups, error)

	UpdateDeviceStatuses(ctx context.Context, userID uint64, deviceStates model.DeviceStatusMap) error

	SelectDevicesSimpleByExternalIDs(ctx context.Context, externalIDs []string) (model.DevicesMapByOwnerID, error)
	DeleteStereopair(ctx context.Context, userID uint64, id string) error
	SelectDeviceTriggersIndexes(ctx context.Context, userID uint64, deviceTriggerIndexKey model.DeviceTriggerIndexKey) (model.DeviceTriggersIndexes, error)
	SelectScenario(context.Context, uint64, string) (model.Scenario, error)
	SelectUserScenarios(context.Context, uint64) (model.Scenarios, error)
	SelectUserScenariosSimple(ctx context.Context, userID uint64) (model.Scenarios, error)
	SelectStereopair(ctx context.Context, userID uint64, stereopairID string) (model.Stereopair, error)
	SelectStereopairs(ctx context.Context, userID uint64) (model.Stereopairs, error)
	SelectStereopairsSimple(ctx context.Context, userID uint64) (model.Stereopairs, error)
	StoreStereopair(ctx context.Context, userID uint64, stereopair model.Stereopair) error
	UpdateStereopairName(ctx context.Context, userID uint64, stereopairID, oldName, newName string) error
	StoreDeviceState(ctx context.Context, userID uint64, device model.Device) (model.Capabilities, model.Properties, error)
	StoreDevicesStates(ctx context.Context, userID uint64, devices []model.Device, loadDevicesForUpdate bool) (model.DeviceCapabilitiesMap, model.DevicePropertiesMap, error)
	StoreDeviceTriggersIndexes(ctx context.Context, userID uint64, scenarioTriggers map[string]model.DevicePropertyScenarioTriggers) error
	CreateScenario(ctx context.Context, userID uint64, scenario model.Scenario) (string, error)
	CreateScenarioWithLaunch(ctx context.Context, userID uint64, scenario model.Scenario, launch model.ScenarioLaunch) error
	DeleteScenario(ctx context.Context, userID uint64, scenarioID string) error
	UpdateScenario(ctx context.Context, userID uint64, scenario model.Scenario) error
	UpdateScenarioAndCreateLaunch(ctx context.Context, userID uint64, scenario model.Scenario, newLaunch model.ScenarioLaunch) error
	UpdateScenarioAndDeleteLaunches(ctx context.Context, userID uint64, scenario model.Scenario) error
	UpdateScenarios(ctx context.Context, userID uint64, scenarios model.Scenarios) error
	CancelScenarioLaunchesByTriggerTypeAndStatus(ctx context.Context, userID uint64, triggerType model.ScenarioTriggerType, status model.ScenarioLaunchStatus) error
	DeleteScenarioLaunches(ctx context.Context, userID uint64, launchIDs []string) error
	SelectScenarioLaunch(ctx context.Context, userID uint64, scenarioLaunchID string) (model.ScenarioLaunch, error)
	SelectScenarioLaunchList(ctx context.Context, userID uint64, limit uint64, triggerTypes []model.ScenarioTriggerType) (model.ScenarioLaunches, error)
	SelectScenarioLaunchesByScenarioID(ctx context.Context, userID uint64, scenarioID string) (model.ScenarioLaunches, error)
	StoreScenarioLaunch(ctx context.Context, userID uint64, scenarioLaunch model.ScenarioLaunch) (string, error)
	UpdateScenarioLaunch(ctx context.Context, userID uint64, scenarioLaunch model.ScenarioLaunch) error
	UpdateScenarioLaunchScheduledTime(ctx context.Context, userID uint64, launchID string, newScheduled timestamp.PastTimestamp) error
	SelectScheduledScenarioLaunches(ctx context.Context, userID uint64) (model.ScenarioLaunches, error)

	StoreFavoriteScenarios(ctx context.Context, user model.User, scenarios model.Scenarios) error
	StoreFavoriteDevices(ctx context.Context, user model.User, devices model.Devices) error
	StoreFavoriteGroups(ctx context.Context, user model.User, groups model.Groups) error
	StoreFavoriteProperties(ctx context.Context, user model.User, properties model.FavoritesDeviceProperties) error
	DeleteFavoriteScenarios(ctx context.Context, user model.User, scenarios model.Scenarios) error
	DeleteFavoriteDevices(ctx context.Context, user model.User, devices model.Devices) error
	DeleteFavoriteGroups(ctx context.Context, user model.User, groups model.Groups) error
	DeleteFavoriteProperties(ctx context.Context, user model.User, properties model.FavoritesDeviceProperties) error
	ReplaceFavoriteDevices(ctx context.Context, user model.User, devices model.Devices) error
	ReplaceFavoriteGroups(ctx context.Context, user model.User, groups model.Groups) error
	ReplaceFavoriteScenarios(ctx context.Context, user model.User, scenarios model.Scenarios) error
	ReplaceFavoriteProperties(ctx context.Context, user model.User, properties model.FavoritesDeviceProperties) error

	DeleteUserStorageConfig(ctx context.Context, user model.User) error
	// SelectUserStorageConfig reads stale data about user storage configs
	SelectUserStorageConfig(ctx context.Context, user model.User) (model.UserStorageConfig, error)
	StoreUserStorageConfig(ctx context.Context, user model.User, config model.UserStorageConfig) error

	SelectUserIntentState(ctx context.Context, userID uint64, intentStateKey model.IntentStateKey) (model.IntentState, error)
	StoreUserIntentState(ctx context.Context, userID uint64, intentStateKey model.IntentStateKey, intentState model.IntentState) error

	StoreSharedHousehold(ctx context.Context, guestID uint64, sharingInfo model.SharingInfo) error
	DeleteSharedHousehold(ctx context.Context, ownerID uint64, guestID uint64, householdID string) error
	RenameSharedHousehold(ctx context.Context, guestID uint64, householdID string, householdName string) error
	SelectGuestSharingInfos(ctx context.Context, guestID uint64) (model.SharingInfos, error)
	SelectHouseholdResidents(ctx context.Context, userID uint64, household model.Household) (model.HouseholdResidents, error)
	StoreHouseholdSharingLink(ctx context.Context, link model.HouseholdSharingLink) error
	SelectHouseholdSharingLinkByID(ctx context.Context, linkID string) (model.HouseholdSharingLink, error)
	SelectHouseholdSharingLink(ctx context.Context, senderID uint64, householdID string) (model.HouseholdSharingLink, error)
	DeleteHouseholdSharingLinks(ctx context.Context, senderID uint64, householdID string) error
	SelectHouseholdInvitationsByGuest(ctx context.Context, guestID uint64) (model.HouseholdInvitations, error)
	SelectHouseholdInvitationsBySender(ctx context.Context, senderID uint64) (model.HouseholdInvitations, error)
	SelectHouseholdInvitationByID(ctx context.Context, ID string) (model.HouseholdInvitation, error)
	DeleteHouseholdInvitation(ctx context.Context, invitation model.HouseholdInvitation) error
	StoreHouseholdInvitation(ctx context.Context, invitation model.HouseholdInvitation) error

	IsDatabaseError(err error) bool
	WithoutTransaction(ctx context.Context) context.Context
}

type DBClientMock struct {
	DeleteStereopairMock                        func(ctx context.Context, userID uint64, id string) error
	SelectUserMock                              func(ctx context.Context, userID uint64) (model.User, error)
	SelectExternalUsersMock                     func(ctx context.Context, externalID, skillID string) ([]model.User, error)
	SelectUserDevicesMock                       func(ctx context.Context, uid uint64) ([]model.Device, error)
	SelectUserProviderDevicesSimpleMock         func(ctx context.Context, uid uint64, skillID string) (model.Devices, error)
	SelectUserProviderArchivedDevicesSimpleMock func(ctx context.Context, userID uint64, skillID string) (map[string]model.Device, error)
	SelectUserDeviceMock                        func(ctx context.Context, uid uint64, deviceID string) (model.Device, error)
	StoreDeviceStateMock                        func(ctx context.Context, uid uint64, device model.Device) error
	StoreDeviceStatesMock                       func(ctx context.Context, userID uint64, devices []model.Device, loadDevicesForUpdate bool) error
	SelectUserInfoMock                          func(ctx context.Context, userID uint64) (model.UserInfo, error)
	SelectStereopairMock                        func(ctx context.Context, userID uint64, stereopairID string) (model.Stereopair, error)
	SelectStereopairsMock                       func(ctx context.Context, userID uint64) (model.Stereopairs, error)
	IsDatabaseErrorMock                         func(err error) bool
	SelectDevicesSimpleByExternalIDsMock        func(ctx context.Context, externalIDs []string) (model.DevicesMapByOwnerID, error)

	timestamper timestamp.ITimestamper
}

func (db *DBClientMock) IsDatabaseError(err error) bool {
	if db.IsDatabaseErrorMock != nil {
		return db.IsDatabaseErrorMock(err)
	}
	return false
}

func (db *DBClientMock) StoreExternalUser(context.Context, string, string, model.User) error {
	return nil
}

func (db *DBClientMock) SelectExternalUsers(ctx context.Context, externalID, skillID string) ([]model.User, error) {
	if db.SelectExternalUsersMock != nil {
		return db.SelectExternalUsersMock(ctx, externalID, skillID)
	}
	return []model.User{}, nil
}

func (db *DBClientMock) DeleteExternalUser(context.Context, string, model.User) error {
	return nil
}

func (db *DBClientMock) SelectUserDevicesSimple(ctx context.Context, uid uint64) (model.Devices, error) {
	var result []model.Device
	if db.SelectUserDevicesMock != nil {
		return db.SelectUserDevicesMock(ctx, uid)
	}
	return result, nil
}

func (db *DBClientMock) SelectUserDevices(ctx context.Context, uid uint64) (model.Devices, error) {
	var result []model.Device
	if db.SelectUserDevicesMock != nil {
		return db.SelectUserDevicesMock(ctx, uid)
	}
	return result, nil
}

func (db *DBClientMock) SelectUserProviderDevicesSimple(ctx context.Context, uid uint64, skillID string) (model.Devices, error) {
	if db.SelectUserProviderDevicesSimpleMock != nil {
		return db.SelectUserProviderDevicesSimpleMock(ctx, uid, skillID)
	}
	return nil, nil
}

func (db *DBClientMock) SelectDevicesSimpleByExternalIDs(ctx context.Context, externalIDs []string) (model.DevicesMapByOwnerID, error) {
	return db.SelectDevicesSimpleByExternalIDsMock(ctx, externalIDs)
}

func (db *DBClientMock) SelectUserProviderDevices(ctx context.Context, userID uint64, skillID string) ([]model.Device, error) {
	return nil, nil
}

func (db *DBClientMock) SelectUserProviderArchivedDevicesSimple(ctx context.Context, userID uint64, skillID string) (map[string]model.Device, error) {
	if db.SelectUserProviderArchivedDevicesSimpleMock != nil {
		return db.SelectUserProviderArchivedDevicesSimpleMock(ctx, userID, skillID)
	}
	return nil, nil
}

func (db *DBClientMock) SelectUserGroupDevices(ctx context.Context, uid uint64, groupID string) ([]model.Device, error) {
	var result []model.Device
	return result, nil
}

func (db *DBClientMock) SelectUserDevice(ctx context.Context, uid uint64, deviceID string) (model.Device, error) {
	var result model.Device
	if db.SelectUserDeviceMock != nil {
		return db.SelectUserDeviceMock(ctx, uid, deviceID)
	}
	return result, nil
}

func (db *DBClientMock) SelectUserDeviceSimple(ctx context.Context, uid uint64, deviceID string) (model.Device, error) {
	var result model.Device
	if db.SelectUserDeviceMock != nil {
		return db.SelectUserDeviceMock(ctx, uid, deviceID)
	}
	return result, nil
}

func (db *DBClientMock) SelectUserRoom(ctx context.Context, uid uint64, roomID string) (model.Room, error) {
	var result model.Room
	return result, nil
}

func (db *DBClientMock) SelectUserGroup(ctx context.Context, uid uint64, groupID string) (model.Group, error) {
	var result model.Group
	return result, nil
}

func (db *DBClientMock) SelectUserRooms(ctx context.Context, uid uint64) ([]model.Room, error) {
	var result []model.Room
	return result, nil
}

func (db *DBClientMock) SelectUserGroups(ctx context.Context, uid uint64) ([]model.Group, error) {
	var result []model.Group
	return result, nil
}

func (db *DBClientMock) StoreUserDevice(ctx context.Context, user model.User, device model.Device) (model.Device, model.StoreResult, error) {
	return model.Device{}, "", nil
}

func (db *DBClientMock) StoreDeviceState(ctx context.Context, userID uint64, device model.Device) (model.Capabilities, model.Properties, error) {
	if db.StoreDeviceStateMock != nil {
		return nil, nil, db.StoreDeviceStateMock(ctx, userID, device)
	}
	return nil, nil, nil
}

func (db *DBClientMock) StoreDeviceTriggersIndexes(ctx context.Context, userID uint64, scenarioTriggers map[string]model.DevicePropertyScenarioTriggers) error {
	return nil
}

func (db *DBClientMock) StoreDevicesStates(ctx context.Context, userID uint64, devices []model.Device, loadDevicesForUpdate bool) (model.DeviceCapabilitiesMap, model.DevicePropertiesMap, error) {
	if db.StoreDeviceStatesMock != nil {
		return nil, nil, db.StoreDeviceStatesMock(ctx, userID, devices, loadDevicesForUpdate)
	}
	return nil, nil, nil
}

func (db *DBClientMock) Transaction(ctx context.Context, name string, f func(ctx context.Context) error) error {
	return f(ctx)
}

func (db *DBClientMock) UpdateDeviceStatuses(ctx context.Context, userID uint64, deviceStates model.DeviceStatusMap) error {
	return nil
}

func (db *DBClientMock) UpdateUserRoomName(ctx context.Context, user model.User, roomID string, name string) error {
	return nil
}

func (db *DBClientMock) UpdateUserGroupName(ctx context.Context, user model.User, groupID string, name string) error {
	return nil
}

func (db *DBClientMock) UpdateUserGroupNameAndAliases(ctx context.Context, user model.User, groupID string, name string, aliases []string) error {
	return nil
}

func (db *DBClientMock) DeleteUserRoom(ctx context.Context, userID uint64, roomID string) error {
	return nil
}

func (db *DBClientMock) DeleteUserGroup(ctx context.Context, userID uint64, groupID string) error {
	return nil
}

func (db *DBClientMock) DeleteStereopair(ctx context.Context, userID uint64, id string) error {
	if db.DeleteStereopairMock == nil {
		return nil
	}
	return db.DeleteStereopairMock(ctx, userID, id)
}

func (db *DBClientMock) DeleteUserDevices(ctx context.Context, userID uint64, deviceIDList []string) error {
	return nil
}

func (db *DBClientMock) UpdateUserDeviceName(ctx context.Context, userID uint64, deviceID string, name string) error {
	return nil
}

func (db *DBClientMock) UpdateUserDeviceNameAndAliases(ctx context.Context, userID uint64, deviceID string, name string, aliases []string) error {
	return nil
}

func (db *DBClientMock) UpdateUserDeviceRoom(ctx context.Context, userID uint64, deviceID string, roomID string) error {
	return nil
}

func (db *DBClientMock) UpdateUserDeviceType(ctx context.Context, userID uint64, deviceID string, newDeviceType model.DeviceType) error {
	return nil
}

func (db *DBClientMock) StoreUserDeviceConfigs(ctx context.Context, userID uint64, deviceConfigs model.DeviceConfigs) error {
	return nil
}

func (db *DBClientMock) UpdateUserDeviceGroups(ctx context.Context, user model.User, deviceID string, groupsIDs []string) error {
	return nil
}

func (db *DBClientMock) CreateUserRoom(ctx context.Context, user model.User, room model.Room) (string, error) {
	return "", nil
}

func (db *DBClientMock) CreateUserGroup(ctx context.Context, user model.User, group model.Group) (string, error) {
	return "", nil
}

func (db *DBClientMock) StoreStereopair(ctx context.Context, userID uint64, stereopair model.Stereopair) error {
	return nil
}

func (db *DBClientMock) UpdateStereopairName(ctx context.Context, userID uint64, stereopairID, oldName, newName string) error {
	return nil
}

func (db *DBClientMock) StoreUser(ctx context.Context, user model.User) error {
	return nil
}

func (db *DBClientMock) StoreUserWithHousehold(ctx context.Context, user model.User, household model.Household) (string, error) {
	return "", nil
}

func (db *DBClientMock) SelectStereopair(ctx context.Context, userID uint64, stereopairID string) (model.Stereopair, error) {
	if db.SelectStereopairMock == nil {
		return model.Stereopair{}, nil
	}
	return db.SelectStereopairMock(ctx, userID, stereopairID)
}

func (db *DBClientMock) SelectStereopairs(ctx context.Context, userID uint64) (model.Stereopairs, error) {
	if db.SelectStereopairsMock == nil {
		return nil, nil
	}
	return db.SelectStereopairsMock(ctx, userID)
}

func (db *DBClientMock) SelectStereopairsSimple(ctx context.Context, userID uint64) (model.Stereopairs, error) {
	if db.SelectStereopairsMock == nil {
		return nil, nil
	}
	return db.SelectStereopairsMock(ctx, userID)
}

func (db *DBClientMock) SelectUser(ctx context.Context, userID uint64) (model.User, error) {
	if db.SelectUserMock != nil {
		return db.SelectUserMock(ctx, userID)
	}
	var user model.User
	return user, nil
}

func (db *DBClientMock) SelectUserScenarios(ctx context.Context, userID uint64) (model.Scenarios, error) {
	var result []model.Scenario
	return result, nil
}

func (db *DBClientMock) SelectUserScenariosSimple(ctx context.Context, userID uint64) (model.Scenarios, error) {
	var result []model.Scenario
	return result, nil
}

func (db *DBClientMock) SelectScenario(ctx context.Context, userID uint64, scenarioID string) (model.Scenario, error) {
	var result model.Scenario
	return result, nil
}

func (db *DBClientMock) CreateScenario(ctx context.Context, userID uint64, scenario model.Scenario) (string, error) {
	return "", nil
}

func (db *DBClientMock) UpdateScenario(ctx context.Context, userID uint64, scenario model.Scenario) error {
	return nil
}

func (db *DBClientMock) UpdateScenarios(context.Context, uint64, model.Scenarios) error {
	return nil
}

func (db *DBClientMock) StoreScenarioLaunch(ctx context.Context, userID uint64, scenarioLaunch model.ScenarioLaunch) (string, error) {
	return "", nil
}

func (db *DBClientMock) SelectScenarioLaunchList(ctx context.Context, userID uint64, limit uint64, triggerTypes []model.ScenarioTriggerType) (model.ScenarioLaunches, error) {
	return nil, nil
}

func (db *DBClientMock) SelectScheduledScenarioLaunches(ctx context.Context, userID uint64) (model.ScenarioLaunches, error) {
	return nil, nil
}

func (db *DBClientMock) SelectScenarioLaunchesByScenarioID(ctx context.Context, userID uint64, scenarioID string) (model.ScenarioLaunches, error) {
	return nil, nil
}

func (db *DBClientMock) SelectScenarioLaunch(ctx context.Context, userID uint64, scenarioLaunchID string) (model.ScenarioLaunch, error) {
	return model.ScenarioLaunch{}, nil
}

func (db *DBClientMock) UpdateScenarioLaunch(ctx context.Context, userID uint64, scenarioLaunch model.ScenarioLaunch) error {
	return nil
}

func (db *DBClientMock) UpdateScenarioLaunchScheduledTime(ctx context.Context, userID uint64, launchID string, newScheduled timestamp.PastTimestamp) error {
	return nil
}

func (db *DBClientMock) DeleteScenarioLaunches(ctx context.Context, userID uint64, launchIDs []string) error {
	return nil
}

func (db *DBClientMock) CreateScenarioWithLaunch(context.Context, uint64, model.Scenario, model.ScenarioLaunch) error {
	return nil
}

func (db *DBClientMock) UpdateScenarioAndCreateLaunch(context.Context, uint64, model.Scenario, model.ScenarioLaunch) error {
	return nil
}

func (db *DBClientMock) UpdateScenarioAndDeleteLaunches(context.Context, uint64, model.Scenario) error {
	return nil
}

func (db *DBClientMock) DeleteScenario(context.Context, uint64, string) error {
	return nil
}

func (db *DBClientMock) SelectUserInfo(ctx context.Context, userID uint64) (model.UserInfo, error) {
	if db.SelectUserInfoMock != nil {
		return db.SelectUserInfoMock(ctx, userID)
	}
	var result model.UserInfo
	return result, nil
}

func (db *DBClientMock) SelectUserSkills(ctx context.Context, userID uint64) ([]string, error) {
	var result []string
	return result, nil
}

func (db *DBClientMock) CheckUserSkillExist(ctx context.Context, userID uint64, skillID string) (bool, error) {
	return false, nil
}

func (db *DBClientMock) StoreUserSkill(ctx context.Context, userID uint64, skillID string) error {
	return nil
}

func (db *DBClientMock) DeleteUserSkill(ctx context.Context, userID uint64, skillID string) error {
	return nil
}

func (db *DBClientMock) CurrentTimestamp() timestamp.PastTimestamp {
	if db.timestamper == nil {
		db.timestamper = timestamp.NewMockTimestamper()
	}
	return db.timestamper.CurrentTimestamp()
}

func (db *DBClientMock) SetTimestamper(timestamper timestamp.ITimestamper) {
	db.timestamper = timestamper
}

func (db *DBClientMock) SelectUserNetworks(ctx context.Context, userID uint64) (model.Networks, error) {
	var networks model.Networks
	return networks, nil
}

func (db *DBClientMock) SelectUserNetwork(ctx context.Context, userID uint64, SSID string) (model.Network, error) {
	return model.Network{}, nil
}

func (db *DBClientMock) StoreUserNetwork(ctx context.Context, userID uint64, network model.Network) error {
	return nil
}

func (db *DBClientMock) DeleteUserNetwork(ctx context.Context, userID uint64, networkID string) error {
	return nil
}

func (db *DBClientMock) SelectAllExperiments(ctx context.Context) (experiments.Experiments, error) {
	return experiments.Experiments{}, nil
}

func (db *DBClientMock) SelectUserHouseholds(ctx context.Context, userID uint64) (model.Households, error) {
	return nil, nil
}

func (db *DBClientMock) SelectUserHousehold(ctx context.Context, userID uint64, ID string) (model.Household, error) {
	return model.Household{}, nil
}

func (db *DBClientMock) CreateUserHousehold(ctx context.Context, userID uint64, household model.Household) (newHouseholdID string, err error) {
	return "", nil
}

func (db *DBClientMock) SelectUserHouseholdDevices(ctx context.Context, userID uint64, householdID string) (model.Devices, error) {
	return nil, nil
}

func (db *DBClientMock) SelectUserHouseholdDevicesSimple(ctx context.Context, userID uint64, householdID string) (model.Devices, error) {
	return nil, nil
}

func (db *DBClientMock) SelectUserHouseholdGroups(ctx context.Context, userID uint64, householdID string) (model.Groups, error) {
	return nil, nil
}

func (db *DBClientMock) SelectUserHouseholdRooms(ctx context.Context, userID uint64, householdID string) (model.Rooms, error) {
	return nil, nil
}

func (db *DBClientMock) DeleteUserHousehold(ctx context.Context, userID uint64, householdID string) error {
	return nil
}

func (db *DBClientMock) SelectCurrentHousehold(ctx context.Context, userID uint64) (model.Household, error) {
	return model.Household{}, nil
}

func (db *DBClientMock) SetCurrentHouseholdForUser(ctx context.Context, userID uint64, householdID string) error {
	return nil
}

func (db *DBClientMock) SelectDeviceTriggersIndexes(ctx context.Context, userID uint64, deviceTriggerIndexKey model.DeviceTriggerIndexKey) (model.DeviceTriggersIndexes, error) {
	return nil, nil
}

func (db *DBClientMock) UpdateUserHousehold(ctx context.Context, userID uint64, household model.Household) error {
	return nil
}

func (db *DBClientMock) MoveUserDevicesToHousehold(ctx context.Context, user model.User, deviceIDs []string, householdID string) error {
	return nil
}

func (db *DBClientMock) UpdateUserGroupNameAndDevices(ctx context.Context, user model.User, group model.Group) error {
	return nil
}

func (db *DBClientMock) UpdateUserRoomNameAndDevices(ctx context.Context, user model.User, room model.Room) error {
	return nil
}

func (db *DBClientMock) SelectUserRoomDevices(ctx context.Context, userID uint64, groupID string) ([]model.Device, error) {
	return nil, nil
}

func (db *DBClientMock) DevicePropertyHistory(ctx context.Context, userID uint64, deviceID string, propertyType model.PropertyType, instance model.PropertyInstance) ([]model.PropertyLogData, error) {
	return nil, nil
}

func (db *DBClientMock) SelectFavorites(ctx context.Context, user model.User) (model.Favorites, error) {
	return model.Favorites{}, nil
}

func (db *DBClientMock) StoreFavoriteScenarios(ctx context.Context, user model.User, scenarios model.Scenarios) error {
	return nil
}

func (db *DBClientMock) DeleteFavoriteScenarios(ctx context.Context, user model.User, scenarios model.Scenarios) error {
	return nil
}

func (db *DBClientMock) StoreFavoriteDevices(ctx context.Context, user model.User, devices model.Devices) error {
	return nil
}

func (db *DBClientMock) DeleteFavoriteDevices(ctx context.Context, user model.User, devices model.Devices) error {
	return nil
}

func (db *DBClientMock) StoreFavoriteGroups(ctx context.Context, user model.User, groups model.Groups) error {
	return nil
}

func (db *DBClientMock) DeleteFavoriteGroups(ctx context.Context, user model.User, groups model.Groups) error {
	return nil
}

func (db *DBClientMock) StoreFavoriteProperties(ctx context.Context, user model.User, properties model.FavoritesDeviceProperties) error {
	return nil
}

func (db *DBClientMock) DeleteFavoriteProperties(ctx context.Context, user model.User, properties model.FavoritesDeviceProperties) error {
	return nil
}

func (db *DBClientMock) ReplaceFavoriteScenarios(ctx context.Context, user model.User, scenarios model.Scenarios) error {
	return nil
}

func (db *DBClientMock) ReplaceFavoriteDevices(ctx context.Context, user model.User, devices model.Devices) error {
	return nil
}

func (db *DBClientMock) ReplaceFavoriteProperties(ctx context.Context, user model.User, properties model.FavoritesDeviceProperties) error {
	return nil
}

func (db *DBClientMock) ReplaceFavoriteGroups(ctx context.Context, user model.User, groups model.Groups) error {
	return nil
}

func (db *DBClientMock) StoreUserStorageConfig(ctx context.Context, user model.User, config model.UserStorageConfig) error {
	return nil
}

func (db *DBClientMock) SelectUserStorageConfig(ctx context.Context, user model.User) (model.UserStorageConfig, error) {
	return nil, nil
}

func (db *DBClientMock) DeleteUserStorageConfig(ctx context.Context, user model.User) error {
	return nil
}

func (db *DBClientMock) deleteFavoritesByType(ctx context.Context, user model.User, favoriteType model.FavoriteType) error {
	return nil
}

func (db *DBClientMock) upsertUserStorageConfig(ctx context.Context, user model.User, config model.UserStorageConfig) error {
	return nil
}

func (db *DBClientMock) CancelScenarioLaunchesByTriggerTypeAndStatus(ctx context.Context, userID uint64, triggerType model.ScenarioTriggerType, status model.ScenarioLaunchStatus) error {
	return nil
}

func (db *DBClientMock) SelectUserIntentState(ctx context.Context, userID uint64, intentStateKey model.IntentStateKey) (model.IntentState, error) {
	return nil, nil
}

func (db *DBClientMock) StoreUserIntentState(ctx context.Context, userID uint64, intentStateKey model.IntentStateKey, intentState model.IntentState) error {
	return nil
}

func (db *DBClientMock) StoreSharedHousehold(ctx context.Context, guestID uint64, sharingInfo model.SharingInfo) error {
	return nil
}

func (db *DBClientMock) DeleteSharedHousehold(ctx context.Context, ownerID uint64, guestID uint64, householdID string) error {
	return nil
}

func (db *DBClientMock) RenameSharedHousehold(ctx context.Context, guestID uint64, householdID string, householdName string) error {
	return nil
}

func (db *DBClientMock) SelectGuestSharingInfos(ctx context.Context, guestID uint64) (model.SharingInfos, error) {
	return nil, nil
}

func (db *DBClientMock) SelectHouseholdResidents(ctx context.Context, userID uint64, household model.Household) (model.HouseholdResidents, error) {
	return nil, nil
}

func (db *DBClientMock) StoreHouseholdSharingLink(ctx context.Context, link model.HouseholdSharingLink) error {
	return nil
}

func (db *DBClientMock) SelectHouseholdSharingLinkByID(ctx context.Context, linkID string) (model.HouseholdSharingLink, error) {
	return model.HouseholdSharingLink{}, nil
}

func (db *DBClientMock) SelectHouseholdSharingLink(ctx context.Context, senderID uint64, householdID string) (model.HouseholdSharingLink, error) {
	return model.HouseholdSharingLink{}, nil
}

func (db *DBClientMock) DeleteHouseholdSharingLinks(ctx context.Context, senderID uint64, householdID string) error {
	return nil
}

func (db *DBClientMock) SelectHouseholdInvitationsByGuest(ctx context.Context, guestID uint64) (model.HouseholdInvitations, error) {
	return nil, nil
}

func (db *DBClientMock) SelectHouseholdInvitationsBySender(ctx context.Context, senderID uint64) (model.HouseholdInvitations, error) {
	return nil, nil
}

func (db *DBClientMock) SelectHouseholdInvitationByID(ctx context.Context, ID string) (model.HouseholdInvitation, error) {
	return model.HouseholdInvitation{}, nil
}

func (db *DBClientMock) DeleteHouseholdInvitation(ctx context.Context, invitation model.HouseholdInvitation) error {
	return nil
}

func (db *DBClientMock) StoreHouseholdInvitation(ctx context.Context, invitation model.HouseholdInvitation) error {
	return nil
}

func (db *DBClientMock) WithoutTransaction(ctx context.Context) context.Context {
	return ctx
}
