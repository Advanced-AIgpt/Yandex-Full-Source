package main

import (
	"encoding/json"
	"fmt"
	"github.com/gofrs/uuid"
	"io/fs"
	"io/ioutil"
	"math"
	"math/rand"
	"strings"
	"time"

	"a.yandex-team.ru/alice/iot/bulbasaur/dto/adapter"
	"a.yandex-team.ru/alice/iot/bulbasaur/dto/callback"
	"a.yandex-team.ru/alice/iot/bulbasaur/dto/stress"
	"a.yandex-team.ru/alice/iot/bulbasaur/model"
	"a.yandex-team.ru/alice/library/go/timestamp"
	"a.yandex-team.ru/library/go/core/xerrors"
)

type Device struct {
	SkillID          string
	ExternalUserID   string
	ExternalDeviceID string
	Properties       []model.PropertyInstance
}

func generateCallbackAmmo() error {
	// https://wiki.yandex-team.ru/Load/tvm/#ammo

	builder := strings.Builder{}
	builder.WriteString("[User-Agent: Tank]")
	devices := []Device{
		{
			SkillID:          model.XiaomiSkill,
			ExternalUserID:   "hHTd_10FC_gI9yQwwn3swNz33wSp-J8m8OG-xKkg",
			ExternalDeviceID: "M1GAxtaW9A0LXNwZWMtdjIVlIAFGAdsdW1pLXYxFRQYE2x1bWkuMTU4ZDAwMDQ0ZmMxN2YVxAEA",
			Properties: []model.PropertyInstance{
				model.TemperaturePropertyInstance,
				model.HumidityPropertyInstance,
			},
		},
		{
			SkillID:          model.TUYA,
			ExternalUserID:   "eu1559314790485mVBQ5",
			ExternalDeviceID: "bfc483ffc2a7004a94df66",
			Properties: []model.PropertyInstance{
				model.PowerPropertyInstance,
				model.VoltagePropertyInstance,
			},
		},
	}

	start := time.Now().Add(-30 * time.Minute)
	end := time.Now().Add(10 * time.Minute)
	step := int64(10)

	for callbackTime := start.Unix(); callbackTime <= end.Unix(); callbackTime += step {
		for _, device := range devices {
			for _, property := range device.Properties {
				request := &callback.UpdateStateRequest{
					Timestamp: timestamp.PastTimestamp(callbackTime),
					Payload: &callback.UpdateStatePayload{
						UserID: device.ExternalUserID,
						DeviceStates: []callback.DeviceStateView{
							{
								ID: device.ExternalDeviceID,
								Properties: []adapter.PropertyStateView{
									{
										Type: model.FloatPropertyType,
										State: &model.FloatPropertyState{
											Instance: property,
											Value:    getRandomFloatValue(property),
										},
									},
								},
							},
						},
					},
				}
				data, err := json.Marshal(request)
				if err != nil {
					return err
				}
				builder.WriteString(fmt.Sprintf("\n%d /v1.0/callback/skills/%s/state\n", len(data), device.SkillID))
				builder.Write(data)
			}
		}
	}

	filename := "callback_ammo.lst"
	if err := ioutil.WriteFile(filename, []byte(builder.String()), fs.ModePerm); err != nil {
		return xerrors.Errorf("failed to write file to disk: %w", err)
	}
	return nil
}

type deviceWithProps struct {
	ID         string
	Properties []model.PropertyInstance
}

func generateHistorySolomonAmmo() error {
	builder := strings.Builder{}
	builder.WriteString("[User-Agent: Tank]")

	specs := []struct {
		Count      int
		Properties []model.PropertyInstance
	}{
		{
			Count: 100,
			Properties: []model.PropertyInstance{
				model.TemperaturePropertyInstance,
				//	model.HumidityPropertyInstance,
			},
		},
		//{
		//	Count: 80,
		//	Properties: []model.PropertyInstance{
		//		model.PowerPropertyInstance,
		//		model.VoltagePropertyInstance,
		//		model.AmperagePropertyInstance,
		//	},
		//},
		//{
		//	Count: 5,
		//	Properties: []model.PropertyInstance{
		//		model.SmokeConcentrationPropertyInstance,
		//		model.GasConcentrationPropertyInstance,
		//	},
		//},
		//{
		//	Count: 5,
		//	Properties: []model.PropertyInstance{
		//		model.CO2LevelPropertyInstance,
		//	},
		//},
		//{
		//	Count: 2,
		//	Properties: []model.PropertyInstance{
		//		model.PM1DensityPropertyInstance,
		//	},
		//},
	}

	start := time.Now().Add(-1440 * time.Minute)
	end := time.Now().Add(10 * time.Minute)
	step := int64(15)

	var devices []deviceWithProps

	for _, spec := range specs {
		for i := 0; i < spec.Count; i++ {
			deviceID, _ := uuid.NewV4()
			devices = append(devices, deviceWithProps{
				ID:         deviceID.String(),
				Properties: append([]model.PropertyInstance{}, spec.Properties...),
			})
		}
	}

	for callbackTime := start.Unix(); callbackTime <= end.Unix(); callbackTime += step {
		for _, device := range devices {
			for _, instance := range device.Properties {
				body := &stress.UpdateSolomonStateRequest{
					Timestamp: timestamp.PastTimestamp(callbackTime),
					DeviceID:  device.ID,
					Instance:  instance,
					Value:     getRandomFloatValue(instance),
				}

				data, err := json.Marshal(body)
				if err != nil {
					return err
				}

				builder.WriteString(fmt.Sprintf("\n%d /stress/history/solomon\n", len(data)))
				builder.Write(data)
			}
		}
	}

	filename := "solomon_amo.lst"
	if err := ioutil.WriteFile(filename, []byte(builder.String()), fs.ModePerm); err != nil {
		return xerrors.Errorf("failed to write file to disk: %w", err)
	}
	return nil
}

func getRandomFloatValue(propertyInstance model.PropertyInstance) float64 {
	var start, end float64
	switch propertyInstance {
	case model.TemperaturePropertyInstance:
		start, end = 8, 32
	default:
		start, end = 0, 100
	}
	val := start + rand.Float64()*(end-start)
	return math.Round(val*10000) / 10000
}

func run() error {
	return generateHistorySolomonAmmo()
}

func main() {
	rand.Seed(time.Now().UnixMilli())
	if err := run(); err != nil {
		panic(err)
	}
}
