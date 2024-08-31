package main

import (
	"context"
	"fmt"
	"os"
	"sync"
	"time"

	"a.yandex-team.ru/alice/iot/adapters/xiaomi_adapter/xiaomi/iotapi"
	"a.yandex-team.ru/alice/iot/bulbasaur/db"
	"a.yandex-team.ru/alice/iot/bulbasaur/model"
	"a.yandex-team.ru/alice/library/go/cli"
	"a.yandex-team.ru/alice/library/go/socialism"
	"a.yandex-team.ru/kikimr/public/sdk/go/ydb"
	"a.yandex-team.ru/library/go/core/log/zap"
	"github.com/go-resty/resty/v2"
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

func do(ctx context.Context, dbClient *DBClient, socialClient socialism.IClient, checker *Checker) error {
	skillID := model.XiaomiSkill
	appName := "d834a181d8594cbea49b043406442d8c"
	skillInfo := socialism.NewSkillInfo(skillID, appName, false)

	type regionInfo struct {
		userCount     int
		devices       map[model.DeviceType]int
		xmodelDevices map[string]int
	}

	regionsInfo := map[iotapi.Region]*regionInfo{
		iotapi.ChinaRegion:     {0, make(map[model.DeviceType]int), make(map[string]int)},
		iotapi.RussiaRegion:    {0, make(map[model.DeviceType]int), make(map[string]int)},
		iotapi.EuropeRegion:    {0, make(map[model.DeviceType]int), make(map[string]int)},
		iotapi.SingaporeRegion: {0, make(map[model.DeviceType]int), make(map[string]int)},
		iotapi.USWestRegion:    {0, make(map[model.DeviceType]int), make(map[string]int)},
	}

	workers := 50
	skillUserIDs := dbClient.GetSkillUserIDs(ctx, skillID)
	skillUserIDsCh := make(chan uint64, len(skillUserIDs))
	userDevicesInfoCh := make(chan map[iotapi.Region]userDevicesInfo, len(skillUserIDs))

	var wg sync.WaitGroup
	for i := 0; i < workers; i++ {
		wg.Add(1)
		go func() {
			defer wg.Done()
			for userID := range skillUserIDsCh {
				token, err := socialClient.GetUserApplicationToken(ctx, userID, skillInfo)
				if err != nil {
					logger.Errorf("fail to get token %v", err)
					continue
				}

				userDevicesInfo, err := checker.GetUserDevicesInfo(ctx, token)
				if err != nil {
					logger.Errorf("fail to get user devices %v", err)
				}
				userDevicesInfoCh <- userDevicesInfo
			}
		}()
	}

	for _, skillUserID := range skillUserIDs {
		skillUserIDsCh <- skillUserID
	}
	close(skillUserIDsCh)

	go func() {
		wg.Wait()
		close(userDevicesInfoCh)
	}()

	for userDevicesInfo := range userDevicesInfoCh {
		for region, devicesInfo := range userDevicesInfo {
			regionsInfo[region].userCount++
			for _, device := range devicesInfo.devicesInfoView {
				regionsInfo[region].devices[device.Type]++
			}
			for _, device := range devicesInfo.devicesXmodel {
				regionsInfo[region].xmodelDevices[device.Type]++
			}
		}
	}

	for region, result := range regionsInfo {
		fmt.Printf("Region: %v - %v users\n", region, result.userCount)
		for device, userCount := range result.devices {
			fmt.Printf("Device %v - %v\n", device, userCount)
		}
		for xmodelDevice, userCount := range result.xmodelDevices {
			fmt.Printf("xmodelDevice %v - %v\n", xmodelDevice, userCount)
		}
	}
	return nil
}

func main() {
	var stop func()
	logger, stop = initLogging()
	defer stop()

	var exists bool
	var apiURL, endpoint, prefix, token, appID string

	if endpoint, exists = os.LookupEnv("YDB_ENDPOINT"); !exists {
		panic("YDB_ENDPOINT env is not set")
	}

	if prefix, exists = os.LookupEnv("YDB_PREFIX"); !exists {
		panic("YDB_PREFIX env is not set")
	}

	if token, exists = os.LookupEnv("YDB_TOKEN"); !exists {
		panic("YDB_TOKEN env is not set")
	}

	if host, exists := os.LookupEnv("SOCIALISM_URL"); !exists {
		panic("SOCIALISM_URL env is not set")
	} else {
		apiURL = fmt.Sprintf("%s/api", host)
	}

	if appID, exists = os.LookupEnv("XIAOMI_APP_ID"); !exists {
		panic("XIAOMI_APP_ID env is not set")
	}

	_, trace := os.LookupEnv("YDB_DEBUG")

	fmt.Print(endpoint, prefix, token, trace)

	dbClient, err := db.NewClient(context.Background(), logger, endpoint, prefix, ydb.AuthTokenCredentials{AuthToken: token}, trace)
	if err != nil {
		panic(err.Error())
	}

	socialClient := socialism.NewClientWithResty(
		socialism.QuasarConsumer, apiURL, resty.New(), logger, socialism.DefaultRetryPolicyOption,
	)

	checker := &Checker{Logger: logger}
	checker.Init(appID)

	msg := fmt.Sprintf("Do you really want to check Xiaomi regions using UserSkills table at `%s%s`", endpoint, prefix)
	c := cli.AskForConfirmation(msg, logger)
	if !c {
		logger.Info("Bye")
		os.Exit(0)
	}

	ctx := context.Background()
	start := time.Now()
	if err := do(ctx, &DBClient{dbClient}, socialClient, checker); err != nil {
		logger.Fatalf("%v", err)
	}
	logger.Infof("Time elapsed: %v", time.Since(start))
}
