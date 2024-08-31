package main

import (
	"context"
	"encoding/csv"
	"fmt"
	"math/rand"
	"net/http"
	"os"
	"path/filepath"
	"strconv"
	"sync"
	"time"

	"a.yandex-team.ru/alice/library/go/authpolicy"
	"a.yandex-team.ru/alice/library/go/zora"

	"github.com/go-resty/resty/v2"
	uberzap "go.uber.org/zap"
	"go.uber.org/zap/zapcore"

	"a.yandex-team.ru/alice/iot/bulbasaur/bass"
	"a.yandex-team.ru/alice/iot/bulbasaur/controller/notificator"
	"a.yandex-team.ru/alice/iot/bulbasaur/controller/query"
	"a.yandex-team.ru/alice/iot/bulbasaur/controller/updates"
	"a.yandex-team.ru/alice/iot/bulbasaur/db"
	"a.yandex-team.ru/alice/iot/bulbasaur/dto/adapter"
	"a.yandex-team.ru/alice/iot/bulbasaur/model"
	"a.yandex-team.ru/alice/iot/bulbasaur/provider"
	libbass "a.yandex-team.ru/alice/library/go/bass"
	"a.yandex-team.ru/alice/library/go/dialogs"
	"a.yandex-team.ru/alice/library/go/socialism"
	"a.yandex-team.ru/alice/library/go/timestamp"
	quasartvm "a.yandex-team.ru/alice/library/go/tvm"
	"a.yandex-team.ru/alice/library/go/xiva"
	"a.yandex-team.ru/kikimr/public/sdk/go/ydb"
	"a.yandex-team.ru/library/go/core/log/zap"
	"a.yandex-team.ru/library/go/core/metrics/mock"
	"a.yandex-team.ru/yt/go/ypath"
	"a.yandex-team.ru/yt/go/yt"
	"a.yandex-team.ru/yt/go/yt/ythttp"
)

const pingerPath = "alice/iot/bulbasaur/cmd/device_pinger/"

func initLogging(logLevel string) (logger *zap.Logger, stop func(), err error) {
	encoderConfig := uberzap.NewDevelopmentEncoderConfig()
	encoderConfig.EncodeLevel = zapcore.CapitalColorLevelEncoder
	encoderConfig.EncodeCaller = zapcore.ShortCallerEncoder
	encoder := zapcore.NewConsoleEncoder(encoderConfig)

	level := uberzap.DebugLevel
	if logLevel != "" {
		if err = level.UnmarshalText([]byte(logLevel)); err != nil {
			return
		}
	}

	core := zapcore.NewCore(encoder, zapcore.AddSync(os.Stderr), level)
	stop = func() {
		_ = core.Sync()
	}
	logger = zap.NewWithCore(core, uberzap.AddStacktrace(uberzap.FatalLevel), uberzap.AddCaller())

	return
}

func main() {
	config, err := Load(filepath.Join(os.Getenv("ARCADIA_ROOT"), pingerPath))
	if err != nil {
		panic(err)
	}

	resultFile, err := os.OpenFile(
		filepath.Join(
			os.Getenv("ARCADIA_ROOT"),
			pingerPath,
			fmt.Sprintf("result_%s.csv", time.Now().Format("20060102150405"))), os.O_CREATE|os.O_WRONLY, 0644)
	if err != nil {
		panic(err)
	}
	defer func() {
		_ = resultFile.Close()
	}()
	resultWriter := csv.NewWriter(resultFile)
	defer resultWriter.Flush()

	logger, stop, err := initLogging(config.Logging.Level)
	if err != nil {
		panic(err.Error())
	}
	defer stop()

	registry := mock.NewRegistry(mock.NewRegistryOpts())

	dbcli, err := db.NewClient(context.Background(), logger, config.Database.YdbEndpoint, config.Database.YdbPrefix, ydb.AuthTokenCredentials{AuthToken: config.Database.YdbToken}, false)
	if err != nil {
		panic(err.Error())
	}

	tvm, _ := quasartvm.NewClientWithMetrics(
		context.Background(),
		fmt.Sprintf("http://localhost:%d", config.TVM.Port),
		config.TVM.SrcAlias,
		config.TVM.Token,
		registry,
	)

	dialogsClient := &dialogs.Client{Logger: logger}
	dialogsClient.Init(tvm, config.Dialogs.TvmAlias, config.Dialogs.URL, registry, dialogs.BulbasaurCachePolicy)

	socialClient := socialism.NewClientWithResty(
		socialism.QuasarConsumer, config.Socialism.URL, resty.New(), logger, socialism.DefaultRetryPolicyOption,
	)

	notificatorMock := notificator.NewMock()

	qController := query.Controller{
		Logger:            logger,
		Database:          dbcli,
		UpdatesController: updates.NewController(logger, xiva.NewMockClient(), dbcli, notificatorMock),
		ProviderFactory: &provider.Factory{
			Logger:        logger,
			Tvm:           tvm,
			Dialogs:       dialogsClient,
			DefaultClient: &http.Client{Timeout: 60 * time.Second},
			ZoraClient:    zora.NewClient(tvm),
			Socialism:     socialClient,
			Bass: bass.NewClient(
				libbass.NewClient(
					libbass.QuasarService,
					libbass.ProductionEndpoint,
					http.DefaultClient,
					logger,
					&authpolicy.TVMWithClientServicePolicy{
						Client: tvm,
						DstID:  libbass.ProductionTVMID,
					},
				),
			),
			TuyaEndpoint: config.Tuya.AdapterURL,
			SberEndpoint: config.Sber.AdapterURL,
			TuyaTVMAlias: config.Tuya.TVMAlias,

			XiaomiEndpoint: config.Xiaomi.AdapterURL,

			SignalsRegistry: provider.NewSignalsRegistry(registry),
		},
	}

	ytClient, err := ythttp.NewClient(&yt.Config{
		Proxy:             config.Yt.Cluster,
		ReadTokenFromFile: true,
	})
	if err != nil {
		logger.Fatal(err.Error())
	}

	ctx := context.Background()
	if err := do(ctx, config, ytClient, dbcli, qController, resultWriter, logger); err != nil {
		panic(err.Error())
	}
}

func do(ctx context.Context, config Config, yt yt.Client, dbc *db.DBClient, qController query.Controller, resultWriter *csv.Writer, logger *zap.Logger) error {
	var wg sync.WaitGroup
	var mu sync.Mutex
	tablePath := ypath.Path(config.Yt.Input)
	for d := range streamDevices(ctx, yt, tablePath, logger) {
		wg.Add(1)

		// smoothing workload
		time.Sleep(time.Duration(rand.Intn(5)) * time.Second)

		go func(d Device) {
			defer wg.Done()
			logger.Warnf("starting to process device: `%s` from user: `%s`", d.DeviceID, d.UserID)
			for range time.Tick(5 * time.Second) {
				ctx := context.Background()
				deviceStateView, err := ping(ctx, d, dbc, qController)
				if err != nil {
					logger.Warnf("cannot ping device: device id: `%s`, user: `%s`: %v", d.DeviceID, d.UserID, err.Error())
					continue
				}

				var online bool
				for _, v := range deviceStateView {
					if len(v.ErrorCode) == 0 {
						online = true
					}
				}

				output := Output{
					timestamp: float64(timestamp.Now()),
					deviceID:  d.DeviceID,
					status:    online,
				}

				if err := saveData(output, &mu, resultWriter); err != nil {
					logger.Warnf("cannot save data to YT: device id: `%s`, user: `%s`: %v", d.DeviceID, d.UserID, err.Error())
					continue
				}

				logger.Warnf("successfully processed: device id: `%s`, user: `%s`, status: %t", d.DeviceID, d.UserID, online)
			}
		}(d)
	}

	wg.Wait()
	return nil
}

type Output struct {
	timestamp float64 `yson:"timestamp"`
	deviceID  string  `yson:"device_id"`
	status    bool    `yson:"status"`
}

func saveData(output Output, mu *sync.Mutex, resultWriter *csv.Writer) error {
	mu.Lock()
	defer mu.Unlock()

	err := resultWriter.Write([]string{
		strconv.FormatFloat(output.timestamp, 'f', 0, 64),
		output.deviceID,
		fmt.Sprintf("%t", output.status),
	})
	if err != nil {
		return err
	}

	return nil
}

func ping(ctx context.Context, d Device, dbc *db.DBClient, qController query.Controller) ([]adapter.DeviceStateView, error) {
	userID, err := strconv.ParseUint(d.UserID, 10, 64)
	if err != nil {
		return []adapter.DeviceStateView{}, err
	}

	userDevice, err := dbc.SelectUserDevice(ctx, userID, d.DeviceID)
	if err != nil {
		return []adapter.DeviceStateView{}, err
	}

	origin := model.NewOrigin(ctx, model.APISurfaceParameters{}, model.User{ID: userID})
	deviceStateView, err := qController.GetProviderDevicesState(ctx, d.SkillID, []model.Device{userDevice}, origin)
	if err != nil {
		return []adapter.DeviceStateView{}, err
	}

	return deviceStateView, nil
}

type Device struct {
	DeviceID string `yson:"device_id"`
	UserID   string `yson:"user_id"`
	//	Error    string `yson:"err"`
	SkillID string `yson:"skill_id"`
}

func streamDevices(ctx context.Context, yt yt.Client, tablePath ypath.Path, logger *zap.Logger) <-chan Device {
	deviceChan := make(chan Device)

	go func() {
		defer close(deviceChan)

		// deviceChan <- Device{
		//	DeviceID: "234737fb-37ee-4796-a75b-cdd69608bd8d",
		//	UserID:   "95491511",
		//	Error:    string(model.DeviceUnreachable),
		//	SkillID:  "T",
		// }

		tr, err := yt.ReadTable(ctx, tablePath, nil)
		if err != nil {
			logger.Fatalf("cannot read table at %s: %v", tablePath, err)
		}
		defer func() { _ = tr.Close() }()

		for tr.Next() {
			var c Device
			err = tr.Scan(&c)
			if err != nil {
				logger.Fatalf("error while reading row %s: %v", tablePath, err)
			}

			// if c.Error != string(model.DeviceUnreachable) {
			//	continue
			// }

			deviceChan <- c
		}

		if tr.Err() != nil {
			logger.Fatalf("cannot read table at %s: %v", tablePath, tr.Err())
		}
	}()

	return deviceChan
}
