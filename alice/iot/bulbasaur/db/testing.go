package db

import (
	"context"
	"path"
	"sort"
	"time"

	"github.com/stretchr/testify/suite"
	"go.uber.org/zap"
	"go.uber.org/zap/zapcore"

	dbSchema "a.yandex-team.ru/alice/iot/bulbasaur/db/schema"
	"a.yandex-team.ru/alice/iot/bulbasaur/model"
	"a.yandex-team.ru/alice/iot/bulbasaur/testing"
	"a.yandex-team.ru/alice/library/go/timestamp"
	"a.yandex-team.ru/alice/library/go/tools"
	"a.yandex-team.ru/kikimr/public/sdk/go/ydb"
	"a.yandex-team.ru/kikimr/public/sdk/go/ydb/scheme"
	"a.yandex-team.ru/library/go/core/log"
)

type DBClientSuite struct {
	suite.Suite
	context                 context.Context
	endpoint, prefix, token string
	trace                   bool
	dbClient                *DBClient
	logger                  log.Logger
}

func (s *DBClientSuite) SetupSuite() {
	s.context = context.Background()

	logConfig := zap.NewDevelopmentConfig()
	logConfig.EncoderConfig.EncodeLevel = zapcore.CapitalColorLevelEncoder

	s.logger = testing.NopLogger()

	dbcli, err := NewClient(context.Background(), s.logger, s.endpoint, s.prefix, ydb.AuthTokenCredentials{AuthToken: s.token}, s.trace)
	if err != nil {
		panic(err.Error())
	}
	dbcli.StaleReadTransactionType = SerializableReadWrite // actual stale reads can fail in tests because they are actually fast for some reason
	s.dbClient = dbcli

	Client := scheme.Client{Driver: *s.dbClient.YDBClient.Driver}

	// ex. /local/2019-05-31T16:35:40+03:00
	s.dbClient.Prefix = path.Join(s.prefix, time.Now().Format(time.RFC3339))
	if err := Client.MakeDirectory(s.context, s.dbClient.Prefix); err != nil {
		panic(err)
	}

	if err := dbSchema.CreateTables(s.context, s.dbClient.SessionPool, s.dbClient.Prefix, ""); err != nil {
		panic(err)
	}
}

func (s *DBClientSuite) SetupTest() {
	s.dbClient.SetTimestamper(timestamp.NewMockTimestamper()) // to keep s.Equal working on Updated field
}

func (s *DBClientSuite) TearDownTest() {
	s.dbClient.SetTimestamper(timestamp.Timestamper{}) // to keep s.Equal working on Updated field
}

// ide hack - this allows to group test part using name, but does not allow to run test part without buildup
func (s *DBClientSuite) Subtest(name string, subtest func()) {
	s.Run(name, subtest)
}

func (s *DBClientSuite) dataPreparationFailed(err error) {
	s.FailNow("can't prepare data for test", err.Error())
}

// common helpers
func sortStrings(strings []string) {
	sort.SliceStable(strings, func(i, j int) bool { return strings[i] < strings[j] })
}

// device helpers
func (s *DBClientSuite) checkUserDevices(userID uint64, devices []model.Device) {
	var (
		expectedDevice, actualDevice   model.Device
		expectedDevices, actualDevices []model.Device
		err                            error
	)

	// SelectUserDevices
	if devices == nil {
		expectedDevices = nil
	} else {
		expectedDevices = make([]model.Device, len(devices))
		for i, device := range devices {
			device = device.Clone()
			expectedDevices[i] = device
		}
	}
	actualDevices, err = s.dbClient.SelectUserDevices(s.context, userID)
	s.NoError(err, "checkUserDevices/SelectUserDevices/err")
	sortDevices(expectedDevices)
	sortDevices(actualDevices)
	s.Equal(expectedDevices, actualDevices, "checkUserDevices/SelectUserDevices/actualDevices")

	// SelectUserDevice
	for _, device := range devices {
		expectedDevice = device
		actualDevice, err = s.dbClient.SelectUserDevice(s.context, userID, device.ID)
		s.NoErrorf(err, "checkUserDevices/SelectUserDevice/%s/err", device.ID)
		sortDevices([]model.Device{expectedDevice})
		sortDevices([]model.Device{actualDevice})
		s.Equalf(expectedDevice, actualDevice, "checkUserDevices/SelectUserDevice/%s/actualDevice", device.ID)
	}

	// SelectUserDevicesSimple
	if devices == nil {
		expectedDevices = nil
	} else {
		expectedDevices = make([]model.Device, len(devices))
		for i, device := range devices {
			expectedDevices[i] = formatDeviceSelectUserDevicesSimple(s.context, device)
		}
	}
	actualDevices, err = s.dbClient.SelectUserDevicesSimple(s.context, userID)
	s.NoError(err, "checkUserDevices/SelectUserDevicesSimple/err")
	sortDevices(expectedDevices)
	sortDevices(actualDevices)
	s.Equal(expectedDevices, actualDevices, "checkUserDevices/SelectUserDevicesSimple/actualDevices")

	// SelectUserDeviceSimple
	for _, device := range devices {
		expectedDevice = formatDeviceSelectUserDevicesSimple(s.context, device)
		actualDevice, err = s.dbClient.SelectUserDeviceSimple(s.context, userID, device.ID)
		s.NoErrorf(err, "checkUserDevices/SelectUserDeviceSimple/%s/err", device.ID)
		sortDevices([]model.Device{expectedDevice})
		sortDevices([]model.Device{actualDevice})
		s.Equalf(expectedDevice, actualDevice, "checkUserDevices/SelectUserDeviceSimple/%s/actualDevice", device.ID)
	}
}

func (s *DBClientSuite) checkUserProviderDevices(userID uint64, skillID string, devices []model.Device) {
	var (
		expectedDevices, actualDevices []model.Device
		err                            error
	)

	// SelectUserProviderDevicesSimple
	if devices == nil {
		expectedDevices = nil
	} else {
		expectedDevices = make([]model.Device, len(devices))
		for i, device := range devices {
			expectedDevices[i] = formatDeviceSelectUserDevicesSimple(s.context, device)
		}
	}
	actualDevices, err = s.dbClient.SelectUserProviderDevicesSimple(s.context, userID, skillID)
	s.NoError(err, "checkUserProviderDevices/SelectUserProviderDevicesSimple/err")
	sortDevices(expectedDevices)
	sortDevices(actualDevices)
	s.Equal(expectedDevices, actualDevices, "checkUserProviderDevices/SelectUserProviderDevicesSimple/actualDevices")
}

func (s *DBClientSuite) checkUserProviderArchivedDevices(userID uint64, skillID string, devicesMap map[string]model.Device) {
	var (
		expectedDevicesMap, actualDevicesMap map[string]model.Device
		err                                  error
	)

	// SelectUserProviderDevicesSimple
	if devicesMap == nil {
		expectedDevicesMap = nil
	} else {
		expectedDevicesMap = make(map[string]model.Device, len(devicesMap))
		for key, device := range devicesMap {
			expectedDevicesMap[key] = formatDeviceSelectUserDevicesSimple(s.context, device)
		}
	}
	actualDevicesMap, err = s.dbClient.SelectUserProviderArchivedDevicesSimple(s.context, userID, skillID)
	s.NoError(err, "checkUserProviderArchivedDevices/SelectUserProviderArchivedDevicesSimple/err")
	s.Equal(expectedDevicesMap, actualDevicesMap, "checkUserProviderArchivedDevices/SelectUserProviderArchivedDevicesSimple/actualDevices")
}

func (s *DBClientSuite) checkUserGroupDevices(userID uint64, groupID string, devices []model.Device) {
	var (
		expectedDevices, actualDevices []model.Device
		err                            error
	)

	// SelectUserGroupDevices
	if devices == nil {
		expectedDevices = nil
	} else {
		expectedDevices = make([]model.Device, len(devices))
		copy(expectedDevices, devices)
	}
	actualDevices, err = s.dbClient.SelectUserGroupDevices(s.context, userID, groupID)
	s.NoError(err, "checkUserGroupDevices/SelectUserGroupDevices/err")
	sortDevices(expectedDevices)
	sortDevices(actualDevices)
	s.Equal(expectedDevices, actualDevices, "checkUserGroupDevices/SelectUserGroupDevices/actualDevices")
}

func formatDeviceStoreUserDevice(ctx context.Context, src model.Device) (dst model.Device) {
	dst.ID = src.ID
	dst.Name = tools.StandardizeSpaces(src.ExternalName)
	dst.Aliases = src.Aliases
	dst.Description = src.Description
	dst.ExternalID = src.ExternalID
	dst.ExternalName = src.ExternalName
	dst.SkillID = src.SkillID
	dst.Type = src.Type
	dst.OriginalType = src.OriginalType
	dst.Room = src.Room
	dst.Groups = src.Groups
	dst.Capabilities = src.Capabilities
	dst.Properties = src.Properties
	dst.HouseholdID = src.HouseholdID
	if src.DeviceInfo == nil {
		dst.DeviceInfo = &model.DeviceInfo{}
	} else {
		dst.DeviceInfo = src.DeviceInfo
	}
	dst.CustomData = src.CustomData
	dst.Updated = src.Updated
	dst.Created = timestamp.CurrentTimestampMock // CurrentTimestamp is mocked and frozen to `1`
	dst.Status = src.Status
	dst.StatusUpdated = src.StatusUpdated
	dst.Favorite = src.Favorite
	dst.InternalConfig = src.InternalConfig.Clone()
	dst.SharingInfo = src.SharingInfo.Clone()
	return
}

func formatDeviceSelectUserDevicesSimple(ctx context.Context, src model.Device) (dst model.Device) {
	dst.ID = src.ID
	dst.Name = src.Name
	dst.Aliases = src.Aliases
	dst.Description = src.Description
	dst.ExternalID = src.ExternalID
	dst.ExternalName = src.ExternalName
	dst.SkillID = src.SkillID
	dst.Type = src.Type
	dst.OriginalType = src.OriginalType
	dst.Room = nil
	dst.Groups = nil
	dst.Capabilities = src.Capabilities
	dst.Properties = src.Properties
	dst.HouseholdID = src.HouseholdID
	if src.DeviceInfo == nil {
		dst.DeviceInfo = &model.DeviceInfo{}
	} else {
		dst.DeviceInfo = src.DeviceInfo
	}
	dst.CustomData = src.CustomData
	dst.Updated = src.Updated
	dst.Created = timestamp.CurrentTimestampMock // CurrentTimestamp is mocked and frozen to `1`
	dst.Status = src.Status
	dst.StatusUpdated = src.StatusUpdated
	dst.InternalConfig = src.InternalConfig.Clone()
	dst.SharingInfo = src.SharingInfo.Clone()
	return
}

func formatDeviceSelectUserInfo(ctx context.Context, src model.Device) (dst model.Device) {
	dst.ID = src.ID
	dst.Name = tools.StandardizeSpaces(src.ExternalName)
	dst.Aliases = src.Aliases
	dst.Description = src.Description
	dst.ExternalID = src.ExternalID
	dst.ExternalName = src.ExternalName
	dst.SkillID = src.SkillID
	dst.Type = src.Type
	dst.OriginalType = src.OriginalType
	if src.Room != nil {
		dst.Room = src.Room
	}
	dst.HouseholdID = src.HouseholdID
	dst.Groups = make([]model.Group, 0)
	if src.Groups != nil {
		dst.Groups = src.Groups
	}
	dst.Capabilities = src.Capabilities
	dst.Properties = src.Properties
	if src.DeviceInfo == nil {
		dst.DeviceInfo = &model.DeviceInfo{}
	} else {
		dst.DeviceInfo = src.DeviceInfo
	}
	dst.CustomData = src.CustomData
	dst.Updated = src.Updated
	dst.Created = timestamp.CurrentTimestampMock // CurrentTimestamp is mocked and frozen to `1`
	dst.Status = src.Status
	dst.StatusUpdated = src.StatusUpdated
	dst.SharingInfo = src.SharingInfo.Clone()
	return
}

func sortDevices(devices []model.Device) {
	sort.SliceStable(devices, func(i, j int) bool { return devices[i].ID < devices[j].ID })
	for i := range devices {
		sortGroups(devices[i].Groups)
	}
}

// group helpers
func (s *DBClientSuite) checkUserGroups(userID uint64, groups []model.Group, _ []model.Group) {
	var (
		expectedGroup, actualGroup   model.Group
		expectedGroups, actualGroups []model.Group
		err                          error
	)

	// SelectUserGroups
	if groups == nil {
		expectedGroups = nil
	} else {
		expectedGroups = make([]model.Group, len(groups))
		copy(expectedGroups, groups)
	}
	actualGroups, err = s.dbClient.SelectUserGroups(s.context, userID)
	s.NoError(err, "checkUserGroups/SelectUserGroups/err")
	sortGroups(expectedGroups)
	sortGroups(actualGroups)
	s.Equal(expectedGroups, actualGroups, "checkUserGroups/SelectUserGroups/actualGroups")

	// SelectUserGroup
	for _, group := range groups {
		expectedGroup = group
		actualGroup, err = s.dbClient.SelectUserGroup(s.context, userID, group.ID)
		s.NoErrorf(err, "checkUserGroups/SelectUserGroup/%s/err", group.ID)
		sortGroups([]model.Group{expectedGroup})
		sortGroups([]model.Group{actualGroup})
		s.Equalf(expectedGroup, actualGroup, "checkUserGroups/SelectUserGroup/%s/actualGroup", group.ID)
	}
}

func formatGroupSelectUserDevice(src model.Group) (dst model.Group) {
	dst.ID = src.ID
	dst.Name = src.Name
	dst.Aliases = src.Aliases
	dst.Type = src.Type
	return
}

func sortGroups(groups []model.Group) {
	sort.SliceStable(groups, func(i, j int) bool { return groups[i].ID < groups[j].ID })
	for i := range groups {
		sortStrings(groups[i].Devices)
	}
}

// room helpers
func (s *DBClientSuite) checkUserRooms(userID uint64, rooms []model.Room, _ []model.Room) {
	var (
		expectedRoom, actualRoom   model.Room
		expectedRooms, actualRooms []model.Room
		err                        error
	)

	// SelectUserRooms
	if rooms == nil {
		expectedRooms = nil
	} else {
		expectedRooms = make([]model.Room, len(rooms))
		copy(expectedRooms, rooms)
	}
	actualRooms, err = s.dbClient.SelectUserRooms(s.context, userID)
	s.NoError(err, "checkUserRooms/SelectUserRooms/err")
	sortRooms(expectedRooms)
	sortRooms(actualRooms)
	s.Equal(expectedRooms, actualRooms, "checkUserRooms/SelectUserRooms/actualRooms")

	// SelectUserRoom
	for _, room := range rooms {
		expectedRoom = room
		actualRoom, err = s.dbClient.SelectUserRoom(s.context, userID, room.ID)
		s.NoErrorf(err, "checkUserRooms/SelectUserRoom/%s/err", room.ID)
		sortRooms([]model.Room{expectedRoom})
		sortRooms([]model.Room{actualRoom})
		s.Equalf(expectedRoom, actualRoom, "checkUserRooms/SelectUserRoom/%s/actualRoom", room.ID)
	}
}

func sortRooms(rooms []model.Room) {
	sort.SliceStable(rooms, func(i, j int) bool { return rooms[i].ID < rooms[j].ID })
	for i := range rooms {
		sortStrings(rooms[i].Devices)
	}
}

func formatRoomSelectUserDevice(src model.Room) (dst *model.Room) {
	room := model.Room{
		ID:   src.ID,
		Name: src.Name,
	}
	return &room
}

// scenario helpers
func (s *DBClientSuite) checkUserScenarios(userID uint64, scenarios []model.Scenario) {
	var (
		expectedScenario                   model.Scenario
		expectedScenarios, actualScenarios []model.Scenario
		err                                error
	)

	// SelectUserScenarios
	if scenarios == nil {
		expectedScenarios = nil
	} else {
		expectedScenarios = make([]model.Scenario, len(scenarios))
		copy(expectedScenarios, scenarios)
	}
	actualScenarios, err = s.dbClient.SelectUserScenarios(s.context, userID)
	s.NoError(err, "checkUserScenarios/SelectUserScenarios/err")
	sortScenarios(expectedScenarios)
	sortScenarios(actualScenarios)
	s.Equal(expectedScenarios, actualScenarios, "checkUserScenarios/SelectUserScenarios/actualScenarios")

	// SelectUserScenario
	for _, scenario := range scenarios {
		expectedScenario = scenario
		actualScenario, err := s.dbClient.SelectScenario(s.context, userID, scenario.ID)
		s.IsType(model.Scenario{}, actualScenario)
		s.NoErrorf(err, "checkUserScenarios/SelectUserScenario/%s/err", scenario.ID)
		sortScenarios([]model.Scenario{expectedScenario})
		sortScenarios([]model.Scenario{actualScenario})
		s.Equalf(expectedScenario, actualScenario, "checkUserScenarios/SelectUserScenario/%s/actualScenario", scenario.ID)
	}
}

func sortScenarios(scenarios []model.Scenario) {
	sort.SliceStable(scenarios, func(i, j int) bool { return scenarios[i].ID < scenarios[j].ID })
	for i := range scenarios {
		sortScenarioDevices(scenarios[i].Devices)
	}
}

func sortScenarioDevices(sds []model.ScenarioDevice) {
	sort.SliceStable(sds, func(i, j int) bool { return sds[i].ID < sds[j].ID })
}

// userInfo helpers
func (s *DBClientSuite) checkUserInfo(userID uint64, expected model.UserInfo, expectedErr error) {
	var (
		expectedDevices, actualDevices                       []model.Device
		expectedGroups, actualGroups                         []model.Group
		expectedRooms, actualRooms                           []model.Room
		expectedScenarios, actualScenarios                   []model.Scenario
		expectedStereopairs, actualStereopairs               model.Stereopairs
		expectedHouseHolds, actualHouseholds                 model.Households
		expectedCurrentHouseholdID, actualCurrentHouseholdID string
	)

	actual, err := s.dbClient.SelectUserInfo(s.context, userID)
	if expectedErr == nil {
		s.NoError(err, "checkUserInfo/SelectUserInfo")
	} else {
		s.ErrorIs(err, expectedErr, "checkUserInfo/SelectUserInfo")
		// content of userinfo no need if error handled
		return
	}

	// devices
	if expected.Devices == nil {
		expectedDevices = nil
	} else {
		expectedDevices = make([]model.Device, len(expected.Devices))
		for i, device := range expected.Devices {
			expectedDevices[i] = formatDeviceSelectUserInfo(s.context, device)
		}
	}
	actualDevices = actual.Devices
	sortDevices(expectedDevices)
	sortDevices(actualDevices)
	s.Equal(expectedDevices, actualDevices, "checkUserInfo/devices")

	// groups
	if expected.Groups == nil {
		expectedGroups = []model.Group{}
	} else {
		expectedGroups = make([]model.Group, len(expected.Groups))
		copy(expectedGroups, expected.Groups)
	}
	actualGroups = actual.Groups
	sortGroups(expectedGroups)
	sortGroups(actualGroups)
	s.Equal(expectedGroups, actualGroups, "checkUserInfo/groups")

	// rooms
	if expected.Rooms == nil {
		expectedRooms = []model.Room{}
	} else {
		expectedRooms = make([]model.Room, len(expected.Rooms))
		copy(expectedRooms, expected.Rooms)
	}
	actualRooms = actual.Rooms
	sortRooms(expectedRooms)
	sortRooms(actualRooms)
	s.Equal(expectedRooms, actualRooms, "checkUserInfo/rooms")

	// scenarios
	if expected.Scenarios == nil {
		expectedScenarios = nil
	} else {
		expectedScenarios = make([]model.Scenario, len(expected.Scenarios))
		copy(expectedScenarios, expected.Scenarios)
	}
	actualScenarios = actual.Scenarios
	sortScenarios(expectedScenarios)
	sortScenarios(actualScenarios)
	s.Equal(expectedScenarios, actualScenarios, "checkUserInfo/scenarios")

	// stereopairs
	if expected.Stereopairs == nil {
		expectedStereopairs = make(model.Stereopairs, 0)
	} else {
		expectedStereopairs = expected.Stereopairs
	}
	actualStereopairs = actual.Stereopairs
	sort.Sort(expectedStereopairs)
	sort.Sort(actualStereopairs)
	s.Equal(expectedStereopairs, actualStereopairs, "checkUserInfo/stereopairs")

	// households
	if expected.Households == nil {
		expectedHouseHolds = make(model.Households, 0)
	} else {
		expectedHouseHolds = expected.Households
	}
	actualHouseholds = actual.Households
	sortHouseholds(expectedHouseHolds)
	sortHouseholds(actualHouseholds)
	s.Equal(expectedHouseHolds, actualHouseholds, "checkUserInfo/households")

	// currentHouseholdID
	expectedCurrentHouseholdID = expected.CurrentHouseholdID
	actualCurrentHouseholdID = actual.CurrentHouseholdID
	s.Equal(expectedCurrentHouseholdID, actualCurrentHouseholdID, "checkUserInfo/CurrentHouseholdID")
}

func sortNetworks(networks []model.Network) {
	sort.SliceStable(networks, func(i, j int) bool { return networks[i].SSID < networks[j].SSID })
}

func (s *DBClientSuite) checkNetworks(userID uint64, expected []model.Network) {
	expectedNetworks := make([]model.Network, 0, len(expected))
	expectedNetworks = append(expectedNetworks, expected...)
	actual, err := s.dbClient.SelectUserNetworks(s.context, userID)
	s.NoError(err, "checkUserNetworks/SelectUserNetworks")

	actualNetworks := make([]model.Network, 0, len(actual))
	actualNetworks = append(actualNetworks, actual...)

	sortNetworks(expectedNetworks)
	sortNetworks(actualNetworks)

	s.Equal(expectedNetworks, actualNetworks)
}

func sortHouseholds(households []model.Household) {
	sort.SliceStable(households, func(i, j int) bool { return households[i].ID < households[j].ID })
}

func (s *DBClientSuite) checkHouseholds(userID uint64, expected []model.Household) {
	expectedHouseholds := make([]model.Household, 0, len(expected))
	expectedHouseholds = append(expectedHouseholds, expected...)
	actual, err := s.dbClient.SelectUserHouseholds(s.context, userID)
	s.NoError(err, "checkUserHouseholds/SelectUserHouseholds")

	actualHouseholds := make([]model.Household, 0, len(actual))
	actualHouseholds = append(actualHouseholds, actual...)

	sortHouseholds(expectedHouseholds)
	sortHouseholds(actualHouseholds)

	s.Equal(expectedHouseholds, actualHouseholds)
}

func (s *DBClientSuite) checkSharingInfos(userID uint64, expected model.SharingInfos) {
	expectedSharingInfos := make(model.SharingInfos, 0, len(expected))
	expectedSharingInfos = append(expectedSharingInfos, expected...)
	actual, err := s.dbClient.SelectGuestSharingInfos(s.context, userID)
	s.NoError(err, "checkUserSharingInfos/SelectUserSharingInfos")

	actualSharingInfos := make(model.SharingInfos, 0, len(actual))
	actualSharingInfos = append(actualSharingInfos, actual...)

	sort.Sort(model.SharingInfoSorting(expectedSharingInfos))
	sort.Sort(model.SharingInfoSorting(actualSharingInfos))

	s.Equal(expectedSharingInfos, actualSharingInfos)
}
