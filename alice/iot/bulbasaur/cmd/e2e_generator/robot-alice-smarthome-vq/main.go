package main

import (
	"context"
	"encoding/csv"
	"fmt"
	"io"
	"os"
	"strings"
	"time"

	"a.yandex-team.ru/alice/iot/bulbasaur/dto/uniproxy"
	"a.yandex-team.ru/alice/library/go/timestamp"
	"a.yandex-team.ru/library/go/core/log/zap"

	"a.yandex-team.ru/alice/iot/bulbasaur/model"
	"a.yandex-team.ru/alice/library/go/tools"
	"a.yandex-team.ru/kikimr/public/sdk/go/ydb"
	"a.yandex-team.ru/kikimr/public/sdk/go/ydb/table"
	"a.yandex-team.ru/library/go/core/xerrors"
	"cuelang.org/go/pkg/strconv"
	uberzap "go.uber.org/zap"
	"go.uber.org/zap/zapcore"

	"a.yandex-team.ru/alice/iot/bulbasaur/db"
	"a.yandex-team.ru/alice/library/go/cli"
)

const idsPrefix string = "e2e-"

type EUser struct {
	ID, Login, Password, Token string
}

type Config struct {
	RequestID    string                `yson:"request_id"`
	UserInfoView uniproxy.UserInfoView `yson:"iot_config"`
}

var logger *zap.Logger

func initLogging() (logger *zap.Logger, stop func()) {
	encoderConfig := uberzap.NewDevelopmentEncoderConfig()
	encoderConfig.EncodeLevel = zapcore.CapitalColorLevelEncoder
	encoderConfig.EncodeCaller = zapcore.ShortCallerEncoder
	encoder := zapcore.NewConsoleEncoder(encoderConfig)

	core := zapcore.NewCore(encoder, zapcore.AddSync(os.Stdout), uberzap.WarnLevel)
	stop = func() {
		_ = core.Sync()
	}
	logger = zap.NewWithCore(core, uberzap.AddStacktrace(uberzap.FatalLevel), uberzap.AddCaller())

	return
}

func getUser() EUser {
	r := csv.NewReader(strings.NewReader(dataset))
	logger.Infof("skipping first line from dataset")
	_, err := r.Read()
	if err != nil {
		logger.Fatalf("unable to skip the first line from dataset: %v", err)
	}
	record, err := r.Read()
	if err != nil {
		switch {
		case xerrors.Is(err, io.EOF):
			break
		default:
			logger.Fatalf("unable to read row from dataset: %v", err)
		}
	}
	return EUser{record[0], record[1], record[2], record[3]}
}

func getConfig() Config {
	return Config{
		RequestID: "deadbeef-dead-beef-dead-beef00000075",
		UserInfoView: uniproxy.UserInfoView{
			Devices: []uniproxy.DeviceUserInfoView{
				{
					ID:         "e2e2e2e2-dead-beef-dead-device000001",
					ExternalID: "e2e2e2e2-dead-beef-dead-device000001",
					Name:       "Гирлянда",
					RoomID:     "e2e2e2e2-dead-beef-dead-room00000001",
					Groups: []uniproxy.GroupUserInfoView{
						{
							ID:   "e2e2e2e2-dead-beef-dead-group0000003",
							Name: "Электроприборы",
						},
					},
					Capabilities: []uniproxy.CapabilityUserInfoView{
						{
							Type:        model.OnOffCapabilityType,
							Instance:    string(model.OnOnOffCapabilityInstance),
							Retrievable: true,
							Parameters: model.OnOffCapabilityParameters{
								Split: false,
							},
							State: model.OnOffCapabilityState{
								Instance: model.OnOnOffCapabilityInstance,
								Value:    true,
							},
						},
					},
					Properties: []uniproxy.PropertyUserInfoView{
						{
							Type:        model.FloatPropertyType,
							Instance:    model.VoltagePropertyInstance.String(),
							Retrievable: true,
							Parameters: model.FloatPropertyParameters{
								Instance: model.VoltagePropertyInstance,
								Unit:     model.UnitVolt,
							},
							State: model.FloatPropertyState{
								Instance: model.VoltagePropertyInstance,
								Value:    10,
							},
						},
						{
							Type:        model.FloatPropertyType,
							Instance:    model.AmperagePropertyInstance.String(),
							Retrievable: true,
							Parameters: model.FloatPropertyParameters{
								Instance: model.AmperagePropertyInstance,
								Unit:     model.UnitAmpere,
							},
							State: model.FloatPropertyState{
								Instance: model.AmperagePropertyInstance,
								Value:    5,
							},
						},
						{
							Type:        model.FloatPropertyType,
							Instance:    model.PowerPropertyInstance.String(),
							Retrievable: true,
							Parameters: model.FloatPropertyParameters{
								Instance: model.PowerPropertyInstance,
								Unit:     model.UnitWatt,
							},
							State: model.FloatPropertyState{
								Instance: model.PowerPropertyInstance,
								Value:    50,
							},
						},
					},
					Type:         model.SocketDeviceType,
					OriginalType: model.SocketDeviceType,
					Created:      timestamp.PastTimestamp(1600709558),
				},
				{
					ID:         "e2e2e2e2-dead-beef-dead-device000002",
					ExternalID: "e2e2e2e2-dead-beef-dead-device000002",
					Name:       "Кондиционер",
					RoomID:     "e2e2e2e2-dead-beef-dead-room00000001",
					Groups: []uniproxy.GroupUserInfoView{
						{
							ID:   "e2e2e2e2-dead-beef-dead-group0000004",
							Name: "Обдуватели",
							Type: model.AcDeviceType,
						},
					},
					Capabilities: []uniproxy.CapabilityUserInfoView{
						{
							Type:        model.OnOffCapabilityType,
							Instance:    string(model.OnOnOffCapabilityInstance),
							Retrievable: true,
							Parameters: model.OnOffCapabilityParameters{
								Split: false,
							},
							State: model.OnOffCapabilityState{
								Instance: model.OnOnOffCapabilityInstance,
								Value:    true,
							},
						},
						{
							Type:        model.RangeCapabilityType,
							Instance:    string(model.TemperatureRangeInstance),
							Retrievable: true,
							Parameters: model.RangeCapabilityParameters{
								Instance:     model.TemperatureRangeInstance,
								Unit:         model.UnitTemperatureCelsius,
								RandomAccess: true,
								Looped:       true,
								Range: &model.Range{
									Min:       1,
									Max:       100,
									Precision: 1,
								},
							},
							State: model.RangeCapabilityState{
								Instance: model.TemperatureRangeInstance,
								Value:    24,
							},
						},
						{
							Type:        model.ModeCapabilityType,
							Instance:    string(model.ThermostatModeInstance),
							Values:      []string{string(model.CoolMode), string(model.HeatMode)},
							Retrievable: true,
							Parameters: model.ModeCapabilityParameters{
								Instance: model.ThermostatModeInstance,
								Modes: []model.Mode{
									model.KnownModes[model.CoolMode],
									model.KnownModes[model.HeatMode],
								},
							},
							State: model.ModeCapabilityState{
								Instance: model.ThermostatModeInstance,
								Value:    model.CoolMode,
							},
						},
						{
							Type:        model.ModeCapabilityType,
							Instance:    string(model.FanSpeedModeInstance),
							Values:      []string{string(model.AutoMode), string(model.FastMode)},
							Retrievable: true,
							Parameters: model.ModeCapabilityParameters{
								Instance: model.FanSpeedModeInstance,
								Modes: []model.Mode{
									model.KnownModes[model.AutoMode],
									model.KnownModes[model.FastMode],
								},
							},
							State: model.ModeCapabilityState{
								Instance: model.FanSpeedModeInstance,
								Value:    model.AutoMode,
							},
						},
						{
							Type:        model.ToggleCapabilityType,
							Instance:    string(model.IonizationToggleCapabilityInstance),
							Retrievable: true,
							Parameters: model.ToggleCapabilityParameters{
								Instance: model.IonizationToggleCapabilityInstance,
							},
							State: model.ToggleCapabilityState{
								Instance: model.IonizationToggleCapabilityInstance,
								Value:    false,
							},
						},
					},
					Properties:   []uniproxy.PropertyUserInfoView{},
					Type:         model.AcDeviceType,
					OriginalType: model.AcDeviceType,
					Created:      timestamp.PastTimestamp(1600709558),
				},
				{
					ID:         "e2e2e2e2-dead-beef-dead-device000003",
					ExternalID: "e2e2e2e2-dead-beef-dead-device000003",
					Name:       "Очиститель",
					RoomID:     "e2e2e2e2-dead-beef-dead-room00000001",
					Capabilities: []uniproxy.CapabilityUserInfoView{
						{
							Type:        model.OnOffCapabilityType,
							Instance:    string(model.OnOnOffCapabilityInstance),
							Retrievable: true,
							Parameters: model.OnOffCapabilityParameters{
								Split: false,
							},
							State: model.OnOffCapabilityState{
								Instance: model.OnOnOffCapabilityInstance,
								Value:    true,
							},
						},
					},
					Properties: []uniproxy.PropertyUserInfoView{
						{
							Type:        model.FloatPropertyType,
							Instance:    model.CO2LevelPropertyInstance.String(),
							Retrievable: true,
							Parameters: model.FloatPropertyParameters{
								Instance: model.CO2LevelPropertyInstance,
								Unit:     model.UnitPPM,
							},
							State: model.FloatPropertyState{
								Instance: model.CO2LevelPropertyInstance,
								Value:    700,
							},
						},
						{
							Type:        model.FloatPropertyType,
							Instance:    model.TemperaturePropertyInstance.String(),
							Retrievable: true,
							Parameters: model.FloatPropertyParameters{
								Instance: model.TemperaturePropertyInstance,
								Unit:     model.UnitTemperatureCelsius,
							},
							State: model.FloatPropertyState{
								Instance: model.TemperaturePropertyInstance,
								Value:    20,
							},
						},
					},
					Type:         model.PurifierDeviceType,
					OriginalType: model.PurifierDeviceType,
					Created:      timestamp.PastTimestamp(1600709558),
				},
				{
					ID:         "e2e2e2e2-dead-beef-dead-device000004",
					ExternalID: "e2e2e2e2-dead-beef-dead-device000004",
					Name:       "Пылесос",
					RoomID:     "e2e2e2e2-dead-beef-dead-room00000001",
					Capabilities: []uniproxy.CapabilityUserInfoView{
						{
							Type:        model.OnOffCapabilityType,
							Instance:    string(model.OnOnOffCapabilityInstance),
							Retrievable: true,
							Parameters: model.OnOffCapabilityParameters{
								Split: false,
							},
							State: model.OnOffCapabilityState{
								Instance: model.OnOnOffCapabilityInstance,
								Value:    true,
							},
						},
						{
							Type:        model.ModeCapabilityType,
							Instance:    string(model.WorkSpeedModeInstance),
							Values:      []string{string(model.TurboMode), string(model.FastMode)},
							Retrievable: true,
							Parameters: model.ModeCapabilityParameters{
								Instance: model.WorkSpeedModeInstance,
								Modes: []model.Mode{
									model.KnownModes[model.TurboMode],
									model.KnownModes[model.FastMode],
								},
							},
							State: model.ModeCapabilityState{
								Instance: model.WorkSpeedModeInstance,
								Value:    model.TurboMode,
							},
						},
						{
							Type:        model.ToggleCapabilityType,
							Instance:    string(model.PauseToggleCapabilityInstance),
							Retrievable: true,
							Parameters: model.ToggleCapabilityParameters{
								Instance: model.PauseToggleCapabilityInstance,
							},
							State: model.ToggleCapabilityState{
								Instance: model.PauseToggleCapabilityInstance,
								Value:    false,
							},
						},
					},
					Properties: []uniproxy.PropertyUserInfoView{
						{
							Type:        model.FloatPropertyType,
							Instance:    model.BatteryLevelPropertyInstance.String(),
							Retrievable: true,
							Parameters: model.FloatPropertyParameters{
								Instance: model.BatteryLevelPropertyInstance,
								Unit:     model.UnitPercent,
							},
							State: model.FloatPropertyState{
								Instance: model.BatteryLevelPropertyInstance,
								Value:    95,
							},
						},
					},
					Type:         model.VacuumCleanerDeviceType,
					OriginalType: model.VacuumCleanerDeviceType,
					Created:      timestamp.PastTimestamp(1600709558),
				},
				{
					ID:         "e2e2e2e2-dead-beef-dead-device000005",
					ExternalID: "e2e2e2e2-dead-beef-dead-device000005",
					Name:       "Теплый пол",
					RoomID:     "e2e2e2e2-dead-beef-dead-room00000001",
					Capabilities: []uniproxy.CapabilityUserInfoView{
						{
							Type:        model.OnOffCapabilityType,
							Instance:    string(model.OnOnOffCapabilityInstance),
							Retrievable: true,
							Parameters: model.OnOffCapabilityParameters{
								Split: false,
							},
							State: model.OnOffCapabilityState{
								Instance: model.OnOnOffCapabilityInstance,
								Value:    true,
							},
						},
						{
							Type:        model.RangeCapabilityType,
							Instance:    string(model.TemperatureRangeInstance),
							Retrievable: true,
							Parameters: model.RangeCapabilityParameters{
								Instance:     model.TemperatureRangeInstance,
								Unit:         model.UnitTemperatureCelsius,
								RandomAccess: true,
								Looped:       true,
								Range: &model.Range{
									Min:       1,
									Max:       100,
									Precision: 1,
								},
							},
							State: model.RangeCapabilityState{
								Instance: model.TemperatureRangeInstance,
								Value:    24,
							},
						},
					},
					Properties:   []uniproxy.PropertyUserInfoView{},
					Type:         model.ThermostatDeviceType,
					OriginalType: model.ThermostatDeviceType,
					Created:      timestamp.PastTimestamp(1600709558),
				},
				{
					ID:         "e2e2e2e2-dead-beef-dead-device000006",
					ExternalID: "e2e2e2e2-dead-beef-dead-device000006",
					Name:       "Лампа",
					RoomID:     "e2e2e2e2-dead-beef-dead-room00000002",
					Capabilities: []uniproxy.CapabilityUserInfoView{
						{
							Type:        model.OnOffCapabilityType,
							Instance:    string(model.OnOnOffCapabilityInstance),
							Retrievable: true,
							Parameters: model.OnOffCapabilityParameters{
								Split: false,
							},
							State: model.OnOffCapabilityState{
								Instance: model.OnOnOffCapabilityInstance,
								Value:    false,
							},
						},
						{
							Type:        model.ColorSettingCapabilityType,
							Instance:    string(model.RgbColorCapabilityInstance),
							Retrievable: true,
							Parameters: model.ColorSettingCapabilityParameters{
								ColorModel: model.CM(model.RgbModelType),
							},
							State: model.ColorSettingCapabilityState{
								Instance: model.RgbColorCapabilityInstance,
								Value:    model.RGB(16714250),
							},
						},
						{
							Type:        model.RangeCapabilityType,
							Instance:    string(model.BrightnessRangeInstance),
							Values:      []string{},
							Retrievable: true,
							Parameters: model.RangeCapabilityParameters{
								Instance:     model.BrightnessRangeInstance,
								Unit:         model.UnitPercent,
								RandomAccess: true,
								Looped:       true,
								Range: &model.Range{
									Min:       1,
									Max:       100,
									Precision: 1,
								},
							},
							State: model.RangeCapabilityState{
								Instance: model.BrightnessRangeInstance,
								Value:    75,
							},
						},
					},
					Properties:   []uniproxy.PropertyUserInfoView{},
					Type:         model.LightDeviceType,
					OriginalType: model.LightDeviceType,
					Created:      timestamp.PastTimestamp(1600709558),
				},
				{
					ID:         "e2e2e2e2-dead-beef-dead-device000007",
					ExternalID: "e2e2e2e2-dead-beef-dead-device000007",
					Name:       "Кофеварка",
					RoomID:     "e2e2e2e2-dead-beef-dead-room00000002",
					Capabilities: []uniproxy.CapabilityUserInfoView{
						{
							Type:        model.OnOffCapabilityType,
							Instance:    string(model.OnOnOffCapabilityInstance),
							Retrievable: true,
							Parameters: model.OnOffCapabilityParameters{
								Split: false,
							},
							State: model.OnOffCapabilityState{
								Instance: model.OnOnOffCapabilityInstance,
								Value:    true,
							},
						},
						{
							Type:        model.ModeCapabilityType,
							Instance:    string(model.CoffeeModeInstance),
							Values:      []string{string(model.EspressoMode)},
							Retrievable: true,
							Parameters: model.ModeCapabilityParameters{
								Instance: model.CoffeeModeInstance,
								Modes: []model.Mode{
									model.KnownModes[model.EspressoMode],
								},
							},
							State: model.ModeCapabilityState{
								Instance: model.CoffeeModeInstance,
								Value:    model.EspressoMode,
							},
						},
					},
					Properties:   []uniproxy.PropertyUserInfoView{},
					Type:         model.CoffeeMakerDeviceType,
					OriginalType: model.CoffeeMakerDeviceType,
					Created:      timestamp.PastTimestamp(1600709558),
				},
				{
					ID:         "e2e2e2e2-dead-beef-dead-device000008",
					ExternalID: "e2e2e2e2-dead-beef-dead-device000008",
					Name:       "Мультиварка",
					RoomID:     "e2e2e2e2-dead-beef-dead-room00000002",
					Capabilities: []uniproxy.CapabilityUserInfoView{
						{
							Type:        model.OnOffCapabilityType,
							Instance:    string(model.OnOnOffCapabilityInstance),
							Retrievable: true,
							Parameters: model.OnOffCapabilityParameters{
								Split: false,
							},
							State: model.OnOffCapabilityState{
								Instance: model.OnOnOffCapabilityInstance,
								Value:    true,
							},
						},
						{
							Type:        model.ModeCapabilityType,
							Instance:    string(model.ProgramModeInstance),
							Values:      []string{string(model.PilafMode)},
							Retrievable: true,
							Parameters: model.ModeCapabilityParameters{
								Instance: model.ProgramModeInstance,
								Modes: []model.Mode{
									model.KnownModes[model.PilafMode],
								},
							},
							State: model.ModeCapabilityState{
								Instance: model.ProgramModeInstance,
								Value:    model.PilafMode,
							},
						},
					},
					Properties:   []uniproxy.PropertyUserInfoView{},
					Type:         model.MulticookerDeviceType,
					OriginalType: model.MulticookerDeviceType,
					Created:      timestamp.PastTimestamp(1600709558),
				},
				{
					ID:         "e2e2e2e2-dead-beef-dead-device000009",
					ExternalID: "e2e2e2e2-dead-beef-dead-device000009",
					Name:       "Посудомойка",
					RoomID:     "e2e2e2e2-dead-beef-dead-room00000002",
					Capabilities: []uniproxy.CapabilityUserInfoView{
						{
							Type:        model.OnOffCapabilityType,
							Instance:    string(model.OnOnOffCapabilityInstance),
							Retrievable: true,
							Parameters: model.OnOffCapabilityParameters{
								Split: false,
							},
							State: model.OnOffCapabilityState{
								Instance: model.OnOnOffCapabilityInstance,
								Value:    true,
							},
						},
						{
							Type:        model.ModeCapabilityType,
							Instance:    string(model.ProgramModeInstance),
							Values:      []string{string(model.EcoMode)},
							Retrievable: true,
							Parameters: model.ModeCapabilityParameters{
								Instance: model.ProgramModeInstance,
								Modes: []model.Mode{
									model.KnownModes[model.EcoMode],
								},
							},
							State: model.ModeCapabilityState{
								Instance: model.ProgramModeInstance,
								Value:    model.EcoMode,
							},
						},
					},
					Properties:   []uniproxy.PropertyUserInfoView{},
					Type:         model.DishwasherDeviceType,
					OriginalType: model.DishwasherDeviceType,
					Created:      timestamp.PastTimestamp(1600709558),
				},
				{
					ID:         "e2e2e2e2-dead-beef-dead-device000010",
					ExternalID: "e2e2e2e2-dead-beef-dead-device000010",
					Name:       "Стиралка",
					RoomID:     "e2e2e2e2-dead-beef-dead-room00000002",
					Capabilities: []uniproxy.CapabilityUserInfoView{
						{
							Type:        model.OnOffCapabilityType,
							Instance:    string(model.OnOnOffCapabilityInstance),
							Retrievable: true,
							Parameters: model.OnOffCapabilityParameters{
								Split: false,
							},
							State: model.OnOffCapabilityState{
								Instance: model.OnOnOffCapabilityInstance,
								Value:    true,
							},
						},
						{
							Type:        model.ModeCapabilityType,
							Instance:    string(model.ProgramModeInstance),
							Values:      []string{string(model.WoolMode)},
							Retrievable: true,
							Parameters: model.ModeCapabilityParameters{
								Instance: model.ProgramModeInstance,
								Modes: []model.Mode{
									model.KnownModes[model.WoolMode],
								},
							},
							State: model.ModeCapabilityState{
								Instance: model.ProgramModeInstance,
								Value:    model.WoolMode,
							},
						},
					},
					Properties:   []uniproxy.PropertyUserInfoView{},
					Type:         model.WashingMachineDeviceType,
					OriginalType: model.WashingMachineDeviceType,
					Created:      timestamp.PastTimestamp(1600709558),
				},
				{
					ID:         "e2e2e2e2-dead-beef-dead-device000011",
					ExternalID: "e2e2e2e2-dead-beef-dead-device000011",
					Name:       "Чайник",
					RoomID:     "e2e2e2e2-dead-beef-dead-room00000002",
					Capabilities: []uniproxy.CapabilityUserInfoView{
						{
							Type:        model.OnOffCapabilityType,
							Instance:    string(model.OnOnOffCapabilityInstance),
							Retrievable: true,
							Parameters: model.OnOffCapabilityParameters{
								Split: false,
							},
							State: model.OnOffCapabilityState{
								Instance: model.OnOnOffCapabilityInstance,
								Value:    true,
							},
						},
						{
							Type:        model.ToggleCapabilityType,
							Instance:    string(model.BacklightToggleCapabilityInstance),
							Retrievable: true,
							Parameters: model.ToggleCapabilityParameters{
								Instance: model.BacklightToggleCapabilityInstance,
							},
							State: model.ToggleCapabilityState{
								Instance: model.BacklightToggleCapabilityInstance,
								Value:    true,
							},
						},
						{
							Type:        model.ToggleCapabilityType,
							Instance:    string(model.KeepWarmToggleCapabilityInstance),
							Retrievable: true,
							Parameters: model.ToggleCapabilityParameters{
								Instance: model.KeepWarmToggleCapabilityInstance,
							},
							State: model.ToggleCapabilityState{
								Instance: model.KeepWarmToggleCapabilityInstance,
								Value:    false,
							},
						},
						{
							Type:        model.ToggleCapabilityType,
							Instance:    string(model.ControlsLockedToggleCapabilityInstance),
							Retrievable: true,
							Parameters: model.ToggleCapabilityParameters{
								Instance: model.ControlsLockedToggleCapabilityInstance,
							},
							State: model.ToggleCapabilityState{
								Instance: model.ControlsLockedToggleCapabilityInstance,
								Value:    true,
							},
						},
					},
					Properties:   []uniproxy.PropertyUserInfoView{},
					Type:         model.KettleDeviceType,
					OriginalType: model.KettleDeviceType,
					Created:      timestamp.PastTimestamp(1600709558),
				},
				{
					ID:         "e2e2e2e2-dead-beef-dead-device000012",
					ExternalID: "e2e2e2e2-dead-beef-dead-device000012",
					Name:       "Телевизор",
					RoomID:     "e2e2e2e2-dead-beef-dead-room00000003",
					Capabilities: []uniproxy.CapabilityUserInfoView{
						{
							Type:        model.OnOffCapabilityType,
							Instance:    string(model.OnOnOffCapabilityInstance),
							Retrievable: true,
							Parameters: model.OnOffCapabilityParameters{
								Split: false,
							},
							State: model.OnOffCapabilityState{
								Instance: model.OnOnOffCapabilityInstance,
								Value:    true,
							},
						},
						{
							Type:        model.RangeCapabilityType,
							Instance:    string(model.ChannelRangeInstance),
							Retrievable: true,
							Parameters: model.RangeCapabilityParameters{
								Instance:     model.ChannelRangeInstance,
								RandomAccess: true,
								Looped:       true,
								Range: &model.Range{
									Min:       1,
									Max:       100,
									Precision: 1,
								},
							},
							State: model.RangeCapabilityState{
								Instance: model.ChannelRangeInstance,
								Value:    3,
							},
						},
						{
							Type:        model.RangeCapabilityType,
							Instance:    string(model.VolumeRangeInstance),
							Retrievable: true,
							Parameters: model.RangeCapabilityParameters{
								Instance:     model.VolumeRangeInstance,
								RandomAccess: true,
								Looped:       true,
								Range: &model.Range{
									Min:       1,
									Max:       100,
									Precision: 1,
								},
							},
							State: model.RangeCapabilityState{
								Instance: model.VolumeRangeInstance,
								Value:    50,
							},
						},
						{
							Type:        model.ModeCapabilityType,
							Instance:    string(model.InputSourceModeInstance),
							Values:      []string{string(model.OneMode)},
							Retrievable: true,
							Parameters: model.ModeCapabilityParameters{
								Instance: model.InputSourceModeInstance,
								Modes: []model.Mode{
									model.KnownModes[model.OneMode],
								},
							},
							State: model.ModeCapabilityState{
								Instance: model.InputSourceModeInstance,
								Value:    model.OneMode,
							},
						},
						{
							Type:        model.ToggleCapabilityType,
							Instance:    string(model.MuteToggleCapabilityInstance),
							Retrievable: true,
							Parameters: model.ToggleCapabilityParameters{
								Instance: model.MuteToggleCapabilityInstance,
							},
							State: model.ToggleCapabilityState{
								Instance: model.MuteToggleCapabilityInstance,
								Value:    false,
							},
						},
						{
							Type:        model.ToggleCapabilityType,
							Instance:    string(model.PauseToggleCapabilityInstance),
							Retrievable: true,
							Parameters: model.ToggleCapabilityParameters{
								Instance: model.PauseToggleCapabilityInstance,
							},
							State: model.ToggleCapabilityState{
								Instance: model.PauseToggleCapabilityInstance,
								Value:    false,
							},
						},
					},
					Properties:   []uniproxy.PropertyUserInfoView{},
					Type:         model.TvDeviceDeviceType,
					OriginalType: model.TvDeviceDeviceType,
					Created:      timestamp.PastTimestamp(1600709558),
				},
				{
					ID:         "e2e2e2e2-dead-beef-dead-device000013",
					ExternalID: "e2e2e2e2-dead-beef-dead-device000013",
					Name:       "Шторы",
					RoomID:     "e2e2e2e2-dead-beef-dead-room00000003",
					Capabilities: []uniproxy.CapabilityUserInfoView{
						{
							Type:        model.OnOffCapabilityType,
							Instance:    string(model.OnOnOffCapabilityInstance),
							Retrievable: true,
							Parameters: model.OnOffCapabilityParameters{
								Split: false,
							},
							State: model.OnOffCapabilityState{
								Instance: model.OnOnOffCapabilityInstance,
								Value:    true,
							},
						},
						{
							Type:        model.RangeCapabilityType,
							Instance:    string(model.OpenRangeInstance),
							Retrievable: true,
							Parameters: model.RangeCapabilityParameters{
								Instance:     model.OpenRangeInstance,
								Unit:         model.UnitPercent,
								RandomAccess: true,
								Looped:       true,
								Range: &model.Range{
									Min:       1,
									Max:       100,
									Precision: 1,
								},
							},
							State: model.RangeCapabilityState{
								Instance: model.OpenRangeInstance,
								Value:    50,
							},
						},
					},
					Properties:   []uniproxy.PropertyUserInfoView{},
					Type:         model.CurtainDeviceType,
					OriginalType: model.CurtainDeviceType,
					Created:      timestamp.PastTimestamp(1600709558),
				},
				{
					ID:         "e2e2e2e2-dead-beef-dead-device000014",
					ExternalID: "e2e2e2e2-dead-beef-dead-device000014",
					Name:       "Увлажнитель воздуха",
					RoomID:     "e2e2e2e2-dead-beef-dead-room00000003",
					Capabilities: []uniproxy.CapabilityUserInfoView{
						{
							Type:        model.OnOffCapabilityType,
							Instance:    string(model.OnOnOffCapabilityInstance),
							Retrievable: true,
							Parameters: model.OnOffCapabilityParameters{
								Split: false,
							},
							State: model.OnOffCapabilityState{
								Instance: model.OnOnOffCapabilityInstance,
								Value:    true,
							},
						},
						{
							Type:        model.ToggleCapabilityType,
							Instance:    string(model.IonizationToggleCapabilityInstance),
							Retrievable: true,
							Parameters: model.ToggleCapabilityParameters{
								Instance: model.IonizationToggleCapabilityInstance,
							},
							State: model.ToggleCapabilityState{
								Instance: model.IonizationToggleCapabilityInstance,
								Value:    true,
							},
						},
						{
							Type:        model.RangeCapabilityType,
							Instance:    string(model.HumidityRangeInstance),
							Retrievable: true,
							Parameters: model.RangeCapabilityParameters{
								Instance:     model.HumidityRangeInstance,
								Unit:         model.UnitPercent,
								RandomAccess: true,
								Looped:       true,
								Range: &model.Range{
									Min:       1,
									Max:       100,
									Precision: 1,
								},
							},
							State: model.RangeCapabilityState{
								Instance: model.HumidityRangeInstance,
								Value:    40,
							},
						},
					},
					Properties: []uniproxy.PropertyUserInfoView{
						{
							Type:        model.FloatPropertyType,
							Instance:    model.WaterLevelPropertyInstance.String(),
							Retrievable: true,
							Parameters: model.FloatPropertyParameters{
								Instance: model.WaterLevelPropertyInstance,
								Unit:     model.UnitPercent,
							},
							State: model.FloatPropertyState{
								Instance: model.WaterLevelPropertyInstance,
								Value:    90,
							},
						},
						{
							Type:        model.FloatPropertyType,
							Instance:    model.HumidityPropertyInstance.String(),
							Retrievable: true,
							Parameters: model.FloatPropertyParameters{
								Instance: model.HumidityPropertyInstance,
								Unit:     model.UnitPercent,
							},
							State: model.FloatPropertyState{
								Instance: model.HumidityPropertyInstance,
								Value:    35,
							},
						},
						{
							Type:        model.FloatPropertyType,
							Instance:    model.TemperaturePropertyInstance.String(),
							Retrievable: true,
							Parameters: model.FloatPropertyParameters{
								Instance: model.TemperaturePropertyInstance,
								Unit:     model.UnitTemperatureCelsius,
							},
							State: model.FloatPropertyState{
								Instance: model.TemperaturePropertyInstance,
								Value:    28,
							},
						},
					},
					Type:         model.HumidifierDeviceType,
					OriginalType: model.HumidifierDeviceType,
					Created:      timestamp.PastTimestamp(1600709558),
				},
				{
					ID:         "e2e2e2e2-dead-beef-dead-device000015",
					ExternalID: "e2e2e2e2-dead-beef-dead-device000015",
					Name:       "Белая лента",
					RoomID:     "e2e2e2e2-dead-beef-dead-room00000003",
					Capabilities: []uniproxy.CapabilityUserInfoView{
						{
							Type:        model.OnOffCapabilityType,
							Instance:    string(model.OnOnOffCapabilityInstance),
							Retrievable: true,
							Parameters: model.OnOffCapabilityParameters{
								Split: false,
							},
							State: model.OnOffCapabilityState{
								Instance: model.OnOnOffCapabilityInstance,
								Value:    false,
							},
						},
						{
							Type:        model.ColorSettingCapabilityType,
							Instance:    string(model.TemperatureKCapabilityInstance),
							Retrievable: true,
							Parameters: model.ColorSettingCapabilityParameters{
								TemperatureK: &model.TemperatureKParameters{
									Min: 2700,
									Max: 6500,
								},
							},
							State: model.ColorSettingCapabilityState{
								Instance: model.TemperatureKCapabilityInstance,
								Value:    model.TemperatureK(3400),
							},
						},
						{
							Type:        model.RangeCapabilityType,
							Instance:    string(model.BrightnessRangeInstance),
							Values:      []string{},
							Retrievable: true,
							Parameters: model.RangeCapabilityParameters{
								Instance:     model.BrightnessRangeInstance,
								Unit:         model.UnitPercent,
								RandomAccess: true,
								Looped:       true,
								Range: &model.Range{
									Min:       1,
									Max:       100,
									Precision: 1,
								},
							},
							State: model.RangeCapabilityState{
								Instance: model.BrightnessRangeInstance,
								Value:    75,
							},
						},
					},
					Properties:   []uniproxy.PropertyUserInfoView{},
					Type:         model.LightDeviceType,
					OriginalType: model.LightDeviceType,
					Created:      timestamp.PastTimestamp(1600709558),
				},
				{
					ID:         "e2e2e2e2-dead-beef-dead-device000016",
					ExternalID: "e2e2e2e2-dead-beef-dead-device000016",
					Name:       "Лампочка 1",
					RoomID:     "e2e2e2e2-dead-beef-dead-room00000004",
					Groups: []uniproxy.GroupUserInfoView{
						{
							ID:   "e2e2e2e2-dead-beef-dead-group0000001",
							Name: "Верхний свет",
							Type: model.LightDeviceType,
						},
					},
					Capabilities: []uniproxy.CapabilityUserInfoView{
						{
							Type:        model.OnOffCapabilityType,
							Instance:    string(model.OnOnOffCapabilityInstance),
							Retrievable: true,
							Parameters: model.OnOffCapabilityParameters{
								Split: false,
							},
							State: model.OnOffCapabilityState{
								Instance: model.OnOnOffCapabilityInstance,
								Value:    false,
							},
						},
						{
							Type:        model.ColorSettingCapabilityType,
							Instance:    string(model.HsvColorCapabilityInstance),
							Retrievable: true,
							Parameters: model.ColorSettingCapabilityParameters{
								ColorModel: model.CM(model.HsvModelType),
							},
							State: model.ColorSettingCapabilityState{
								Instance: model.HsvColorCapabilityInstance,
								Value:    model.HSV{H: 137, S: 100, V: 78},
							},
						},
						{
							Type:        model.RangeCapabilityType,
							Instance:    string(model.BrightnessRangeInstance),
							Values:      []string{},
							Retrievable: true,
							Parameters: model.RangeCapabilityParameters{
								Instance:     model.BrightnessRangeInstance,
								Unit:         model.UnitPercent,
								RandomAccess: true,
								Looped:       true,
								Range: &model.Range{
									Min:       1,
									Max:       100,
									Precision: 1,
								},
							},
							State: model.RangeCapabilityState{
								Instance: model.BrightnessRangeInstance,
								Value:    75,
							},
						},
					},
					Properties:   []uniproxy.PropertyUserInfoView{},
					Type:         model.LightDeviceType,
					OriginalType: model.LightDeviceType,
					Created:      timestamp.PastTimestamp(1600709558),
				},
				{
					ID:         "e2e2e2e2-dead-beef-dead-device000017",
					ExternalID: "e2e2e2e2-dead-beef-dead-device000017",
					Name:       "Лампочка 2",
					RoomID:     "e2e2e2e2-dead-beef-dead-room00000004",
					Groups: []uniproxy.GroupUserInfoView{
						{
							ID:   "e2e2e2e2-dead-beef-dead-group0000001",
							Name: "Верхний свет",
							Type: model.LightDeviceType,
						},
					},
					Capabilities: []uniproxy.CapabilityUserInfoView{
						{
							Type:        model.OnOffCapabilityType,
							Instance:    string(model.OnOnOffCapabilityInstance),
							Retrievable: true,
							Parameters: model.OnOffCapabilityParameters{
								Split: false,
							},
							State: model.OnOffCapabilityState{
								Instance: model.OnOnOffCapabilityInstance,
								Value:    true,
							},
						},
						{
							Type:        model.ColorSettingCapabilityType,
							Instance:    string(model.HsvColorCapabilityInstance),
							Retrievable: true,
							Parameters: model.ColorSettingCapabilityParameters{
								ColorModel: model.CM(model.HsvModelType),
							},
							State: model.ColorSettingCapabilityState{
								Instance: model.HsvColorCapabilityInstance,
								Value:    model.HSV{H: 137, S: 100, V: 78},
							},
						},
						{
							Type:        model.RangeCapabilityType,
							Instance:    string(model.BrightnessRangeInstance),
							Values:      []string{},
							Retrievable: true,
							Parameters: model.RangeCapabilityParameters{
								Instance:     model.BrightnessRangeInstance,
								Unit:         model.UnitPercent,
								RandomAccess: true,
								Looped:       true,
								Range: &model.Range{
									Min:       1,
									Max:       100,
									Precision: 1,
								},
							},
							State: model.RangeCapabilityState{
								Instance: model.BrightnessRangeInstance,
								Value:    12,
							},
						},
					},
					Properties:   []uniproxy.PropertyUserInfoView{},
					Type:         model.LightDeviceType,
					OriginalType: model.LightDeviceType,
					Created:      timestamp.PastTimestamp(1600709558),
				},
				{
					ID:         "e2e2e2e2-dead-beef-dead-device000018",
					ExternalID: "e2e2e2e2-dead-beef-dead-device000018",
					Name:       "Вентилятор",
					RoomID:     "e2e2e2e2-dead-beef-dead-room00000004",
					Groups:     []uniproxy.GroupUserInfoView{},
					Capabilities: []uniproxy.CapabilityUserInfoView{
						{
							Type:        model.OnOffCapabilityType,
							Instance:    string(model.OnOnOffCapabilityInstance),
							Retrievable: true,
							Parameters: model.OnOffCapabilityParameters{
								Split: false,
							},
							State: model.OnOffCapabilityState{
								Instance: model.OnOnOffCapabilityInstance,
								Value:    false,
							},
						},
						{
							Type:        model.ToggleCapabilityType,
							Instance:    string(model.IonizationToggleCapabilityInstance),
							Retrievable: true,
							Parameters: model.ToggleCapabilityParameters{
								Instance: model.IonizationToggleCapabilityInstance,
							},
							State: model.ToggleCapabilityState{
								Instance: model.IonizationToggleCapabilityInstance,
								Value:    false,
							},
						},
						{
							Type:        model.ModeCapabilityType,
							Instance:    string(model.FanSpeedModeInstance),
							Values:      []string{string(model.AutoMode), string(model.FastMode)},
							Retrievable: true,
							Parameters: model.ModeCapabilityParameters{
								Instance: model.FanSpeedModeInstance,
								Modes: []model.Mode{
									model.KnownModes[model.AutoMode],
									model.KnownModes[model.FastMode],
								},
							},
							State: model.ModeCapabilityState{
								Instance: model.FanSpeedModeInstance,
								Value:    model.AutoMode,
							},
						},
					},
					Properties:   []uniproxy.PropertyUserInfoView{},
					Type:         model.FanDeviceType,
					OriginalType: model.FanDeviceType,
					Created:      timestamp.PastTimestamp(1600709558),
				},
				{
					ID:         "e2e2e2e2-dead-beef-dead-device000019",
					ExternalID: "e2e2e2e2-dead-beef-dead-device000019",
					Name:       "Розетка",
					RoomID:     "e2e2e2e2-dead-beef-dead-room00000001",
					Groups: []uniproxy.GroupUserInfoView{
						{
							ID:   "e2e2e2e2-dead-beef-dead-group0000003",
							Name: "Электроприборы",
						},
					},
					Capabilities: []uniproxy.CapabilityUserInfoView{
						{
							Type:        model.OnOffCapabilityType,
							Instance:    string(model.OnOnOffCapabilityInstance),
							Retrievable: true,
							Parameters: model.OnOffCapabilityParameters{
								Split: false,
							},
							State: model.OnOffCapabilityState{
								Instance: model.OnOnOffCapabilityInstance,
								Value:    true,
							},
						},
					},
					Properties: []uniproxy.PropertyUserInfoView{
						{
							Type:        model.FloatPropertyType,
							Instance:    model.VoltagePropertyInstance.String(),
							Retrievable: true,
							Parameters: model.FloatPropertyParameters{
								Instance: model.VoltagePropertyInstance,
								Unit:     model.UnitVolt,
							},
							State: model.FloatPropertyState{
								Instance: model.VoltagePropertyInstance,
								Value:    10,
							},
						},
						{
							Type:        model.FloatPropertyType,
							Instance:    model.AmperagePropertyInstance.String(),
							Retrievable: true,
							Parameters: model.FloatPropertyParameters{
								Instance: model.AmperagePropertyInstance,
								Unit:     model.UnitAmpere,
							},
							State: model.FloatPropertyState{
								Instance: model.AmperagePropertyInstance,
								Value:    5,
							},
						},
						{
							Type:        model.FloatPropertyType,
							Instance:    model.PowerPropertyInstance.String(),
							Retrievable: true,
							Parameters: model.FloatPropertyParameters{
								Instance: model.PowerPropertyInstance,
								Unit:     model.UnitWatt,
							},
							State: model.FloatPropertyState{
								Instance: model.PowerPropertyInstance,
								Value:    50,
							},
						},
					},
					Type:         model.SocketDeviceType,
					OriginalType: model.SocketDeviceType,
					Created:      timestamp.PastTimestamp(1600709558),
				},
				{
					ID:         "e2e2e2e2-dead-beef-dead-device000020",
					ExternalID: "e2e2e2e2-dead-beef-dead-device000020",
					Name:       "Обдув",
					RoomID:     "e2e2e2e2-dead-beef-dead-room00000001",
					Groups: []uniproxy.GroupUserInfoView{
						{
							ID:   "e2e2e2e2-dead-beef-dead-group0000004",
							Name: "Обдуватели",
							Type: model.AcDeviceType,
						},
					},
					Capabilities: []uniproxy.CapabilityUserInfoView{
						{
							Type:        model.OnOffCapabilityType,
							Instance:    string(model.OnOnOffCapabilityInstance),
							Retrievable: true,
							Parameters: model.OnOffCapabilityParameters{
								Split: false,
							},
							State: model.OnOffCapabilityState{
								Instance: model.OnOnOffCapabilityInstance,
								Value:    false,
							},
						},
						{
							Type:        model.RangeCapabilityType,
							Instance:    string(model.TemperatureRangeInstance),
							Retrievable: true,
							Parameters: model.RangeCapabilityParameters{
								Instance:     model.TemperatureRangeInstance,
								Unit:         model.UnitTemperatureCelsius,
								RandomAccess: true,
								Looped:       true,
								Range: &model.Range{
									Min:       1,
									Max:       100,
									Precision: 1,
								},
							},
							State: model.RangeCapabilityState{
								Instance: model.TemperatureRangeInstance,
								Value:    28,
							},
						},
						{
							Type:        model.ModeCapabilityType,
							Instance:    string(model.ThermostatModeInstance),
							Values:      []string{string(model.CoolMode), string(model.HeatMode)},
							Retrievable: true,
							Parameters: model.ModeCapabilityParameters{
								Instance: model.ThermostatModeInstance,
								Modes: []model.Mode{
									model.KnownModes[model.HeatMode],
									model.KnownModes[model.CoolMode],
								},
							},
							State: model.ModeCapabilityState{
								Instance: model.ThermostatModeInstance,
								Value:    model.HeatMode,
							},
						},
						{
							Type:        model.ModeCapabilityType,
							Instance:    string(model.FanSpeedModeInstance),
							Values:      []string{string(model.AutoMode), string(model.FastMode)},
							Retrievable: true,
							Parameters: model.ModeCapabilityParameters{
								Instance: model.FanSpeedModeInstance,
								Modes: []model.Mode{
									model.KnownModes[model.AutoMode],
									model.KnownModes[model.FastMode],
								},
							},
							State: model.ModeCapabilityState{
								Instance: model.FanSpeedModeInstance,
								Value:    model.FastMode,
							},
						},
						{
							Type:        model.ToggleCapabilityType,
							Instance:    string(model.IonizationToggleCapabilityInstance),
							Retrievable: true,
							Parameters: model.ToggleCapabilityParameters{
								Instance: model.IonizationToggleCapabilityInstance,
							},
							State: model.ToggleCapabilityState{
								Instance: model.IonizationToggleCapabilityInstance,
								Value:    true,
							},
						},
					},
					Properties:   []uniproxy.PropertyUserInfoView{},
					Type:         model.AcDeviceType,
					OriginalType: model.AcDeviceType,
					Created:      timestamp.PastTimestamp(1600709558),
				},
				{
					ID:         "e2e2e2e2-dead-beef-dead-device000021",
					ExternalID: "e2e2e2e2-dead-beef-dead-device000021",
					Name:       "Сокет",
					RoomID:     "e2e2e2e2-dead-beef-dead-room00000001",
					Groups:     []uniproxy.GroupUserInfoView{},
					Capabilities: []uniproxy.CapabilityUserInfoView{
						{
							Type:        model.OnOffCapabilityType,
							Instance:    string(model.OnOnOffCapabilityInstance),
							Retrievable: true,
							Parameters: model.OnOffCapabilityParameters{
								Split: false,
							},
							State: model.OnOffCapabilityState{
								Instance: model.OnOnOffCapabilityInstance,
								Value:    true,
							},
						},
					},
					Properties: []uniproxy.PropertyUserInfoView{
						{
							Type:        model.FloatPropertyType,
							Instance:    model.VoltagePropertyInstance.String(),
							Retrievable: true,
							Parameters: model.FloatPropertyParameters{
								Instance: model.VoltagePropertyInstance,
								Unit:     model.UnitVolt,
							},
							State: model.FloatPropertyState{
								Instance: model.VoltagePropertyInstance,
								Value:    5,
							},
						},
						{
							Type:        model.FloatPropertyType,
							Instance:    model.AmperagePropertyInstance.String(),
							Retrievable: true,
							Parameters: model.FloatPropertyParameters{
								Instance: model.AmperagePropertyInstance,
								Unit:     model.UnitAmpere,
							},
							State: model.FloatPropertyState{
								Instance: model.AmperagePropertyInstance,
								Value:    10,
							},
						},
						{
							Type:        model.FloatPropertyType,
							Instance:    model.PowerPropertyInstance.String(),
							Retrievable: true,
							Parameters: model.FloatPropertyParameters{
								Instance: model.PowerPropertyInstance,
								Unit:     model.UnitWatt,
							},
							State: model.FloatPropertyState{
								Instance: model.PowerPropertyInstance,
								Value:    50,
							},
						},
					},
					Type:         model.SocketDeviceType,
					OriginalType: model.SocketDeviceType,
					Created:      timestamp.PastTimestamp(1600709558),
				},
			},
			Rooms: []uniproxy.RoomUserInfoView{
				{
					ID:   "e2e2e2e2-dead-beef-dead-room00000001",
					Name: "Гостиная",
				},
				{
					ID:   "e2e2e2e2-dead-beef-dead-room00000002",
					Name: "Кухня",
				},
				{
					ID:   "e2e2e2e2-dead-beef-dead-room00000003",
					Name: "Спальня",
				},
				{
					ID:   "e2e2e2e2-dead-beef-dead-room00000004",
					Name: "Мастерская",
				},
			},
			Groups: []uniproxy.GroupUserInfoView{
				{
					ID:   "e2e2e2e2-dead-beef-dead-group0000001",
					Name: "Верхний свет",
					Type: model.LightDeviceType,
				},
				{
					ID:   "e2e2e2e2-dead-beef-dead-group0000002",
					Name: "Климат",
					Type: model.AcDeviceType,
				},
				{
					ID:   "e2e2e2e2-dead-beef-dead-group0000003",
					Name: "Электроприборы",
					Type: model.SocketDeviceType,
				},
				{
					ID:   "e2e2e2e2-dead-beef-dead-group0000004",
					Name: "Обдуватели",
					Type: model.AcDeviceType,
				},
			},
		},
	}
}

func cleanStuff(ctx context.Context, db *db.DBClient, euser EUser) error {
	uid, _ := strconv.ParseUint(euser.ID, 10, 64)
	for _, tableName := range [...]string{
		"DeviceGroups",
		"Devices",
		//"ExternalUsers",
		"Groups",
		"Rooms",
		"Scenarios",
		//"StationOwners",
		"UserSkills",
	} {
		query := fmt.Sprintf(`
			PRAGMA TablePathPrefix("%s");
			DECLARE $huid AS Uint64;
			DELETE FROM
				%s
			WHERE
				huid == $huid`, db.Prefix, tableName)
		params := table.NewQueryParameters(
			table.ValueParam("$huid", ydb.Uint64Value(tools.Huidify(uid))),
		)
		logger.Debugf("cleaning <%s> records for user: %d", tableName, uid)
		if err := db.Write(ctx, query, params); err != nil {
			return err
		}
	}

	for _, tableName := range []string{
		"Users",
	} {
		query := fmt.Sprintf(`
			PRAGMA TablePathPrefix("%s");
			DECLARE $huid AS Uint64;
			DELETE FROM
				%s
			WHERE
				hid == $huid`, db.Prefix, tableName)
		params := table.NewQueryParameters(
			table.ValueParam("$huid", ydb.Uint64Value(tools.Huidify(uid))),
		)
		logger.Debugf("cleaning <%s> records for user: %d", tableName, uid)
		if err := db.Write(ctx, query, params); err != nil {
			return err
		}
	}

	return nil
}

func createStuff(ctx context.Context, db *db.DBClient, euser EUser, config Config) error {
	uid, _ := strconv.ParseUint(euser.ID, 10, 64)

	// -- store user
	user := model.User{ID: uid, Login: euser.Login}
	err := db.StoreUser(ctx, user)
	if err != nil {
		return fmt.Errorf(fmt.Sprintf("Failed to create user %#v.", user), err)
	}

	// -- store rooms
	roomsMap := make(map[string]string) // map [originalRoomID]newRoomID
	for _, room := range config.UserInfoView.Rooms {
		logger.Debugf("creating room `%s` for user `%d`", room.Name, uid)
		roomID, err := db.CreateUserRoom(ctx, user, model.Room{Name: room.Name})
		if err != nil {
			return fmt.Errorf(fmt.Sprintf("Failed to store room with name `%s` for user `%d`, config `%s`", room.Name, uid, config.RequestID), err)
		}
		roomsMap[room.ID] = roomID
	}

	// -- store groups
	groupsMap := make(map[string]string) // map [originalGroupID]newGroupID
	for _, group := range config.UserInfoView.Groups {
		logger.Debugf("creating group `%s` for user `%d`", group.Name, uid)
		groupID, err := db.CreateUserGroup(ctx, user, model.Group{Name: group.Name})
		if err != nil {
			return fmt.Errorf(fmt.Sprintf("Failed to store group with name `%s` for user `%d`, config `%s`", group.Name, uid, config.RequestID), err)
		}
		groupsMap[group.ID] = groupID
	}

	// -- store devices
	devicesMap := make(map[string]string) // map [originalDeviceID]newDeviceID
	for _, d := range config.UserInfoView.Devices {
		device := d.ToDevice()
		device.ExternalName = device.Name         // db.StoreUserDevice doesn't expect device.Name to be provided
		device.ExternalID = idsPrefix + device.ID // cause externalID is unknown from megamind.UserInfoViewPayload
		device.SkillID = model.VIRTUAL
		if device.Room != nil {
			device.Room.ID = roomsMap[device.Room.ID]
		}
		storedDevice, _, err := db.StoreUserDevice(ctx, user, device)
		if err != nil {
			return fmt.Errorf(fmt.Sprintf("Failed to store device with id `%s` for user `%d`, config `%s`", device.ID, uid, config.RequestID), err)
		}
		devicesMap[device.ID] = storedDevice.ID

		// -- store devices groups
		if len(device.Groups) > 0 {
			deviceGroups := make([]string, 0)
			for _, group := range device.Groups {
				deviceGroups = append(deviceGroups, groupsMap[group.ID])
			}

			if err := db.UpdateUserDeviceGroups(ctx, user, storedDevice.ID, deviceGroups); err != nil {
				return fmt.Errorf(fmt.Sprintf("Failed to update device `%s` groups for user `%d`, config `%s`. Groups: %#v", device.ID, uid, config.RequestID, deviceGroups), err)
			}
		}
	}

	// -- store scenarios
	for _, s := range config.UserInfoView.Scenarios {
		scenario := s.ToScenario()

		// FIXME: back those days megamind.UserInfoViewPayload was missing these fields
		if len(scenario.Icon) == 0 {
			scenario.Icon = model.ScenarioIconAlarm
		}

		scenarioDevices := make([]model.ScenarioDevice, 0)
		for _, sDevice := range scenario.Devices {
			sDevice.ID = devicesMap[sDevice.ID]
			scenarioDevices = append(scenarioDevices, sDevice)
		}
		scenario.Devices = scenarioDevices
		if _, err := db.CreateScenario(ctx, uid, scenario); err != nil {
			return fmt.Errorf(fmt.Sprintf("Failed to store scenario `%#v` for user `%d`, config `%s`", scenario, uid, config.RequestID), err)
		}
	}

	return nil
}

func do(ctx context.Context, db *db.DBClient) error {
	user, config := getUser(), getConfig()

	if err := cleanStuff(ctx, db, user); err != nil {
		return xerrors.Errorf("can't clean stuff: %w", err)
	}
	if err := createStuff(ctx, db, user, config); err != nil {
		return xerrors.Errorf("can't create stuff: %w", err)
	}
	logger.Warnf("%s,%s,%s", config.RequestID, user.ID, user.Token)
	return nil
}

func main() {
	var stop func()
	logger, stop = initLogging()
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

	msg := fmt.Sprintf("Do you really want to generate user config for VoiceQueries. at `%s%s`", endpoint, prefix)
	c := cli.AskForConfirmation(msg, logger)
	if !c {
		logger.Info("Bye")
		os.Exit(0)
	}

	ctx := context.Background()
	start := time.Now()
	if err := do(ctx, dbcli); err != nil {
		logger.Fatal(err.Error())
	}
	logger.Warnf("Time elapsed: %v", time.Since(start))
}
