package philips

import (
	"encoding/json"
	"testing"

	"a.yandex-team.ru/alice/iot/bulbasaur/dto/adapter"
	"a.yandex-team.ru/alice/iot/bulbasaur/model"
	"a.yandex-team.ru/alice/library/go/tools"
	"github.com/stretchr/testify/assert"
	"golang.org/x/xerrors"
)

func TestLightInfo_ToCapabilityInfoViews(t *testing.T) {
	type testCase struct {
		name         string
		rawLightInfo string // JSON
		capabilities []adapter.CapabilityInfoView
	}

	cases := []testCase{
		{
			name:         "Color light: Hue bloom (LLC011)",
			rawLightInfo: `{"state":{"on":false,"bri":126,"hue":8229,"sat":251,"effect":"none","xy":[0.6014,0.3877],"alert":"none","colormode":"xy","mode":"homeautomation","reachable":true},"swupdate":{"state":"noupdates","lastinstall":null},"type":"Color light","name":"Hue bloom","modelid":"LLC011","manufacturername":"Philips","productname":"Hue bloom","capabilities":{"certified":true,"control":{"mindimlevel":10000,"maxlumen":120,"colorgamuttype":"A","colorgamut":[[0.704,0.296],[0.2151,0.7106],[0.138,0.08]]},"streaming":{"renderer":true,"proxy":false}},"config":{"archetype":"huebloom","function":"decorative","direction":"upwards","startup":{"mode":"powerfail","configured":true}},"uniqueid":"00:17:88:01:ff:ff:ff:ff-ff","swversion":"5.127.1.26581"}`,
			capabilities: []adapter.CapabilityInfoView{
				{
					Type:        model.OnOffCapabilityType,
					Retrievable: true,
				},
				{
					Type:        model.RangeCapabilityType,
					Retrievable: true,
					Parameters: model.RangeCapabilityParameters{
						Instance:     model.BrightnessRangeInstance,
						Unit:         model.UnitPercent,
						RandomAccess: true,
						Range: &model.Range{
							Min:       1,
							Max:       100,
							Precision: 1,
						},
					},
				},
				{
					Type:        model.ColorSettingCapabilityType,
					Retrievable: true,
					Parameters: model.ColorSettingCapabilityParameters{
						ColorModel: AOColorModelType(model.HsvModelType),
					},
				},
			},
		},
		{
			name:         "Color light: Hue iris (LLC010)",
			rawLightInfo: `{"state":{"on":false,"bri":144,"hue":11973,"sat":149,"effect":"none","xy":[0.5053,0.4206],"alert":"select","colormode":"xy","mode":"homeautomation","reachable":true},"swupdate":{"state":"noupdates","lastinstall":"2018-12-13T13:51:13"},"type":"Color light","name":"Hue iris","modelid":"LLC010","manufacturername":"Philips","productname":"Hue iris","capabilities":{"certified":true,"control":{"mindimlevel":10000,"maxlumen":210,"colorgamuttype":"A","colorgamut":[[0.704,0.296],[0.2151,0.7106],[0.138,0.08]]},"streaming":{"renderer":true,"proxy":false}},"config":{"archetype":"hueiris","function":"decorative","direction":"upwards","startup":{"mode":"safety","configured":true}},"uniqueid":"00:17:88:01:ff:ff:ff:ff-ff","swversion":"5.127.1.26581"}`,
			capabilities: []adapter.CapabilityInfoView{
				{
					Type:        model.OnOffCapabilityType,
					Retrievable: true,
				},
				{
					Type:        model.RangeCapabilityType,
					Retrievable: true,
					Parameters: model.RangeCapabilityParameters{
						Instance:     model.BrightnessRangeInstance,
						Unit:         model.UnitPercent,
						RandomAccess: true,
						Range: &model.Range{
							Min:       1,
							Max:       100,
							Precision: 1,
						},
					},
				},
				{
					Type:        model.ColorSettingCapabilityType,
					Retrievable: true,
					Parameters: model.ColorSettingCapabilityParameters{
						ColorModel: AOColorModelType(model.HsvModelType),
					},
				},
			},
		},
		{
			name:         "Color light: Hue lightstrip (LST001)",
			rawLightInfo: `{"state":{"on":false,"bri":254,"hue":4551,"sat":243,"effect":"none","xy":[0.6497,0.3395],"alert":"select","colormode":"hs","mode":"homeautomation","reachable":true},"swupdate":{"state":"noupdates","lastinstall":"2019-08-23T09:36:52"},"type":"Color light","name":"Hue lightstrip","modelid":"LST001","manufacturername":"Philips","productname":"Hue lightstrip","capabilities":{"certified":true,"control":{"mindimlevel":10000,"maxlumen":120,"colorgamuttype":"A","colorgamut":[[0.704,0.296],[0.2151,0.7106],[0.138,0.08]]},"streaming":{"renderer":true,"proxy":false}},"config":{"archetype":"huelightstrip","function":"mixed","direction":"omnidirectional","startup":{"mode":"safety","configured":true}},"uniqueid":"00:17:88:01:ff:ff:ff:ff-ff","swversion":"5.127.1.26581"}`,
			capabilities: []adapter.CapabilityInfoView{
				{
					Type:        model.OnOffCapabilityType,
					Retrievable: true,
				},
				{
					Type:        model.RangeCapabilityType,
					Retrievable: true,
					Parameters: model.RangeCapabilityParameters{
						Instance:     model.BrightnessRangeInstance,
						Unit:         model.UnitPercent,
						RandomAccess: true,
						Range: &model.Range{
							Min:       1,
							Max:       100,
							Precision: 1,
						},
					},
				},
				{
					Type:        model.ColorSettingCapabilityType,
					Retrievable: true,
					Parameters: model.ColorSettingCapabilityParameters{
						ColorModel: AOColorModelType(model.HsvModelType),
					},
				},
			},
		},
		{
			name:         "Color light: LivingColors (LLC001)",
			rawLightInfo: `{"state":{"on":true,"bri":86,"hue":2095,"sat":200,"effect":"none","xy":[0.633,0.3318],"alert":"select","colormode":"xy","mode":"homeautomation","reachable":true},"swupdate":{"state":"notupdatable","lastinstall":"2019-03-07T10:41:27"},"type":"Color light","name":"LivingColors","modelid":"LLC001","manufacturername":"Philips","productname":"LivingColors","capabilities":{"certified":true,"control":{"colorgamuttype":"A","colorgamut":[[0.704,0.296],[0.2151,0.7106],[0.138,0.08]]},"streaming":{"renderer":false,"proxy":false}},"config":{"archetype":"floorshade","function":"decorative","direction":"omnidirectional"},"uniqueid":"00:17:88:01:ff:ff:ff:ff-ff","swversion":"2.0.0.5206"}`,
			capabilities: []adapter.CapabilityInfoView{
				{
					Type:        model.OnOffCapabilityType,
					Retrievable: true,
				},
				{
					Type:        model.RangeCapabilityType,
					Retrievable: true,
					Parameters: model.RangeCapabilityParameters{
						Instance:     model.BrightnessRangeInstance,
						Unit:         model.UnitPercent,
						RandomAccess: true,
						Range: &model.Range{
							Min:       1,
							Max:       100,
							Precision: 1,
						},
					},
				},
				{
					Type:        model.ColorSettingCapabilityType,
					Retrievable: true,
					Parameters: model.ColorSettingCapabilityParameters{
						ColorModel: AOColorModelType(model.HsvModelType),
					},
				},
			},
		},
		{
			name:         "Color light: LivingColors (LLC007)",
			rawLightInfo: `{"state":{"on":false,"bri":0,"hue":1962,"sat":217,"effect":"none","xy":[0.6509,0.3246],"alert":"none","colormode":"hs","mode":"homeautomation","reachable":true},"swupdate":{"state":"notupdatable","lastinstall":null},"type":"Color light","name":"LivingColors","modelid":"LLC007","manufacturername":"Philips","productname":"LivingColors","capabilities":{"certified":true,"control":{"mindimlevel":10000,"maxlumen":120,"colorgamuttype":"A","colorgamut":[[0.704,0.296],[0.2151,0.7106],[0.138,0.08]]},"streaming":{"renderer":false,"proxy":false}},"config":{"archetype":"huebloom","function":"decorative","direction":"upwards"},"uniqueid":"00:17:88:01:ff:ff:ff:ff-ff","swversion":"4.6.0.8274"}`,
			capabilities: []adapter.CapabilityInfoView{
				{
					Type:        model.OnOffCapabilityType,
					Retrievable: true,
				},
				{
					Type:        model.RangeCapabilityType,
					Retrievable: true,
					Parameters: model.RangeCapabilityParameters{
						Instance:     model.BrightnessRangeInstance,
						Unit:         model.UnitPercent,
						RandomAccess: true,
						Range: &model.Range{
							Min:       1,
							Max:       100,
							Precision: 1,
						},
					},
				},
				{
					Type:        model.ColorSettingCapabilityType,
					Retrievable: true,
					Parameters: model.ColorSettingCapabilityParameters{
						ColorModel: AOColorModelType(model.HsvModelType),
					},
				},
			},
		},
		{
			name:         "Color temperature light: Color temperature light (GL-C-006)",
			rawLightInfo: `{"state":{"on":true,"bri":254,"ct":307,"alert":"select","colormode":"ct","mode":"homeautomation","reachable":false},"swupdate":{"state":"notupdatable","lastinstall":"2019-05-24T19:15:22"},"type":"Color temperature light","name":"Color temperature light","modelid":"GL-C-006","manufacturername":"GLEDOPTO","productname":"Color temperature light","capabilities":{"certified":false,"control":{"ct":{"min":158,"max":500}},"streaming":{"renderer":false,"proxy":false}},"config":{"archetype":"classicbulb","function":"functional","direction":"omnidirectional"},"uniqueid":"00:12:4b:00:ff:ff:ff:ff-ff","swversion":"1.0.7"}`,
			capabilities: []adapter.CapabilityInfoView{
				{
					Type:        model.OnOffCapabilityType,
					Retrievable: true,
				},
				{
					Type:        model.RangeCapabilityType,
					Retrievable: true,
					Parameters: model.RangeCapabilityParameters{
						Instance:     model.BrightnessRangeInstance,
						Unit:         model.UnitPercent,
						RandomAccess: true,
						Range: &model.Range{
							Min:       1,
							Max:       100,
							Precision: 1,
						},
					},
				},
				{
					Type:        model.ColorSettingCapabilityType,
					Retrievable: true,
					Parameters: model.ColorSettingCapabilityParameters{
						TemperatureK: &model.TemperatureKParameters{Min: 2000, Max: 6329},
					},
				},
			},
		},
		{
			name:         "Color temperature light: Color temperature light (Surface Light TW)",
			rawLightInfo: `{"state":{"on":true,"bri":254,"ct":370,"alert":"select","colormode":"ct","mode":"homeautomation","reachable":false},"swupdate":{"state":"notupdatable","lastinstall":"2019-03-07T10:47:34"},"type":"Color temperature light","name":"Color temperature light","modelid":"Surface Light TW","manufacturername":"OSRAM","productname":"Color temperature light","capabilities":{"certified":false,"control":{"ct":{"min":153,"max":370}},"streaming":{"renderer":false,"proxy":false}},"config":{"archetype":"classicbulb","function":"functional","direction":"omnidirectional"},"uniqueid":"84:18:26:00:ff:ff:ff:ff-ff","swversion":"V1.03.20"}`,
			capabilities: []adapter.CapabilityInfoView{
				{
					Type:        model.OnOffCapabilityType,
					Retrievable: true,
				},
				{
					Type:        model.RangeCapabilityType,
					Retrievable: true,
					Parameters: model.RangeCapabilityParameters{
						Instance:     model.BrightnessRangeInstance,
						Unit:         model.UnitPercent,
						RandomAccess: true,
						Range: &model.Range{
							Min:       1,
							Max:       100,
							Precision: 1,
						},
					},
				},
				{
					Type:        model.ColorSettingCapabilityType,
					Retrievable: true,
					Parameters: model.ColorSettingCapabilityParameters{
						TemperatureK: &model.TemperatureKParameters{Min: 2702, Max: 6535},
					},
				},
			},
		},
		{
			name:         "Color temperature light: Hue ambiance candle (LTW012)",
			rawLightInfo: `{"state":{"on":false,"bri":234,"ct":298,"alert":"none","colormode":"ct","mode":"homeautomation","reachable":true},"swupdate":{"state":"noupdates","lastinstall":null},"type":"Color temperature light","name":"Hue ambiance candle","modelid":"LTW012","manufacturername":"Philips","productname":"Hue ambiance candle","capabilities":{"certified":true,"control":{"mindimlevel":2000,"maxlumen":450,"ct":{"min":153,"max":454}},"streaming":{"renderer":false,"proxy":false}},"config":{"archetype":"candlebulb","function":"decorative","direction":"omnidirectional","startup":{"mode":"powerfail","configured":true}},"uniqueid":"00:17:88:01:ff:ff:ff:ff-ff","swversion":"1.46.13_r26312","swconfigid":"5E2017D8","productid":"Philips-LTW012-1-E14CTv1"}`,
			capabilities: []adapter.CapabilityInfoView{
				{
					Type:        model.OnOffCapabilityType,
					Retrievable: true,
				},
				{
					Type:        model.RangeCapabilityType,
					Retrievable: true,
					Parameters: model.RangeCapabilityParameters{
						Instance:     model.BrightnessRangeInstance,
						Unit:         model.UnitPercent,
						RandomAccess: true,
						Range: &model.Range{
							Min:       1,
							Max:       100,
							Precision: 1,
						},
					},
				},
				{
					Type:        model.ColorSettingCapabilityType,
					Retrievable: true,
					Parameters: model.ColorSettingCapabilityParameters{
						TemperatureK: &model.TemperatureKParameters{Min: 2202, Max: 6535},
					},
				},
			},
		},
		{
			name:         "Color temperature light: Hue ambiance ceiling (LTC002)",
			rawLightInfo: `{"state":{"on":false,"bri":148,"ct":343,"alert":"none","colormode":"ct","mode":"homeautomation","reachable":true},"swupdate":{"state":"noupdates","lastinstall":"2019-04-17T11:18:55"},"type":"Color temperature light","name":"Hue ambiance ceiling","modelid":"LTC002","manufacturername":"Philips","productname":"Hue ambiance ceiling","capabilities":{"certified":true,"control":{"mindimlevel":1000,"maxlumen":3000,"ct":{"min":153,"max":454}},"streaming":{"renderer":false,"proxy":false}},"config":{"archetype":"ceilinground","function":"functional","direction":"downwards","startup":{"mode":"powerfail","configured":true}},"uniqueid":"00:17:88:01:ff:ff:ff:ff-ff","swversion":"1.46.13_r26312","swconfigid":"A987DFF5","productid":"ENA_LTC002_1_FairCeiling_v1"}`,
			capabilities: []adapter.CapabilityInfoView{
				{
					Type:        model.OnOffCapabilityType,
					Retrievable: true,
				},
				{
					Type:        model.RangeCapabilityType,
					Retrievable: true,
					Parameters: model.RangeCapabilityParameters{
						Instance:     model.BrightnessRangeInstance,
						Unit:         model.UnitPercent,
						RandomAccess: true,
						Range: &model.Range{
							Min:       1,
							Max:       100,
							Precision: 1,
						},
					},
				},
				{
					Type:        model.ColorSettingCapabilityType,
					Retrievable: true,
					Parameters: model.ColorSettingCapabilityParameters{
						TemperatureK: &model.TemperatureKParameters{Min: 2202, Max: 6535},
					},
				},
			},
		},
		{
			name:         "Color temperature light: Hue ambiance lamp (LTA001)",
			rawLightInfo: `{"state":{"on":false,"bri":137,"ct":454,"alert":"none","colormode":"ct","mode":"homeautomation","reachable":false},"swupdate":{"state":"noupdates","lastinstall":"2019-04-24T07:30:19"},"type":"Color temperature light","name":"Hue ambiance lamp","modelid":"LTA001","manufacturername":"Philips","productname":"Hue ambiance lamp","capabilities":{"certified":true,"control":{"mindimlevel":200,"maxlumen":800,"ct":{"min":153,"max":454}},"streaming":{"renderer":false,"proxy":false}},"config":{"archetype":"sultanbulb","function":"functional","direction":"omnidirectional","startup":{"mode":"safety","configured":true}},"uniqueid":"00:17:88:01:ff:ff:ff:ff-ff","swversion":"1.53.3_r27175","swconfigid":"2E12ED35","productid":"Philips-LTA001-2-A19CTv3"}`,
			capabilities: []adapter.CapabilityInfoView{
				{
					Type:        model.OnOffCapabilityType,
					Retrievable: true,
				},
				{
					Type:        model.RangeCapabilityType,
					Retrievable: true,
					Parameters: model.RangeCapabilityParameters{
						Instance:     model.BrightnessRangeInstance,
						Unit:         model.UnitPercent,
						RandomAccess: true,
						Range: &model.Range{
							Min:       1,
							Max:       100,
							Precision: 1,
						},
					},
				},
				{
					Type:        model.ColorSettingCapabilityType,
					Retrievable: true,
					Parameters: model.ColorSettingCapabilityParameters{
						TemperatureK: &model.TemperatureKParameters{Min: 2202, Max: 6535},
					},
				},
			},
		},
		{
			name:         "Color temperature light: Hue ambiance lamp (LTW004)",
			rawLightInfo: `{"state":{"on":false,"bri":254,"ct":230,"alert":"none","colormode":"ct","mode":"homeautomation","reachable":true},"swupdate":{"state":"noupdates","lastinstall":"2018-12-18T18:49:53"},"type":"Color temperature light","name":"Hue ambiance lamp","modelid":"LTW004","manufacturername":"Philips","productname":"Hue ambiance lamp","capabilities":{"certified":true,"control":{"mindimlevel":1000,"maxlumen":800,"ct":{"min":153,"max":454}},"streaming":{"renderer":false,"proxy":false}},"config":{"archetype":"sultanbulb","function":"functional","direction":"omnidirectional","startup":{"mode":"powerfail","configured":true}},"uniqueid":"00:17:88:01:ff:ff:ff:ff-ff","swversion":"5.127.1.26420"}`,
			capabilities: []adapter.CapabilityInfoView{
				{
					Type:        model.OnOffCapabilityType,
					Retrievable: true,
				},
				{
					Type:        model.RangeCapabilityType,
					Retrievable: true,
					Parameters: model.RangeCapabilityParameters{
						Instance:     model.BrightnessRangeInstance,
						Unit:         model.UnitPercent,
						RandomAccess: true,
						Range: &model.Range{
							Min:       1,
							Max:       100,
							Precision: 1,
						},
					},
				},
				{
					Type:        model.ColorSettingCapabilityType,
					Retrievable: true,
					Parameters: model.ColorSettingCapabilityParameters{
						TemperatureK: &model.TemperatureKParameters{Min: 2202, Max: 6535},
					},
				},
			},
		},
		{
			name:         "Color temperature light: Hue ambiance lamp (LTW010)",
			rawLightInfo: `{"state":{"on":false,"bri":1,"ct":436,"alert":"none","colormode":"ct","mode":"homeautomation","reachable":true},"swupdate":{"state":"noupdates","lastinstall":null},"type":"Color temperature light","name":"Hue ambiance lamp","modelid":"LTW010","manufacturername":"Philips","productname":"Hue ambiance lamp","capabilities":{"certified":true,"control":{"mindimlevel":1000,"maxlumen":806,"ct":{"min":153,"max":454}},"streaming":{"renderer":false,"proxy":false}},"config":{"archetype":"sultanbulb","function":"functional","direction":"omnidirectional","startup":{"mode":"powerfail","configured":true}},"uniqueid":"00:17:88:01:ff:ff:ff:ff-ff","swversion":"1.46.13_r26312","swconfigid":"7A2E82CC","productid":"Philips-LTW010-1-A19CTv2"}`,
			capabilities: []adapter.CapabilityInfoView{
				{
					Type:        model.OnOffCapabilityType,
					Retrievable: true,
				},
				{
					Type:        model.RangeCapabilityType,
					Retrievable: true,
					Parameters: model.RangeCapabilityParameters{
						Instance:     model.BrightnessRangeInstance,
						Unit:         model.UnitPercent,
						RandomAccess: true,
						Range: &model.Range{
							Min:       1,
							Max:       100,
							Precision: 1,
						},
					},
				},
				{
					Type:        model.ColorSettingCapabilityType,
					Retrievable: true,
					Parameters: model.ColorSettingCapabilityParameters{
						TemperatureK: &model.TemperatureKParameters{Min: 2202, Max: 6535},
					},
				},
			},
		},
		{
			name:         "Color temperature light: Hue ambiance lamp (LTW015)",
			rawLightInfo: `{"state":{"on":true,"bri":254,"ct":366,"alert":"select","colormode":"ct","mode":"homeautomation","reachable":false},"swupdate":{"state":"noupdates","lastinstall":"2019-06-20T21:48:01"},"type":"Color temperature light","name":"Hue ambiance lamp","modelid":"LTW015","manufacturername":"Philips","productname":"Hue ambiance lamp","capabilities":{"certified":true,"control":{"mindimlevel":1000,"maxlumen":800,"ct":{"min":153,"max":454}},"streaming":{"renderer":false,"proxy":false}},"config":{"archetype":"sultanbulb","function":"functional","direction":"omnidirectional","startup":{"mode":"safety","configured":true}},"uniqueid":"00:17:88:01:ff:ff:ff:ff-ff","swversion":"1.46.13_r26312","swconfigid":"28823930","productid":"Philips-LTW015-1-A19CTv2"}`,
			capabilities: []adapter.CapabilityInfoView{
				{
					Type:        model.OnOffCapabilityType,
					Retrievable: true,
				},
				{
					Type:        model.RangeCapabilityType,
					Retrievable: true,
					Parameters: model.RangeCapabilityParameters{
						Instance:     model.BrightnessRangeInstance,
						Unit:         model.UnitPercent,
						RandomAccess: true,
						Range: &model.Range{
							Min:       1,
							Max:       100,
							Precision: 1,
						},
					},
				},
				{
					Type:        model.ColorSettingCapabilityType,
					Retrievable: true,
					Parameters: model.ColorSettingCapabilityParameters{
						TemperatureK: &model.TemperatureKParameters{Min: 2202, Max: 6535},
					},
				},
			},
		},
		{
			name:         "Color temperature light: Hue ambiance spot (LTW013)",
			rawLightInfo: `{"state":{"on":true,"bri":254,"ct":366,"alert":"none","colormode":"ct","mode":"homeautomation","reachable":false},"swupdate":{"state":"transferring","lastinstall":"2018-07-28T12:03:06"},"type":"Color temperature light","name":"Hue ambiance spot","modelid":"LTW013","manufacturername":"Philips","productname":"Hue ambiance spot","capabilities":{"certified":true,"control":{"mindimlevel":1000,"maxlumen":250,"ct":{"min":153,"max":454}},"streaming":{"renderer":false,"proxy":false}},"config":{"archetype":"spotbulb","function":"functional","direction":"downwards"},"uniqueid":"00:17:88:01:ff:ff:ff:ff-ff","swversion":"1.29.0_r21169","swconfigid":"797DDD7C","productid":"Philips-LTW013-1-GU10CTv1"}`,
			capabilities: []adapter.CapabilityInfoView{
				{
					Type:        model.OnOffCapabilityType,
					Retrievable: true,
				},
				{
					Type:        model.RangeCapabilityType,
					Retrievable: true,
					Parameters: model.RangeCapabilityParameters{
						Instance:     model.BrightnessRangeInstance,
						Unit:         model.UnitPercent,
						RandomAccess: true,
						Range: &model.Range{
							Min:       1,
							Max:       100,
							Precision: 1,
						},
					},
				},
				{
					Type:        model.ColorSettingCapabilityType,
					Retrievable: true,
					Parameters: model.ColorSettingCapabilityParameters{
						TemperatureK: &model.TemperatureKParameters{Min: 2202, Max: 6535},
					},
				},
			},
		},
		{
			name:         "Dimmable light: Dimmable light (Classic A60 W clear - LIGHTIFY)",
			rawLightInfo: `{"state":{"on":true,"bri":254,"alert":"none","mode":"homeautomation","reachable":true},"swupdate":{"state":"notupdatable","lastinstall":"2019-03-07T12:59:54"},"type":"Dimmable light","name":"Dimmable light","modelid":"Classic A60 W clear - LIGHTIFY","manufacturername":"OSRAM","productname":"Dimmable light","capabilities":{"certified":false,"control":{},"streaming":{"renderer":false,"proxy":false}},"config":{"archetype":"classicbulb","function":"functional","direction":"omnidirectional"},"uniqueid":"84:18:26:00:ff:ff:ff:ff-ff","swversion":"V1.05.10"}`,
			capabilities: []adapter.CapabilityInfoView{
				{
					Type:        model.OnOffCapabilityType,
					Retrievable: true,
				},
				{
					Type:        model.RangeCapabilityType,
					Retrievable: true,
					Parameters: model.RangeCapabilityParameters{
						Instance:     model.BrightnessRangeInstance,
						Unit:         model.UnitPercent,
						RandomAccess: true,
						Range: &model.Range{
							Min:       1,
							Max:       100,
							Precision: 1,
						},
					},
				},
			},
		},
		{
			name:         "Dimmable light: Dimmable light (GLEDOPTO)",
			rawLightInfo: `{"state":{"on":false,"bri":1,"alert":"select","mode":"homeautomation","reachable":true},"swupdate":{"state":"notupdatable","lastinstall":"2019-04-17T21:04:40"},"type":"Dimmable light","name":"Dimmable light","modelid":"GLEDOPTO","manufacturername":"GLEDOPTO","productname":"Dimmable light","capabilities":{"certified":false,"control":{},"streaming":{"renderer":false,"proxy":false}},"config":{"archetype":"huelightstrip","function":"functional","direction":"omnidirectional"},"uniqueid":"00:12:4b:00:ff:ff:ff:ff-ff","swversion":"1.0.2"}`,
			capabilities: []adapter.CapabilityInfoView{
				{
					Type:        model.OnOffCapabilityType,
					Retrievable: true,
				},
				{
					Type:        model.RangeCapabilityType,
					Retrievable: true,
					Parameters: model.RangeCapabilityParameters{
						Instance:     model.BrightnessRangeInstance,
						Unit:         model.UnitPercent,
						RandomAccess: true,
						Range: &model.Range{
							Min:       1,
							Max:       100,
							Precision: 1,
						},
					},
				},
			},
		},
		{
			name:         "Dimmable light: Dimmable light (LXN56-DC27LX1.1)",
			rawLightInfo: `{"state":{"on":false,"bri":1,"alert":"select","mode":"homeautomation","reachable":true},"swupdate":{"state":"notupdatable","lastinstall":"2019-08-07T18:58:22"},"type":"Dimmable light","name":"Dimmable light","modelid":"LXN56-DC27LX1.1","manufacturername":"3A Smart Home DE","productname":"Dimmable light","capabilities":{"certified":false,"control":{},"streaming":{"renderer":false,"proxy":false}},"config":{"archetype":"classicbulb","function":"functional","direction":"omnidirectional"},"uniqueid":"00:15:8d:00:ff:ff:ff:ff-ff","swversion":"1000-0001"}`,
			capabilities: []adapter.CapabilityInfoView{
				{
					Type:        model.OnOffCapabilityType,
					Retrievable: true,
				},
				{
					Type:        model.RangeCapabilityType,
					Retrievable: true,
					Parameters: model.RangeCapabilityParameters{
						Instance:     model.BrightnessRangeInstance,
						Unit:         model.UnitPercent,
						RandomAccess: true,
						Range: &model.Range{
							Min:       1,
							Max:       100,
							Precision: 1,
						},
					},
				},
			},
		},
		{
			name:         "Dimmable light: Dimmable light (TRADFRI bulb E27 W opal 1000lm)",
			rawLightInfo: `{"state":{"on":false,"bri":254,"alert":"select","mode":"homeautomation","reachable":false},"swupdate":{"state":"notupdatable","lastinstall":"2019-08-19T22:59:45"},"type":"Dimmable light","name":"Dimmable light","modelid":"TRADFRI bulb E27 W opal 1000lm","manufacturername":"IKEA of Sweden","productname":"Dimmable light","capabilities":{"certified":false,"control":{},"streaming":{"renderer":false,"proxy":false}},"config":{"archetype":"classicbulb","function":"functional","direction":"omnidirectional"},"uniqueid":"08:6b:d7:ff:ff:ff:ff:ff-ff","swversion":"1.2.214"}`,
			capabilities: []adapter.CapabilityInfoView{
				{
					Type:        model.OnOffCapabilityType,
					Retrievable: true,
				},
				{
					Type:        model.RangeCapabilityType,
					Retrievable: true,
					Parameters: model.RangeCapabilityParameters{
						Instance:     model.BrightnessRangeInstance,
						Unit:         model.UnitPercent,
						RandomAccess: true,
						Range: &model.Range{
							Min:       1,
							Max:       100,
							Precision: 1,
						},
					},
				},
			},
		},
		{
			name:         "Dimmable light: Hue white lamp (LWB004)",
			rawLightInfo: `{"state":{"on":false,"bri":127,"alert":"none","mode":"homeautomation","reachable":true},"swupdate":{"state":"noupdates","lastinstall":null},"type":"Dimmable light","name":"Hue white lamp","modelid":"LWB004","manufacturername":"Philips","productname":"Hue white lamp","capabilities":{"certified":true,"control":{"mindimlevel":2000,"maxlumen":750},"streaming":{"renderer":false,"proxy":false}},"config":{"archetype":"sultanbulb","function":"functional","direction":"omnidirectional","startup":{"mode":"powerfail","configured":true}},"uniqueid":"00:17:88:01:ff:ff:ff:ff-ff","swversion":"5.127.1.26420"}`,
			capabilities: []adapter.CapabilityInfoView{
				{
					Type:        model.OnOffCapabilityType,
					Retrievable: true,
				},
				{
					Type:        model.RangeCapabilityType,
					Retrievable: true,
					Parameters: model.RangeCapabilityParameters{
						Instance:     model.BrightnessRangeInstance,
						Unit:         model.UnitPercent,
						RandomAccess: true,
						Range: &model.Range{
							Min:       1,
							Max:       100,
							Precision: 1,
						},
					},
				},
			},
		},
		{
			name:         "Dimmable light: Hue white lamp (LWB010)",
			rawLightInfo: `{"state":{"on":true,"bri":254,"alert":"select","mode":"homeautomation","reachable":true},"swupdate":{"state":"noupdates","lastinstall":"2019-08-04T19:36:27"},"type":"Dimmable light","name":"Hue white lamp","modelid":"LWB010","manufacturername":"Philips","productname":"Hue white lamp","capabilities":{"certified":true,"control":{"mindimlevel":2000,"maxlumen":806},"streaming":{"renderer":false,"proxy":false}},"config":{"archetype":"classicbulb","function":"functional","direction":"omnidirectional","startup":{"mode":"safety","configured":true}},"uniqueid":"00:17:88:01:ff:ff:ff:ff-ff","swversion":"1.46.13_r26312","swconfigid":"322BB2EC","productid":"Philips-LWB010-1-A19DLv4"}`,
			capabilities: []adapter.CapabilityInfoView{
				{
					Type:        model.OnOffCapabilityType,
					Retrievable: true,
				},
				{
					Type:        model.RangeCapabilityType,
					Retrievable: true,
					Parameters: model.RangeCapabilityParameters{
						Instance:     model.BrightnessRangeInstance,
						Unit:         model.UnitPercent,
						RandomAccess: true,
						Range: &model.Range{
							Min:       1,
							Max:       100,
							Precision: 1,
						},
					},
				},
			},
		},
		{
			name:         "Dimmable light: Hue white lamp (LWB014)",
			rawLightInfo: `{"state":{"on":false,"bri":1,"alert":"none","mode":"homeautomation","reachable":true},"swupdate":{"state":"noupdates","lastinstall":"2018-12-07T09:51:00"},"type":"Dimmable light","name":"Hue white lamp","modelid":"LWB014","manufacturername":"Philips","productname":"Hue white lamp","capabilities":{"certified":true,"control":{"mindimlevel":2000,"maxlumen":840},"streaming":{"renderer":false,"proxy":false}},"config":{"archetype":"classicbulb","function":"functional","direction":"omnidirectional","startup":{"mode":"powerfail","configured":true}},"uniqueid":"00:17:88:01:ff:ff:ff:ff-ff","swversion":"1.46.13_r26312","swconfigid":"69806BE9","productid":"Philips-LWB014-1-A19DLv4"}`,
			capabilities: []adapter.CapabilityInfoView{
				{
					Type:        model.OnOffCapabilityType,
					Retrievable: true,
				},
				{
					Type:        model.RangeCapabilityType,
					Retrievable: true,
					Parameters: model.RangeCapabilityParameters{
						Instance:     model.BrightnessRangeInstance,
						Unit:         model.UnitPercent,
						RandomAccess: true,
						Range: &model.Range{
							Min:       1,
							Max:       100,
							Precision: 1,
						},
					},
				},
			},
		},
		{
			name:         "Dimmable plug-in unit: LivingWhites Plug (LWL001)",
			rawLightInfo: `{"state":{"on":false,"bri":0,"alert":"select","mode":"homeautomation","reachable":true},"swupdate":{"state":"notupdatable","lastinstall":null},"type":"Dimmable plug-in unit","name":"LivingWhites Plug","modelid":"LWL001","manufacturername":"Philips","productname":"LivingWhites Plug","capabilities":{"certified":true,"control":{},"streaming":{"renderer":false,"proxy":false}},"config":{"archetype":"plug","function":"functional","direction":"omnidirectional"},"uniqueid":"00:17:88:01:ff:ff:ff:ff-ff","swversion":"1.0.1.4591"}`,
			capabilities: []adapter.CapabilityInfoView{
				{
					Type:        model.OnOffCapabilityType,
					Retrievable: true,
				},
				{
					Type:        model.RangeCapabilityType,
					Retrievable: true,
					Parameters: model.RangeCapabilityParameters{
						Instance:     model.BrightnessRangeInstance,
						Unit:         model.UnitPercent,
						RandomAccess: true,
						Range: &model.Range{
							Min:       1,
							Max:       100,
							Precision: 1,
						},
					},
				},
			},
		},
		{
			// this device is strange. It has full Extended color light state, but no capabilities. And even it's name is empty. Maybe it was HUE API bug, so let's use it :)
			name:         "Extended color light: unknown (broken LCT001)",
			rawLightInfo: `{"state":{"on":false,"bri":122,"hue":0,"sat":0,"effect":"none","xy":[0,0],"ct":0,"alert":"none","colormode":"hs","reachable":false},"type":"Extended color light","name":"unknown","modelid":"LCT001","manufacturername":"Philips","uniqueid":"00:17:88:01:ff:ff:ff:ff-ff","swversion":"5.105.0.21169"}`,
			capabilities: []adapter.CapabilityInfoView{
				{
					Type:        model.OnOffCapabilityType,
					Retrievable: true,
				},
				{
					Type:        model.RangeCapabilityType,
					Retrievable: true,
					Parameters: model.RangeCapabilityParameters{
						Instance:     model.BrightnessRangeInstance,
						Unit:         model.UnitPercent,
						RandomAccess: true,
						Range: &model.Range{
							Min:       1,
							Max:       100,
							Precision: 1,
						},
					},
				},
			},
		},
		{
			name:         "Extended color light: Hue color lamp (LCT001)",
			rawLightInfo: `{"state":{"on":false,"bri":254,"hue":14893,"sat":143,"effect":"none","xy":[0.4591,0.4101],"ct":369,"alert":"none","colormode":"xy","mode":"homeautomation","reachable":true},"swupdate":{"state":"noupdates","lastinstall":"2019-01-02T22:46:13"},"type":"Extended color light","name":"Hue color lamp","modelid":"LCT001","manufacturername":"Philips","productname":"Hue color lamp","capabilities":{"certified":true,"control":{"mindimlevel":5000,"maxlumen":600,"colorgamuttype":"B","colorgamut":[[0.675,0.322],[0.409,0.518],[0.167,0.04]],"ct":{"min":153,"max":500}},"streaming":{"renderer":true,"proxy":false}},"config":{"archetype":"sultanbulb","function":"mixed","direction":"omnidirectional","startup":{"mode":"safety","configured":true}},"uniqueid":"00:17:88:01:ff:ff:ff:ff-ff","swversion":"5.127.1.26581"}`,
			capabilities: []adapter.CapabilityInfoView{
				{
					Type:        model.OnOffCapabilityType,
					Retrievable: true,
				},
				{
					Type:        model.RangeCapabilityType,
					Retrievable: true,
					Parameters: model.RangeCapabilityParameters{
						Instance:     model.BrightnessRangeInstance,
						Unit:         model.UnitPercent,
						RandomAccess: true,
						Range: &model.Range{
							Min:       1,
							Max:       100,
							Precision: 1,
						},
					},
				},
				{
					Type:        model.ColorSettingCapabilityType,
					Retrievable: true,
					Parameters: model.ColorSettingCapabilityParameters{
						ColorModel:   AOColorModelType(model.HsvModelType),
						TemperatureK: &model.TemperatureKParameters{Min: 2000, Max: 6535},
					},
				},
			},
		},
		{
			name:         "Extended color light: Dimmable light (GLEDOPTO)",
			rawLightInfo: `{"state":{"on":false,"bri":1,"hue":0,"sat":0,"effect":"none","xy":[0,0],"ct":0,"alert":"select","colormode":"hs","mode":"homeautomation","reachable":false},"swupdate":{"state":"notupdatable","lastinstall":"2019-04-17T21:04:40"},"type":"Extended color light","name":"Dimmable light","modelid":"GLEDOPTO","manufacturername":"GLEDOPTO","productname":"Dimmable light","capabilities":{"certified":false,"control":{},"streaming":{"renderer":false,"proxy":false}},"config":{"archetype":"huelightstrip","function":"functional","direction":"omnidirectional"},"uniqueid":"00:12:4b:00:ff:ff:ff:ff-ff","swversion":"1.0.2"}`,
			capabilities: []adapter.CapabilityInfoView{
				{
					Type:        model.OnOffCapabilityType,
					Retrievable: true,
				},
				{
					Type:        model.RangeCapabilityType,
					Retrievable: true,
					Parameters: model.RangeCapabilityParameters{
						Instance:     model.BrightnessRangeInstance,
						Unit:         model.UnitPercent,
						RandomAccess: true,
						Range: &model.Range{
							Min:       1,
							Max:       100,
							Precision: 1,
						},
					},
				},
			},
		},
		{
			name:         "Extended color light: Extended color light (GL-B-008Z)",
			rawLightInfo: `{"state":{"on":false,"bri":255,"hue":2304,"sat":253,"effect":"none","xy":[0.6799,0.3108],"ct":367,"alert":"select","colormode":"xy","mode":"homeautomation","reachable":true},"swupdate":{"state":"notupdatable","lastinstall":"2019-04-23T16:56:20"},"type":"Extended color light","name":"Extended color light","modelid":"GL-B-008Z","manufacturername":"GLEDOPTO","productname":"Extended color light","capabilities":{"certified":false,"control":{"colorgamuttype":"other","ct":{"min":0,"max":65535}},"streaming":{"renderer":false,"proxy":false}},"config":{"archetype":"floodbulb","function":"mixed","direction":"omnidirectional"},"uniqueid":"00:12:4b:00:ff:ff:ff:ff-ff","swversion":"2.0.0"}`,
			capabilities: []adapter.CapabilityInfoView{
				{
					Type:        model.OnOffCapabilityType,
					Retrievable: true,
				},
				{
					Type:        model.RangeCapabilityType,
					Retrievable: true,
					Parameters: model.RangeCapabilityParameters{
						Instance:     model.BrightnessRangeInstance,
						Unit:         model.UnitPercent,
						RandomAccess: true,
						Range: &model.Range{
							Min:       1,
							Max:       100,
							Precision: 1,
						},
					},
				},
				{
					Type:        model.ColorSettingCapabilityType,
					Retrievable: true,
					Parameters: model.ColorSettingCapabilityParameters{
						ColorModel:   AOColorModelType(model.HsvModelType),
						TemperatureK: &model.TemperatureKParameters{Min: 15, Max: 1000000},
					},
				},
			},
		},
		{
			name:         "Extended color light: Extended color light (GL-C-007)",
			rawLightInfo: `{"state":{"on":false,"bri":245,"hue":17920,"sat":253,"effect":"none","xy":[0.1879,0.6949],"ct":395,"alert":"select","colormode":"xy","mode":"homeautomation","reachable":false},"swupdate":{"state":"notupdatable","lastinstall":"2019-05-24T19:15:21"},"type":"Extended color light","name":"Extended color light","modelid":"GL-C-007","manufacturername":"GLEDOPTO","productname":"Extended color light","capabilities":{"certified":false,"control":{"colorgamuttype":"other","ct":{"min":0,"max":65535}},"streaming":{"renderer":false,"proxy":false}},"config":{"archetype":"classicbulb","function":"mixed","direction":"omnidirectional"},"uniqueid":"00:12:4b:00:ff:ff:ff:ff-ff","swversion":"1.0.7"}`,
			capabilities: []adapter.CapabilityInfoView{
				{
					Type:        model.OnOffCapabilityType,
					Retrievable: true,
				},
				{
					Type:        model.RangeCapabilityType,
					Retrievable: true,
					Parameters: model.RangeCapabilityParameters{
						Instance:     model.BrightnessRangeInstance,
						Unit:         model.UnitPercent,
						RandomAccess: true,
						Range: &model.Range{
							Min:       1,
							Max:       100,
							Precision: 1,
						},
					},
				},
				{
					Type:        model.ColorSettingCapabilityType,
					Retrievable: true,
					Parameters: model.ColorSettingCapabilityParameters{
						ColorModel:   AOColorModelType(model.HsvModelType),
						TemperatureK: &model.TemperatureKParameters{Min: 15, Max: 1000000},
					},
				},
			},
		},
		{
			name:         "Extended color light: Extended color light (GL-FL-004TZ)",
			rawLightInfo: `{"state":{"on":false,"bri":254,"hue":35072,"sat":32,"effect":"none","xy":[0.2979,0.3201],"ct":159,"alert":"none","colormode":"ct","mode":"homeautomation","reachable":false},"swupdate":{"state":"notupdatable","lastinstall":"2019-05-24T19:09:05"},"type":"Extended color light","name":"Extended color light","modelid":"GL-FL-004TZ","manufacturername":"GLEDOPTO","productname":"Extended color light","capabilities":{"certified":false,"control":{"colorgamuttype":"other","ct":{"min":0,"max":65535}},"streaming":{"renderer":false,"proxy":false}},"config":{"archetype":"classicbulb","function":"mixed","direction":"omnidirectional"},"uniqueid":"00:12:4b:00:ff:ff:ff:ff-ff","swversion":"2.0.0"}`,
			capabilities: []adapter.CapabilityInfoView{
				{
					Type:        model.OnOffCapabilityType,
					Retrievable: true,
				},
				{
					Type:        model.RangeCapabilityType,
					Retrievable: true,
					Parameters: model.RangeCapabilityParameters{
						Instance:     model.BrightnessRangeInstance,
						Unit:         model.UnitPercent,
						RandomAccess: true,
						Range: &model.Range{
							Min:       1,
							Max:       100,
							Precision: 1,
						},
					},
				},
				{
					Type:        model.ColorSettingCapabilityType,
					Retrievable: true,
					Parameters: model.ColorSettingCapabilityParameters{
						ColorModel:   AOColorModelType(model.HsvModelType),
						TemperatureK: &model.TemperatureKParameters{Min: 15, Max: 1000000},
					},
				},
			},
		},
		{
			name:         "Extended color light: Extended color light (GLEDOPTO)",
			rawLightInfo: `{"state":{"on":false,"bri":253,"hue":3584,"sat":9,"effect":"none","xy":[0.642,0.3521],"ct":293,"alert":"select","colormode":"xy","mode":"homeautomation","reachable":false},"swupdate":{"state":"notupdatable","lastinstall":"2019-04-17T21:04:40"},"type":"Extended color light","name":"Extended color light","modelid":"GLEDOPTO","manufacturername":"GLEDOPTO","productname":"Extended color light","capabilities":{"certified":false,"control":{"colorgamuttype":"other","ct":{"min":0,"max":65535}},"streaming":{"renderer":false,"proxy":false}},"config":{"archetype":"huelightstrip","function":"mixed","direction":"omnidirectional"},"uniqueid":"00:12:4b:00:ff:ff:ff:ff-ff","swversion":"1.0.2"}`,
			capabilities: []adapter.CapabilityInfoView{
				{
					Type:        model.OnOffCapabilityType,
					Retrievable: true,
				},
				{
					Type:        model.RangeCapabilityType,
					Retrievable: true,
					Parameters: model.RangeCapabilityParameters{
						Instance:     model.BrightnessRangeInstance,
						Unit:         model.UnitPercent,
						RandomAccess: true,
						Range: &model.Range{
							Min:       1,
							Max:       100,
							Precision: 1,
						},
					},
				},
				{
					Type:        model.ColorSettingCapabilityType,
					Retrievable: true,
					Parameters: model.ColorSettingCapabilityParameters{
						ColorModel:   AOColorModelType(model.HsvModelType),
						TemperatureK: &model.TemperatureKParameters{Min: 15, Max: 1000000},
					},
				},
			},
		},
		{
			name:         "Extended color light: Hue beyond down (LLM001)",
			rawLightInfo: `{"state":{"on":true,"bri":144,"hue":13524,"sat":200,"effect":"none","xy":[0.5017,0.4152],"ct":443,"alert":"select","colormode":"xy","mode":"homeautomation","reachable":true},"swupdate":{"state":"readytoinstall","lastinstall":"2019-06-15T11:32:03"},"type":"Extended color light","name":"Hue beyond down","modelid":"LLM001","manufacturername":"Philips","productname":"Hue beyond down","capabilities":{"certified":true,"control":{"mindimlevel":5000,"maxlumen":300,"colorgamuttype":"B","colorgamut":[[0.675,0.322],[0.409,0.518],[0.167,0.04]],"ct":{"min":153,"max":500}},"streaming":{"renderer":false,"proxy":false}},"config":{"archetype":"pendantround","function":"decorative","direction":"downwards"},"uniqueid":"00:17:88:01:ff:ff:ff:ff-ff","luminaireuniqueid":"00:c3:42:2a-02-01","swversion":"5.17.1.12040"}`,
			capabilities: []adapter.CapabilityInfoView{
				{
					Type:        model.OnOffCapabilityType,
					Retrievable: true,
				},
				{
					Type:        model.RangeCapabilityType,
					Retrievable: true,
					Parameters: model.RangeCapabilityParameters{
						Instance:     model.BrightnessRangeInstance,
						Unit:         model.UnitPercent,
						RandomAccess: true,
						Range: &model.Range{
							Min:       1,
							Max:       100,
							Precision: 1,
						},
					},
				},
				{
					Type:        model.ColorSettingCapabilityType,
					Retrievable: true,
					Parameters: model.ColorSettingCapabilityParameters{
						ColorModel:   AOColorModelType(model.HsvModelType),
						TemperatureK: &model.TemperatureKParameters{Min: 2000, Max: 6535},
					},
				},
			},
		},
		{
			name:         "Extended color light: Hue beyond up (LLM001)",
			rawLightInfo: `{"state":{"on":true,"bri":144,"hue":13524,"sat":200,"effect":"none","xy":[0.5017,0.4152],"ct":443,"alert":"select","colormode":"xy","mode":"homeautomation","reachable":true},"swupdate":{"state":"readytoinstall","lastinstall":"2019-06-15T11:32:02"},"type":"Extended color light","name":"Hue beyond up","modelid":"LLM001","manufacturername":"Philips","productname":"Hue beyond up","capabilities":{"certified":true,"control":{"mindimlevel":5000,"maxlumen":300,"colorgamuttype":"B","colorgamut":[[0.675,0.322],[0.409,0.518],[0.167,0.04]],"ct":{"min":153,"max":500}},"streaming":{"renderer":false,"proxy":false}},"config":{"archetype":"pendantround","function":"decorative","direction":"upwards"},"uniqueid":"00:17:88:01:ff:ff:ff:ff-ff","luminaireuniqueid":"00:c3:42:2a-01-00","swversion":"5.17.1.12040"}`,
			capabilities: []adapter.CapabilityInfoView{
				{
					Type:        model.OnOffCapabilityType,
					Retrievable: true,
				},
				{
					Type:        model.RangeCapabilityType,
					Retrievable: true,
					Parameters: model.RangeCapabilityParameters{
						Instance:     model.BrightnessRangeInstance,
						Unit:         model.UnitPercent,
						RandomAccess: true,
						Range: &model.Range{
							Min:       1,
							Max:       100,
							Precision: 1,
						},
					},
				},
				{
					Type:        model.ColorSettingCapabilityType,
					Retrievable: true,
					Parameters: model.ColorSettingCapabilityParameters{
						ColorModel:   AOColorModelType(model.HsvModelType),
						TemperatureK: &model.TemperatureKParameters{Min: 2000, Max: 6535},
					},
				},
			},
		},
		{
			name:         "Extended color light: Hue color candle (LCT012)",
			rawLightInfo: `{"state":{"on":true,"bri":254,"hue":8418,"sat":140,"effect":"none","xy":[0.4573,0.41],"ct":366,"alert":"select","colormode":"ct","mode":"homeautomation","reachable":true},"swupdate":{"state":"noupdates","lastinstall":"2018-12-30T12:33:40"},"type":"Extended color light","name":"Hue color candle","modelid":"LCT012","manufacturername":"Philips","productname":"Hue color candle","capabilities":{"certified":true,"control":{"mindimlevel":2000,"maxlumen":450,"colorgamuttype":"C","colorgamut":[[0.6915,0.3083],[0.17,0.7],[0.1532,0.0475]],"ct":{"min":153,"max":500}},"streaming":{"renderer":true,"proxy":true}},"config":{"archetype":"candlebulb","function":"mixed","direction":"omnidirectional","startup":{"mode":"safety","configured":true}},"uniqueid":"00:17:88:01:ff:ff:ff:ff-ff","swversion":"1.46.13_r26312","swconfigid":"8C67986F","productid":"Philips-LCT012-1-E14ECLv1"}`,
			capabilities: []adapter.CapabilityInfoView{
				{
					Type:        model.OnOffCapabilityType,
					Retrievable: true,
				},
				{
					Type:        model.RangeCapabilityType,
					Retrievable: true,
					Parameters: model.RangeCapabilityParameters{
						Instance:     model.BrightnessRangeInstance,
						Unit:         model.UnitPercent,
						RandomAccess: true,
						Range: &model.Range{
							Min:       1,
							Max:       100,
							Precision: 1,
						},
					},
				},
				{
					Type:        model.ColorSettingCapabilityType,
					Retrievable: true,
					Parameters: model.ColorSettingCapabilityParameters{
						ColorModel:   AOColorModelType(model.HsvModelType),
						TemperatureK: &model.TemperatureKParameters{Min: 2000, Max: 6535},
					},
				},
			},
		},
		{
			name:         "Extended color light: Hue color downlight (LCB001)",
			rawLightInfo: `{"state":{"on":false,"bri":137,"hue":8418,"sat":140,"effect":"none","xy":[0.7006,0.2993],"ct":454,"alert":"none","colormode":"xy","mode":"homeautomation","reachable":false},"swupdate":{"state":"noupdates","lastinstall":"2019-04-25T15:12:57"},"type":"Extended color light","name":"Hue color downlight","modelid":"LCB001","manufacturername":"Philips","productname":"Hue color downlight","capabilities":{"certified":true,"control":{"mindimlevel":200,"maxlumen":650,"colorgamuttype":"C","colorgamut":[[0.6915,0.3083],[0.17,0.7],[0.1532,0.0475]],"ct":{"min":153,"max":500}},"streaming":{"renderer":true,"proxy":true}},"config":{"archetype":"floodbulb","function":"mixed","direction":"downwards","startup":{"mode":"safety","configured":true}},"uniqueid":"00:17:88:01:ff:ff:ff:ff-ff","swversion":"1.53.3_r27175","swconfigid":"E4A52056","productid":"Philips-LCB001-1-BR30ECLv4"}`,
			capabilities: []adapter.CapabilityInfoView{
				{
					Type:        model.OnOffCapabilityType,
					Retrievable: true,
				},
				{
					Type:        model.RangeCapabilityType,
					Retrievable: true,
					Parameters: model.RangeCapabilityParameters{
						Instance:     model.BrightnessRangeInstance,
						Unit:         model.UnitPercent,
						RandomAccess: true,
						Range: &model.Range{
							Min:       1,
							Max:       100,
							Precision: 1,
						},
					},
				},
				{
					Type:        model.ColorSettingCapabilityType,
					Retrievable: true,
					Parameters: model.ColorSettingCapabilityParameters{
						ColorModel:   AOColorModelType(model.HsvModelType),
						TemperatureK: &model.TemperatureKParameters{Min: 2000, Max: 6535},
					},
				},
			},
		},
		{
			name:         "Extended color light: Hue color downlight (LCT002)",
			rawLightInfo: `{"state":{"on":true,"bri":254,"hue":14988,"sat":141,"effect":"none","xy":[0.4575,0.4101],"ct":366,"alert":"select","colormode":"ct","mode":"homeautomation","reachable":true},"swupdate":{"state":"noupdates","lastinstall":"2018-12-13T20:09:40"},"type":"Extended color light","name":"Hue color downlight","modelid":"LCT002","manufacturername":"Philips","productname":"Hue color downlight","capabilities":{"certified":true,"control":{"mindimlevel":5000,"maxlumen":630,"colorgamuttype":"B","colorgamut":[[0.675,0.322],[0.409,0.518],[0.167,0.04]],"ct":{"min":153,"max":500}},"streaming":{"renderer":true,"proxy":false}},"config":{"archetype":"floodbulb","function":"mixed","direction":"downwards","startup":{"mode":"safety","configured":true}},"uniqueid":"00:17:88:01:ff:ff:ff:ff-ff","swversion":"5.127.1.26581"}`,
			capabilities: []adapter.CapabilityInfoView{
				{
					Type:        model.OnOffCapabilityType,
					Retrievable: true,
				},
				{
					Type:        model.RangeCapabilityType,
					Retrievable: true,
					Parameters: model.RangeCapabilityParameters{
						Instance:     model.BrightnessRangeInstance,
						Unit:         model.UnitPercent,
						RandomAccess: true,
						Range: &model.Range{
							Min:       1,
							Max:       100,
							Precision: 1,
						},
					},
				},
				{
					Type:        model.ColorSettingCapabilityType,
					Retrievable: true,
					Parameters: model.ColorSettingCapabilityParameters{
						ColorModel:   AOColorModelType(model.HsvModelType),
						TemperatureK: &model.TemperatureKParameters{Min: 2000, Max: 6535},
					},
				},
			},
		},
		{
			name:         "Extended color light: Hue color lamp (LCA001)",
			rawLightInfo: `{"state":{"on":false,"bri":137,"hue":8418,"sat":140,"effect":"none","xy":[0.7006,0.2993],"ct":366,"alert":"select","colormode":"xy","mode":"homeautomation","reachable":false},"swupdate":{"state":"noupdates","lastinstall":"2019-06-27T10:08:08"},"type":"Extended color light","name":"Hue color lamp","modelid":"LCA001","manufacturername":"Philips","productname":"Hue color lamp","capabilities":{"certified":true,"control":{"mindimlevel":200,"maxlumen":800,"colorgamuttype":"C","colorgamut":[[0.6915,0.3083],[0.17,0.7],[0.1532,0.0475]],"ct":{"min":153,"max":500}},"streaming":{"renderer":true,"proxy":true}},"config":{"archetype":"sultanbulb","function":"mixed","direction":"omnidirectional","startup":{"mode":"safety","configured":true}},"uniqueid":"00:17:88:01:ff:ff:ff:ff-ff","swversion":"1.53.3_r27175","swconfigid":"297C5CDD","productid":"Philips-LCA001-5-A19ECLv6"}`,
			capabilities: []adapter.CapabilityInfoView{
				{
					Type:        model.OnOffCapabilityType,
					Retrievable: true,
				},
				{
					Type:        model.RangeCapabilityType,
					Retrievable: true,
					Parameters: model.RangeCapabilityParameters{
						Instance:     model.BrightnessRangeInstance,
						Unit:         model.UnitPercent,
						RandomAccess: true,
						Range: &model.Range{
							Min:       1,
							Max:       100,
							Precision: 1,
						},
					},
				},
				{
					Type:        model.ColorSettingCapabilityType,
					Retrievable: true,
					Parameters: model.ColorSettingCapabilityParameters{
						ColorModel:   AOColorModelType(model.HsvModelType),
						TemperatureK: &model.TemperatureKParameters{Min: 2000, Max: 6535},
					},
				},
			},
		},
		{
			name:         "Extended color light: Hue color lamp (LCT007)",
			rawLightInfo: `{"state":{"on":true,"bri":254,"hue":14988,"sat":141,"effect":"none","xy":[0.4575,0.4101],"ct":366,"alert":"lselect","colormode":"ct","mode":"homeautomation","reachable":false},"swupdate":{"state":"noupdates","lastinstall":"2019-01-12T12:31:48"},"type":"Extended color light","name":"Hue color lamp","modelid":"LCT007","manufacturername":"Philips","productname":"Hue color lamp","capabilities":{"certified":true,"control":{"mindimlevel":2000,"maxlumen":800,"colorgamuttype":"B","colorgamut":[[0.675,0.322],[0.409,0.518],[0.167,0.04]],"ct":{"min":153,"max":500}},"streaming":{"renderer":true,"proxy":true}},"config":{"archetype":"sultanbulb","function":"mixed","direction":"omnidirectional","startup":{"mode":"safety","configured":true}},"uniqueid":"00:17:88:01:ff:ff:ff:ff-ff","swversion":"5.127.1.26420"}`,
			capabilities: []adapter.CapabilityInfoView{
				{
					Type:        model.OnOffCapabilityType,
					Retrievable: true,
				},
				{
					Type:        model.RangeCapabilityType,
					Retrievable: true,
					Parameters: model.RangeCapabilityParameters{
						Instance:     model.BrightnessRangeInstance,
						Unit:         model.UnitPercent,
						RandomAccess: true,
						Range: &model.Range{
							Min:       1,
							Max:       100,
							Precision: 1,
						},
					},
				},
				{
					Type:        model.ColorSettingCapabilityType,
					Retrievable: true,
					Parameters: model.ColorSettingCapabilityParameters{
						ColorModel:   AOColorModelType(model.HsvModelType),
						TemperatureK: &model.TemperatureKParameters{Min: 2000, Max: 6535},
					},
				},
			},
		},
		{
			name:         "Extended color light: Hue color lamp (LCT010)",
			rawLightInfo: `{"state":{"on":false,"bri":254,"hue":8402,"sat":140,"effect":"none","xy":[0.4575,0.4099],"ct":366,"alert":"none","colormode":"xy","mode":"homeautomation","reachable":true},"swupdate":{"state":"noupdates","lastinstall":"2018-12-09T12:55:12"},"type":"Extended color light","name":"Hue color lamp","modelid":"LCT010","manufacturername":"Philips","productname":"Hue color lamp","capabilities":{"certified":true,"control":{"mindimlevel":1000,"maxlumen":806,"colorgamuttype":"C","colorgamut":[[0.6915,0.3083],[0.17,0.7],[0.1532,0.0475]],"ct":{"min":153,"max":500}},"streaming":{"renderer":true,"proxy":true}},"config":{"archetype":"sultanbulb","function":"mixed","direction":"omnidirectional","startup":{"mode":"powerfail","configured":true}},"uniqueid":"00:17:88:01:ff:ff:ff:ff-ff","swversion":"1.46.13_r26312","swconfigid":"0CE67A8F","productid":"Philips-LCT010-1-A19ECLv4"}`,
			capabilities: []adapter.CapabilityInfoView{
				{
					Type:        model.OnOffCapabilityType,
					Retrievable: true,
				},
				{
					Type:        model.RangeCapabilityType,
					Retrievable: true,
					Parameters: model.RangeCapabilityParameters{
						Instance:     model.BrightnessRangeInstance,
						Unit:         model.UnitPercent,
						RandomAccess: true,
						Range: &model.Range{
							Min:       1,
							Max:       100,
							Precision: 1,
						},
					},
				},
				{
					Type:        model.ColorSettingCapabilityType,
					Retrievable: true,
					Parameters: model.ColorSettingCapabilityParameters{
						ColorModel:   AOColorModelType(model.HsvModelType),
						TemperatureK: &model.TemperatureKParameters{Min: 2000, Max: 6535},
					},
				},
			},
		},
		{
			name:         "Extended color light: Hue color lamp (LCT015)",
			rawLightInfo: `{"state":{"on":false,"bri":144,"hue":7676,"sat":199,"effect":"none","xy":[0.5016,0.4151],"ct":443,"alert":"select","colormode":"xy","mode":"homeautomation","reachable":true},"swupdate":{"state":"noupdates","lastinstall":"2018-12-05T13:56:33"},"type":"Extended color light","name":"Hue color lamp","modelid":"LCT015","manufacturername":"Philips","productname":"Hue color lamp","capabilities":{"certified":true,"control":{"mindimlevel":1000,"maxlumen":806,"colorgamuttype":"C","colorgamut":[[0.6915,0.3083],[0.17,0.7],[0.1532,0.0475]],"ct":{"min":153,"max":500}},"streaming":{"renderer":true,"proxy":true}},"config":{"archetype":"sultanbulb","function":"mixed","direction":"omnidirectional","startup":{"mode":"safety","configured":true}},"uniqueid":"00:17:88:01:ff:ff:ff:ff-ff","swversion":"1.46.13_r26312","swconfigid":"52E3234B","productid":"Philips-LCT015-1-A19ECLv5"}`,
			capabilities: []adapter.CapabilityInfoView{
				{
					Type:        model.OnOffCapabilityType,
					Retrievable: true,
				},
				{
					Type:        model.RangeCapabilityType,
					Retrievable: true,
					Parameters: model.RangeCapabilityParameters{
						Instance:     model.BrightnessRangeInstance,
						Unit:         model.UnitPercent,
						RandomAccess: true,
						Range: &model.Range{
							Min:       1,
							Max:       100,
							Precision: 1,
						},
					},
				},
				{
					Type:        model.ColorSettingCapabilityType,
					Retrievable: true,
					Parameters: model.ColorSettingCapabilityParameters{
						ColorModel:   AOColorModelType(model.HsvModelType),
						TemperatureK: &model.TemperatureKParameters{Min: 2000, Max: 6535},
					},
				},
			},
		},
		{
			name:         "Extended color light: Hue color lamp (LCT016)",
			rawLightInfo: `{"state":{"on":true,"bri":254,"hue":8418,"sat":140,"effect":"none","xy":[0.4573,0.41],"ct":366,"alert":"select","colormode":"ct","mode":"homeautomation","reachable":true},"swupdate":{"state":"noupdates","lastinstall":"2019-04-22T11:42:14"},"type":"Extended color light","name":"Hue color lamp","modelid":"LCT016","manufacturername":"Philips","productname":"Hue color lamp","capabilities":{"certified":true,"control":{"mindimlevel":1000,"maxlumen":800,"colorgamuttype":"C","colorgamut":[[0.6915,0.3083],[0.17,0.7],[0.1532,0.0475]],"ct":{"min":153,"max":500}},"streaming":{"renderer":true,"proxy":true}},"config":{"archetype":"sultanbulb","function":"mixed","direction":"omnidirectional","startup":{"mode":"safety","configured":true}},"uniqueid":"00:17:88:01:ff:ff:ff:ff-ff","swversion":"1.46.13_r26312","swconfigid":"9DC82D22","productid":"Philips-LCT016-1-A19ECLv5"}`,
			capabilities: []adapter.CapabilityInfoView{
				{
					Type:        model.OnOffCapabilityType,
					Retrievable: true,
				},
				{
					Type:        model.RangeCapabilityType,
					Retrievable: true,
					Parameters: model.RangeCapabilityParameters{
						Instance:     model.BrightnessRangeInstance,
						Unit:         model.UnitPercent,
						RandomAccess: true,
						Range: &model.Range{
							Min:       1,
							Max:       100,
							Precision: 1,
						},
					},
				},
				{
					Type:        model.ColorSettingCapabilityType,
					Retrievable: true,
					Parameters: model.ColorSettingCapabilityParameters{
						ColorModel:   AOColorModelType(model.HsvModelType),
						TemperatureK: &model.TemperatureKParameters{Min: 2000, Max: 6535},
					},
				},
			},
		},
		{
			name:         "Extended color light: Hue color module (LLM001)",
			rawLightInfo: `{"state":{"on":false,"bri":137,"hue":0,"sat":0,"effect":"none","xy":[0.7006,0.2993],"ct":447,"alert":"none","colormode":"xy","mode":"homeautomation","reachable":false},"swupdate":{"state":"transferring","lastinstall":"2018-02-01T10:15:58"},"type":"Extended color light","name":"Hue color module","modelid":"LLM001","manufacturername":"Philips","productname":"Hue color module","capabilities":{"certified":true,"control":{"mindimlevel":5000,"maxlumen":300,"colorgamuttype":"B","colorgamut":[[0.675,0.322],[0.409,0.518],[0.167,0.04]],"ct":{"min":153,"max":500}},"streaming":{"renderer":true,"proxy":true}},"config":{"archetype":"tableshade","function":"decorative","direction":"downwards"},"uniqueid":"00:17:88:01:ff:ff:ff:ff-ff","luminaireuniqueid":"00:e3:c3:c8-02-00","swversion":"5.105.0.21536"}`,
			capabilities: []adapter.CapabilityInfoView{
				{
					Type:        model.OnOffCapabilityType,
					Retrievable: true,
				},
				{
					Type:        model.RangeCapabilityType,
					Retrievable: true,
					Parameters: model.RangeCapabilityParameters{
						Instance:     model.BrightnessRangeInstance,
						Unit:         model.UnitPercent,
						RandomAccess: true,
						Range: &model.Range{
							Min:       1,
							Max:       100,
							Precision: 1,
						},
					},
				},
				{
					Type:        model.ColorSettingCapabilityType,
					Retrievable: true,
					Parameters: model.ColorSettingCapabilityParameters{
						ColorModel:   AOColorModelType(model.HsvModelType),
						TemperatureK: &model.TemperatureKParameters{Min: 2000, Max: 6535},
					},
				},
			},
		},
		{
			name:         "Extended color light: Hue color spot (LCT003)",
			rawLightInfo: `{"state":{"on":false,"bri":254,"hue":49128,"sat":244,"effect":"none","xy":[0.2289,0.083],"ct":153,"alert":"none","colormode":"xy","mode":"homeautomation","reachable":true},"swupdate":{"state":"noupdates","lastinstall":"2019-04-06T21:05:18"},"type":"Extended color light","name":"Hue color spot","modelid":"LCT003","manufacturername":"Philips","productname":"Hue color spot","capabilities":{"certified":true,"control":{"mindimlevel":5000,"maxlumen":250,"colorgamuttype":"B","colorgamut":[[0.675,0.322],[0.409,0.518],[0.167,0.04]],"ct":{"min":153,"max":500}},"streaming":{"renderer":true,"proxy":true}},"config":{"archetype":"spotbulb","function":"mixed","direction":"downwards","startup":{"mode":"safety","configured":true}},"uniqueid":"00:17:88:01:ff:ff:ff:ff-ff","swversion":"5.127.1.26420"}`,
			capabilities: []adapter.CapabilityInfoView{
				{
					Type:        model.OnOffCapabilityType,
					Retrievable: true,
				},
				{
					Type:        model.RangeCapabilityType,
					Retrievable: true,
					Parameters: model.RangeCapabilityParameters{
						Instance:     model.BrightnessRangeInstance,
						Unit:         model.UnitPercent,
						RandomAccess: true,
						Range: &model.Range{
							Min:       1,
							Max:       100,
							Precision: 1,
						},
					},
				},
				{
					Type:        model.ColorSettingCapabilityType,
					Retrievable: true,
					Parameters: model.ColorSettingCapabilityParameters{
						ColorModel:   AOColorModelType(model.HsvModelType),
						TemperatureK: &model.TemperatureKParameters{Min: 2000, Max: 6535},
					},
				},
			},
		},
		{
			name:         "Extended color light: Hue go (LLC020)",
			rawLightInfo: `{"state":{"on":false,"bri":144,"hue":7676,"sat":199,"effect":"none","xy":[0.5016,0.4151],"ct":443,"alert":"none","colormode":"xy","mode":"homeautomation","reachable":true},"swupdate":{"state":"noupdates","lastinstall":"2018-12-12T13:39:18"},"type":"Extended color light","name":"Hue go","modelid":"LLC020","manufacturername":"Philips","productname":"Hue go","capabilities":{"certified":true,"control":{"mindimlevel":40,"maxlumen":300,"colorgamuttype":"C","colorgamut":[[0.6915,0.3083],[0.17,0.7],[0.1532,0.0475]],"ct":{"min":153,"max":500}},"streaming":{"renderer":true,"proxy":true}},"config":{"archetype":"huego","function":"decorative","direction":"omnidirectional","startup":{"mode":"safety","configured":true}},"uniqueid":"00:17:88:01:ff:ff:ff:ff-ff","swversion":"5.127.1.26420"}`,
			capabilities: []adapter.CapabilityInfoView{
				{
					Type:        model.OnOffCapabilityType,
					Retrievable: true,
				},
				{
					Type:        model.RangeCapabilityType,
					Retrievable: true,
					Parameters: model.RangeCapabilityParameters{
						Instance:     model.BrightnessRangeInstance,
						Unit:         model.UnitPercent,
						RandomAccess: true,
						Range: &model.Range{
							Min:       1,
							Max:       100,
							Precision: 1,
						},
					},
				},
				{
					Type:        model.ColorSettingCapabilityType,
					Retrievable: true,
					Parameters: model.ColorSettingCapabilityParameters{
						ColorModel:   AOColorModelType(model.HsvModelType),
						TemperatureK: &model.TemperatureKParameters{Min: 2000, Max: 6535},
					},
				},
			},
		},
		{
			name:         "Extended color light: Hue lightstrip plus (LST002)",
			rawLightInfo: `{"state":{"on":false,"bri":144,"hue":7676,"sat":199,"effect":"none","xy":[0.5016,0.4151],"ct":443,"alert":"select","colormode":"xy","mode":"homeautomation","reachable":true},"swupdate":{"state":"noupdates","lastinstall":"2018-12-12T13:39:21"},"type":"Extended color light","name":"Hue lightstrip plus","modelid":"LST002","manufacturername":"Philips","productname":"Hue lightstrip plus","capabilities":{"certified":true,"control":{"mindimlevel":25,"maxlumen":1600,"colorgamuttype":"C","colorgamut":[[0.6915,0.3083],[0.17,0.7],[0.1532,0.0475]],"ct":{"min":153,"max":500}},"streaming":{"renderer":true,"proxy":true}},"config":{"archetype":"huelightstrip","function":"mixed","direction":"omnidirectional","startup":{"mode":"safety","configured":true}},"uniqueid":"00:17:88:01:ff:ff:ff:ff-ff","swversion":"5.127.1.26420"}`,
			capabilities: []adapter.CapabilityInfoView{
				{
					Type:        model.OnOffCapabilityType,
					Retrievable: true,
				},
				{
					Type:        model.RangeCapabilityType,
					Retrievable: true,
					Parameters: model.RangeCapabilityParameters{
						Instance:     model.BrightnessRangeInstance,
						Unit:         model.UnitPercent,
						RandomAccess: true,
						Range: &model.Range{
							Min:       1,
							Max:       100,
							Precision: 1,
						},
					},
				},
				{
					Type:        model.ColorSettingCapabilityType,
					Retrievable: true,
					Parameters: model.ColorSettingCapabilityParameters{
						ColorModel:   AOColorModelType(model.HsvModelType),
						TemperatureK: &model.TemperatureKParameters{Min: 2000, Max: 6535},
					},
				},
			},
		},
		{
			name:         "Extended color light: Hue play (LCT024)",
			rawLightInfo: `{"state":{"on":false,"bri":224,"hue":45774,"sat":254,"effect":"none","xy":[0.1542,0.0879],"ct":153,"alert":"none","colormode":"xy","mode":"homeautomation","reachable":true},"swupdate":{"state":"noupdates","lastinstall":"2018-12-20T20:05:04"},"type":"Extended color light","name":"Hue play","modelid":"LCT024","manufacturername":"Philips","productname":"Hue play","capabilities":{"certified":true,"control":{"mindimlevel":500,"maxlumen":540,"colorgamuttype":"C","colorgamut":[[0.6915,0.3083],[0.17,0.7],[0.1532,0.0475]],"ct":{"min":153,"max":500}},"streaming":{"renderer":true,"proxy":true}},"config":{"archetype":"hueplay","function":"decorative","direction":"upwards","startup":{"mode":"powerfail","configured":true}},"uniqueid":"00:17:88:01:ff:ff:ff:ff-ff","swversion":"1.46.13_r26312","swconfigid":"869E2FE2","productid":"3241-3127-7871-LS00"}`,
			capabilities: []adapter.CapabilityInfoView{
				{
					Type:        model.OnOffCapabilityType,
					Retrievable: true,
				},
				{
					Type:        model.RangeCapabilityType,
					Retrievable: true,
					Parameters: model.RangeCapabilityParameters{
						Instance:     model.BrightnessRangeInstance,
						Unit:         model.UnitPercent,
						RandomAccess: true,
						Range: &model.Range{
							Min:       1,
							Max:       100,
							Precision: 1,
						},
					},
				},
				{
					Type:        model.ColorSettingCapabilityType,
					Retrievable: true,
					Parameters: model.ColorSettingCapabilityParameters{
						ColorModel:   AOColorModelType(model.HsvModelType),
						TemperatureK: &model.TemperatureKParameters{Min: 2000, Max: 6535},
					},
				},
			},
		},
		{
			name:         "On/Off plug-in unit: Extended color light (GLEDOPTO)",
			rawLightInfo: `{"state":{"on":false,"alert":"select","mode":"homeautomation","reachable":false},"swupdate":{"state":"notupdatable","lastinstall":null},"type":"On/Off plug-in unit","name":"Extended color light","modelid":"GLEDOPTO","manufacturername":"GLEDOPTO","productname":"Extended color light","capabilities":{"certified":false,"control":{"colorgamuttype":"other","ct":{"min":0,"max":65535}},"streaming":{"renderer":false,"proxy":false}},"config":{"archetype":"huelightstrip","function":"mixed","direction":"omnidirectional"},"uniqueid":"00:12:4b:00:ff:ff:ff:ff-ff","swversion":"1.0.2"}`,
			capabilities: []adapter.CapabilityInfoView{
				{
					Type:        model.OnOffCapabilityType,
					Retrievable: true,
				},
				{
					Type:        model.ColorSettingCapabilityType,
					Retrievable: false,
					Parameters: model.ColorSettingCapabilityParameters{
						ColorModel:   AOColorModelType(model.HsvModelType),
						TemperatureK: &model.TemperatureKParameters{Min: 15, Max: 1000000},
					},
				},
			},
		},
		{
			name:         "On/Off plug-in unit: Hue Smart plug (LOM001)",
			rawLightInfo: `{"state":{"on":false,"alert":"none","mode":"homeautomation","reachable":true},"swupdate":{"state":"noupdates","lastinstall":"2019-08-15T12:15:29"},"type":"On/Off plug-in unit","name":"Hue Smart plug","modelid":"LOM001","manufacturername":"Philips","productname":"Hue Smart plug","capabilities":{"certified":true,"control":{"mindimlevel":5000},"streaming":{"renderer":false,"proxy":false}},"config":{"archetype":"plug","function":"functional","direction":"omnidirectional","startup":{"mode":"safety","configured":true}},"uniqueid":"00:17:88:01:ff:ff:ff:ff-ff","swversion":"1.55.7_r28462","swconfigid":"68918527","productid":"SmartPlug_OnOff_v01-00_01"}`,
			capabilities: []adapter.CapabilityInfoView{
				{
					Type:        model.OnOffCapabilityType,
					Retrievable: true,
				},
				{
					Type:        model.RangeCapabilityType,
					Retrievable: false,
					Parameters: model.RangeCapabilityParameters{
						Instance:     model.BrightnessRangeInstance,
						Unit:         model.UnitPercent,
						RandomAccess: true,
						Range: &model.Range{
							Min:       1,
							Max:       100,
							Precision: 1,
						},
					},
				},
			},
		},
		{
			name:         "On/Off plug-in unit: On/Off plug (Plug 01)",
			rawLightInfo: `{"state":{"on":false,"alert":"select","mode":"homeautomation","reachable":true},"swupdate":{"state":"notupdatable","lastinstall":null},"type":"On/Off plug-in unit","name":"On/Off plug","modelid":"Plug 01","manufacturername":"OSRAM","productname":"On/Off plug","capabilities":{"certified":false,"control":{},"streaming":{"renderer":false,"proxy":false}},"config":{"archetype":"recessedfloor","function":"functional","direction":"omnidirectional"},"uniqueid":"7c:b0:3e:aa:ff:ff:ff:ff-ff","swversion":"V1.04.12"}`,
			capabilities: []adapter.CapabilityInfoView{
				{
					Type:        model.OnOffCapabilityType,
					Retrievable: true,
				},
			},
		},
		{
			name:         "On/Off plug-in unit: On/Off plug (SP 120)",
			rawLightInfo: `{"state":{"on":false,"alert":"select","mode":"homeautomation","reachable":true},"swupdate":{"state":"notupdatable","lastinstall":"2019-03-29T18:17:18"},"type":"On/Off plug-in unit","name":"On/Off plug","modelid":"SP 120","manufacturername":"innr","productname":"On/Off plug","capabilities":{"certified":false,"control":{},"streaming":{"renderer":false,"proxy":false}},"config":{"archetype":"recessedfloor","function":"functional","direction":"omnidirectional"},"uniqueid":"00:15:8d:00:ff:ff:ff:ff-ff","swversion":"2.0"}`,
			capabilities: []adapter.CapabilityInfoView{
				{
					Type:        model.OnOffCapabilityType,
					Retrievable: true,
				},
			},
		},
	}

	for _, tc := range cases {
		t.Run(tc.name, func(t *testing.T) {
			var lightInfo LightInfo
			err := json.Unmarshal([]byte(tc.rawLightInfo), &lightInfo)
			if err != nil {
				t.Fatal(xerrors.Errorf("rawLightInfo unmarshal failed: %w", err))
			}

			actualCapabilities := lightInfo.ToCapabilityInfoViews()

			assert.Equal(t, tc.capabilities, actualCapabilities)
		})
	}
}

func TestLightInfo_ToCapabilityStateViews(t *testing.T) {
	type testCase struct {
		name         string
		rawLightInfo string // JSON
		capabilities []adapter.CapabilityStateView
	}

	cases := []testCase{
		{
			name:         "Color light: Hue bloom (LLC011)",
			rawLightInfo: `{"state":{"on":false,"bri":126,"hue":8229,"sat":251,"effect":"none","xy":[0.6014,0.3877],"alert":"none","colormode":"xy","mode":"homeautomation","reachable":true},"swupdate":{"state":"noupdates","lastinstall":null},"type":"Color light","name":"Hue bloom","modelid":"LLC011","manufacturername":"Philips","productname":"Hue bloom","capabilities":{"certified":true,"control":{"mindimlevel":10000,"maxlumen":120,"colorgamuttype":"A","colorgamut":[[0.704,0.296],[0.2151,0.7106],[0.138,0.08]]},"streaming":{"renderer":true,"proxy":false}},"config":{"archetype":"huebloom","function":"decorative","direction":"upwards","startup":{"mode":"powerfail","configured":true}},"uniqueid":"00:17:88:01:ff:ff:ff:ff-ff","swversion":"5.127.1.26581"}`,
			capabilities: []adapter.CapabilityStateView{
				{
					Type: model.OnOffCapabilityType,
					State: model.OnOffCapabilityState{
						Instance: model.OnOnOffCapabilityInstance,
						Value:    false,
					},
				},
				{
					Type: model.RangeCapabilityType,
					State: model.RangeCapabilityState{
						Instance: model.BrightnessRangeInstance,
						Value:    49,
					},
				},
				// TODO: color was skipped, because colormode=xy is not supported
			},
		},
		{
			name:         "Color light: Hue iris (LLC010)",
			rawLightInfo: `{"state":{"on":false,"bri":144,"hue":11973,"sat":149,"effect":"none","xy":[0.5053,0.4206],"alert":"select","colormode":"xy","mode":"homeautomation","reachable":true},"swupdate":{"state":"noupdates","lastinstall":"2018-12-13T13:51:13"},"type":"Color light","name":"Hue iris","modelid":"LLC010","manufacturername":"Philips","productname":"Hue iris","capabilities":{"certified":true,"control":{"mindimlevel":10000,"maxlumen":210,"colorgamuttype":"A","colorgamut":[[0.704,0.296],[0.2151,0.7106],[0.138,0.08]]},"streaming":{"renderer":true,"proxy":false}},"config":{"archetype":"hueiris","function":"decorative","direction":"upwards","startup":{"mode":"safety","configured":true}},"uniqueid":"00:17:88:01:ff:ff:ff:ff-ff","swversion":"5.127.1.26581"}`,
			capabilities: []adapter.CapabilityStateView{
				{
					Type: model.OnOffCapabilityType,
					State: model.OnOffCapabilityState{
						Instance: model.OnOnOffCapabilityInstance,
						Value:    false,
					},
				},
				{
					Type: model.RangeCapabilityType,
					State: model.RangeCapabilityState{
						Instance: model.BrightnessRangeInstance,
						Value:    56,
					},
				},
				// TODO: color was skipped, because colormode=xy is not supported
			},
		},
		{
			name:         "Color light: Hue lightstrip (LST001)",
			rawLightInfo: `{"state":{"on":false,"bri":254,"hue":4551,"sat":243,"effect":"none","xy":[0.6497,0.3395],"alert":"select","colormode":"hs","mode":"homeautomation","reachable":true},"swupdate":{"state":"noupdates","lastinstall":"2019-08-23T09:36:52"},"type":"Color light","name":"Hue lightstrip","modelid":"LST001","manufacturername":"Philips","productname":"Hue lightstrip","capabilities":{"certified":true,"control":{"mindimlevel":10000,"maxlumen":120,"colorgamuttype":"A","colorgamut":[[0.704,0.296],[0.2151,0.7106],[0.138,0.08]]},"streaming":{"renderer":true,"proxy":false}},"config":{"archetype":"huelightstrip","function":"mixed","direction":"omnidirectional","startup":{"mode":"safety","configured":true}},"uniqueid":"00:17:88:01:ff:ff:ff:ff-ff","swversion":"5.127.1.26581"}`,
			capabilities: []adapter.CapabilityStateView{
				{
					Type: model.OnOffCapabilityType,
					State: model.OnOffCapabilityState{
						Instance: model.OnOnOffCapabilityInstance,
						Value:    false,
					},
				},
				{
					Type: model.RangeCapabilityType,
					State: model.RangeCapabilityState{
						Instance: model.BrightnessRangeInstance,
						Value:    100,
					},
				},
				{
					Type: model.ColorSettingCapabilityType,
					State: model.ColorSettingCapabilityState{
						Instance: model.HsvColorCapabilityInstance,
						Value:    model.HSV{H: 24, S: 95, V: 100},
					},
				},
			},
		},
		{
			name:         "Color light: LivingColors (LLC001)",
			rawLightInfo: `{"state":{"on":true,"bri":86,"hue":2095,"sat":200,"effect":"none","xy":[0.633,0.3318],"alert":"select","colormode":"xy","mode":"homeautomation","reachable":true},"swupdate":{"state":"notupdatable","lastinstall":"2019-03-07T10:41:27"},"type":"Color light","name":"LivingColors","modelid":"LLC001","manufacturername":"Philips","productname":"LivingColors","capabilities":{"certified":true,"control":{"colorgamuttype":"A","colorgamut":[[0.704,0.296],[0.2151,0.7106],[0.138,0.08]]},"streaming":{"renderer":false,"proxy":false}},"config":{"archetype":"floorshade","function":"decorative","direction":"omnidirectional"},"uniqueid":"00:17:88:01:ff:ff:ff:ff-ff","swversion":"2.0.0.5206"}`,
			capabilities: []adapter.CapabilityStateView{
				{
					Type: model.OnOffCapabilityType,
					State: model.OnOffCapabilityState{
						Instance: model.OnOnOffCapabilityInstance,
						Value:    true,
					},
				},
				{
					Type: model.RangeCapabilityType,
					State: model.RangeCapabilityState{
						Instance: model.BrightnessRangeInstance,
						Value:    34,
					},
				},
				// TODO: color was skipped, because colormode=xy is not supported
			},
		},
		{
			name:         "Color light: LivingColors (LLC007)",
			rawLightInfo: `{"state":{"on":false,"bri":0,"hue":1962,"sat":217,"effect":"none","xy":[0.6509,0.3246],"alert":"none","colormode":"hs","mode":"homeautomation","reachable":true},"swupdate":{"state":"notupdatable","lastinstall":null},"type":"Color light","name":"LivingColors","modelid":"LLC007","manufacturername":"Philips","productname":"LivingColors","capabilities":{"certified":true,"control":{"mindimlevel":10000,"maxlumen":120,"colorgamuttype":"A","colorgamut":[[0.704,0.296],[0.2151,0.7106],[0.138,0.08]]},"streaming":{"renderer":false,"proxy":false}},"config":{"archetype":"huebloom","function":"decorative","direction":"upwards"},"uniqueid":"00:17:88:01:ff:ff:ff:ff-ff","swversion":"4.6.0.8274"}`,
			capabilities: []adapter.CapabilityStateView{
				{
					Type: model.OnOffCapabilityType,
					State: model.OnOffCapabilityState{
						Instance: model.OnOnOffCapabilityInstance,
						Value:    false,
					},
				},
				{
					Type: model.RangeCapabilityType,
					State: model.RangeCapabilityState{
						Instance: model.BrightnessRangeInstance,
						Value:    1,
					},
				},
				{
					Type: model.ColorSettingCapabilityType,
					State: model.ColorSettingCapabilityState{
						Instance: model.HsvColorCapabilityInstance,
						Value:    model.HSV{H: 10, S: 85, V: 1},
					},
				},
			},
		},
		{
			name:         "Color temperature light: Color temperature light (GL-C-006)",
			rawLightInfo: `{"state":{"on":true,"bri":254,"ct":307,"alert":"select","colormode":"ct","mode":"homeautomation","reachable":false},"swupdate":{"state":"notupdatable","lastinstall":"2019-05-24T19:15:22"},"type":"Color temperature light","name":"Color temperature light","modelid":"GL-C-006","manufacturername":"GLEDOPTO","productname":"Color temperature light","capabilities":{"certified":false,"control":{"ct":{"min":158,"max":500}},"streaming":{"renderer":false,"proxy":false}},"config":{"archetype":"classicbulb","function":"functional","direction":"omnidirectional"},"uniqueid":"00:12:4b:00:ff:ff:ff:ff-ff","swversion":"1.0.7"}`,
			capabilities: []adapter.CapabilityStateView{
				{
					Type: model.OnOffCapabilityType,
					State: model.OnOffCapabilityState{
						Instance: model.OnOnOffCapabilityInstance,
						Value:    true,
					},
				},
				{
					Type: model.RangeCapabilityType,
					State: model.RangeCapabilityState{
						Instance: model.BrightnessRangeInstance,
						Value:    100,
					},
				},
				{
					Type: model.ColorSettingCapabilityType,
					State: model.ColorSettingCapabilityState{
						Instance: model.TemperatureKCapabilityInstance,
						Value:    model.TemperatureK(3257),
					},
				},
			},
		},
		{
			name:         "Color temperature light: Color temperature light (Surface Light TW)",
			rawLightInfo: `{"state":{"on":true,"bri":254,"ct":370,"alert":"select","colormode":"ct","mode":"homeautomation","reachable":false},"swupdate":{"state":"notupdatable","lastinstall":"2019-03-07T10:47:34"},"type":"Color temperature light","name":"Color temperature light","modelid":"Surface Light TW","manufacturername":"OSRAM","productname":"Color temperature light","capabilities":{"certified":false,"control":{"ct":{"min":153,"max":370}},"streaming":{"renderer":false,"proxy":false}},"config":{"archetype":"classicbulb","function":"functional","direction":"omnidirectional"},"uniqueid":"84:18:26:00:ff:ff:ff:ff-ff","swversion":"V1.03.20"}`,
			capabilities: []adapter.CapabilityStateView{
				{
					Type: model.OnOffCapabilityType,
					State: model.OnOffCapabilityState{
						Instance: model.OnOnOffCapabilityInstance,
						Value:    true,
					},
				},
				{
					Type: model.RangeCapabilityType,
					State: model.RangeCapabilityState{
						Instance: model.BrightnessRangeInstance,
						Value:    100,
					},
				},
				{
					Type: model.ColorSettingCapabilityType,
					State: model.ColorSettingCapabilityState{
						Instance: model.TemperatureKCapabilityInstance,
						Value:    model.TemperatureK(2702),
					},
				},
			},
		},
		{
			name:         "Color temperature light: Hue ambiance candle (LTW012)",
			rawLightInfo: `{"state":{"on":false,"bri":234,"ct":298,"alert":"none","colormode":"ct","mode":"homeautomation","reachable":true},"swupdate":{"state":"noupdates","lastinstall":null},"type":"Color temperature light","name":"Hue ambiance candle","modelid":"LTW012","manufacturername":"Philips","productname":"Hue ambiance candle","capabilities":{"certified":true,"control":{"mindimlevel":2000,"maxlumen":450,"ct":{"min":153,"max":454}},"streaming":{"renderer":false,"proxy":false}},"config":{"archetype":"candlebulb","function":"decorative","direction":"omnidirectional","startup":{"mode":"powerfail","configured":true}},"uniqueid":"00:17:88:01:ff:ff:ff:ff-ff","swversion":"1.46.13_r26312","swconfigid":"5E2017D8","productid":"Philips-LTW012-1-E14CTv1"}`,
			capabilities: []adapter.CapabilityStateView{
				{
					Type: model.OnOffCapabilityType,
					State: model.OnOffCapabilityState{
						Instance: model.OnOnOffCapabilityInstance,
						Value:    false,
					},
				},
				{
					Type: model.RangeCapabilityType,
					State: model.RangeCapabilityState{
						Instance: model.BrightnessRangeInstance,
						Value:    92,
					},
				},
				{
					Type: model.ColorSettingCapabilityType,
					State: model.ColorSettingCapabilityState{
						Instance: model.TemperatureKCapabilityInstance,
						Value:    model.TemperatureK(3355),
					},
				},
			},
		},
		{
			name:         "Color temperature light: Hue ambiance ceiling (LTC002)",
			rawLightInfo: `{"state":{"on":false,"bri":148,"ct":343,"alert":"none","colormode":"ct","mode":"homeautomation","reachable":true},"swupdate":{"state":"noupdates","lastinstall":"2019-04-17T11:18:55"},"type":"Color temperature light","name":"Hue ambiance ceiling","modelid":"LTC002","manufacturername":"Philips","productname":"Hue ambiance ceiling","capabilities":{"certified":true,"control":{"mindimlevel":1000,"maxlumen":3000,"ct":{"min":153,"max":454}},"streaming":{"renderer":false,"proxy":false}},"config":{"archetype":"ceilinground","function":"functional","direction":"downwards","startup":{"mode":"powerfail","configured":true}},"uniqueid":"00:17:88:01:ff:ff:ff:ff-ff","swversion":"1.46.13_r26312","swconfigid":"A987DFF5","productid":"ENA_LTC002_1_FairCeiling_v1"}`,
			capabilities: []adapter.CapabilityStateView{
				{
					Type: model.OnOffCapabilityType,
					State: model.OnOffCapabilityState{
						Instance: model.OnOnOffCapabilityInstance,
						Value:    false,
					},
				},
				{
					Type: model.RangeCapabilityType,
					State: model.RangeCapabilityState{
						Instance: model.BrightnessRangeInstance,
						Value:    58,
					},
				},
				{
					Type: model.ColorSettingCapabilityType,
					State: model.ColorSettingCapabilityState{
						Instance: model.TemperatureKCapabilityInstance,
						Value:    model.TemperatureK(2915),
					},
				},
			},
		},
		{
			name:         "Color temperature light: Hue ambiance lamp (LTA001)",
			rawLightInfo: `{"state":{"on":false,"bri":137,"ct":454,"alert":"none","colormode":"ct","mode":"homeautomation","reachable":false},"swupdate":{"state":"noupdates","lastinstall":"2019-04-24T07:30:19"},"type":"Color temperature light","name":"Hue ambiance lamp","modelid":"LTA001","manufacturername":"Philips","productname":"Hue ambiance lamp","capabilities":{"certified":true,"control":{"mindimlevel":200,"maxlumen":800,"ct":{"min":153,"max":454}},"streaming":{"renderer":false,"proxy":false}},"config":{"archetype":"sultanbulb","function":"functional","direction":"omnidirectional","startup":{"mode":"safety","configured":true}},"uniqueid":"00:17:88:01:ff:ff:ff:ff-ff","swversion":"1.53.3_r27175","swconfigid":"2E12ED35","productid":"Philips-LTA001-2-A19CTv3"}`,
			capabilities: []adapter.CapabilityStateView{
				{
					Type: model.OnOffCapabilityType,
					State: model.OnOffCapabilityState{
						Instance: model.OnOnOffCapabilityInstance,
						Value:    false,
					},
				},
				{
					Type: model.RangeCapabilityType,
					State: model.RangeCapabilityState{
						Instance: model.BrightnessRangeInstance,
						Value:    54,
					},
				},
				{
					Type: model.ColorSettingCapabilityType,
					State: model.ColorSettingCapabilityState{
						Instance: model.TemperatureKCapabilityInstance,
						Value:    model.TemperatureK(2202),
					},
				},
			},
		},
		{
			name:         "Color temperature light: Hue ambiance lamp (LTW004)",
			rawLightInfo: `{"state":{"on":false,"bri":254,"ct":230,"alert":"none","colormode":"ct","mode":"homeautomation","reachable":true},"swupdate":{"state":"noupdates","lastinstall":"2018-12-18T18:49:53"},"type":"Color temperature light","name":"Hue ambiance lamp","modelid":"LTW004","manufacturername":"Philips","productname":"Hue ambiance lamp","capabilities":{"certified":true,"control":{"mindimlevel":1000,"maxlumen":800,"ct":{"min":153,"max":454}},"streaming":{"renderer":false,"proxy":false}},"config":{"archetype":"sultanbulb","function":"functional","direction":"omnidirectional","startup":{"mode":"powerfail","configured":true}},"uniqueid":"00:17:88:01:ff:ff:ff:ff-ff","swversion":"5.127.1.26420"}`,
			capabilities: []adapter.CapabilityStateView{
				{
					Type: model.OnOffCapabilityType,
					State: model.OnOffCapabilityState{
						Instance: model.OnOnOffCapabilityInstance,
						Value:    false,
					},
				},
				{
					Type: model.RangeCapabilityType,
					State: model.RangeCapabilityState{
						Instance: model.BrightnessRangeInstance,
						Value:    100,
					},
				},
				{
					Type: model.ColorSettingCapabilityType,
					State: model.ColorSettingCapabilityState{
						Instance: model.TemperatureKCapabilityInstance,
						Value:    model.TemperatureK(4347),
					},
				},
			},
		},
		{
			name:         "Color temperature light: Hue ambiance lamp (LTW010)",
			rawLightInfo: `{"state":{"on":false,"bri":1,"ct":436,"alert":"none","colormode":"ct","mode":"homeautomation","reachable":true},"swupdate":{"state":"noupdates","lastinstall":null},"type":"Color temperature light","name":"Hue ambiance lamp","modelid":"LTW010","manufacturername":"Philips","productname":"Hue ambiance lamp","capabilities":{"certified":true,"control":{"mindimlevel":1000,"maxlumen":806,"ct":{"min":153,"max":454}},"streaming":{"renderer":false,"proxy":false}},"config":{"archetype":"sultanbulb","function":"functional","direction":"omnidirectional","startup":{"mode":"powerfail","configured":true}},"uniqueid":"00:17:88:01:ff:ff:ff:ff-ff","swversion":"1.46.13_r26312","swconfigid":"7A2E82CC","productid":"Philips-LTW010-1-A19CTv2"}`,
			capabilities: []adapter.CapabilityStateView{
				{
					Type: model.OnOffCapabilityType,
					State: model.OnOffCapabilityState{
						Instance: model.OnOnOffCapabilityInstance,
						Value:    false,
					},
				},
				{
					Type: model.RangeCapabilityType,
					State: model.RangeCapabilityState{
						Instance: model.BrightnessRangeInstance,
						Value:    1,
					},
				},
				{
					Type: model.ColorSettingCapabilityType,
					State: model.ColorSettingCapabilityState{
						Instance: model.TemperatureKCapabilityInstance,
						Value:    model.TemperatureK(2293),
					},
				},
			},
		},
		{
			name:         "Color temperature light: Hue ambiance lamp (LTW015)",
			rawLightInfo: `{"state":{"on":true,"bri":254,"ct":366,"alert":"select","colormode":"ct","mode":"homeautomation","reachable":false},"swupdate":{"state":"noupdates","lastinstall":"2019-06-20T21:48:01"},"type":"Color temperature light","name":"Hue ambiance lamp","modelid":"LTW015","manufacturername":"Philips","productname":"Hue ambiance lamp","capabilities":{"certified":true,"control":{"mindimlevel":1000,"maxlumen":800,"ct":{"min":153,"max":454}},"streaming":{"renderer":false,"proxy":false}},"config":{"archetype":"sultanbulb","function":"functional","direction":"omnidirectional","startup":{"mode":"safety","configured":true}},"uniqueid":"00:17:88:01:ff:ff:ff:ff-ff","swversion":"1.46.13_r26312","swconfigid":"28823930","productid":"Philips-LTW015-1-A19CTv2"}`,
			capabilities: []adapter.CapabilityStateView{
				{
					Type: model.OnOffCapabilityType,
					State: model.OnOffCapabilityState{
						Instance: model.OnOnOffCapabilityInstance,
						Value:    true,
					},
				},
				{
					Type: model.RangeCapabilityType,
					State: model.RangeCapabilityState{
						Instance: model.BrightnessRangeInstance,
						Value:    100,
					},
				},
				{
					Type: model.ColorSettingCapabilityType,
					State: model.ColorSettingCapabilityState{
						Instance: model.TemperatureKCapabilityInstance,
						Value:    model.TemperatureK(2732),
					},
				},
			},
		},
		{
			name:         "Color temperature light: Hue ambiance spot (LTW013)",
			rawLightInfo: `{"state":{"on":true,"bri":254,"ct":366,"alert":"none","colormode":"ct","mode":"homeautomation","reachable":false},"swupdate":{"state":"transferring","lastinstall":"2018-07-28T12:03:06"},"type":"Color temperature light","name":"Hue ambiance spot","modelid":"LTW013","manufacturername":"Philips","productname":"Hue ambiance spot","capabilities":{"certified":true,"control":{"mindimlevel":1000,"maxlumen":250,"ct":{"min":153,"max":454}},"streaming":{"renderer":false,"proxy":false}},"config":{"archetype":"spotbulb","function":"functional","direction":"downwards"},"uniqueid":"00:17:88:01:ff:ff:ff:ff-ff","swversion":"1.29.0_r21169","swconfigid":"797DDD7C","productid":"Philips-LTW013-1-GU10CTv1"}`,
			capabilities: []adapter.CapabilityStateView{
				{
					Type: model.OnOffCapabilityType,
					State: model.OnOffCapabilityState{
						Instance: model.OnOnOffCapabilityInstance,
						Value:    true,
					},
				},
				{
					Type: model.RangeCapabilityType,
					State: model.RangeCapabilityState{
						Instance: model.BrightnessRangeInstance,
						Value:    100,
					},
				},
				{
					Type: model.ColorSettingCapabilityType,
					State: model.ColorSettingCapabilityState{
						Instance: model.TemperatureKCapabilityInstance,
						Value:    model.TemperatureK(2732),
					},
				},
			},
		},
		{
			name:         "Dimmable light: Dimmable light (Classic A60 W clear - LIGHTIFY)",
			rawLightInfo: `{"state":{"on":true,"bri":254,"alert":"none","mode":"homeautomation","reachable":true},"swupdate":{"state":"notupdatable","lastinstall":"2019-03-07T12:59:54"},"type":"Dimmable light","name":"Dimmable light","modelid":"Classic A60 W clear - LIGHTIFY","manufacturername":"OSRAM","productname":"Dimmable light","capabilities":{"certified":false,"control":{},"streaming":{"renderer":false,"proxy":false}},"config":{"archetype":"classicbulb","function":"functional","direction":"omnidirectional"},"uniqueid":"84:18:26:00:ff:ff:ff:ff-ff","swversion":"V1.05.10"}`,
			capabilities: []adapter.CapabilityStateView{
				{
					Type: model.OnOffCapabilityType,
					State: model.OnOffCapabilityState{
						Instance: model.OnOnOffCapabilityInstance,
						Value:    true,
					},
				},
				{
					Type: model.RangeCapabilityType,
					State: model.RangeCapabilityState{
						Instance: model.BrightnessRangeInstance,
						Value:    100,
					},
				},
			},
		},
		{
			name:         "Dimmable light: Dimmable light (GLEDOPTO)",
			rawLightInfo: `{"state":{"on":false,"bri":1,"alert":"select","mode":"homeautomation","reachable":true},"swupdate":{"state":"notupdatable","lastinstall":"2019-04-17T21:04:40"},"type":"Dimmable light","name":"Dimmable light","modelid":"GLEDOPTO","manufacturername":"GLEDOPTO","productname":"Dimmable light","capabilities":{"certified":false,"control":{},"streaming":{"renderer":false,"proxy":false}},"config":{"archetype":"huelightstrip","function":"functional","direction":"omnidirectional"},"uniqueid":"00:12:4b:00:ff:ff:ff:ff-ff","swversion":"1.0.2"}`,
			capabilities: []adapter.CapabilityStateView{
				{
					Type: model.OnOffCapabilityType,
					State: model.OnOffCapabilityState{
						Instance: model.OnOnOffCapabilityInstance,
						Value:    false,
					},
				},
				{
					Type: model.RangeCapabilityType,
					State: model.RangeCapabilityState{
						Instance: model.BrightnessRangeInstance,
						Value:    1,
					},
				},
			},
		},
		{
			name:         "Dimmable light: Dimmable light (LXN56-DC27LX1.1)",
			rawLightInfo: `{"state":{"on":false,"bri":1,"alert":"select","mode":"homeautomation","reachable":true},"swupdate":{"state":"notupdatable","lastinstall":"2019-08-07T18:58:22"},"type":"Dimmable light","name":"Dimmable light","modelid":"LXN56-DC27LX1.1","manufacturername":"3A Smart Home DE","productname":"Dimmable light","capabilities":{"certified":false,"control":{},"streaming":{"renderer":false,"proxy":false}},"config":{"archetype":"classicbulb","function":"functional","direction":"omnidirectional"},"uniqueid":"00:15:8d:00:ff:ff:ff:ff-ff","swversion":"1000-0001"}`,
			capabilities: []adapter.CapabilityStateView{
				{
					Type: model.OnOffCapabilityType,
					State: model.OnOffCapabilityState{
						Instance: model.OnOnOffCapabilityInstance,
						Value:    false,
					},
				},
				{
					Type: model.RangeCapabilityType,
					State: model.RangeCapabilityState{
						Instance: model.BrightnessRangeInstance,
						Value:    1,
					},
				},
			},
		},
		{
			name:         "Dimmable light: Dimmable light (TRADFRI bulb E27 W opal 1000lm)",
			rawLightInfo: `{"state":{"on":false,"bri":254,"alert":"select","mode":"homeautomation","reachable":false},"swupdate":{"state":"notupdatable","lastinstall":"2019-08-19T22:59:45"},"type":"Dimmable light","name":"Dimmable light","modelid":"TRADFRI bulb E27 W opal 1000lm","manufacturername":"IKEA of Sweden","productname":"Dimmable light","capabilities":{"certified":false,"control":{},"streaming":{"renderer":false,"proxy":false}},"config":{"archetype":"classicbulb","function":"functional","direction":"omnidirectional"},"uniqueid":"08:6b:d7:ff:ff:ff:ff:ff-ff","swversion":"1.2.214"}`,
			capabilities: []adapter.CapabilityStateView{
				{
					Type: model.OnOffCapabilityType,
					State: model.OnOffCapabilityState{
						Instance: model.OnOnOffCapabilityInstance,
						Value:    false,
					},
				},
				{
					Type: model.RangeCapabilityType,
					State: model.RangeCapabilityState{
						Instance: model.BrightnessRangeInstance,
						Value:    100,
					},
				},
			},
		},
		{
			name:         "Dimmable light: Hue white lamp (LWB004)",
			rawLightInfo: `{"state":{"on":false,"bri":127,"alert":"none","mode":"homeautomation","reachable":true},"swupdate":{"state":"noupdates","lastinstall":null},"type":"Dimmable light","name":"Hue white lamp","modelid":"LWB004","manufacturername":"Philips","productname":"Hue white lamp","capabilities":{"certified":true,"control":{"mindimlevel":2000,"maxlumen":750},"streaming":{"renderer":false,"proxy":false}},"config":{"archetype":"sultanbulb","function":"functional","direction":"omnidirectional","startup":{"mode":"powerfail","configured":true}},"uniqueid":"00:17:88:01:ff:ff:ff:ff-ff","swversion":"5.127.1.26420"}`,
			capabilities: []adapter.CapabilityStateView{
				{
					Type: model.OnOffCapabilityType,
					State: model.OnOffCapabilityState{
						Instance: model.OnOnOffCapabilityInstance,
						Value:    false,
					},
				},
				{
					Type: model.RangeCapabilityType,
					State: model.RangeCapabilityState{
						Instance: model.BrightnessRangeInstance,
						Value:    50,
					},
				},
			},
		},
		{
			name:         "Dimmable light: Hue white lamp (LWB010)",
			rawLightInfo: `{"state":{"on":true,"bri":254,"alert":"select","mode":"homeautomation","reachable":true},"swupdate":{"state":"noupdates","lastinstall":"2019-08-04T19:36:27"},"type":"Dimmable light","name":"Hue white lamp","modelid":"LWB010","manufacturername":"Philips","productname":"Hue white lamp","capabilities":{"certified":true,"control":{"mindimlevel":2000,"maxlumen":806},"streaming":{"renderer":false,"proxy":false}},"config":{"archetype":"classicbulb","function":"functional","direction":"omnidirectional","startup":{"mode":"safety","configured":true}},"uniqueid":"00:17:88:01:ff:ff:ff:ff-ff","swversion":"1.46.13_r26312","swconfigid":"322BB2EC","productid":"Philips-LWB010-1-A19DLv4"}`,
			capabilities: []adapter.CapabilityStateView{
				{
					Type: model.OnOffCapabilityType,
					State: model.OnOffCapabilityState{
						Instance: model.OnOnOffCapabilityInstance,
						Value:    true,
					},
				},
				{
					Type: model.RangeCapabilityType,
					State: model.RangeCapabilityState{
						Instance: model.BrightnessRangeInstance,
						Value:    100,
					},
				},
			},
		},
		{
			name:         "Dimmable light: Hue white lamp (LWB014)",
			rawLightInfo: `{"state":{"on":false,"bri":1,"alert":"none","mode":"homeautomation","reachable":true},"swupdate":{"state":"noupdates","lastinstall":"2018-12-07T09:51:00"},"type":"Dimmable light","name":"Hue white lamp","modelid":"LWB014","manufacturername":"Philips","productname":"Hue white lamp","capabilities":{"certified":true,"control":{"mindimlevel":2000,"maxlumen":840},"streaming":{"renderer":false,"proxy":false}},"config":{"archetype":"classicbulb","function":"functional","direction":"omnidirectional","startup":{"mode":"powerfail","configured":true}},"uniqueid":"00:17:88:01:ff:ff:ff:ff-ff","swversion":"1.46.13_r26312","swconfigid":"69806BE9","productid":"Philips-LWB014-1-A19DLv4"}`,
			capabilities: []adapter.CapabilityStateView{
				{
					Type: model.OnOffCapabilityType,
					State: model.OnOffCapabilityState{
						Instance: model.OnOnOffCapabilityInstance,
						Value:    false,
					},
				},
				{
					Type: model.RangeCapabilityType,
					State: model.RangeCapabilityState{
						Instance: model.BrightnessRangeInstance,
						Value:    1,
					},
				},
			},
		},
		{
			name:         "Dimmable plug-in unit: LivingWhites Plug (LWL001)",
			rawLightInfo: `{"state":{"on":false,"bri":0,"alert":"select","mode":"homeautomation","reachable":true},"swupdate":{"state":"notupdatable","lastinstall":null},"type":"Dimmable plug-in unit","name":"LivingWhites Plug","modelid":"LWL001","manufacturername":"Philips","productname":"LivingWhites Plug","capabilities":{"certified":true,"control":{},"streaming":{"renderer":false,"proxy":false}},"config":{"archetype":"plug","function":"functional","direction":"omnidirectional"},"uniqueid":"00:17:88:01:ff:ff:ff:ff-ff","swversion":"1.0.1.4591"}`,
			capabilities: []adapter.CapabilityStateView{
				{
					Type: model.OnOffCapabilityType,
					State: model.OnOffCapabilityState{
						Instance: model.OnOnOffCapabilityInstance,
						Value:    false,
					},
				},
				{
					Type: model.RangeCapabilityType,
					State: model.RangeCapabilityState{
						Instance: model.BrightnessRangeInstance,
						Value:    1,
					},
				},
			},
		},
		{
			// this device is strange. It has full Extended color light state, but no capabilities. And even it's name is empty. Maybe it was HUE API bug, so let's use it :)
			name:         "Extended color light: unknown (broken LCT001)",
			rawLightInfo: `{"state":{"on":false,"bri":122,"hue":0,"sat":0,"effect":"none","xy":[0,0],"ct":0,"alert":"none","colormode":"hs","reachable":false},"type":"Extended color light","name":"unknown","modelid":"LCT001","manufacturername":"Philips","uniqueid":"00:17:88:01:ff:ff:ff:ff-ff","swversion":"5.105.0.21169"}`,
			capabilities: []adapter.CapabilityStateView{
				{
					Type: model.OnOffCapabilityType,
					State: model.OnOffCapabilityState{
						Instance: model.OnOnOffCapabilityInstance,
						Value:    false,
					},
				},
				{
					Type: model.RangeCapabilityType,
					State: model.RangeCapabilityState{
						Instance: model.BrightnessRangeInstance,
						Value:    48,
					},
				},
			},
		},
		{
			name:         "Extended color light: Hue color lamp (LCT001)",
			rawLightInfo: `{"state":{"on":false,"bri":254,"hue":14893,"sat":143,"effect":"none","xy":[0.4591,0.4101],"ct":369,"alert":"none","colormode":"xy","mode":"homeautomation","reachable":true},"swupdate":{"state":"noupdates","lastinstall":"2019-01-02T22:46:13"},"type":"Extended color light","name":"Hue color lamp","modelid":"LCT001","manufacturername":"Philips","productname":"Hue color lamp","capabilities":{"certified":true,"control":{"mindimlevel":5000,"maxlumen":600,"colorgamuttype":"B","colorgamut":[[0.675,0.322],[0.409,0.518],[0.167,0.04]],"ct":{"min":153,"max":500}},"streaming":{"renderer":true,"proxy":false}},"config":{"archetype":"sultanbulb","function":"mixed","direction":"omnidirectional","startup":{"mode":"safety","configured":true}},"uniqueid":"00:17:88:01:ff:ff:ff:ff-ff","swversion":"5.127.1.26581"}`,
			capabilities: []adapter.CapabilityStateView{
				{
					Type: model.OnOffCapabilityType,
					State: model.OnOffCapabilityState{
						Instance: model.OnOnOffCapabilityInstance,
						Value:    false,
					},
				},
				{
					Type: model.RangeCapabilityType,
					State: model.RangeCapabilityState{
						Instance: model.BrightnessRangeInstance,
						Value:    100,
					},
				},
				// TODO: color was skipped, because colormode=xy is not supported
			},
		},
		{
			name:         "Extended color light: Dimmable light (GLEDOPTO)",
			rawLightInfo: `{"state":{"on":false,"bri":1,"hue":0,"sat":0,"effect":"none","xy":[0,0],"ct":0,"alert":"select","colormode":"hs","mode":"homeautomation","reachable":false},"swupdate":{"state":"notupdatable","lastinstall":"2019-04-17T21:04:40"},"type":"Extended color light","name":"Dimmable light","modelid":"GLEDOPTO","manufacturername":"GLEDOPTO","productname":"Dimmable light","capabilities":{"certified":false,"control":{},"streaming":{"renderer":false,"proxy":false}},"config":{"archetype":"huelightstrip","function":"functional","direction":"omnidirectional"},"uniqueid":"00:12:4b:00:ff:ff:ff:ff-ff","swversion":"1.0.2"}`,
			capabilities: []adapter.CapabilityStateView{
				{
					Type: model.OnOffCapabilityType,
					State: model.OnOffCapabilityState{
						Instance: model.OnOnOffCapabilityInstance,
						Value:    false,
					},
				},
				{
					Type: model.RangeCapabilityType,
					State: model.RangeCapabilityState{
						Instance: model.BrightnessRangeInstance,
						Value:    1,
					},
				},
			},
		},
		{
			name:         "Extended color light: Extended color light (GL-B-008Z)",
			rawLightInfo: `{"state":{"on":false,"bri":255,"hue":2304,"sat":253,"effect":"none","xy":[0.6799,0.3108],"ct":367,"alert":"select","colormode":"xy","mode":"homeautomation","reachable":true},"swupdate":{"state":"notupdatable","lastinstall":"2019-04-23T16:56:20"},"type":"Extended color light","name":"Extended color light","modelid":"GL-B-008Z","manufacturername":"GLEDOPTO","productname":"Extended color light","capabilities":{"certified":false,"control":{"colorgamuttype":"other","ct":{"min":0,"max":65535}},"streaming":{"renderer":false,"proxy":false}},"config":{"archetype":"floodbulb","function":"mixed","direction":"omnidirectional"},"uniqueid":"00:12:4b:00:ff:ff:ff:ff-ff","swversion":"2.0.0"}`,
			capabilities: []adapter.CapabilityStateView{
				{
					Type: model.OnOffCapabilityType,
					State: model.OnOffCapabilityState{
						Instance: model.OnOnOffCapabilityInstance,
						Value:    false,
					},
				},
				{
					Type: model.RangeCapabilityType,
					State: model.RangeCapabilityState{
						Instance: model.BrightnessRangeInstance,
						Value:    100,
					},
				},
				// TODO: color was skipped, because colormode=xy is not supported
			},
		},
		{
			name:         "Extended color light: Extended color light (GL-C-007)",
			rawLightInfo: `{"state":{"on":false,"bri":245,"hue":17920,"sat":253,"effect":"none","xy":[0.1879,0.6949],"ct":395,"alert":"select","colormode":"xy","mode":"homeautomation","reachable":false},"swupdate":{"state":"notupdatable","lastinstall":"2019-05-24T19:15:21"},"type":"Extended color light","name":"Extended color light","modelid":"GL-C-007","manufacturername":"GLEDOPTO","productname":"Extended color light","capabilities":{"certified":false,"control":{"colorgamuttype":"other","ct":{"min":0,"max":65535}},"streaming":{"renderer":false,"proxy":false}},"config":{"archetype":"classicbulb","function":"mixed","direction":"omnidirectional"},"uniqueid":"00:12:4b:00:ff:ff:ff:ff-ff","swversion":"1.0.7"}`,
			capabilities: []adapter.CapabilityStateView{
				{
					Type: model.OnOffCapabilityType,
					State: model.OnOffCapabilityState{
						Instance: model.OnOnOffCapabilityInstance,
						Value:    false,
					},
				},
				{
					Type: model.RangeCapabilityType,
					State: model.RangeCapabilityState{
						Instance: model.BrightnessRangeInstance,
						Value:    96,
					},
				},
				// TODO: color was skipped, because colormode=xy is not supported
			},
		},
		{
			name:         "Extended color light: Extended color light (GL-FL-004TZ)",
			rawLightInfo: `{"state":{"on":false,"bri":254,"hue":35072,"sat":32,"effect":"none","xy":[0.2979,0.3201],"ct":159,"alert":"none","colormode":"ct","mode":"homeautomation","reachable":false},"swupdate":{"state":"notupdatable","lastinstall":"2019-05-24T19:09:05"},"type":"Extended color light","name":"Extended color light","modelid":"GL-FL-004TZ","manufacturername":"GLEDOPTO","productname":"Extended color light","capabilities":{"certified":false,"control":{"colorgamuttype":"other","ct":{"min":0,"max":65535}},"streaming":{"renderer":false,"proxy":false}},"config":{"archetype":"classicbulb","function":"mixed","direction":"omnidirectional"},"uniqueid":"00:12:4b:00:ff:ff:ff:ff-ff","swversion":"2.0.0"}`,
			capabilities: []adapter.CapabilityStateView{
				{
					Type: model.OnOffCapabilityType,
					State: model.OnOffCapabilityState{
						Instance: model.OnOnOffCapabilityInstance,
						Value:    false,
					},
				},
				{
					Type: model.RangeCapabilityType,
					State: model.RangeCapabilityState{
						Instance: model.BrightnessRangeInstance,
						Value:    100,
					},
				},
				{
					Type: model.ColorSettingCapabilityType,
					State: model.ColorSettingCapabilityState{
						Instance: model.TemperatureKCapabilityInstance,
						Value:    model.TemperatureK(6289),
					},
				},
			},
		},
		{
			name:         "Extended color light: Extended color light (GLEDOPTO)",
			rawLightInfo: `{"state":{"on":false,"bri":253,"hue":3584,"sat":9,"effect":"none","xy":[0.642,0.3521],"ct":293,"alert":"select","colormode":"xy","mode":"homeautomation","reachable":false},"swupdate":{"state":"notupdatable","lastinstall":"2019-04-17T21:04:40"},"type":"Extended color light","name":"Extended color light","modelid":"GLEDOPTO","manufacturername":"GLEDOPTO","productname":"Extended color light","capabilities":{"certified":false,"control":{"colorgamuttype":"other","ct":{"min":0,"max":65535}},"streaming":{"renderer":false,"proxy":false}},"config":{"archetype":"huelightstrip","function":"mixed","direction":"omnidirectional"},"uniqueid":"00:12:4b:00:ff:ff:ff:ff-ff","swversion":"1.0.2"}`,
			capabilities: []adapter.CapabilityStateView{
				{
					Type: model.OnOffCapabilityType,
					State: model.OnOffCapabilityState{
						Instance: model.OnOnOffCapabilityInstance,
						Value:    false,
					},
				},
				{
					Type: model.RangeCapabilityType,
					State: model.RangeCapabilityState{
						Instance: model.BrightnessRangeInstance,
						Value:    99,
					},
				},
				// TODO: color was skipped, because colormode=xy is not supported
			},
		},
		{
			name:         "Extended color light: Hue beyond down (LLM001)",
			rawLightInfo: `{"state":{"on":true,"bri":144,"hue":13524,"sat":200,"effect":"none","xy":[0.5017,0.4152],"ct":443,"alert":"select","colormode":"xy","mode":"homeautomation","reachable":true},"swupdate":{"state":"readytoinstall","lastinstall":"2019-06-15T11:32:03"},"type":"Extended color light","name":"Hue beyond down","modelid":"LLM001","manufacturername":"Philips","productname":"Hue beyond down","capabilities":{"certified":true,"control":{"mindimlevel":5000,"maxlumen":300,"colorgamuttype":"B","colorgamut":[[0.675,0.322],[0.409,0.518],[0.167,0.04]],"ct":{"min":153,"max":500}},"streaming":{"renderer":false,"proxy":false}},"config":{"archetype":"pendantround","function":"decorative","direction":"downwards"},"uniqueid":"00:17:88:01:ff:ff:ff:ff-ff","luminaireuniqueid":"00:c3:42:2a-02-01","swversion":"5.17.1.12040"}`,
			capabilities: []adapter.CapabilityStateView{
				{
					Type: model.OnOffCapabilityType,
					State: model.OnOffCapabilityState{
						Instance: model.OnOnOffCapabilityInstance,
						Value:    true,
					},
				},
				{
					Type: model.RangeCapabilityType,
					State: model.RangeCapabilityState{
						Instance: model.BrightnessRangeInstance,
						Value:    56,
					},
				},
				// TODO: color was skipped, because colormode=xy is not supported
			},
		},
		{
			name:         "Extended color light: Hue beyond up (LLM001)",
			rawLightInfo: `{"state":{"on":true,"bri":144,"hue":13524,"sat":200,"effect":"none","xy":[0.5017,0.4152],"ct":443,"alert":"select","colormode":"xy","mode":"homeautomation","reachable":true},"swupdate":{"state":"readytoinstall","lastinstall":"2019-06-15T11:32:02"},"type":"Extended color light","name":"Hue beyond up","modelid":"LLM001","manufacturername":"Philips","productname":"Hue beyond up","capabilities":{"certified":true,"control":{"mindimlevel":5000,"maxlumen":300,"colorgamuttype":"B","colorgamut":[[0.675,0.322],[0.409,0.518],[0.167,0.04]],"ct":{"min":153,"max":500}},"streaming":{"renderer":false,"proxy":false}},"config":{"archetype":"pendantround","function":"decorative","direction":"upwards"},"uniqueid":"00:17:88:01:ff:ff:ff:ff-ff","luminaireuniqueid":"00:c3:42:2a-01-00","swversion":"5.17.1.12040"}`,
			capabilities: []adapter.CapabilityStateView{
				{
					Type: model.OnOffCapabilityType,
					State: model.OnOffCapabilityState{
						Instance: model.OnOnOffCapabilityInstance,
						Value:    true,
					},
				},
				{
					Type: model.RangeCapabilityType,
					State: model.RangeCapabilityState{
						Instance: model.BrightnessRangeInstance,
						Value:    56,
					},
				},
				// TODO: color was skipped, because colormode=xy is not supported
			},
		},
		{
			name:         "Extended color light: Hue color candle (LCT012)",
			rawLightInfo: `{"state":{"on":true,"bri":254,"hue":8418,"sat":140,"effect":"none","xy":[0.4573,0.41],"ct":366,"alert":"select","colormode":"ct","mode":"homeautomation","reachable":true},"swupdate":{"state":"noupdates","lastinstall":"2018-12-30T12:33:40"},"type":"Extended color light","name":"Hue color candle","modelid":"LCT012","manufacturername":"Philips","productname":"Hue color candle","capabilities":{"certified":true,"control":{"mindimlevel":2000,"maxlumen":450,"colorgamuttype":"C","colorgamut":[[0.6915,0.3083],[0.17,0.7],[0.1532,0.0475]],"ct":{"min":153,"max":500}},"streaming":{"renderer":true,"proxy":true}},"config":{"archetype":"candlebulb","function":"mixed","direction":"omnidirectional","startup":{"mode":"safety","configured":true}},"uniqueid":"00:17:88:01:ff:ff:ff:ff-ff","swversion":"1.46.13_r26312","swconfigid":"8C67986F","productid":"Philips-LCT012-1-E14ECLv1"}`,
			capabilities: []adapter.CapabilityStateView{
				{
					Type: model.OnOffCapabilityType,
					State: model.OnOffCapabilityState{
						Instance: model.OnOnOffCapabilityInstance,
						Value:    true,
					},
				},
				{
					Type: model.RangeCapabilityType,
					State: model.RangeCapabilityState{
						Instance: model.BrightnessRangeInstance,
						Value:    100,
					},
				},
				{
					Type: model.ColorSettingCapabilityType,
					State: model.ColorSettingCapabilityState{
						Instance: model.TemperatureKCapabilityInstance,
						Value:    model.TemperatureK(2732),
					},
				},
			},
		},
		{
			name:         "Extended color light: Hue color downlight (LCB001)",
			rawLightInfo: `{"state":{"on":false,"bri":137,"hue":8418,"sat":140,"effect":"none","xy":[0.7006,0.2993],"ct":454,"alert":"none","colormode":"xy","mode":"homeautomation","reachable":false},"swupdate":{"state":"noupdates","lastinstall":"2019-04-25T15:12:57"},"type":"Extended color light","name":"Hue color downlight","modelid":"LCB001","manufacturername":"Philips","productname":"Hue color downlight","capabilities":{"certified":true,"control":{"mindimlevel":200,"maxlumen":650,"colorgamuttype":"C","colorgamut":[[0.6915,0.3083],[0.17,0.7],[0.1532,0.0475]],"ct":{"min":153,"max":500}},"streaming":{"renderer":true,"proxy":true}},"config":{"archetype":"floodbulb","function":"mixed","direction":"downwards","startup":{"mode":"safety","configured":true}},"uniqueid":"00:17:88:01:ff:ff:ff:ff-ff","swversion":"1.53.3_r27175","swconfigid":"E4A52056","productid":"Philips-LCB001-1-BR30ECLv4"}`,
			capabilities: []adapter.CapabilityStateView{
				{
					Type: model.OnOffCapabilityType,
					State: model.OnOffCapabilityState{
						Instance: model.OnOnOffCapabilityInstance,
						Value:    false,
					},
				},
				{
					Type: model.RangeCapabilityType,
					State: model.RangeCapabilityState{
						Instance: model.BrightnessRangeInstance,
						Value:    54,
					},
				},
				// TODO: color was skipped, because colormode=xy is not supported
			},
		},
		{
			name:         "Extended color light: Hue color downlight (LCT002)",
			rawLightInfo: `{"state":{"on":true,"bri":254,"hue":14988,"sat":141,"effect":"none","xy":[0.4575,0.4101],"ct":366,"alert":"select","colormode":"ct","mode":"homeautomation","reachable":true},"swupdate":{"state":"noupdates","lastinstall":"2018-12-13T20:09:40"},"type":"Extended color light","name":"Hue color downlight","modelid":"LCT002","manufacturername":"Philips","productname":"Hue color downlight","capabilities":{"certified":true,"control":{"mindimlevel":5000,"maxlumen":630,"colorgamuttype":"B","colorgamut":[[0.675,0.322],[0.409,0.518],[0.167,0.04]],"ct":{"min":153,"max":500}},"streaming":{"renderer":true,"proxy":false}},"config":{"archetype":"floodbulb","function":"mixed","direction":"downwards","startup":{"mode":"safety","configured":true}},"uniqueid":"00:17:88:01:ff:ff:ff:ff-ff","swversion":"5.127.1.26581"}`,
			capabilities: []adapter.CapabilityStateView{
				{
					Type: model.OnOffCapabilityType,
					State: model.OnOffCapabilityState{
						Instance: model.OnOnOffCapabilityInstance,
						Value:    true,
					},
				},
				{
					Type: model.RangeCapabilityType,
					State: model.RangeCapabilityState{
						Instance: model.BrightnessRangeInstance,
						Value:    100,
					},
				},
				{
					Type: model.ColorSettingCapabilityType,
					State: model.ColorSettingCapabilityState{
						Instance: model.TemperatureKCapabilityInstance,
						Value:    model.TemperatureK(2732),
					},
				},
			},
		},
		{
			name:         "Extended color light: Hue color lamp (LCA001)",
			rawLightInfo: `{"state":{"on":false,"bri":137,"hue":8418,"sat":140,"effect":"none","xy":[0.7006,0.2993],"ct":366,"alert":"select","colormode":"xy","mode":"homeautomation","reachable":false},"swupdate":{"state":"noupdates","lastinstall":"2019-06-27T10:08:08"},"type":"Extended color light","name":"Hue color lamp","modelid":"LCA001","manufacturername":"Philips","productname":"Hue color lamp","capabilities":{"certified":true,"control":{"mindimlevel":200,"maxlumen":800,"colorgamuttype":"C","colorgamut":[[0.6915,0.3083],[0.17,0.7],[0.1532,0.0475]],"ct":{"min":153,"max":500}},"streaming":{"renderer":true,"proxy":true}},"config":{"archetype":"sultanbulb","function":"mixed","direction":"omnidirectional","startup":{"mode":"safety","configured":true}},"uniqueid":"00:17:88:01:ff:ff:ff:ff-ff","swversion":"1.53.3_r27175","swconfigid":"297C5CDD","productid":"Philips-LCA001-5-A19ECLv6"}`,
			capabilities: []adapter.CapabilityStateView{
				{
					Type: model.OnOffCapabilityType,
					State: model.OnOffCapabilityState{
						Instance: model.OnOnOffCapabilityInstance,
						Value:    false,
					},
				},
				{
					Type: model.RangeCapabilityType,
					State: model.RangeCapabilityState{
						Instance: model.BrightnessRangeInstance,
						Value:    54,
					},
				},
				// TODO: color was skipped, because colormode=xy is not supported
			},
		},
		{
			name:         "Extended color light: Hue color lamp (LCT007)",
			rawLightInfo: `{"state":{"on":true,"bri":254,"hue":14988,"sat":141,"effect":"none","xy":[0.4575,0.4101],"ct":366,"alert":"lselect","colormode":"ct","mode":"homeautomation","reachable":false},"swupdate":{"state":"noupdates","lastinstall":"2019-01-12T12:31:48"},"type":"Extended color light","name":"Hue color lamp","modelid":"LCT007","manufacturername":"Philips","productname":"Hue color lamp","capabilities":{"certified":true,"control":{"mindimlevel":2000,"maxlumen":800,"colorgamuttype":"B","colorgamut":[[0.675,0.322],[0.409,0.518],[0.167,0.04]],"ct":{"min":153,"max":500}},"streaming":{"renderer":true,"proxy":true}},"config":{"archetype":"sultanbulb","function":"mixed","direction":"omnidirectional","startup":{"mode":"safety","configured":true}},"uniqueid":"00:17:88:01:ff:ff:ff:ff-ff","swversion":"5.127.1.26420"}`,
			capabilities: []adapter.CapabilityStateView{
				{
					Type: model.OnOffCapabilityType,
					State: model.OnOffCapabilityState{
						Instance: model.OnOnOffCapabilityInstance,
						Value:    true,
					},
				},
				{
					Type: model.RangeCapabilityType,
					State: model.RangeCapabilityState{
						Instance: model.BrightnessRangeInstance,
						Value:    100,
					},
				},
				{
					Type: model.ColorSettingCapabilityType,
					State: model.ColorSettingCapabilityState{
						Instance: model.TemperatureKCapabilityInstance,
						Value:    model.TemperatureK(2732),
					},
				},
			},
		},
		{
			name:         "Extended color light: Hue color lamp (LCT010)",
			rawLightInfo: `{"state":{"on":false,"bri":254,"hue":8402,"sat":140,"effect":"none","xy":[0.4575,0.4099],"ct":366,"alert":"none","colormode":"xy","mode":"homeautomation","reachable":true},"swupdate":{"state":"noupdates","lastinstall":"2018-12-09T12:55:12"},"type":"Extended color light","name":"Hue color lamp","modelid":"LCT010","manufacturername":"Philips","productname":"Hue color lamp","capabilities":{"certified":true,"control":{"mindimlevel":1000,"maxlumen":806,"colorgamuttype":"C","colorgamut":[[0.6915,0.3083],[0.17,0.7],[0.1532,0.0475]],"ct":{"min":153,"max":500}},"streaming":{"renderer":true,"proxy":true}},"config":{"archetype":"sultanbulb","function":"mixed","direction":"omnidirectional","startup":{"mode":"powerfail","configured":true}},"uniqueid":"00:17:88:01:ff:ff:ff:ff-ff","swversion":"1.46.13_r26312","swconfigid":"0CE67A8F","productid":"Philips-LCT010-1-A19ECLv4"}`,
			capabilities: []adapter.CapabilityStateView{
				{
					Type: model.OnOffCapabilityType,
					State: model.OnOffCapabilityState{
						Instance: model.OnOnOffCapabilityInstance,
						Value:    false,
					},
				},
				{
					Type: model.RangeCapabilityType,
					State: model.RangeCapabilityState{
						Instance: model.BrightnessRangeInstance,
						Value:    100,
					},
				},
				// TODO: color was skipped, because colormode=xy is not supported
			},
		},
		{
			name:         "Extended color light: Hue color lamp (LCT015)",
			rawLightInfo: `{"state":{"on":false,"bri":144,"hue":7676,"sat":199,"effect":"none","xy":[0.5016,0.4151],"ct":443,"alert":"select","colormode":"xy","mode":"homeautomation","reachable":true},"swupdate":{"state":"noupdates","lastinstall":"2018-12-05T13:56:33"},"type":"Extended color light","name":"Hue color lamp","modelid":"LCT015","manufacturername":"Philips","productname":"Hue color lamp","capabilities":{"certified":true,"control":{"mindimlevel":1000,"maxlumen":806,"colorgamuttype":"C","colorgamut":[[0.6915,0.3083],[0.17,0.7],[0.1532,0.0475]],"ct":{"min":153,"max":500}},"streaming":{"renderer":true,"proxy":true}},"config":{"archetype":"sultanbulb","function":"mixed","direction":"omnidirectional","startup":{"mode":"safety","configured":true}},"uniqueid":"00:17:88:01:ff:ff:ff:ff-ff","swversion":"1.46.13_r26312","swconfigid":"52E3234B","productid":"Philips-LCT015-1-A19ECLv5"}`,
			capabilities: []adapter.CapabilityStateView{
				{
					Type: model.OnOffCapabilityType,
					State: model.OnOffCapabilityState{
						Instance: model.OnOnOffCapabilityInstance,
						Value:    false,
					},
				},
				{
					Type: model.RangeCapabilityType,
					State: model.RangeCapabilityState{
						Instance: model.BrightnessRangeInstance,
						Value:    56,
					},
				},
				// TODO: color was skipped, because colormode=xy is not supported
			},
		},
		{
			name:         "Extended color light: Hue color lamp (LCT016)",
			rawLightInfo: `{"state":{"on":true,"bri":254,"hue":8418,"sat":140,"effect":"none","xy":[0.4573,0.41],"ct":366,"alert":"select","colormode":"ct","mode":"homeautomation","reachable":true},"swupdate":{"state":"noupdates","lastinstall":"2019-04-22T11:42:14"},"type":"Extended color light","name":"Hue color lamp","modelid":"LCT016","manufacturername":"Philips","productname":"Hue color lamp","capabilities":{"certified":true,"control":{"mindimlevel":1000,"maxlumen":800,"colorgamuttype":"C","colorgamut":[[0.6915,0.3083],[0.17,0.7],[0.1532,0.0475]],"ct":{"min":153,"max":500}},"streaming":{"renderer":true,"proxy":true}},"config":{"archetype":"sultanbulb","function":"mixed","direction":"omnidirectional","startup":{"mode":"safety","configured":true}},"uniqueid":"00:17:88:01:ff:ff:ff:ff-ff","swversion":"1.46.13_r26312","swconfigid":"9DC82D22","productid":"Philips-LCT016-1-A19ECLv5"}`,
			capabilities: []adapter.CapabilityStateView{
				{
					Type: model.OnOffCapabilityType,
					State: model.OnOffCapabilityState{
						Instance: model.OnOnOffCapabilityInstance,
						Value:    true,
					},
				},
				{
					Type: model.RangeCapabilityType,
					State: model.RangeCapabilityState{
						Instance: model.BrightnessRangeInstance,
						Value:    100,
					},
				},
				{
					Type: model.ColorSettingCapabilityType,
					State: model.ColorSettingCapabilityState{
						Instance: model.TemperatureKCapabilityInstance,
						Value:    model.TemperatureK(2732),
					},
				},
			},
		},
		{
			name:         "Extended color light: Hue color module (LLM001)",
			rawLightInfo: `{"state":{"on":false,"bri":137,"hue":0,"sat":0,"effect":"none","xy":[0.7006,0.2993],"ct":447,"alert":"none","colormode":"xy","mode":"homeautomation","reachable":false},"swupdate":{"state":"transferring","lastinstall":"2018-02-01T10:15:58"},"type":"Extended color light","name":"Hue color module","modelid":"LLM001","manufacturername":"Philips","productname":"Hue color module","capabilities":{"certified":true,"control":{"mindimlevel":5000,"maxlumen":300,"colorgamuttype":"B","colorgamut":[[0.675,0.322],[0.409,0.518],[0.167,0.04]],"ct":{"min":153,"max":500}},"streaming":{"renderer":true,"proxy":true}},"config":{"archetype":"tableshade","function":"decorative","direction":"downwards"},"uniqueid":"00:17:88:01:ff:ff:ff:ff-ff","luminaireuniqueid":"00:e3:c3:c8-02-00","swversion":"5.105.0.21536"}`,
			capabilities: []adapter.CapabilityStateView{
				{
					Type: model.OnOffCapabilityType,
					State: model.OnOffCapabilityState{
						Instance: model.OnOnOffCapabilityInstance,
						Value:    false,
					},
				},
				{
					Type: model.RangeCapabilityType,
					State: model.RangeCapabilityState{
						Instance: model.BrightnessRangeInstance,
						Value:    54,
					},
				},
				// TODO: color was skipped, because colormode=xy is not supported
			},
		},
		{
			name:         "Extended color light: Hue color spot (LCT003)",
			rawLightInfo: `{"state":{"on":false,"bri":254,"hue":49128,"sat":244,"effect":"none","xy":[0.2289,0.083],"ct":153,"alert":"none","colormode":"xy","mode":"homeautomation","reachable":true},"swupdate":{"state":"noupdates","lastinstall":"2019-04-06T21:05:18"},"type":"Extended color light","name":"Hue color spot","modelid":"LCT003","manufacturername":"Philips","productname":"Hue color spot","capabilities":{"certified":true,"control":{"mindimlevel":5000,"maxlumen":250,"colorgamuttype":"B","colorgamut":[[0.675,0.322],[0.409,0.518],[0.167,0.04]],"ct":{"min":153,"max":500}},"streaming":{"renderer":true,"proxy":true}},"config":{"archetype":"spotbulb","function":"mixed","direction":"downwards","startup":{"mode":"safety","configured":true}},"uniqueid":"00:17:88:01:ff:ff:ff:ff-ff","swversion":"5.127.1.26420"}`,
			capabilities: []adapter.CapabilityStateView{
				{
					Type: model.OnOffCapabilityType,
					State: model.OnOffCapabilityState{
						Instance: model.OnOnOffCapabilityInstance,
						Value:    false,
					},
				},
				{
					Type: model.RangeCapabilityType,
					State: model.RangeCapabilityState{
						Instance: model.BrightnessRangeInstance,
						Value:    100,
					},
				},
				// TODO: color was skipped, because colormode=xy is not supported
			},
		},
		{
			name:         "Extended color light: Hue go (LLC020)",
			rawLightInfo: `{"state":{"on":false,"bri":144,"hue":7676,"sat":199,"effect":"none","xy":[0.5016,0.4151],"ct":443,"alert":"none","colormode":"xy","mode":"homeautomation","reachable":true},"swupdate":{"state":"noupdates","lastinstall":"2018-12-12T13:39:18"},"type":"Extended color light","name":"Hue go","modelid":"LLC020","manufacturername":"Philips","productname":"Hue go","capabilities":{"certified":true,"control":{"mindimlevel":40,"maxlumen":300,"colorgamuttype":"C","colorgamut":[[0.6915,0.3083],[0.17,0.7],[0.1532,0.0475]],"ct":{"min":153,"max":500}},"streaming":{"renderer":true,"proxy":true}},"config":{"archetype":"huego","function":"decorative","direction":"omnidirectional","startup":{"mode":"safety","configured":true}},"uniqueid":"00:17:88:01:ff:ff:ff:ff-ff","swversion":"5.127.1.26420"}`,
			capabilities: []adapter.CapabilityStateView{
				{
					Type: model.OnOffCapabilityType,
					State: model.OnOffCapabilityState{
						Instance: model.OnOnOffCapabilityInstance,
						Value:    false,
					},
				},
				{
					Type: model.RangeCapabilityType,
					State: model.RangeCapabilityState{
						Instance: model.BrightnessRangeInstance,
						Value:    56,
					},
				},
				// TODO: color was skipped, because colormode=xy is not supported
			},
		},
		{
			name:         "Extended color light: Hue lightstrip plus (LST002)",
			rawLightInfo: `{"state":{"on":false,"bri":144,"hue":7676,"sat":199,"effect":"none","xy":[0.5016,0.4151],"ct":443,"alert":"select","colormode":"xy","mode":"homeautomation","reachable":true},"swupdate":{"state":"noupdates","lastinstall":"2018-12-12T13:39:21"},"type":"Extended color light","name":"Hue lightstrip plus","modelid":"LST002","manufacturername":"Philips","productname":"Hue lightstrip plus","capabilities":{"certified":true,"control":{"mindimlevel":25,"maxlumen":1600,"colorgamuttype":"C","colorgamut":[[0.6915,0.3083],[0.17,0.7],[0.1532,0.0475]],"ct":{"min":153,"max":500}},"streaming":{"renderer":true,"proxy":true}},"config":{"archetype":"huelightstrip","function":"mixed","direction":"omnidirectional","startup":{"mode":"safety","configured":true}},"uniqueid":"00:17:88:01:ff:ff:ff:ff-ff","swversion":"5.127.1.26420"}`,
			capabilities: []adapter.CapabilityStateView{
				{
					Type: model.OnOffCapabilityType,
					State: model.OnOffCapabilityState{
						Instance: model.OnOnOffCapabilityInstance,
						Value:    false,
					},
				},
				{
					Type: model.RangeCapabilityType,
					State: model.RangeCapabilityState{
						Instance: model.BrightnessRangeInstance,
						Value:    56,
					},
				},
				// TODO: color was skipped, because colormode=xy is not supported
			},
		},
		{
			name:         "Extended color light: Hue play (LCT024)",
			rawLightInfo: `{"state":{"on":false,"bri":224,"hue":45774,"sat":254,"effect":"none","xy":[0.1542,0.0879],"ct":153,"alert":"none","colormode":"xy","mode":"homeautomation","reachable":true},"swupdate":{"state":"noupdates","lastinstall":"2018-12-20T20:05:04"},"type":"Extended color light","name":"Hue play","modelid":"LCT024","manufacturername":"Philips","productname":"Hue play","capabilities":{"certified":true,"control":{"mindimlevel":500,"maxlumen":540,"colorgamuttype":"C","colorgamut":[[0.6915,0.3083],[0.17,0.7],[0.1532,0.0475]],"ct":{"min":153,"max":500}},"streaming":{"renderer":true,"proxy":true}},"config":{"archetype":"hueplay","function":"decorative","direction":"upwards","startup":{"mode":"powerfail","configured":true}},"uniqueid":"00:17:88:01:ff:ff:ff:ff-ff","swversion":"1.46.13_r26312","swconfigid":"869E2FE2","productid":"3241-3127-7871-LS00"}`,
			capabilities: []adapter.CapabilityStateView{
				{
					Type: model.OnOffCapabilityType,
					State: model.OnOffCapabilityState{
						Instance: model.OnOnOffCapabilityInstance,
						Value:    false,
					},
				},
				{
					Type: model.RangeCapabilityType,
					State: model.RangeCapabilityState{
						Instance: model.BrightnessRangeInstance,
						Value:    88,
					},
				},
				// TODO: color was skipped, because colormode=xy is not supported
			},
		},
		{
			name:         "On/Off plug-in unit: Extended color light (GLEDOPTO)",
			rawLightInfo: `{"state":{"on":false,"alert":"select","mode":"homeautomation","reachable":false},"swupdate":{"state":"notupdatable","lastinstall":null},"type":"On/Off plug-in unit","name":"Extended color light","modelid":"GLEDOPTO","manufacturername":"GLEDOPTO","productname":"Extended color light","capabilities":{"certified":false,"control":{"colorgamuttype":"other","ct":{"min":0,"max":65535}},"streaming":{"renderer":false,"proxy":false}},"config":{"archetype":"huelightstrip","function":"mixed","direction":"omnidirectional"},"uniqueid":"00:12:4b:00:ff:ff:ff:ff-ff","swversion":"1.0.2"}`,
			capabilities: []adapter.CapabilityStateView{
				{
					Type: model.OnOffCapabilityType,
					State: model.OnOffCapabilityState{
						Instance: model.OnOnOffCapabilityInstance,
						Value:    false,
					},
				},
			},
		},
		{
			name:         "On/Off plug-in unit: Hue Smart plug (LOM001)",
			rawLightInfo: `{"state":{"on":false,"alert":"none","mode":"homeautomation","reachable":true},"swupdate":{"state":"noupdates","lastinstall":"2019-08-15T12:15:29"},"type":"On/Off plug-in unit","name":"Hue Smart plug","modelid":"LOM001","manufacturername":"Philips","productname":"Hue Smart plug","capabilities":{"certified":true,"control":{"mindimlevel":5000},"streaming":{"renderer":false,"proxy":false}},"config":{"archetype":"plug","function":"functional","direction":"omnidirectional","startup":{"mode":"safety","configured":true}},"uniqueid":"00:17:88:01:ff:ff:ff:ff-ff","swversion":"1.55.7_r28462","swconfigid":"68918527","productid":"SmartPlug_OnOff_v01-00_01"}`,
			capabilities: []adapter.CapabilityStateView{
				{
					Type: model.OnOffCapabilityType,
					State: model.OnOffCapabilityState{
						Instance: model.OnOnOffCapabilityInstance,
						Value:    false,
					},
				},
			},
		},
		{
			name:         "On/Off plug-in unit: On/Off plug (Plug 01)",
			rawLightInfo: `{"state":{"on":false,"alert":"select","mode":"homeautomation","reachable":true},"swupdate":{"state":"notupdatable","lastinstall":null},"type":"On/Off plug-in unit","name":"On/Off plug","modelid":"Plug 01","manufacturername":"OSRAM","productname":"On/Off plug","capabilities":{"certified":false,"control":{},"streaming":{"renderer":false,"proxy":false}},"config":{"archetype":"recessedfloor","function":"functional","direction":"omnidirectional"},"uniqueid":"7c:b0:3e:aa:ff:ff:ff:ff-ff","swversion":"V1.04.12"}`,
			capabilities: []adapter.CapabilityStateView{
				{
					Type: model.OnOffCapabilityType,
					State: model.OnOffCapabilityState{
						Instance: model.OnOnOffCapabilityInstance,
						Value:    false,
					},
				},
			},
		},
		{
			name:         "On/Off plug-in unit: On/Off plug (SP 120)",
			rawLightInfo: `{"state":{"on":false,"alert":"select","mode":"homeautomation","reachable":true},"swupdate":{"state":"notupdatable","lastinstall":"2019-03-29T18:17:18"},"type":"On/Off plug-in unit","name":"On/Off plug","modelid":"SP 120","manufacturername":"innr","productname":"On/Off plug","capabilities":{"certified":false,"control":{},"streaming":{"renderer":false,"proxy":false}},"config":{"archetype":"recessedfloor","function":"functional","direction":"omnidirectional"},"uniqueid":"00:15:8d:00:ff:ff:ff:ff-ff","swversion":"2.0"}`,
			capabilities: []adapter.CapabilityStateView{
				{
					Type: model.OnOffCapabilityType,
					State: model.OnOffCapabilityState{
						Instance: model.OnOnOffCapabilityInstance,
						Value:    false,
					},
				},
			},
		},
	}

	for _, tc := range cases {
		t.Run(tc.name, func(t *testing.T) {
			var lightInfo LightInfo
			err := json.Unmarshal([]byte(tc.rawLightInfo), &lightInfo)
			if err != nil {
				t.Fatal(xerrors.Errorf("rawLightInfo unmarshal failed: %w", err))
			}

			actualCapabilityStates := lightInfo.ToCapabilityStateViews()

			assert.Equal(t, tc.capabilities, actualCapabilityStates)
		})
	}
}

func TestProcessActionRequestAndReverse(t *testing.T) {
	// prepare testing lamp
	lamp := LightInfo{
		State: LightState{
			On:        tools.AOB(true),
			Bri:       tools.AOUI8(255),
			Reachable: tools.AOB(true),
		},
	}

	// check that after action state of lamp is set
	for i := 1; i <= 100; i++ {
		// actions by bulbasaur
		actions := []adapter.CapabilityActionView{
			{
				Type: model.RangeCapabilityType,
				State: model.RangeCapabilityState{
					Instance: model.BrightnessRangeInstance,
					Value:    float64(i),
				},
			},
		}
		_, _, changeRequest := lamp.ProcessActionRequests(actions)

		lamp.State.Bri = changeRequest.Bri
		lampCapabilities := lamp.ToCapabilityStateViews()

		// zero index is on/off capability
		assert.Equal(t, actions[0].State.(model.RangeCapabilityState).Value, lampCapabilities[1].State.(model.RangeCapabilityState).Value)
	}
}
