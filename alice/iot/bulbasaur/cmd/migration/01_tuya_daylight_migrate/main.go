package main

import (
	"bufio"
	"context"
	"encoding/json"
	"fmt"
	"os"
	"strings"

	"a.yandex-team.ru/library/go/core/log/ctxlog"
	"a.yandex-team.ru/library/go/core/log/zap"
	"github.com/mitchellh/mapstructure"

	uberzap "go.uber.org/zap"
	"go.uber.org/zap/zapcore"

	"a.yandex-team.ru/alice/iot/adapters/tuya_adapter/tuya"
	"a.yandex-team.ru/alice/iot/bulbasaur/db"
	"a.yandex-team.ru/alice/iot/bulbasaur/model"
	modelTuya "a.yandex-team.ru/alice/iot/bulbasaur/model/tuya"
	"a.yandex-team.ru/kikimr/public/sdk/go/ydb"
	"a.yandex-team.ru/kikimr/public/sdk/go/ydb/table"
	"a.yandex-team.ru/library/go/core/xerrors"
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

func main() {
	logger, stop := initLogging()
	defer stop()

	endpoint := os.Getenv("YDB_ENDPOINT")
	if len(endpoint) == 0 {
		panic("YDB_ENDPOINT env is not set")
	}

	prefix := os.Getenv("YDB_PREFIX")
	if len(prefix) == 0 {
		panic("YDB_PREFIX env is not set")
	}

	token := os.Getenv("YDB_TOKEN")
	if len(token) == 0 {
		panic("YDB_TOKEN env is not set")
	}

	_, trace := os.LookupEnv("YDB_DEBUG")

	dbcli, err := db.NewClient(context.Background(), logger, endpoint, prefix, ydb.AuthTokenCredentials{AuthToken: token}, trace)
	if err != nil {
		panic(err.Error())
	}

	TABLE := "Devices0627" //FIXME: user real table name

	c := askForConfirmation(fmt.Sprintf("Do you really want to perform migrations at `%s%s/%s`", endpoint, prefix, TABLE))
	if c {
		ctx := context.Background()
		if err := migrate(ctx, dbcli, TABLE); err != nil {
			logger.Error(err.Error())
			os.Exit(1)
		}
	} else {
		logger.Info("Bye")
	}
}

func migrate(ctx context.Context, db *db.DBClient, tableName string) error {
	s, err := db.SessionPool.Get(ctx)
	if err != nil {
		return err
	}

	readTx := table.TxControl(table.BeginTx(table.WithOnlineReadOnly()), table.CommitTx())
	records, err := getDevices(ctx, db, s, readTx, tableName)
	if err != nil {
		return err
	}

	processed := 0
	for _, record := range records {
		writeTx := table.TxControl(
			table.BeginTx(table.WithSerializableReadWrite()),
			table.CommitTx(),
		)

		logger.Infof("Changing device_id=%s", record.ID)
		if c, exists := record.GetCapabilityByTypeAndInstance(model.ColorSettingCapabilityType, ""); exists {
			var customData modelTuya.CustomData
			if err := mapstructure.Decode(record.CustomData, customData); err != nil {
				return err
			}
			var pid string
			if customData.ProductID != nil {
				pid = *customData.ProductID
			}
			lampSpec := tuya.LampSpecByPID(tuya.TuyaDeviceProductID(pid))

			params := c.Parameters().(model.ColorSettingCapabilityParameters)
			params.TemperatureK = &model.TemperatureKParameters{
				Min: model.TemperatureK(lampSpec.TempKSpec.Min),
				Max: model.TemperatureK(lampSpec.TempKSpec.Max),
			}

			c.SetParameters(params)
		}

		if err := setDeviceCapabilities(ctx, db, s, writeTx, tableName, record); err != nil {
			return err
		}

		processed += 1
	}

	logger.Infof("Processed `%d` rows", processed)
	return nil
}

func setDeviceCapabilities(ctx context.Context, db *db.DBClient, s *table.Session, tc *table.TransactionControl, tableName string, device model.Device) error {
	logger.Infof("Updating <%s>, device_id=%s", tableName, device.ID)

	capabilitiesB, err := json.Marshal(device.Capabilities)
	if err != nil {
		return xerrors.Errorf("cannot marshal capabilities: %w", err)
	}

	query := fmt.Sprintf(`
		--!syntax_v1
		PRAGMA TablePathPrefix("%s");
		DECLARE $device_id AS String;
		DECLARE $capabilities AS Json;

		UPDATE %s
		SET capabilities = $capabilities
		WHERE id = $device_id`, db.Prefix, tableName)
	params := table.NewQueryParameters(
		table.ValueParam("$device_id", ydb.StringValue([]byte(device.ID))),
		table.ValueParam("$capabilities", ydb.JSONValue(string(capabilitiesB))),
	)

	preparedQuery, err := s.Prepare(ctx, query)
	if err != nil {
		return err
	}

	_, _, err = preparedQuery.Execute(ctx, tc, params)
	if err != nil {
		return err
	}

	logger.Info("Done")
	return nil
}

func getDevices(ctx context.Context, db *db.DBClient, s *table.Session, tc *table.TransactionControl, tableName string) ([]model.Device, error) {
	records := make([]model.Device, 0)
	logger.Info("fetching new batch")
	query := fmt.Sprintf(`
		--!syntax_v1
		PRAGMA TablePathPrefix("%s");

		SELECT id, capabilities
		FROM %s
		WHERE skill_id == "T"
		AND type == "devices.types.light"
		LIMIT 1000`, db.Prefix, tableName)
	params := table.NewQueryParameters()

	preparedQuery, err := s.Prepare(ctx, query)
	if err != nil {
		return nil, err
	}

	_, res, err := preparedQuery.Execute(ctx, tc, params)
	if err != nil {
		return nil, err
	}

	for res.NextSet() {
		for res.NextRow() {
			var device model.Device
			res.SeekItem("id")
			device.ID = string(res.OString())

			// capabilities
			res.NextItem()
			var capabilities []model.ICapability
			if err := json.Unmarshal([]byte(res.OJSON()), &capabilities); err != nil {
				return nil, xerrors.Errorf("failed to parse `capabilities` field for device %s: %w", device.ID, err)
			}
			device.Capabilities = capabilities

			if res.Err() != nil {
				ctxlog.Errorf(ctx, db.Logger, "failed to parse YDB response row: %v", res.Err())
				return nil, xerrors.Errorf("failed to parse YDB response row: %w", res.Err())
			}
			records = append(records, device)
		}
	}

	logger.Info("fetching Done")
	return records, nil
}

func askForConfirmation(s string) bool {
	reader := bufio.NewReader(os.Stdin)
	logger.Infof("%s [y/n]: ", s)
	response, err := reader.ReadString('\n')
	if err != nil {
		logger.Fatal(err.Error())
	}

	response = strings.ToLower(strings.TrimSpace(response))
	if strings.ToLower(response) == "y" || strings.ToLower(response) == "yes" {
		return true
	}
	return false
}
