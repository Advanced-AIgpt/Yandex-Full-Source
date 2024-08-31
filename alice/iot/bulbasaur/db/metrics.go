package db

import (
	"context"
	"sync"
	"time"

	"a.yandex-team.ru/alice/iot/bulbasaur/controller/experiments"
	"a.yandex-team.ru/alice/iot/bulbasaur/model"
	quasarmetrics "a.yandex-team.ru/alice/library/go/metrics"
	"a.yandex-team.ru/alice/library/go/timestamp"
	"a.yandex-team.ru/library/go/core/metrics"
)

type ClientWithMetrics struct {
	dbClient *DBClient

	selectUserDevices                            quasarmetrics.YDBSignals
	selectUserDevicesSimple                      quasarmetrics.YDBSignals
	selectDevicesSimpleByExternalIDs             quasarmetrics.YDBSignals
	selectUserProviderDevices                    quasarmetrics.YDBSignals
	selectUserProviderDevicesSimple              quasarmetrics.YDBSignals
	selectUserProviderArchivedDevicesSimple      quasarmetrics.YDBSignals
	selectUserGroupDevices                       quasarmetrics.YDBSignals
	selectUserDevice                             quasarmetrics.YDBSignals
	selectUserDeviceSimple                       quasarmetrics.YDBSignals
	selectUserRoom                               quasarmetrics.YDBSignals
	selectUserGroup                              quasarmetrics.YDBSignals
	selectUserRooms                              quasarmetrics.YDBSignals
	selectUserGroups                             quasarmetrics.YDBSignals
	storeUserDevice                              quasarmetrics.YDBSignals
	storeDeviceState                             quasarmetrics.YDBSignals
	storeDevicesStates                           quasarmetrics.YDBSignals
	storeDevicesTriggerIndexes                   quasarmetrics.YDBSignals
	updateDeviceStatuses                         quasarmetrics.YDBSignals
	updateUserRoomName                           quasarmetrics.YDBSignals
	updateUserGroupName                          quasarmetrics.YDBSignals
	updateUserGroupNameAndAliases                quasarmetrics.YDBSignals
	deleteUserRoom                               quasarmetrics.YDBSignals
	deleteUserGroup                              quasarmetrics.YDBSignals
	deleteUserDevices                            quasarmetrics.YDBSignals
	updateUserDeviceName                         quasarmetrics.YDBSignals
	updateUserDeviceNameAndAliases               quasarmetrics.YDBSignals
	updateUserDeviceRoom                         quasarmetrics.YDBSignals
	updateUserDeviceType                         quasarmetrics.YDBSignals
	updateUserDeviceGroups                       quasarmetrics.YDBSignals
	createUserRoom                               quasarmetrics.YDBSignals
	createUserGroup                              quasarmetrics.YDBSignals
	storeUser                                    quasarmetrics.YDBSignals
	storeUserWithHousehold                       quasarmetrics.YDBSignals
	selectUser                                   quasarmetrics.YDBSignals
	storeExternalUser                            quasarmetrics.YDBSignals
	selectExternalUsers                          quasarmetrics.YDBSignals
	deleteExternalUser                           quasarmetrics.YDBSignals
	selectUserScenarios                          quasarmetrics.YDBSignals
	selectUserScenariosSimple                    quasarmetrics.YDBSignals
	storeScenarioLaunch                          quasarmetrics.YDBSignals
	selectScenarioLaunchList                     quasarmetrics.YDBSignals
	selectScheduledScenarioLaunches              quasarmetrics.YDBSignals
	selectScenarioLaunchesByScenarioID           quasarmetrics.YDBSignals
	selectScenarioLaunch                         quasarmetrics.YDBSignals
	updateScenarioLaunch                         quasarmetrics.YDBSignals
	updateScenarioLaunchScheduledTime            quasarmetrics.YDBSignals
	deleteScenarioLaunches                       quasarmetrics.YDBSignals
	createScenarioWithLaunch                     quasarmetrics.YDBSignals
	updateScenarioAndCreateLaunch                quasarmetrics.YDBSignals
	updateScenarioAndDeleteLaunches              quasarmetrics.YDBSignals
	deleteScenario                               quasarmetrics.YDBSignals
	cancelScenarioLaunchesByTriggerTypeAndStatus quasarmetrics.YDBSignals
	selectScenario                               quasarmetrics.YDBSignals
	createScenario                               quasarmetrics.YDBSignals
	updateScenario                               quasarmetrics.YDBSignals
	updateScenarios                              quasarmetrics.YDBSignals
	selectUserInfo                               quasarmetrics.YDBSignals
	selectUserSkills                             quasarmetrics.YDBSignals
	checkUserSkillExist                          quasarmetrics.YDBSignals
	storeUserSkill                               quasarmetrics.YDBSignals
	deleteUserSkill                              quasarmetrics.YDBSignals
	selectUserNetworks                           quasarmetrics.YDBSignals
	selectUserNetwork                            quasarmetrics.YDBSignals
	storeUserNetwork                             quasarmetrics.YDBSignals
	deleteUserNetwork                            quasarmetrics.YDBSignals
	selectAllExperiments                         quasarmetrics.YDBSignals
	selectUserHouseholds                         quasarmetrics.YDBSignals
	selectUserHousehold                          quasarmetrics.YDBSignals
	createUserHousehold                          quasarmetrics.YDBSignals
	selectUserHouseholdDevices                   quasarmetrics.YDBSignals
	selectUserHouseholdDevicesSimple             quasarmetrics.YDBSignals
	selectUserHouseholdGroups                    quasarmetrics.YDBSignals
	selectUserHouseholdRooms                     quasarmetrics.YDBSignals
	deleteUserHousehold                          quasarmetrics.YDBSignals
	selectCurrentHousehold                       quasarmetrics.YDBSignals
	setCurrentHouseholdForUser                   quasarmetrics.YDBSignals
	updateUserHousehold                          quasarmetrics.YDBSignals
	selectDeviceTriggersIndexes                  quasarmetrics.YDBSignals
	moveUserDevicesToHousehold                   quasarmetrics.YDBSignals
	updateUserGroupNameAndDevices                quasarmetrics.YDBSignals
	updateUserRoomNameAndDevices                 quasarmetrics.YDBSignals
	selectUserRoomDevices                        quasarmetrics.YDBSignals
	deleteStereopair                             quasarmetrics.YDBSignals
	selectStereopair                             quasarmetrics.YDBSignals
	selectStereopairs                            quasarmetrics.YDBSignals
	selectStereopairsSimple                      quasarmetrics.YDBSignals
	storeStereopair                              quasarmetrics.YDBSignals
	updateStereopairName                         quasarmetrics.YDBSignals
	selectFavorites                              quasarmetrics.YDBSignals
	storeFavoriteScenarios                       quasarmetrics.YDBSignals
	deleteFavoriteScenarios                      quasarmetrics.YDBSignals
	storeFavoriteDevices                         quasarmetrics.YDBSignals
	deleteFavoriteDevices                        quasarmetrics.YDBSignals
	storeFavoriteGroups                          quasarmetrics.YDBSignals
	deleteFavoriteGroups                         quasarmetrics.YDBSignals
	storeFavoriteProperties                      quasarmetrics.YDBSignals
	deleteFavoriteProperties                     quasarmetrics.YDBSignals
	replaceFavoriteScenarios                     quasarmetrics.YDBSignals
	replaceFavoriteDevices                       quasarmetrics.YDBSignals
	replaceFavoriteProperties                    quasarmetrics.YDBSignals
	replaceFavoriteGroups                        quasarmetrics.YDBSignals
	storeUserStorageConfig                       quasarmetrics.YDBSignals
	selectUserStorageConfig                      quasarmetrics.YDBSignals
	deleteUserStorageConfig                      quasarmetrics.YDBSignals
	storeUserDeviceConfigs                       quasarmetrics.YDBSignals
	selectUserIntentState                        quasarmetrics.YDBSignals
	storeUserIntentState                         quasarmetrics.YDBSignals
	storeSharedHousehold                         quasarmetrics.YDBSignals
	deleteSharedHousehold                        quasarmetrics.YDBSignals
	renameSharedHousehold                        quasarmetrics.YDBSignals
	selectGuestSharingInfos                      quasarmetrics.YDBSignals
	selectHouseholdResidents                     quasarmetrics.YDBSignals
	storeHouseholdSharingLink                    quasarmetrics.YDBSignals
	selectHouseholdSharingLinkByID               quasarmetrics.YDBSignals
	selectHouseholdSharingLink                   quasarmetrics.YDBSignals
	deleteHouseholdSharingLinks                  quasarmetrics.YDBSignals
	selectHouseholdInvitationsByGuest            quasarmetrics.YDBSignals
	selectHouseholdInvitationsBySender           quasarmetrics.YDBSignals
	selectHouseholdInvitationByID                quasarmetrics.YDBSignals
	deleteHouseholdInvitation                    quasarmetrics.YDBSignals
	storeHouseholdInvitation                     quasarmetrics.YDBSignals

	registry metrics.Registry
	policy   quasarmetrics.BucketsGenerationPolicy

	m                  sync.RWMutex
	transactionMetrics map[string]quasarmetrics.YDBSignals // metrics for transactions
}

func NewMetricsClientWithDB(client *DBClient, registry metrics.Registry, policy quasarmetrics.BucketsGenerationPolicy) *ClientWithMetrics {
	return &ClientWithMetrics{
		dbClient:                                     client,
		selectUserDevices:                            quasarmetrics.NewYDBSignals("selectUserDevices", registry, policy),
		selectUserDevicesSimple:                      quasarmetrics.NewYDBSignals("selectUserDevicesSimple", registry, policy),
		selectDevicesSimpleByExternalIDs:             quasarmetrics.NewYDBSignals("selectDevicesSimpleByExternalIDs", registry, policy),
		selectUserProviderDevices:                    quasarmetrics.NewYDBSignals("selectUserProviderDevices", registry, policy),
		selectUserProviderDevicesSimple:              quasarmetrics.NewYDBSignals("selectUserProviderDevicesSimple", registry, policy),
		selectUserProviderArchivedDevicesSimple:      quasarmetrics.NewYDBSignals("selectUserProviderArchivedDevicesSimple", registry, policy),
		selectUserGroupDevices:                       quasarmetrics.NewYDBSignals("selectUserGroupDevices", registry, policy),
		selectUserDevice:                             quasarmetrics.NewYDBSignals("selectUserDevice", registry, policy),
		selectUserDeviceSimple:                       quasarmetrics.NewYDBSignals("selectUserDeviceSimple", registry, policy),
		selectUserRoom:                               quasarmetrics.NewYDBSignals("selectUserRoom", registry, policy),
		selectUserGroup:                              quasarmetrics.NewYDBSignals("selectUserGroup", registry, policy),
		selectUserRooms:                              quasarmetrics.NewYDBSignals("selectUserRooms", registry, policy),
		selectUserGroups:                             quasarmetrics.NewYDBSignals("selectUserGroups", registry, policy),
		storeUserDevice:                              quasarmetrics.NewYDBSignals("storeUserDevice", registry, policy),
		storeDeviceState:                             quasarmetrics.NewYDBSignals("storeDeviceState", registry, policy),
		storeDevicesStates:                           quasarmetrics.NewYDBSignals("storeDevicesStates", registry, policy),
		updateDeviceStatuses:                         quasarmetrics.NewYDBSignals("updateDeviceStatuses", registry, policy),
		updateUserRoomName:                           quasarmetrics.NewYDBSignals("updateUserRoomName", registry, policy),
		updateUserGroupName:                          quasarmetrics.NewYDBSignals("updateUserGroupName", registry, policy),
		updateUserGroupNameAndAliases:                quasarmetrics.NewYDBSignals("updateUserGroupNameAndAliases", registry, policy),
		deleteUserRoom:                               quasarmetrics.NewYDBSignals("deleteUserRoom", registry, policy),
		deleteUserGroup:                              quasarmetrics.NewYDBSignals("deleteUserGroup", registry, policy),
		deleteUserDevices:                            quasarmetrics.NewYDBSignals("deleteUserDevices", registry, policy),
		updateUserDeviceName:                         quasarmetrics.NewYDBSignals("updateUserDeviceName", registry, policy),
		updateUserDeviceNameAndAliases:               quasarmetrics.NewYDBSignals("updateUserDeviceNameAndAliases", registry, policy),
		updateUserDeviceRoom:                         quasarmetrics.NewYDBSignals("updateUserDeviceRoom", registry, policy),
		updateUserDeviceType:                         quasarmetrics.NewYDBSignals("updateUserDeviceType", registry, policy),
		updateUserDeviceGroups:                       quasarmetrics.NewYDBSignals("updateUserDeviceGroups", registry, policy),
		createUserRoom:                               quasarmetrics.NewYDBSignals("createUserRoom", registry, policy),
		createUserGroup:                              quasarmetrics.NewYDBSignals("createUserGroup", registry, policy),
		storeUser:                                    quasarmetrics.NewYDBSignals("storeUser", registry, policy),
		storeUserWithHousehold:                       quasarmetrics.NewYDBSignals("storeUserWithHousehold", registry, policy),
		selectUser:                                   quasarmetrics.NewYDBSignals("selectUser", registry, policy),
		storeExternalUser:                            quasarmetrics.NewYDBSignals("storeExternalUser", registry, policy),
		selectExternalUsers:                          quasarmetrics.NewYDBSignals("selectExternalUsers", registry, policy),
		deleteExternalUser:                           quasarmetrics.NewYDBSignals("deleteExternalUser", registry, policy),
		selectUserScenarios:                          quasarmetrics.NewYDBSignals("selectUserScenarios", registry, policy),
		selectUserScenariosSimple:                    quasarmetrics.NewYDBSignals("selectUserScenariosSimple", registry, policy),
		storeScenarioLaunch:                          quasarmetrics.NewYDBSignals("storeScenarioLaunch", registry, policy),
		selectScenarioLaunchList:                     quasarmetrics.NewYDBSignals("selectScenarioLaunchList", registry, policy),
		selectScheduledScenarioLaunches:              quasarmetrics.NewYDBSignals("selectScheduledScenarioLaunches", registry, policy),
		selectScenarioLaunchesByScenarioID:           quasarmetrics.NewYDBSignals("selectScenarioLaunchesByScenarioID", registry, policy),
		selectScenarioLaunch:                         quasarmetrics.NewYDBSignals("selectScenarioLaunch", registry, policy),
		storeDevicesTriggerIndexes:                   quasarmetrics.NewYDBSignals("storeDevicesTriggerIndexes", registry, policy),
		updateScenarioLaunch:                         quasarmetrics.NewYDBSignals("updateScenarioLaunch", registry, policy),
		updateScenarioLaunchScheduledTime:            quasarmetrics.NewYDBSignals("updateScenarioLaunchScheduledTime", registry, policy),
		deleteScenarioLaunches:                       quasarmetrics.NewYDBSignals("deleteScenarioLaunches", registry, policy),
		cancelScenarioLaunchesByTriggerTypeAndStatus: quasarmetrics.NewYDBSignals("cancelScenarioLaunchesByTriggerTypeAndStatus", registry, policy),
		createScenarioWithLaunch:                     quasarmetrics.NewYDBSignals("createScenarioWithLaunch", registry, policy),
		updateScenarioAndCreateLaunch:                quasarmetrics.NewYDBSignals("updateScenarioAndCreateLaunch", registry, policy),
		updateScenarioAndDeleteLaunches:              quasarmetrics.NewYDBSignals("updateScenarioAndDeleteLaunches", registry, policy),
		deleteScenario:                               quasarmetrics.NewYDBSignals("deleteScenario", registry, policy),
		selectScenario:                               quasarmetrics.NewYDBSignals("selectUserScenario", registry, policy),
		createScenario:                               quasarmetrics.NewYDBSignals("createUserScenario", registry, policy),
		updateScenario:                               quasarmetrics.NewYDBSignals("updateUserScenario", registry, policy),
		updateScenarios:                              quasarmetrics.NewYDBSignals("updateUserScenarios", registry, policy),
		selectUserInfo:                               quasarmetrics.NewYDBSignals("selectUserInfo", registry, policy),
		selectUserSkills:                             quasarmetrics.NewYDBSignals("selectUserSkills", registry, policy),
		checkUserSkillExist:                          quasarmetrics.NewYDBSignals("checkUserSkillExist", registry, policy),
		storeUserSkill:                               quasarmetrics.NewYDBSignals("storeUserSkill", registry, policy),
		deleteUserSkill:                              quasarmetrics.NewYDBSignals("deleteUserSkill", registry, policy),
		selectUserNetworks:                           quasarmetrics.NewYDBSignals("selectUserNetworks", registry, policy),
		selectUserNetwork:                            quasarmetrics.NewYDBSignals("selectUserNetwork", registry, policy),
		storeUserNetwork:                             quasarmetrics.NewYDBSignals("storeUserNetwork", registry, policy),
		deleteUserNetwork:                            quasarmetrics.NewYDBSignals("deleteUserNetwork", registry, policy),
		selectAllExperiments:                         quasarmetrics.NewYDBSignals("selectAllExperiments", registry, policy),
		selectUserHouseholds:                         quasarmetrics.NewYDBSignals("selectUserHouseholds", registry, policy),
		selectUserHousehold:                          quasarmetrics.NewYDBSignals("selectUserHousehold", registry, policy),
		createUserHousehold:                          quasarmetrics.NewYDBSignals("createUserHousehold", registry, policy),
		selectUserHouseholdDevices:                   quasarmetrics.NewYDBSignals("selectUserHouseholdDevices", registry, policy),
		selectUserHouseholdDevicesSimple:             quasarmetrics.NewYDBSignals("selectUserHouseholdDevicesSimple", registry, policy),
		selectUserHouseholdGroups:                    quasarmetrics.NewYDBSignals("selectUserHouseholdGroups", registry, policy),
		selectUserHouseholdRooms:                     quasarmetrics.NewYDBSignals("selectUserHouseholdRooms", registry, policy),
		deleteUserHousehold:                          quasarmetrics.NewYDBSignals("deleteUserHousehold", registry, policy),
		selectCurrentHousehold:                       quasarmetrics.NewYDBSignals("selectCurrentHousehold", registry, policy),
		setCurrentHouseholdForUser:                   quasarmetrics.NewYDBSignals("setCurrentHouseholdForUser", registry, policy),
		updateUserHousehold:                          quasarmetrics.NewYDBSignals("updateUserHousehold", registry, policy),
		selectDeviceTriggersIndexes:                  quasarmetrics.NewYDBSignals("selectDeviceTriggersIndexes", registry, policy),
		moveUserDevicesToHousehold:                   quasarmetrics.NewYDBSignals("moveUserDevicesToHousehold", registry, policy),
		updateUserGroupNameAndDevices:                quasarmetrics.NewYDBSignals("updateUserGroupNameAndDevices", registry, policy),
		updateUserRoomNameAndDevices:                 quasarmetrics.NewYDBSignals("updateUserRoomNameAndDevices", registry, policy),
		selectUserRoomDevices:                        quasarmetrics.NewYDBSignals("selectUserRoomDevices", registry, policy),
		deleteStereopair:                             quasarmetrics.NewYDBSignals("deleteStereopair", registry, policy),
		selectStereopair:                             quasarmetrics.NewYDBSignals("selectStereopair", registry, policy),
		selectStereopairs:                            quasarmetrics.NewYDBSignals("selectStereopairs", registry, policy),
		selectStereopairsSimple:                      quasarmetrics.NewYDBSignals("selectStereopairsSimple", registry, policy),
		storeStereopair:                              quasarmetrics.NewYDBSignals("storeStereopair", registry, policy),
		updateStereopairName:                         quasarmetrics.NewYDBSignals("updateStereopairName", registry, policy),
		selectFavorites:                              quasarmetrics.NewYDBSignals("selectFavorites", registry, policy),
		storeFavoriteScenarios:                       quasarmetrics.NewYDBSignals("storeFavoriteScenarios", registry, policy),
		deleteFavoriteScenarios:                      quasarmetrics.NewYDBSignals("deleteFavoriteScenarios", registry, policy),
		storeFavoriteDevices:                         quasarmetrics.NewYDBSignals("storeFavoriteDevices", registry, policy),
		deleteFavoriteDevices:                        quasarmetrics.NewYDBSignals("deleteFavoriteDevices", registry, policy),
		storeFavoriteGroups:                          quasarmetrics.NewYDBSignals("storeFavoriteGroups", registry, policy),
		deleteFavoriteGroups:                         quasarmetrics.NewYDBSignals("deleteFavoriteGroups", registry, policy),
		storeFavoriteProperties:                      quasarmetrics.NewYDBSignals("storeFavoriteProperties", registry, policy),
		deleteFavoriteProperties:                     quasarmetrics.NewYDBSignals("deleteFavoriteProperties", registry, policy),
		replaceFavoriteScenarios:                     quasarmetrics.NewYDBSignals("replaceFavoriteScenarios", registry, policy),
		replaceFavoriteDevices:                       quasarmetrics.NewYDBSignals("replaceFavoriteDevices", registry, policy),
		replaceFavoriteProperties:                    quasarmetrics.NewYDBSignals("replaceFavoriteProperties", registry, policy),
		replaceFavoriteGroups:                        quasarmetrics.NewYDBSignals("replaceFavoriteGroups", registry, policy),
		storeUserStorageConfig:                       quasarmetrics.NewYDBSignals("storeUserStorageConfig", registry, policy),
		selectUserStorageConfig:                      quasarmetrics.NewYDBSignals("selectUserStorageConfig", registry, policy),
		deleteUserStorageConfig:                      quasarmetrics.NewYDBSignals("deleteUserStorageConfig", registry, policy),
		storeUserDeviceConfigs:                       quasarmetrics.NewYDBSignals("storeUserDeviceConfigs", registry, policy),
		selectUserIntentState:                        quasarmetrics.NewYDBSignals("selectUserIntentState", registry, policy),
		storeUserIntentState:                         quasarmetrics.NewYDBSignals("storeUserIntentState", registry, policy),
		storeSharedHousehold:                         quasarmetrics.NewYDBSignals("storeSharedHousehold", registry, policy),
		deleteSharedHousehold:                        quasarmetrics.NewYDBSignals("deleteSharedHousehold", registry, policy),
		renameSharedHousehold:                        quasarmetrics.NewYDBSignals("renameSharedHousehold", registry, policy),
		selectGuestSharingInfos:                      quasarmetrics.NewYDBSignals("selectGuestSharingInfos", registry, policy),
		selectHouseholdResidents:                     quasarmetrics.NewYDBSignals("selectHouseholdResidents", registry, policy),
		storeHouseholdSharingLink:                    quasarmetrics.NewYDBSignals("storeHouseholdSharingLink", registry, policy),
		selectHouseholdSharingLinkByID:               quasarmetrics.NewYDBSignals("selectHouseholdSharingLinkByID", registry, policy),
		selectHouseholdSharingLink:                   quasarmetrics.NewYDBSignals("selectHouseholdSharingLink", registry, policy),
		deleteHouseholdSharingLinks:                  quasarmetrics.NewYDBSignals("deleteHouseholdSharingLinks", registry, policy),
		selectHouseholdInvitationsByGuest:            quasarmetrics.NewYDBSignals("selectHouseholdInvitationsByGuest", registry, policy),
		selectHouseholdInvitationsBySender:           quasarmetrics.NewYDBSignals("selectHouseholdInvitationsBySender", registry, policy),
		selectHouseholdInvitationByID:                quasarmetrics.NewYDBSignals("selectHouseholdInvitationByID", registry, policy),
		deleteHouseholdInvitation:                    quasarmetrics.NewYDBSignals("deleteHouseholdInvitation", registry, policy),
		storeHouseholdInvitation:                     quasarmetrics.NewYDBSignals("storeHouseholdInvitation", registry, policy),
		registry:                                     registry,
		policy:                                       policy,
		transactionMetrics:                           make(map[string]quasarmetrics.YDBSignals),
	}
}

func (db *ClientWithMetrics) IsDatabaseError(err error) bool {
	// no need metrics for check error
	return db.dbClient.IsDatabaseError(err)
}

func (db *ClientWithMetrics) SelectUserDevices(ctx context.Context, userID uint64) (_ model.Devices, err error) {
	start := time.Now()
	defer func() { db.recordMetric(ctx, start, db.selectUserDevices, err) }()

	devices, err := db.dbClient.SelectUserDevices(ctx, userID)

	return devices, err
}
func (db *ClientWithMetrics) SelectUserDevicesSimple(ctx context.Context, userID uint64) (_ model.Devices, err error) {
	start := time.Now()
	defer func() { db.recordMetric(ctx, start, db.selectUserDevicesSimple, err) }()

	devices, err := db.dbClient.SelectUserDevicesSimple(ctx, userID)

	return devices, err
}
func (db *ClientWithMetrics) SelectUserProviderDevices(ctx context.Context, userID uint64, skillID string) (_ []model.Device, err error) {
	start := time.Now()
	defer func() { db.recordMetric(ctx, start, db.selectUserProviderDevices, err) }()

	devices, err := db.dbClient.SelectUserProviderDevices(ctx, userID, skillID)

	return devices, err
}
func (db *ClientWithMetrics) SelectUserProviderDevicesSimple(ctx context.Context, userID uint64, skillID string) (_ model.Devices, err error) {
	start := time.Now()
	defer func() { db.recordMetric(ctx, start, db.selectUserProviderDevicesSimple, err) }()

	devices, err := db.dbClient.SelectUserProviderDevicesSimple(ctx, userID, skillID)

	return devices, err
}
func (db *ClientWithMetrics) SelectDevicesSimpleByExternalIDs(ctx context.Context, externalIDs []string) (_ model.DevicesMapByOwnerID, err error) {
	start := time.Now()
	defer func() { db.recordMetric(ctx, start, db.selectDevicesSimpleByExternalIDs, err) }()

	devices, err := db.dbClient.SelectDevicesSimpleByExternalIDs(ctx, externalIDs)

	return devices, err
}
func (db *ClientWithMetrics) SelectUserProviderArchivedDevicesSimple(ctx context.Context, userID uint64, skillID string) (_ map[string]model.Device, err error) {
	start := time.Now()
	defer func() { db.recordMetric(ctx, start, db.selectUserProviderArchivedDevicesSimple, err) }()

	devices, err := db.dbClient.SelectUserProviderArchivedDevicesSimple(ctx, userID, skillID)

	return devices, err
}
func (db *ClientWithMetrics) SelectUserGroupDevices(ctx context.Context, userID uint64, groupID string) (_ []model.Device, err error) {
	start := time.Now()
	defer func() { db.recordMetric(ctx, start, db.selectUserGroupDevices, err) }()

	groupDevices, err := db.dbClient.SelectUserGroupDevices(ctx, userID, groupID)

	return groupDevices, err
}
func (db *ClientWithMetrics) SelectUserDevice(ctx context.Context, userID uint64, deviceID string) (_ model.Device, err error) {
	start := time.Now()
	defer func() { db.recordMetric(ctx, start, db.selectUserDevice, err) }()

	device, err := db.dbClient.SelectUserDevice(ctx, userID, deviceID)

	return device, err
}
func (db *ClientWithMetrics) SelectUserDeviceSimple(ctx context.Context, userID uint64, deviceID string) (_ model.Device, err error) {
	start := time.Now()
	defer func() { db.recordMetric(ctx, start, db.selectUserDeviceSimple, err) }()

	device, err := db.dbClient.SelectUserDeviceSimple(ctx, userID, deviceID)

	return device, err
}
func (db *ClientWithMetrics) SelectUserRoom(ctx context.Context, userID uint64, roomID string) (_ model.Room, err error) {
	start := time.Now()
	defer func() { db.recordMetric(ctx, start, db.selectUserRoom, err) }()

	room, err := db.dbClient.SelectUserRoom(ctx, userID, roomID)

	return room, err
}
func (db *ClientWithMetrics) SelectUserGroup(ctx context.Context, userID uint64, groupID string) (_ model.Group, err error) {
	start := time.Now()
	defer func() { db.recordMetric(ctx, start, db.selectUserGroup, err) }()

	group, err := db.dbClient.SelectUserGroup(ctx, userID, groupID)

	return group, err
}
func (db *ClientWithMetrics) SelectUserRooms(ctx context.Context, userID uint64) (_ []model.Room, err error) {
	start := time.Now()
	defer func() { db.recordMetric(ctx, start, db.selectUserRooms, err) }()

	rooms, err := db.dbClient.SelectUserRooms(ctx, userID)

	return rooms, err
}
func (db *ClientWithMetrics) SelectUserGroups(ctx context.Context, userID uint64) (_ []model.Group, err error) {
	start := time.Now()
	defer func() { db.recordMetric(ctx, start, db.selectUserGroups, err) }()

	groups, err := db.dbClient.SelectUserGroups(ctx, userID)

	return groups, err
}
func (db *ClientWithMetrics) StoreUserDevice(ctx context.Context, user model.User, device model.Device) (resultDevice model.Device, storeResult model.StoreResult, err error) {
	start := time.Now()
	defer func() { db.recordMetric(ctx, start, db.storeUserDevice, err) }()

	resultDevice, storeResult, err = db.dbClient.StoreUserDevice(ctx, user, device)

	return
}
func (db *ClientWithMetrics) StoreDeviceState(ctx context.Context, userID uint64, device model.Device) (capabilities model.Capabilities, properties model.Properties, err error) {
	start := time.Now()
	defer func() { db.recordMetric(ctx, start, db.storeDeviceState, err) }()

	capabilities, properties, err = db.dbClient.StoreDeviceState(ctx, userID, device)

	return capabilities, properties, err
}
func (db *ClientWithMetrics) StoreDevicesStates(ctx context.Context, userID uint64, devices []model.Device, loadDevicesForUpdate bool) (capabilitiesMap model.DeviceCapabilitiesMap, propertiesMap model.DevicePropertiesMap, err error) {
	start := time.Now()
	defer func() { db.recordMetric(ctx, start, db.storeDevicesStates, err) }()

	capabilitiesMap, propertiesMap, err = db.dbClient.StoreDevicesStates(ctx, userID, devices, loadDevicesForUpdate)

	return capabilitiesMap, propertiesMap, err
}

func (db *ClientWithMetrics) StoreDeviceTriggersIndexes(ctx context.Context, userID uint64, scenarioTriggers map[string]model.DevicePropertyScenarioTriggers) (err error) {
	start := time.Now()
	defer func() { db.recordMetric(ctx, start, db.storeDevicesTriggerIndexes, err) }()

	err = db.dbClient.StoreDeviceTriggersIndexes(ctx, userID, scenarioTriggers)

	return err
}

func (db *ClientWithMetrics) UpdateDeviceStatuses(ctx context.Context, userID uint64, deviceStates model.DeviceStatusMap) (err error) {
	start := time.Now()
	defer func() { db.recordMetric(ctx, start, db.updateDeviceStatuses, err) }()

	err = db.dbClient.UpdateDeviceStatuses(ctx, userID, deviceStates)

	return err
}
func (db *ClientWithMetrics) UpdateUserRoomName(ctx context.Context, user model.User, roomID string, name string) (err error) {
	start := time.Now()
	defer func() { db.recordMetric(ctx, start, db.updateUserRoomName, err) }()

	err = db.dbClient.UpdateUserRoomName(ctx, user, roomID, name)

	return err
}
func (db *ClientWithMetrics) UpdateUserGroupName(ctx context.Context, user model.User, groupID string, name string) (err error) {
	start := time.Now()
	defer func() { db.recordMetric(ctx, start, db.updateUserGroupName, err) }()

	err = db.dbClient.UpdateUserGroupName(ctx, user, groupID, name)

	return err
}
func (db *ClientWithMetrics) UpdateUserGroupNameAndAliases(ctx context.Context, user model.User, groupID string, name string, aliases []string) (err error) {
	start := time.Now()
	defer func() { db.recordMetric(ctx, start, db.updateUserGroupNameAndAliases, err) }()

	err = db.dbClient.UpdateUserGroupNameAndAliases(ctx, user, groupID, name, aliases)

	return err
}
func (db *ClientWithMetrics) DeleteUserRoom(ctx context.Context, userID uint64, roomID string) (err error) {
	start := time.Now()
	defer func() { db.recordMetric(ctx, start, db.deleteUserRoom, err) }()

	err = db.dbClient.DeleteUserRoom(ctx, userID, roomID)

	return err
}
func (db *ClientWithMetrics) DeleteUserGroup(ctx context.Context, userID uint64, groupID string) (err error) {
	start := time.Now()
	defer func() { db.recordMetric(ctx, start, db.deleteUserGroup, err) }()

	err = db.dbClient.DeleteUserGroup(ctx, userID, groupID)

	return err
}

func (db *ClientWithMetrics) DeleteStereopair(ctx context.Context, userID uint64, id string) (err error) {
	start := time.Now()
	defer func() { db.recordMetric(ctx, start, db.deleteStereopair, err) }()

	err = db.dbClient.DeleteStereopair(ctx, userID, id)

	return err

}

func (db *ClientWithMetrics) DeleteUserDevices(ctx context.Context, userID uint64, deviceIDList []string) (err error) {
	start := time.Now()
	defer func() { db.recordMetric(ctx, start, db.deleteUserDevices, err) }()

	err = db.dbClient.DeleteUserDevices(ctx, userID, deviceIDList)

	return err
}
func (db *ClientWithMetrics) UpdateUserDeviceNameAndAliases(ctx context.Context, userID uint64, deviceID string, name string, aliases []string) (err error) {
	start := time.Now()
	defer func() { db.recordMetric(ctx, start, db.updateUserDeviceNameAndAliases, err) }()

	err = db.dbClient.UpdateUserDeviceNameAndAliases(ctx, userID, deviceID, name, aliases)

	return err
}
func (db *ClientWithMetrics) UpdateUserDeviceName(ctx context.Context, userID uint64, deviceID string, name string) (err error) {
	start := time.Now()
	defer func() { db.recordMetric(ctx, start, db.updateUserDeviceName, err) }()

	err = db.dbClient.UpdateUserDeviceName(ctx, userID, deviceID, name)

	return err
}
func (db *ClientWithMetrics) UpdateUserDeviceRoom(ctx context.Context, userID uint64, deviceID string, roomID string) (err error) {
	start := time.Now()
	defer func() { db.recordMetric(ctx, start, db.updateUserDeviceRoom, err) }()

	err = db.dbClient.UpdateUserDeviceRoom(ctx, userID, deviceID, roomID)

	return err
}
func (db *ClientWithMetrics) UpdateUserDeviceType(ctx context.Context, userID uint64, deviceID string, newDeviceType model.DeviceType) (err error) {
	start := time.Now()
	defer func() { db.recordMetric(ctx, start, db.updateUserDeviceType, err) }()

	err = db.dbClient.UpdateUserDeviceType(ctx, userID, deviceID, newDeviceType)

	return err
}
func (db *ClientWithMetrics) StoreUserDeviceConfigs(ctx context.Context, userID uint64, deviceConfigs model.DeviceConfigs) (err error) {
	start := time.Now()
	defer func() { db.recordMetric(ctx, start, db.storeUserDeviceConfigs, err) }()

	err = db.dbClient.StoreUserDeviceConfigs(ctx, userID, deviceConfigs)

	return err
}
func (db *ClientWithMetrics) UpdateUserDeviceGroups(ctx context.Context, user model.User, deviceID string, groupsIDs []string) (err error) {
	start := time.Now()
	defer func() { db.recordMetric(ctx, start, db.updateUserDeviceGroups, err) }()

	err = db.dbClient.UpdateUserDeviceGroups(ctx, user, deviceID, groupsIDs)

	return err
}
func (db *ClientWithMetrics) CreateUserRoom(ctx context.Context, user model.User, room model.Room) (roomID string, err error) {
	start := time.Now()
	defer func() { db.recordMetric(ctx, start, db.createUserRoom, err) }()

	roomID, err = db.dbClient.CreateUserRoom(ctx, user, room)

	return
}
func (db *ClientWithMetrics) CreateUserGroup(ctx context.Context, user model.User, group model.Group) (groupID string, err error) {
	start := time.Now()
	defer func() { db.recordMetric(ctx, start, db.createUserGroup, err) }()

	groupID, err = db.dbClient.CreateUserGroup(ctx, user, group)

	return
}

func (db *ClientWithMetrics) StoreStereopair(ctx context.Context, userID uint64, stereopair model.Stereopair) (err error) {
	start := time.Now()
	defer func() { db.recordMetric(ctx, start, db.storeStereopair, err) }()

	err = db.dbClient.StoreStereopair(ctx, userID, stereopair)

	return err
}

func (db *ClientWithMetrics) UpdateStereopairName(ctx context.Context, userID uint64, stereopairID, oldName, newName string) (err error) {
	start := time.Now()
	defer func() { db.recordMetric(ctx, start, db.updateStereopairName, err) }()

	err = db.dbClient.UpdateStereopairName(ctx, userID, stereopairID, oldName, newName)

	return err
}

func (db *ClientWithMetrics) StoreUser(ctx context.Context, user model.User) (err error) {
	start := time.Now()
	defer func() { db.recordMetric(ctx, start, db.storeUser, err) }()

	err = db.dbClient.StoreUser(ctx, user)

	return err
}

func (db *ClientWithMetrics) StoreUserWithHousehold(ctx context.Context, user model.User, household model.Household) (householdID string, err error) {
	start := time.Now()
	defer func() { db.recordMetric(ctx, start, db.storeUserWithHousehold, err) }()

	householdID, err = db.dbClient.StoreUserWithHousehold(ctx, user, household)

	return householdID, err
}

func (db *ClientWithMetrics) SelectUser(ctx context.Context, userID uint64) (_ model.User, err error) {
	start := time.Now()
	defer func() { db.recordMetric(ctx, start, db.selectUser, err) }()

	user, err := db.dbClient.SelectUser(ctx, userID)

	return user, err
}
func (db *ClientWithMetrics) StoreExternalUser(ctx context.Context, externalID, skillID string, user model.User) (err error) {
	start := time.Now()
	defer func() { db.recordMetric(ctx, start, db.storeExternalUser, err) }()

	err = db.dbClient.StoreExternalUser(ctx, externalID, skillID, user)

	return err
}
func (db *ClientWithMetrics) SelectExternalUsers(ctx context.Context, externalID, skillID string) (_ []model.User, err error) {
	start := time.Now()
	defer func() { db.recordMetric(ctx, start, db.selectExternalUsers, err) }()

	users, err := db.dbClient.SelectExternalUsers(ctx, externalID, skillID)

	return users, err
}
func (db *ClientWithMetrics) DeleteExternalUser(ctx context.Context, skillID string, user model.User) (err error) {
	start := time.Now()
	defer func() { db.recordMetric(ctx, start, db.deleteExternalUser, err) }()

	err = db.dbClient.DeleteExternalUser(ctx, skillID, user)

	return err
}
func (db *ClientWithMetrics) SelectUserScenarios(ctx context.Context, userID uint64) (_ model.Scenarios, err error) {
	start := time.Now()
	defer func() { db.recordMetric(ctx, start, db.selectUserScenarios, err) }()

	scenarios, err := db.dbClient.SelectUserScenarios(ctx, userID)

	return scenarios, err
}
func (db *ClientWithMetrics) SelectUserScenariosSimple(ctx context.Context, userID uint64) (_ model.Scenarios, err error) {
	start := time.Now()
	defer func() { db.recordMetric(ctx, start, db.selectUserScenariosSimple, err) }()

	scenarios, err := db.dbClient.SelectUserScenariosSimple(ctx, userID)

	return scenarios, err
}
func (db *ClientWithMetrics) SelectScenario(ctx context.Context, userID uint64, s string) (_ model.Scenario, err error) {
	start := time.Now()
	defer func() { db.recordMetric(ctx, start, db.selectScenario, err) }()

	scenario, err := db.dbClient.SelectScenario(ctx, userID, s)

	return scenario, err
}
func (db *ClientWithMetrics) CreateScenario(ctx context.Context, userID uint64, s model.Scenario) (scenarioID string, err error) {
	start := time.Now()
	defer func() { db.recordMetric(ctx, start, db.createScenario, err) }()

	scenarioID, err = db.dbClient.CreateScenario(ctx, userID, s)

	return
}
func (db *ClientWithMetrics) UpdateScenario(ctx context.Context, userID uint64, s model.Scenario) (err error) {
	start := time.Now()
	defer func() { db.recordMetric(ctx, start, db.updateScenario, err) }()

	err = db.dbClient.UpdateScenario(ctx, userID, s)

	return err
}
func (db *ClientWithMetrics) UpdateScenarios(ctx context.Context, userID uint64, scenarios model.Scenarios) (err error) {
	start := time.Now()
	defer func() { db.recordMetric(ctx, start, db.updateScenarios, err) }()

	err = db.dbClient.UpdateScenarios(ctx, userID, scenarios)

	return err

}

func (db *ClientWithMetrics) StoreScenarioLaunch(ctx context.Context, userID uint64, scenarioLaunch model.ScenarioLaunch) (_ string, err error) {
	start := time.Now()
	defer func() { db.recordMetric(ctx, start, db.storeScenarioLaunch, err) }()

	launchID, err := db.dbClient.StoreScenarioLaunch(ctx, userID, scenarioLaunch)

	return launchID, err
}
func (db *ClientWithMetrics) SelectScenarioLaunchList(ctx context.Context, userID uint64, limit uint64, triggerTypes []model.ScenarioTriggerType) (_ model.ScenarioLaunches, err error) {
	start := time.Now()
	defer func() { db.recordMetric(ctx, start, db.selectScenarioLaunchList, err) }()

	scenarios, err := db.dbClient.SelectScenarioLaunchList(ctx, userID, limit, triggerTypes)

	return scenarios, err
}
func (db *ClientWithMetrics) SelectScheduledScenarioLaunches(ctx context.Context, userID uint64) (_ model.ScenarioLaunches, err error) {
	start := time.Now()
	defer func() { db.recordMetric(ctx, start, db.selectScheduledScenarioLaunches, err) }()

	launches, err := db.dbClient.SelectScheduledScenarioLaunches(ctx, userID)

	return launches, err
}
func (db *ClientWithMetrics) SelectScenarioLaunchesByScenarioID(ctx context.Context, userID uint64, scenarioID string) (_ model.ScenarioLaunches, err error) {
	start := time.Now()
	defer func() { db.recordMetric(ctx, start, db.selectScenarioLaunchesByScenarioID, err) }()

	launches, err := db.dbClient.SelectScenarioLaunchesByScenarioID(ctx, userID, scenarioID)

	return launches, err
}
func (db *ClientWithMetrics) SelectScenarioLaunch(ctx context.Context, userID uint64, scenarioLaunchID string) (_ model.ScenarioLaunch, err error) {
	start := time.Now()
	defer func() { db.recordMetric(ctx, start, db.selectScenarioLaunch, err) }()

	launch, err := db.dbClient.SelectScenarioLaunch(ctx, userID, scenarioLaunchID)

	return launch, err
}
func (db *ClientWithMetrics) UpdateScenarioLaunch(ctx context.Context, userID uint64, launch model.ScenarioLaunch) (err error) {
	start := time.Now()
	defer func() { db.recordMetric(ctx, start, db.updateScenarioLaunch, err) }()

	err = db.dbClient.UpdateScenarioLaunch(ctx, userID, launch)

	return err
}
func (db *ClientWithMetrics) UpdateScenarioLaunchScheduledTime(ctx context.Context, userID uint64, launchID string, newScheduled timestamp.PastTimestamp) (err error) {
	start := time.Now()
	defer func() { db.recordMetric(ctx, start, db.updateScenarioLaunchScheduledTime, err) }()

	err = db.dbClient.UpdateScenarioLaunchScheduledTime(ctx, userID, launchID, newScheduled)

	return err
}
func (db *ClientWithMetrics) DeleteScenarioLaunches(ctx context.Context, userID uint64, launchIDs []string) (err error) {
	start := time.Now()
	defer func() { db.recordMetric(ctx, start, db.deleteScenarioLaunches, err) }()

	err = db.dbClient.DeleteScenarioLaunches(ctx, userID, launchIDs)

	return err
}
func (db *ClientWithMetrics) CreateScenarioWithLaunch(ctx context.Context, userID uint64, scenario model.Scenario, launch model.ScenarioLaunch) (err error) {
	start := time.Now()
	defer func() { db.recordMetric(ctx, start, db.createScenarioWithLaunch, err) }()

	err = db.dbClient.CreateScenarioWithLaunch(ctx, userID, scenario, launch)

	return err
}
func (db *ClientWithMetrics) UpdateScenarioAndCreateLaunch(ctx context.Context, userID uint64, scenario model.Scenario, newLaunch model.ScenarioLaunch) (err error) {
	start := time.Now()
	defer func() { db.recordMetric(ctx, start, db.updateScenarioAndCreateLaunch, err) }()

	err = db.dbClient.UpdateScenarioAndCreateLaunch(ctx, userID, scenario, newLaunch)

	return err
}
func (db *ClientWithMetrics) UpdateScenarioAndDeleteLaunches(ctx context.Context, userID uint64, scenario model.Scenario) (err error) {
	start := time.Now()
	defer func() { db.recordMetric(ctx, start, db.updateScenarioAndDeleteLaunches, err) }()

	err = db.dbClient.UpdateScenarioAndDeleteLaunches(ctx, userID, scenario)

	return err
}
func (db *ClientWithMetrics) DeleteScenario(ctx context.Context, userID uint64, scenarioID string) (err error) {
	start := time.Now()
	defer func() { db.recordMetric(ctx, start, db.deleteScenario, err) }()

	err = db.dbClient.DeleteScenario(ctx, userID, scenarioID)

	return err
}

func (db *ClientWithMetrics) CancelScenarioLaunchesByTriggerTypeAndStatus(ctx context.Context, userID uint64, triggerType model.ScenarioTriggerType, status model.ScenarioLaunchStatus) (err error) {
	start := time.Now()
	defer func() { db.recordMetric(ctx, start, db.cancelScenarioLaunchesByTriggerTypeAndStatus, err) }()

	err = db.dbClient.CancelScenarioLaunchesByTriggerTypeAndStatus(ctx, userID, triggerType, status)

	return err
}

func (db *ClientWithMetrics) SelectUserInfo(ctx context.Context, userID uint64) (_ model.UserInfo, err error) {
	start := time.Now()
	defer func() { db.recordMetric(ctx, start, db.selectUserInfo, err) }()

	res, err := db.dbClient.SelectUserInfo(ctx, userID)

	return res, err
}
func (db *ClientWithMetrics) SelectUserSkills(ctx context.Context, userID uint64) (_ []string, err error) {
	start := time.Now()
	defer func() { db.recordMetric(ctx, start, db.selectUserSkills, err) }()

	res, err := db.dbClient.SelectUserSkills(ctx, userID)

	return res, err
}

func (db *ClientWithMetrics) SelectStereopair(ctx context.Context, userID uint64, stereopairID string) (_ model.Stereopair, err error) {
	start := time.Now()
	defer func() { db.recordMetric(ctx, start, db.selectStereopair, err) }()

	res, err := db.dbClient.SelectStereopair(ctx, userID, stereopairID)

	return res, err

}

func (db *ClientWithMetrics) SelectStereopairs(ctx context.Context, userID uint64) (_ model.Stereopairs, err error) {
	start := time.Now()
	defer func() { db.recordMetric(ctx, start, db.selectStereopairs, err) }()

	res, err := db.dbClient.SelectStereopairs(ctx, userID)

	return res, err

}

func (db *ClientWithMetrics) SelectStereopairsSimple(ctx context.Context, userID uint64) (_ model.Stereopairs, err error) {
	start := time.Now()
	defer func() { db.recordMetric(ctx, start, db.selectStereopairsSimple, err) }()

	res, err := db.dbClient.SelectStereopairsSimple(ctx, userID)

	return res, err

}

func (db *ClientWithMetrics) CheckUserSkillExist(ctx context.Context, userID uint64, skillID string) (_ bool, err error) {
	start := time.Now()
	defer func() { db.recordMetric(ctx, start, db.checkUserSkillExist, err) }()

	exist, err := db.dbClient.CheckUserSkillExist(ctx, userID, skillID)

	return exist, err
}
func (db *ClientWithMetrics) StoreUserSkill(ctx context.Context, userID uint64, skillID string) (err error) {
	start := time.Now()
	defer func() { db.recordMetric(ctx, start, db.storeUserSkill, err) }()

	err = db.dbClient.StoreUserSkill(ctx, userID, skillID)

	return err
}

func (db *ClientWithMetrics) Transaction(ctx context.Context, name string, f func(ctx context.Context) error) (err error) {
	signals := db.trYdbSignals(name)
	start := time.Now()
	defer func() { db.recordMetric(ctx, start, signals, err) }()

	err = db.dbClient.Transaction(ctx, name, f)

	return err
}

func (db *ClientWithMetrics) WithoutTransaction(ctx context.Context) context.Context {
	return db.dbClient.WithoutTransaction(ctx)
}

func (db *ClientWithMetrics) DeleteUserSkill(ctx context.Context, userID uint64, skillID string) (err error) {
	start := time.Now()
	defer func() { db.recordMetric(ctx, start, db.deleteUserSkill, err) }()

	err = db.dbClient.DeleteUserSkill(ctx, userID, skillID)

	return err
}
func (db *ClientWithMetrics) SelectUserNetworks(ctx context.Context, userID uint64) (_ model.Networks, err error) {
	start := time.Now()
	defer func() { db.recordMetric(ctx, start, db.selectUserNetworks, err) }()

	networks, err := db.dbClient.SelectUserNetworks(ctx, userID)

	return networks, err
}
func (db *ClientWithMetrics) SelectUserNetwork(ctx context.Context, userID uint64, SSID string) (_ model.Network, err error) {
	start := time.Now()
	defer func() { db.recordMetric(ctx, start, db.selectUserNetwork, err) }()

	network, err := db.dbClient.SelectUserNetwork(ctx, userID, SSID)

	return network, err
}
func (db *ClientWithMetrics) StoreUserNetwork(ctx context.Context, userID uint64, network model.Network) (err error) {
	start := time.Now()
	defer func() { db.recordMetric(ctx, start, db.storeUserNetwork, err) }()

	err = db.dbClient.StoreUserNetwork(ctx, userID, network)

	return err
}
func (db *ClientWithMetrics) DeleteUserNetwork(ctx context.Context, userID uint64, networkID string) (err error) {
	start := time.Now()
	defer func() { db.recordMetric(ctx, start, db.deleteUserNetwork, err) }()

	err = db.dbClient.DeleteUserNetwork(ctx, userID, networkID)

	return err
}

func (db *ClientWithMetrics) SelectAllExperiments(ctx context.Context) (_ experiments.Experiments, err error) {
	start := time.Now()
	defer func() { db.recordMetric(ctx, start, db.selectAllExperiments, err) }()

	exps, err := db.dbClient.SelectAllExperiments(ctx)

	return exps, err
}

func (db *ClientWithMetrics) SelectUserHouseholds(ctx context.Context, userID uint64) (_ model.Households, err error) {
	start := time.Now()
	defer func() { db.recordMetric(ctx, start, db.selectUserHouseholds, err) }()

	households, err := db.dbClient.SelectUserHouseholds(ctx, userID)

	return households, err
}
func (db *ClientWithMetrics) SelectUserHousehold(ctx context.Context, userID uint64, ID string) (_ model.Household, err error) {
	start := time.Now()
	defer func() { db.recordMetric(ctx, start, db.selectUserHousehold, err) }()

	household, err := db.dbClient.SelectUserHousehold(ctx, userID, ID)

	return household, err
}
func (db *ClientWithMetrics) CreateUserHousehold(ctx context.Context, userID uint64, household model.Household) (newHouseholdID string, err error) {
	start := time.Now()
	defer func() { db.recordMetric(ctx, start, db.createUserHousehold, err) }()

	householdID, err := db.dbClient.CreateUserHousehold(ctx, userID, household)

	return householdID, err
}
func (db *ClientWithMetrics) SelectUserHouseholdDevices(ctx context.Context, userID uint64, householdID string) (_ model.Devices, err error) {
	start := time.Now()
	defer func() { db.recordMetric(ctx, start, db.selectUserHouseholdDevices, err) }()

	devices, err := db.dbClient.SelectUserHouseholdDevices(ctx, userID, householdID)

	return devices, err
}
func (db *ClientWithMetrics) SelectUserHouseholdDevicesSimple(ctx context.Context, userID uint64, householdID string) (_ model.Devices, err error) {
	start := time.Now()
	defer func() { db.recordMetric(ctx, start, db.selectUserHouseholdDevicesSimple, err) }()

	devices, err := db.dbClient.SelectUserHouseholdDevicesSimple(ctx, userID, householdID)

	return devices, err
}
func (db *ClientWithMetrics) SelectUserHouseholdGroups(ctx context.Context, userID uint64, householdID string) (_ model.Groups, err error) {
	start := time.Now()
	defer func() { db.recordMetric(ctx, start, db.selectUserHouseholdGroups, err) }()

	groups, err := db.dbClient.SelectUserHouseholdGroups(ctx, userID, householdID)

	return groups, err
}
func (db *ClientWithMetrics) SelectUserHouseholdRooms(ctx context.Context, userID uint64, householdID string) (_ model.Rooms, err error) {
	start := time.Now()
	defer func() { db.recordMetric(ctx, start, db.selectUserHouseholdRooms, err) }()

	rooms, err := db.dbClient.SelectUserHouseholdRooms(ctx, userID, householdID)

	return rooms, err
}
func (db *ClientWithMetrics) DeleteUserHousehold(ctx context.Context, userID uint64, householdID string) (err error) {
	start := time.Now()
	defer func() { db.recordMetric(ctx, start, db.deleteUserHousehold, err) }()

	err = db.dbClient.DeleteUserHousehold(ctx, userID, householdID)

	return err
}
func (db *ClientWithMetrics) SelectCurrentHousehold(ctx context.Context, userID uint64) (_ model.Household, err error) {
	start := time.Now()
	defer func() { db.recordMetric(ctx, start, db.selectCurrentHousehold, err) }()

	res, err := db.dbClient.SelectCurrentHousehold(ctx, userID)

	return res, err
}
func (db *ClientWithMetrics) SetCurrentHouseholdForUser(ctx context.Context, userID uint64, householdID string) (err error) {
	start := time.Now()
	defer func() { db.recordMetric(ctx, start, db.setCurrentHouseholdForUser, err) }()

	err = db.dbClient.SetCurrentHouseholdForUser(ctx, userID, householdID)

	return err
}
func (db *ClientWithMetrics) SelectDeviceTriggersIndexes(ctx context.Context, userID uint64, deviceTriggerIndexKey model.DeviceTriggerIndexKey) (_ model.DeviceTriggersIndexes, err error) {
	start := time.Now()
	defer func() { db.recordMetric(ctx, start, db.selectDeviceTriggersIndexes, err) }()

	result, err := db.dbClient.SelectDeviceTriggersIndexes(ctx, userID, deviceTriggerIndexKey)

	return result, err
}
func (db *ClientWithMetrics) UpdateUserHousehold(ctx context.Context, userID uint64, household model.Household) (err error) {
	start := time.Now()
	defer func() { db.recordMetric(ctx, start, db.updateUserHousehold, err) }()

	err = db.dbClient.UpdateUserHousehold(ctx, userID, household)

	return err
}
func (db *ClientWithMetrics) MoveUserDevicesToHousehold(ctx context.Context, user model.User, deviceIDs []string, householdID string) (err error) {
	start := time.Now()
	defer func() { db.recordMetric(ctx, start, db.moveUserDevicesToHousehold, err) }()

	err = db.dbClient.MoveUserDevicesToHousehold(ctx, user, deviceIDs, householdID)

	return err
}
func (db *ClientWithMetrics) UpdateUserGroupNameAndDevices(ctx context.Context, user model.User, group model.Group) (err error) {
	start := time.Now()
	defer func() { db.recordMetric(ctx, start, db.updateUserGroupNameAndDevices, err) }()

	err = db.dbClient.UpdateUserGroupNameAndDevices(ctx, user, group)

	return err
}
func (db *ClientWithMetrics) UpdateUserRoomNameAndDevices(ctx context.Context, user model.User, room model.Room) (err error) {
	start := time.Now()
	defer func() { db.recordMetric(ctx, start, db.updateUserRoomNameAndDevices, err) }()

	err = db.dbClient.UpdateUserRoomNameAndDevices(ctx, user, room)

	return err
}
func (db *ClientWithMetrics) SelectUserRoomDevices(ctx context.Context, userID uint64, roomID string) (_ []model.Device, err error) {
	start := time.Now()
	defer func() { db.recordMetric(ctx, start, db.selectUserRoomDevices, err) }()

	devices, err := db.dbClient.SelectUserRoomDevices(ctx, userID, roomID)

	return devices, err
}
func (db *ClientWithMetrics) SelectFavorites(ctx context.Context, user model.User) (_ model.Favorites, err error) {
	start := time.Now()
	defer func() { db.recordMetric(ctx, start, db.selectFavorites, err) }()

	data, err := db.dbClient.SelectFavorites(ctx, user)

	return data, err
}
func (db *ClientWithMetrics) StoreFavoriteScenarios(ctx context.Context, user model.User, scenarios model.Scenarios) (err error) {
	start := time.Now()
	defer func() { db.recordMetric(ctx, start, db.storeFavoriteScenarios, err) }()

	err = db.dbClient.StoreFavoriteScenarios(ctx, user, scenarios)

	return err
}
func (db *ClientWithMetrics) StoreFavoriteDevices(ctx context.Context, user model.User, devices model.Devices) (err error) {
	start := time.Now()
	defer func() { db.recordMetric(ctx, start, db.storeFavoriteDevices, err) }()

	err = db.dbClient.StoreFavoriteDevices(ctx, user, devices)

	return err
}
func (db *ClientWithMetrics) StoreFavoriteGroups(ctx context.Context, user model.User, groups model.Groups) (err error) {
	start := time.Now()
	defer func() { db.recordMetric(ctx, start, db.storeFavoriteGroups, err) }()

	err = db.dbClient.StoreFavoriteGroups(ctx, user, groups)

	return err
}
func (db *ClientWithMetrics) StoreFavoriteProperties(ctx context.Context, user model.User, properties model.FavoritesDeviceProperties) (err error) {
	start := time.Now()
	defer func() { db.recordMetric(ctx, start, db.storeFavoriteProperties, err) }()

	err = db.dbClient.StoreFavoriteProperties(ctx, user, properties)

	return err
}
func (db *ClientWithMetrics) DeleteFavoriteScenarios(ctx context.Context, user model.User, scenarios model.Scenarios) (err error) {
	start := time.Now()
	defer func() { db.recordMetric(ctx, start, db.deleteFavoriteScenarios, err) }()

	err = db.dbClient.DeleteFavoriteScenarios(ctx, user, scenarios)

	return err
}
func (db *ClientWithMetrics) DeleteFavoriteDevices(ctx context.Context, user model.User, devices model.Devices) (err error) {
	start := time.Now()
	defer func() { db.recordMetric(ctx, start, db.deleteFavoriteDevices, err) }()

	err = db.dbClient.DeleteFavoriteDevices(ctx, user, devices)

	return err
}
func (db *ClientWithMetrics) DeleteFavoriteGroups(ctx context.Context, user model.User, groups model.Groups) (err error) {
	start := time.Now()
	defer func() { db.recordMetric(ctx, start, db.deleteFavoriteGroups, err) }()

	err = db.dbClient.DeleteFavoriteGroups(ctx, user, groups)

	return err
}
func (db *ClientWithMetrics) DeleteFavoriteProperties(ctx context.Context, user model.User, properties model.FavoritesDeviceProperties) (err error) {
	start := time.Now()
	defer func() { db.recordMetric(ctx, start, db.deleteFavoriteProperties, err) }()

	err = db.dbClient.DeleteFavoriteProperties(ctx, user, properties)

	return err
}
func (db *ClientWithMetrics) ReplaceFavoriteScenarios(ctx context.Context, user model.User, scenarios model.Scenarios) (err error) {
	start := time.Now()
	defer func() { db.recordMetric(ctx, start, db.replaceFavoriteScenarios, err) }()

	err = db.dbClient.ReplaceFavoriteScenarios(ctx, user, scenarios)

	return err
}
func (db *ClientWithMetrics) ReplaceFavoriteDevices(ctx context.Context, user model.User, devices model.Devices) (err error) {
	start := time.Now()
	defer func() { db.recordMetric(ctx, start, db.replaceFavoriteDevices, err) }()

	err = db.dbClient.ReplaceFavoriteDevices(ctx, user, devices)

	return err
}
func (db *ClientWithMetrics) ReplaceFavoriteProperties(ctx context.Context, user model.User, properties model.FavoritesDeviceProperties) (err error) {
	start := time.Now()
	defer func() { db.recordMetric(ctx, start, db.replaceFavoriteProperties, err) }()

	err = db.dbClient.ReplaceFavoriteProperties(ctx, user, properties)

	return err
}
func (db *ClientWithMetrics) ReplaceFavoriteGroups(ctx context.Context, user model.User, groups model.Groups) (err error) {
	start := time.Now()
	defer func() { db.recordMetric(ctx, start, db.replaceFavoriteGroups, err) }()

	err = db.dbClient.ReplaceFavoriteGroups(ctx, user, groups)

	return err
}
func (db *ClientWithMetrics) StoreUserStorageConfig(ctx context.Context, user model.User, config model.UserStorageConfig) (err error) {
	start := time.Now()
	defer func() { db.recordMetric(ctx, start, db.storeUserStorageConfig, err) }()

	err = db.dbClient.StoreUserStorageConfig(ctx, user, config)

	return err
}
func (db *ClientWithMetrics) SelectUserStorageConfig(ctx context.Context, user model.User) (_ model.UserStorageConfig, err error) {
	start := time.Now()
	defer func() { db.recordMetric(ctx, start, db.selectUserStorageConfig, err) }()

	config, err := db.dbClient.SelectUserStorageConfig(ctx, user)

	return config, err
}
func (db *ClientWithMetrics) DeleteUserStorageConfig(ctx context.Context, user model.User) (err error) {
	start := time.Now()
	defer func() { db.recordMetric(ctx, start, db.deleteUserStorageConfig, err) }()

	err = db.dbClient.DeleteUserStorageConfig(ctx, user)

	return err
}

func (db *ClientWithMetrics) SelectUserIntentState(ctx context.Context, userID uint64, intentStateKey model.IntentStateKey) (_ model.IntentState, err error) {
	start := time.Now()
	defer func() { db.recordMetric(ctx, start, db.selectUserIntentState, err) }()

	state, err := db.dbClient.SelectUserIntentState(ctx, userID, intentStateKey)

	return state, err
}

func (db *ClientWithMetrics) StoreUserIntentState(ctx context.Context, userID uint64, intentStateKey model.IntentStateKey, intentState model.IntentState) (err error) {
	start := time.Now()
	defer func() { db.recordMetric(ctx, start, db.storeUserIntentState, err) }()

	err = db.dbClient.StoreUserIntentState(ctx, userID, intentStateKey, intentState)

	return err
}

func (db *ClientWithMetrics) StoreSharedHousehold(ctx context.Context, guestID uint64, sharingInfo model.SharingInfo) (err error) {
	start := time.Now()
	defer func() { db.recordMetric(ctx, start, db.storeSharedHousehold, err) }()

	err = db.dbClient.StoreSharedHousehold(ctx, guestID, sharingInfo)

	return err
}

func (db *ClientWithMetrics) DeleteSharedHousehold(ctx context.Context, ownerID uint64, guestID uint64, householdID string) (err error) {
	start := time.Now()
	defer func() { db.recordMetric(ctx, start, db.deleteSharedHousehold, err) }()

	err = db.dbClient.DeleteSharedHousehold(ctx, ownerID, guestID, householdID)

	return err
}

func (db *ClientWithMetrics) RenameSharedHousehold(ctx context.Context, guestID uint64, householdID string, householdName string) (err error) {
	start := time.Now()
	defer func() { db.recordMetric(ctx, start, db.renameSharedHousehold, err) }()

	err = db.dbClient.RenameSharedHousehold(ctx, guestID, householdID, householdName)

	return err
}

func (db *ClientWithMetrics) SelectGuestSharingInfos(ctx context.Context, guestID uint64) (_ model.SharingInfos, err error) {
	start := time.Now()
	defer func() { db.recordMetric(ctx, start, db.selectGuestSharingInfos, err) }()

	sharingInfos, err := db.dbClient.SelectGuestSharingInfos(ctx, guestID)

	return sharingInfos, err
}

func (db *ClientWithMetrics) SelectHouseholdResidents(ctx context.Context, userID uint64, household model.Household) (_ model.HouseholdResidents, err error) {
	start := time.Now()
	defer func() { db.recordMetric(ctx, start, db.selectHouseholdResidents, err) }()

	residents, err := db.dbClient.SelectHouseholdResidents(ctx, userID, household)

	return residents, err
}

func (db *ClientWithMetrics) StoreHouseholdSharingLink(ctx context.Context, link model.HouseholdSharingLink) (err error) {
	start := time.Now()
	defer func() { db.recordMetric(ctx, start, db.storeHouseholdSharingLink, err) }()

	err = db.dbClient.StoreHouseholdSharingLink(ctx, link)

	return err
}

func (db *ClientWithMetrics) SelectHouseholdSharingLinkByID(ctx context.Context, linkID string) (_ model.HouseholdSharingLink, err error) {
	start := time.Now()
	defer func() { db.recordMetric(ctx, start, db.selectHouseholdSharingLinkByID, err) }()

	link, err := db.dbClient.SelectHouseholdSharingLinkByID(ctx, linkID)

	return link, err
}

func (db *ClientWithMetrics) SelectHouseholdSharingLink(ctx context.Context, senderID uint64, householdID string) (_ model.HouseholdSharingLink, err error) {
	start := time.Now()
	defer func() { db.recordMetric(ctx, start, db.selectHouseholdSharingLink, err) }()

	link, err := db.dbClient.SelectHouseholdSharingLink(ctx, senderID, householdID)

	return link, err
}

func (db *ClientWithMetrics) DeleteHouseholdSharingLinks(ctx context.Context, senderID uint64, householdID string) (err error) {
	start := time.Now()
	defer func() { db.recordMetric(ctx, start, db.deleteHouseholdSharingLinks, err) }()

	err = db.dbClient.DeleteHouseholdSharingLinks(ctx, senderID, householdID)

	return err
}

func (db *ClientWithMetrics) SelectHouseholdInvitationsByGuest(ctx context.Context, guestID uint64) (_ model.HouseholdInvitations, err error) {
	start := time.Now()
	defer func() { db.recordMetric(ctx, start, db.selectHouseholdInvitationsByGuest, err) }()

	invitations, err := db.dbClient.SelectHouseholdInvitationsByGuest(ctx, guestID)

	return invitations, err
}
func (db *ClientWithMetrics) SelectHouseholdInvitationsBySender(ctx context.Context, senderID uint64) (_ model.HouseholdInvitations, err error) {
	start := time.Now()
	defer func() { db.recordMetric(ctx, start, db.selectHouseholdInvitationsBySender, err) }()

	invitations, err := db.dbClient.SelectHouseholdInvitationsBySender(ctx, senderID)

	return invitations, err
}
func (db *ClientWithMetrics) SelectHouseholdInvitationByID(ctx context.Context, ID string) (_ model.HouseholdInvitation, err error) {
	start := time.Now()
	defer func() { db.recordMetric(ctx, start, db.selectHouseholdInvitationByID, err) }()

	invitation, err := db.dbClient.SelectHouseholdInvitationByID(ctx, ID)

	return invitation, err
}
func (db *ClientWithMetrics) DeleteHouseholdInvitation(ctx context.Context, invitation model.HouseholdInvitation) (err error) {
	start := time.Now()
	defer func() { db.recordMetric(ctx, start, db.deleteHouseholdInvitation, err) }()

	err = db.dbClient.DeleteHouseholdInvitation(ctx, invitation)

	return err
}
func (db *ClientWithMetrics) StoreHouseholdInvitation(ctx context.Context, invitation model.HouseholdInvitation) (err error) {
	start := time.Now()
	defer func() { db.recordMetric(ctx, start, db.storeHouseholdInvitation, err) }()

	err = db.dbClient.StoreHouseholdInvitation(ctx, invitation)

	return err
}

func (db *ClientWithMetrics) trYdbSignals(name string) quasarmetrics.YDBSignals {
	db.m.RLock()
	signals, ok := db.transactionMetrics[name]
	db.m.RUnlock()
	if ok {
		return signals
	}

	db.m.Lock()
	defer db.m.Unlock()
	signals, ok = db.transactionMetrics[name] // second check for prevent race condition
	if !ok {
		signals = quasarmetrics.NewYDBSignals("tr_"+name, db.registry, db.policy)
		db.transactionMetrics[name] = signals
	}
	return signals
}

func (db *ClientWithMetrics) recordMetric(ctx context.Context, start time.Time, signal quasarmetrics.YDBSignals, err error) {
	if !db.dbClient.HasTransaction(ctx) {
		signal.RecordDurationSince(start)
		signal.RecordMetrics(err)
	}
}
