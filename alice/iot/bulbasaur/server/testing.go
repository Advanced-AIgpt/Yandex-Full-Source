package server

import (
	"bytes"
	"context"
	"encoding/json"
	"fmt"
	"io"
	"io/ioutil"
	"net/http"
	"net/http/httptest"
	"os"
	"path"
	"runtime/debug"
	"strconv"
	"strings"
	"time"

	tuyaclient "a.yandex-team.ru/alice/iot/adapters/tuya_adapter/client"
	"a.yandex-team.ru/alice/iot/bulbasaur/controller/deferredevents"
	"a.yandex-team.ru/alice/iot/bulbasaur/controller/discovery"
	historydb "a.yandex-team.ru/alice/iot/bulbasaur/controller/history/db"
	"a.yandex-team.ru/alice/iot/bulbasaur/controller/irhub"
	"a.yandex-team.ru/alice/iot/bulbasaur/controller/localscenarios"
	"a.yandex-team.ru/alice/iot/bulbasaur/controller/notificator"
	"a.yandex-team.ru/alice/iot/bulbasaur/controller/quasarconfig"
	"a.yandex-team.ru/alice/iot/bulbasaur/controller/scenario"
	"a.yandex-team.ru/alice/iot/bulbasaur/controller/scenario/timetable"
	"a.yandex-team.ru/alice/iot/bulbasaur/controller/unlink"
	"a.yandex-team.ru/alice/iot/bulbasaur/megamind"
	steelixclient "a.yandex-team.ru/alice/iot/steelix/client"
	vulpixmegamind "a.yandex-team.ru/alice/iot/vulpix/controller/megamind"
	vulpixdb "a.yandex-team.ru/alice/iot/vulpix/db"
	"a.yandex-team.ru/alice/library/go/libquasar"
	"a.yandex-team.ru/library/go/core/log"

	"github.com/golang/protobuf/proto"
	"github.com/stretchr/testify/suite"
	"go.uber.org/zap/zaptest/observer"

	"a.yandex-team.ru/alice/iot/bulbasaur/bass"
	"a.yandex-team.ru/alice/iot/bulbasaur/begemot"
	"a.yandex-team.ru/alice/iot/bulbasaur/config"
	"a.yandex-team.ru/alice/iot/bulbasaur/controller/action"
	"a.yandex-team.ru/alice/iot/bulbasaur/controller/experiments"
	"a.yandex-team.ru/alice/iot/bulbasaur/controller/history"
	historyDBSchema "a.yandex-team.ru/alice/iot/bulbasaur/controller/history/db/schema"
	"a.yandex-team.ru/alice/iot/bulbasaur/controller/query"
	"a.yandex-team.ru/alice/iot/bulbasaur/controller/repository"
	"a.yandex-team.ru/alice/iot/bulbasaur/controller/settings"
	"a.yandex-team.ru/alice/iot/bulbasaur/controller/sup"
	"a.yandex-team.ru/alice/iot/bulbasaur/controller/timemachine"
	"a.yandex-team.ru/alice/iot/bulbasaur/controller/updates"
	"a.yandex-team.ru/alice/iot/bulbasaur/db"
	"a.yandex-team.ru/alice/iot/bulbasaur/db/dbfiller"
	dbSchema "a.yandex-team.ru/alice/iot/bulbasaur/db/schema"
	"a.yandex-team.ru/alice/iot/bulbasaur/model"
	"a.yandex-team.ru/alice/iot/bulbasaur/provider"
	"a.yandex-team.ru/alice/iot/bulbasaur/render"
	"a.yandex-team.ru/alice/iot/bulbasaur/testing"
	"a.yandex-team.ru/alice/library/go/blackbox"
	"a.yandex-team.ru/alice/library/go/cipher"
	"a.yandex-team.ru/alice/library/go/csrf"
	"a.yandex-team.ru/alice/library/go/datasync"
	"a.yandex-team.ru/alice/library/go/dialogs"
	"a.yandex-team.ru/alice/library/go/geosuggest"
	"a.yandex-team.ru/alice/library/go/inflector"
	"a.yandex-team.ru/alice/library/go/jsonmatcher"
	libmemento "a.yandex-team.ru/alice/library/go/memento"
	quasarmetrics "a.yandex-team.ru/alice/library/go/metrics"
	libnotificator "a.yandex-team.ru/alice/library/go/notificator"
	"a.yandex-team.ru/alice/library/go/recorder"
	r "a.yandex-team.ru/alice/library/go/render"
	"a.yandex-team.ru/alice/library/go/requestid"
	"a.yandex-team.ru/alice/library/go/socialism"
	libsup "a.yandex-team.ru/alice/library/go/sup"
	"a.yandex-team.ru/alice/library/go/timestamp"
	quasartvm "a.yandex-team.ru/alice/library/go/tvm"
	"a.yandex-team.ru/alice/library/go/userctx"
	"a.yandex-team.ru/alice/library/go/xiva"
	"a.yandex-team.ru/alice/megamind/protos/common"
	"a.yandex-team.ru/alice/megamind/protos/scenarios"
	"a.yandex-team.ru/kikimr/public/sdk/go/ydb"
	"a.yandex-team.ru/kikimr/public/sdk/go/ydb/scheme"
	"a.yandex-team.ru/library/go/core/log/zap"
	"a.yandex-team.ru/library/go/core/metrics/solomon"
	"a.yandex-team.ru/library/go/core/xerrors"
	"a.yandex-team.ru/library/go/yandex/tvm"
)

type ServerSuite struct {
	suite.Suite

	takeoutTvmID         string
	steelixTvmID         string
	timeMachineTvmID     string
	dbCredentials        dbCredentials
	historyDBCredentials dbCredentials
	trace                bool
}

type dbCredentials struct {
	endpoint    string
	prefix      string
	suitePrefix string
	token       string
}

func (credentials *dbCredentials) SetupSuitePrefix(now time.Time) {
	credentials.suitePrefix = path.Join(credentials.prefix, now.Format(time.RFC3339))
}

func (suite *ServerSuite) Group(name string, subtests func()) {
	suite.Run(name, subtests) // no other way to group tests in go
}

func (suite *ServerSuite) RunServerTest(name string, subtest func(server *TestServer, dbfiller *dbfiller.Filler)) {
	server := suite.newTestServer()

	filler := dbfiller.NewFiller(server.logger, server.dbClient)
	suite.Run(name, func() { subtest(server, filler) })
}

type megamindSubtest struct {
	testRun   func(server *TestServer, user *model.User, runRequestData []byte) (*scenarios.TScenarioRunResponse, error)
	testApply func(server *TestServer, user *model.User, applyRequestData []byte)
}

func (suite *ServerSuite) RunMegamindTest(testdata string, subtest megamindSubtest) {
	server := suite.newTestServer()
	filler := dbfiller.NewFiller(server.logger, server.dbClient)

	file, err := os.Open(testdata)
	suite.Require().NoError(err)
	data, err := ioutil.ReadAll(file)
	suite.Require().NoError(err)

	var runRequest scenarios.TScenarioRunRequest
	err = proto.Unmarshal(data, &runRequest)
	suite.Require().NoError(err)

	protoUserInfo := runRequest.GetDataSources()[int32(common.EDataSourceType_IOT_USER_INFO)].GetIoTUserInfo()
	var userInfo model.UserInfo
	err = userInfo.FromUserInfoProto(context.Background(), protoUserInfo)
	suite.Require().NoError(err)
	// for backward compatibility: some tests do not contain current household id yet
	if userInfo.CurrentHouseholdID == "" && len(userInfo.Households) == 1 {
		userInfo.CurrentHouseholdID = userInfo.Households[0].ID
	}
	alice, replaces, err := filler.InsertUserInfo(server.ctx, userInfo)
	suite.Require().NoError(err)

	data = replaces.ApplyToData(data)
	runResponse, err := subtest.testRun(server, alice, data)
	if subtest.testApply != nil {
		if err != nil {
			suite.Fail(fmt.Sprintf("can't run apply after run failed: %v", err))
			return
		}
		applyRequest := &scenarios.TScenarioApplyRequest{
			BaseRequest: runRequest.GetBaseRequest(),
			Arguments:   runResponse.GetApplyArguments(),
		}
		applyRequestData, err := proto.Marshal(applyRequest)
		suite.Require().NoError(err)
		subtest.testApply(server, alice, applyRequestData)
	}
}

func (suite *ServerSuite) CheckJSONResponseMatch(server *TestServer, expectedCode, actualCode int, expectedBody, actualBody string) bool {
	return suite.Equal(expectedCode, actualCode, server.Logs()) && suite.JSONContentsMatch(expectedBody, actualBody, server.Logs())
}

func (suite *ServerSuite) JSONResponseMatch(server *TestServer, req *request, expectedCode int, expectedBody string) bool {
	actualCode, _, actualBody := server.doRequest(req)
	return suite.CheckJSONResponseMatch(server, expectedCode, actualCode, expectedBody, actualBody)
}

func (suite *ServerSuite) ProtoResponseEqual(server *TestServer, req *request, expectedCode int, expectedProto proto.Message) bool {
	actualCode, _, actualBody := server.doRequest(req)
	expectedProtoBytes, err := proto.Marshal(expectedProto)
	suite.NoError(err)
	return suite.Equal(expectedCode, actualCode, server.Logs()) && suite.EqualValues(expectedProtoBytes, actualBody, server.Logs())
}

func (suite *ServerSuite) prepareEnvVariables() map[string]string {
	backupEnvVariables := make(map[string]string)

	backupEnvVariables["TAKEOUT_TVM_ID"] = os.Getenv("TAKEOUT_TVM_ID")
	_ = os.Setenv("TAKEOUT_TVM_ID", suite.takeoutTvmID)

	backupEnvVariables["STEELIX_TVM_ID"] = os.Getenv("STEELIX_TVM_ID")
	_ = os.Setenv("STEELIX_TVM_ID", suite.steelixTvmID)

	backupEnvVariables["TIME_MACHINE_TVM_ID"] = os.Getenv("TIME_MACHINE_TVM_ID")
	_ = os.Setenv("TIME_MACHINE_TVM_ID", suite.timeMachineTvmID)

	return backupEnvVariables
}

func (suite *ServerSuite) restoreEnvVariables(backupEnvVariables map[string]string) {
	for name, value := range backupEnvVariables {
		_ = os.Setenv(name, value)
	}
}

func (suite *ServerSuite) JSONContentsMatch(expected, actual string, msgAndArgs ...interface{}) bool {
	return suite.NoError(jsonmatcher.JSONContentsMatch(expected, actual), msgAndArgs...)
}

func (suite *ServerSuite) SetupSuite() {
	ctx := context.Background()
	logger, _ := testing.ObservedLogger()
	now := time.Now()

	suite.dbCredentials.SetupSuitePrefix(now)
	suite.SetupDB(ctx, logger, suite.dbCredentials)

	suite.historyDBCredentials.SetupSuitePrefix(now)
	suite.SetupHistoryDB(ctx, logger, suite.historyDBCredentials)
}

func (suite *ServerSuite) SetupDB(ctx context.Context, logger log.Logger, credentials dbCredentials) {
	dbClient, err := db.NewClient(context.Background(), logger, credentials.endpoint, credentials.prefix, ydb.AuthTokenCredentials{AuthToken: credentials.token}, suite.trace)
	if err != nil {
		panic(err.Error())
	}
	schemeClient := scheme.Client{Driver: *dbClient.YDBClient.Driver}

	if err := schemeClient.MakeDirectory(ctx, credentials.suitePrefix); err != nil {
		suite.T().Fatalf("Error while preparing directory for suite, prefix:%s, err: %s", credentials.suitePrefix, err)
	}
	if err := dbSchema.CreateTables(ctx, dbClient.SessionPool, credentials.suitePrefix, ""); err != nil {
		suite.T().Fatalf("Error while creating tables, prefix: %s, err: %s", credentials.suitePrefix, err)
	}
}

func (suite *ServerSuite) SetupHistoryDB(ctx context.Context, logger log.Logger, credentials dbCredentials) {
	dbClient, err := db.NewClient(context.Background(), logger, credentials.endpoint, credentials.prefix, ydb.AuthTokenCredentials{AuthToken: credentials.token}, suite.trace)
	if err != nil {
		panic(err.Error())
	}
	schemeClient := scheme.Client{Driver: *dbClient.YDBClient.Driver}

	if err := schemeClient.MakeDirectory(ctx, credentials.suitePrefix); err != nil {
		suite.T().Fatalf("Error while preparing directory for suite, prefix:%s, err: %s", credentials.suitePrefix, err)
	}
	if err := historyDBSchema.CreateTables(ctx, dbClient.SessionPool, credentials.suitePrefix, ""); err != nil {
		suite.T().Fatalf("Error while creating tables, prefix: %s, err: %s", credentials.suitePrefix, err)
	}
}

func (suite *ServerSuite) newTestServer() *TestServer {
	logger, logs := testing.ObservedLogger()
	logger.Info("Initializing testing server")

	dbClient, err := db.NewClient(context.Background(), logger, suite.dbCredentials.endpoint, suite.dbCredentials.prefix, ydb.AuthTokenCredentials{AuthToken: suite.dbCredentials.token}, suite.trace)
	if err != nil {
		panic(err.Error())
	}
	dbClient.SetTimestamper(timestamp.NewMockTimestamper())
	dbClient.Prefix = suite.dbCredentials.suitePrefix

	historyDBClient, err := historydb.NewClient(context.Background(), logger, suite.historyDBCredentials.endpoint, suite.historyDBCredentials.prefix, ydb.AuthTokenCredentials{AuthToken: suite.historyDBCredentials.token}, suite.trace)
	if err != nil {
		panic(err.Error())
	}
	historyDBClient.Prefix = suite.historyDBCredentials.suitePrefix

	bassMock := bass.NewMock()
	xivaMock := xiva.NewMockClient()
	updatesController := updates.NewController(logger, xivaMock, dbClient, notificator.NewMock())
	historyController := history.NewController(
		historyDBClient,
		logger,
		nil,
		nil,
		history.SolomonShard{},
	)
	testingServer := &TestServer{
		ctx: experiments.ContextWithManager(context.Background(), experiments.MockManager{
			experiments.NotificatorSpeakerActions: true,
		}),
		logger:          logger,
		logs:            logs,
		dbClient:        dbClient,
		historyDBClient: historyDBClient,
		bass:            bassMock,
		begemot:         &begemot.Mock{},
		socialism:       &socialism.Mock{},
		tvm: &quasartvm.ClientMock{
			Logger: logger,
		},
		csrf:    &csrf.CsrfMock{},
		dialogs: &dialogs.DialogsMock{},
		blackbox: &blackbox.ClientMock{
			Logger: logger,
		},
		xiva:               xivaMock,
		pfMock:             provider.NewFactoryMock(logger),
		inflector:          inflector.NewInflectorMock(logger),
		metrics:            solomon.NewRegistry(solomon.NewRegistryOpts()),
		timestamper:        timestamp.NewMockTimestamper(),
		timestamperFactory: timestamp.NewTimestamperFactoryMock(),
		sup:                libsup.NewClientMock(),
		geosuggest:         geosuggest.NewMock(),
		datasync:           datasync.NewMock(),
		timemachine:        timemachine.NewMockTimeMachine(),
		expManagerFactory: &experiments.MockFactory{
			NewManagerFunc: func() experiments.IManager {
				return experiments.MockManager{experiments.NotificatorSpeakerActions: true}
			},
		},
		notificatorClient: libnotificator.NewMock(),
		notificator:       notificator.NewMock(),
		quasar: &quasarconfig.ControllerMock{
			UpdateDevicesLocationMock: func(ctx context.Context, user model.User, devices model.Devices) error {
				return nil
			},
		},
		quasarClient: &libquasar.ClientMock{},
	}

	recorderFactory := recorder.NewDebugInfoRecorderFactory(logger, recorder.MessageFormatter)

	backupEnvVariables := suite.prepareEnvVariables()
	defer suite.restoreEnvVariables(backupEnvVariables)

	steelixTvmID, _ := strconv.ParseUint(suite.steelixTvmID, 10, 32)
	takeoutTvmID, _ := strconv.ParseUint(suite.takeoutTvmID, 10, 32)
	timeMachineTvmID, _ := strconv.ParseUint(suite.timeMachineTvmID, 10, 32)
	testingConfig := config.Config{
		Steelix:     config.Steelix{TVMID: uint32(steelixTvmID)},
		Timemachine: config.Timemachine{TVMID: uint32(timeMachineTvmID)},
		Takeout:     config.Takeout{TVMID: uint32(takeoutTvmID)},
		IotApp:      config.IotApp{ClientIDs: []string{"iot-app-client-id"}},
	}

	server := Server{
		Config:                testingConfig,
		Logger:                testingServer.logger,
		db:                    testingServer.dbClient,
		bass:                  testingServer.bass,
		begemot:               testingServer.begemot,
		csrfTool:              testingServer.csrf,
		socialism:             testingServer.socialism,
		dialogs:               testingServer.dialogs,
		recorderFactory:       recorderFactory,
		tvm:                   testingServer.tvm,
		blackbox:              testingServer.blackbox,
		xiva:                  testingServer.xiva,
		providerFactory:       testingServer.pfMock,
		render:                &render.Render{JSONRenderer: &r.JSONRenderer{Logger: logger}, ProtoRenderer: &r.ProtoRenderer{Logger: logger}},
		inflector:             testingServer.inflector,
		crypter:               &cipher.CrypterMock{},
		bulbasaurMetrics:      testingServer.metrics,
		perfMetrics:           quasarmetrics.NewPerfMetrics(testingServer.metrics),
		providerMetrics:       testingServer.metrics,
		vulpixMetrics:         testingServer.metrics,
		timestamperFactory:    testingServer.timestamperFactory,
		timestamper:           testingServer.timestamper,
		expManagerFactory:     testingServer.expManagerFactory,
		notificatorClient:     testingServer.notificatorClient,
		notificatorController: testingServer.notificator,
		queryController: query.NewController(
			testingServer.logger,
			testingServer.dbClient,
			testingServer.pfMock,
			updatesController,
			historyController,
			testingServer.notificator,
		),
		actionController: &action.Controller{
			ProviderFactory:   testingServer.pfMock,
			Database:          testingServer.dbClient,
			UpdatesController: updatesController,
			Logger:            testingServer.logger,
			RetryPolicy: action.RetryPolicy{
				Type:       action.UniformParallelRetryPolicyType,
				LatencyMs:  100,
				RetryCount: 0,
			},
			NotificatorController: testingServer.notificator,
			BassClient:            testingServer.bass,
		},
		settingsController: &settings.Controller{
			Logger: testingServer.logger,
			Client: libmemento.NewMock(),
		},
		supController:     &sup.Controller{Logger: testingServer.logger, Client: testingServer.sup},
		updatesController: updatesController,
		timemachine:       testingServer.timemachine,
		repository: &repository.Controller{
			Logger:   testingServer.logger,
			Database: testingServer.dbClient,
		},
		appLinksGenerator:        sup.AppLinksGenerator{},
		geosuggest:               testingServer.geosuggest,
		datasync:                 testingServer.datasync,
		quasarController:         testingServer.quasar,
		quasarClient:             testingServer.quasarClient,
		deferredEventsController: deferredevents.NewController(testingServer.logger, testingServer.timemachine, testingServer.dbClient, "", 0),
		historyController:        historyController,
	}
	server.irHubController = irhub.NewController(logger, server.providerFactory, server.db)
	server.localScenariosController = localscenarios.NewController(
		logger,
		server.notificatorController,
	)
	server.unlinkController = unlink.NewController(
		logger,
		server.db,
		server.notificatorController,
		server.quasarController,
		server.updatesController,
		server.dialogs,
		server.socialism,
		server.providerFactory,
		server.localScenariosController,
	)
	server.scenarioController = &scenario.Controller{
		DB:                       testingServer.dbClient,
		UpdatesController:        server.updatesController,
		Timemachine:              testingServer.timemachine,
		Logger:                   testingServer.logger,
		Timestamper:              testingServer.timestamper,
		ActionController:         server.actionController,
		LinksGenerator:           server.appLinksGenerator,
		SupController:            server.supController,
		TimetableCalculator:      timetable.NewCalculator(&timetable.NopJitter{}),
		LocalScenariosController: server.localScenariosController,
	}

	server.discoveryController = &discovery.Controller{
		ProviderFactory:       server.providerFactory,
		Database:              server.db,
		UpdatesController:     updatesController,
		Logger:                server.Logger,
		Sup:                   server.supController,
		AppLinksGenerator:     server.appLinksGenerator,
		NotificatorController: server.notificatorController,
		UnlinkController:      server.unlinkController,
		QuasarController:      server.quasarController,
	}

	begemotProcessor := &megamind.BegemotProcessor{
		Logger:             server.Logger,
		Inflector:          server.inflector,
		QueryController:    server.queryController,
		ActionController:   server.actionController,
		ScenarioController: server.scenarioController,
		SettingsController: server.settingsController,
	}

	server.bulbasaurMegamind = megamind.NewController(server.Logger, server.repository).
		WithProcessor(begemotProcessor).
		WithRunProcessor(&megamind.TimeSpecifiedProcessor{
			Logger:           server.Logger,
			BegemotProcessor: begemotProcessor,
		})

	server.vulpixMegamind = vulpixmegamind.NewController(server.vulpixMetrics, testingServer.notificatorClient, server.supClient, vulpixdb.NewMock(),
		tuyaclient.NewMock(), steelixclient.NewMock(), server.Logger)

	server.InitCallbackController()
	server.InitRepositoryController()
	server.InitSharingController()
	server.InitCache()
	server.InitFrameRouter()
	server.InitMegamindDispatcher()
	server.InitEndpointCapabilities()
	server.InitServices()
	server.InitAPI()
	server.InitRouter()
	testingServer.server = &server

	return testingServer
}

func (suite *ServerSuite) recoverTestServer(server *TestServer) {
	if rErr := recover(); rErr != nil {
		suite.T().Errorf("Test %s encountered panic: %v\nstacktrace: \n%s\nlogs: \n%s", suite.T().Name(), rErr, debug.Stack(), server.Logs())
	}
}

func (suite *ServerSuite) CheckUserDevices(server *TestServer, user *model.User, expectedDevices model.Devices) {
	actualDevices, err := server.dbClient.SelectUserDevices(server.ctx, user.ID)
	suite.Require().NoError(err, server.Logs())

	expectedDevicesMap := make(map[string]model.Device, len(expectedDevices))
	for i := range expectedDevices {
		device := expectedDevices[i].Clone()
		device.ID = ""
		expectedDevicesMap[device.ExternalKey()] = device
	}
	actualDevicesMap := make(map[string]model.Device, len(actualDevices))
	for i := range actualDevices {
		device := actualDevices[i].Clone()
		device.ID = ""
		actualDevicesMap[device.ExternalKey()] = device
	}

	for key, expectedDevice := range expectedDevicesMap {
		actualDevice, found := actualDevicesMap[key]
		suite.Truef(found, "expected device %s not found in actual devices %v. Logs:\n%s", key, actualDevices.ExternalKeys(), server.Logs())
		suite.Equalf(expectedDevice, actualDevice, "devices mismatch. Logs:\n%s", server.Logs())
	}
	for key := range actualDevicesMap {
		_, found := expectedDevicesMap[key]
		suite.Truef(found, "actual device %s not found in expected devices %v. Logs:\n%s", key, expectedDevices.ExternalKeys(), server.Logs())
	}
}

func (suite *ServerSuite) CheckUserScenarios(server *TestServer, user *model.User, expected []model.Scenario) bool {
	scenarios, err := server.dbClient.SelectUserScenarios(server.ctx, user.ID)
	suite.Require().NoError(err, server.Logs())

	for i := range scenarios {
		suite.Require().True(len(scenarios[i].ID) > 0, server.Logs())
		scenarios[i].ID = "" // we have no idea what id is generated for bulk inserted scenarios -> drop it from tests
	}
	return suite.ElementsMatch(expected, scenarios, server.Logs())
}

func (suite *ServerSuite) GetUserScenariosByName(server *TestServer, user *model.User) map[model.ScenarioName]model.Scenario {
	scenarios, err := server.dbClient.SelectUserScenarios(server.ctx, user.ID)
	suite.Require().NoError(err, server.Logs())

	result := make(map[model.ScenarioName]model.Scenario)
	for _, s := range scenarios {
		result[s.Name] = s
	}
	return result
}

func (suite *ServerSuite) CheckExternalUsers(server *TestServer, externalID, skillID string, expectedUsers []*model.User) {
	users, err := server.dbClient.SelectExternalUsers(server.ctx, externalID, skillID)
	suite.Require().NoError(err, server.Logs())

	actualIDs := make([]uint64, 0, len(users))
	for _, user := range users {
		actualIDs = append(actualIDs, user.ID) // we have no user logins/tvm tickets for external users, only ids
	}
	expectedIDs := make([]uint64, 0, len(expectedUsers))
	for _, user := range expectedUsers {
		expectedIDs = append(expectedIDs, user.ID)
	}
	suite.ElementsMatch(expectedIDs, actualIDs, server.Logs())
}

func (suite *ServerSuite) GetExternalDevice(server *TestServer, user *model.User, externalID, skillID string) (model.Device, error) {
	devices, err := server.dbClient.SelectUserDevices(server.ctx, user.ID)
	suite.Require().NoError(err, server.Logs())

	for i := range devices {
		if devices[i].ExternalID == externalID && devices[i].SkillID == skillID {
			return devices[i], nil
		}
	}
	return model.Device{}, xerrors.New("external device not found")
}

type TestServer struct {
	ctx             context.Context
	logger          *zap.Logger
	logs            *observer.ObservedLogs
	dbClient        *db.DBClient
	historyDBClient *historydb.Client

	bass               *bass.Mock
	begemot            *begemot.Mock
	socialism          *socialism.Mock
	inflector          *inflector.Mock
	tvm                *quasartvm.ClientMock
	blackbox           *blackbox.ClientMock
	csrf               *csrf.CsrfMock
	dialogs            *dialogs.DialogsMock
	pfMock             *provider.FactoryMock
	metrics            *solomon.Registry
	timestamperFactory *timestamp.TimestamperFactoryMock
	timestamper        *timestamp.TimestamperMock
	server             *Server
	xiva               *xiva.MockClient
	sup                *libsup.ClientMock
	geosuggest         *geosuggest.Mock
	datasync           *datasync.Mock
	timemachine        *timemachine.MockTimeMachine
	expManagerFactory  *experiments.MockFactory
	notificatorClient  *libnotificator.Mock
	notificator        *notificator.Mock
	quasar             *quasarconfig.ControllerMock
	quasarClient       *libquasar.ClientMock
}

func (s *TestServer) Logs() string {
	logs := make([]string, 0, s.logs.Len())
	for _, logEntry := range s.logs.All() {
		logs = append(logs, fmt.Sprintf("%s: %s", logEntry.Time.Format("15:04:05"), logEntry.Message))
	}
	return strings.Join(logs, "\n")
}

func (s *TestServer) doRequest(r *request) (int, http.Header, string) {
	request, _ := http.NewRequest(r.method, r.url, r.body)

	request.Header.Set(requestid.XRequestID, r.id)

	request.Header.Set("X-CSRF-Token", "valid")
	if r.isCsrfInvalid {
		// invalidate csrf token (default mock will fail if X-CSRF-Token != valid)
		request.Header.Set("X-CSRF-Token", "invalid")
	}

	if tvmData := r.tvmData; tvmData != nil {
		// add tvm headers to request
		request.Header.Set("X-Ya-Service-Ticket", "default-service-ticket-value")
		if user := tvmData.user; user != nil {
			request.Header.Set("X-Ya-User-Ticket", fmt.Sprintf("%d-%s-user-ticket", user.ID, user.Login))
		}

		// modify tvmMock for server
		s.tvm.SrcServiceID = tvmData.srcServiceID
		if user := tvmData.user; user != nil {
			s.tvm.UserID = tvm.UID(user.ID)
		} else {
			s.tvm.UserID = 0
		}
		defer func() {
			s.tvm.SrcServiceID = 0
			s.tvm.UserID = 0
		}()
	}

	if user := r.blackboxUser; user != nil {
		tvmUserTicket := fmt.Sprintf("%d-%s-user-ticket", user.ID, user.Login)
		s.blackbox.User = &userctx.User{ID: user.ID, Login: user.Login, Ticket: tvmUserTicket}
		defer func() {
			s.blackbox.User = nil
		}()

		request.Header.Set("Cookie", "Session_id=test_session_id_cookie;sessionid2=test_sessionid2_cookie")
		request.Header.Set("X-Forwarded-For-Y", "127.0.0.1")
		request.Header.Set("Authorization", "OAuth test_oauth_token")
	}

	if oauthToken := r.oauthToken; oauthToken != nil {
		s.blackbox.OAuthToken = oauthToken
		defer func() {
			s.blackbox.OAuthToken = nil
		}()
	}

	for key, value := range r.headers {
		request.Header.Set(key, value)
	}

	urlQuery := request.URL.Query()
	for key, value := range r.query {
		urlQuery.Set(key, value)
	}
	request.URL.RawQuery = urlQuery.Encode()

	request.Host = "bulbasaur.testing.server"
	request.RequestURI = request.URL.RequestURI()

	rr := httptest.NewRecorder()
	s.server.Router.ServeHTTP(rr, request)
	return rr.Code, rr.Header(), rr.Body.String()
}

type errBody string

func (e errBody) Read(_ []byte) (n int, err error) {
	return 0, xerrors.New(string(e))
}

type tvmData struct {
	user         *model.User
	srcServiceID tvm.ClientID
}

type (
	JSONObject    map[string]interface{}
	JSONArray     []interface{}
	RawBodyString string
)

type request struct {
	method string
	url    string
	body   io.Reader

	id string

	blackboxUser  *model.User
	oauthToken    *userctx.OAuth
	tvmData       *tvmData
	isCsrfInvalid bool

	headers map[string]string
	query   map[string]string
}

func newRequest(method, url string) *request {
	return &request{method: method, url: url, body: http.NoBody, id: "default-req-id", query: make(map[string]string)}
}

func (r *request) withQueryParameter(key, value string) *request {
	r.query[key] = value
	return r
}

func (r *request) withRequestID(id string) *request {
	r.id = id
	return r
}

func (r *request) withBlackboxUser(blackboxUser *model.User) *request {
	r.blackboxUser = blackboxUser
	return r
}

func (r *request) withOAuth(oauthToken *userctx.OAuth) *request {
	r.oauthToken = oauthToken
	return r
}

func (r *request) withTvmData(tvmData *tvmData) *request {
	r.tvmData = tvmData
	return r
}

func (r *request) withInvalidCSRF() *request {
	r.isCsrfInvalid = true
	return r
}

func (r *request) withHeaders(headers map[string]string) *request {
	r.headers = headers
	return r
}

func (r *request) withBody(rawBody interface{}) *request {
	switch body := rawBody.(type) {
	case []byte:
		r.body = bytes.NewReader(body)
	case RawBodyString:
		r.body = bytes.NewReader([]byte(body))
	default:
		requestBody, _ := json.Marshal(body)
		r.body = bytes.NewReader(requestBody)
	}
	return r
}
