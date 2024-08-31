package mobile

import (
	"fmt"
	"strings"
	"testing"

	"github.com/stretchr/testify/assert"

	"a.yandex-team.ru/alice/iot/bulbasaur/model"
	"a.yandex-team.ru/alice/library/go/inflector"
)

func TestActionSuggestionsFromCapabilities(t *testing.T) {
	hsvModelType := model.HsvModelType
	someDeviceInflection := inflector.Inflection{
		Im:   "устройство",
		Rod:  "устройства",
		Dat:  "устройству",
		Vin:  "устройство",
		Tvor: "устройством",
		Pr:   "устройстве",
	}
	lightInflection := inflector.Inflection{
		Im:   "свет",
		Rod:  "света",
		Dat:  "свету",
		Vin:  "свет",
		Tvor: "светом",
		Pr:   "свете",
	}
	kettleInflection := inflector.Inflection{
		Im:   "чайник",
		Rod:  "чайника",
		Dat:  "чайнику",
		Vin:  "чайник",
		Tvor: "чайником",
		Pr:   "чайнике",
	}
	coffeeMakerInflection := inflector.Inflection{
		Im:   "кофеварка",
		Rod:  "кофеварки",
		Dat:  "кофеварке",
		Vin:  "кофеварку",
		Tvor: "кофеваркой",
		Pr:   "кофеварке",
	}
	heatedFloorInflection := inflector.Inflection{
		Im:   "теплый пол",
		Rod:  "теплого пола",
		Dat:  "теплому полу",
		Vin:  "теплый пол",
		Tvor: "теплым полом",
		Pr:   "теплом поле",
	}
	curtainInflection := inflector.Inflection{
		Im:   "шторы",
		Rod:  "штор",
		Dat:  "шторам",
		Vin:  "шторы",
		Tvor: "шторами",
		Pr:   "шторах",
	}
	carInflection := inflector.Inflection{
		Im:   "машина",
		Rod:  "машины",
		Dat:  "машине",
		Vin:  "машину",
		Tvor: "машиной",
		Pr:   "машине",
	}
	customControlInflection := inflector.Inflection{
		Im:   "пекарь",
		Rod:  "пекаря",
		Dat:  "пекарю",
		Vin:  "пекаря",
		Tvor: "пекарем",
		Pr:   "пекаре",
	}
	tests := []struct {
		name         string
		expectations []string
		deviceType   model.DeviceType
		capabilities []model.ICapability
		inflection   inflector.Inflection
	}{
		{
			"Test OnOff",
			[]string{
				"включи устройство",
				"выключи устройство",
			},
			model.OtherDeviceType,
			[]model.ICapability{
				model.MakeCapabilityByType(model.OnOffCapabilityType),
			},
			someDeviceInflection,
		},
		{
			"Test OnOff For LightDeviceType",
			[]string{
				"включи свет",
				"включи устройство",
				"выключи свет",
				"выключи устройство",
			},
			model.LightDeviceType,
			[]model.ICapability{
				model.MakeCapabilityByType(model.OnOffCapabilityType),
			},
			someDeviceInflection,
		},
		{
			"Test OnOff For LightDeviceType named \"свет\"",
			[]string{
				"включи свет",
				"выключи свет",
			},
			model.LightDeviceType,
			[]model.ICapability{
				model.MakeCapabilityByType(model.OnOffCapabilityType),
			},
			lightInflection,
		},
		{
			"Test Brightness",
			[]string{
				"прибавь яркость для устройства",
				"увеличь яркость на устройстве",
				"уменьши яркость для устройства",
				"убавь яркость на устройстве",
				"включи яркость устройства 20 процентов",
				"установи яркость устройства в 40 процентов",
				"включи яркость устройства на максимум",
				"включи максимальную яркость устройства",
				"включи яркость устройства на минимум",
				"включи минимальную яркость для устройства",
			},
			model.OtherDeviceType,
			[]model.ICapability{
				model.NewCapability(model.RangeCapabilityType).
					WithParameters(model.RangeCapabilityParameters{
						Instance:     model.BrightnessRangeInstance,
						Range:        &model.Range{Min: 1, Max: 100, Precision: 1},
						RandomAccess: true,
						Unit:         model.UnitPercent,
					}),
			},
			someDeviceInflection,
		},
		{
			"Test Brightness For LightDeviceType",
			[]string{
				"прибавь яркость света",
				"прибавь яркость для устройства",
				"увеличь яркость на устройстве",
				"убавь яркость света",
				"уменьши яркость для устройства",
				"убавь яркость на устройстве",
				"включи яркость света 20 процентов",
				"установи яркость света в 40 процентов",
				"включи яркость устройства 20 процентов",
				"установи яркость устройства в 40 процентов",
				"включи яркость света на максимум",
				"включи максимальную яркость света",
				"включи яркость устройства на максимум",
				"включи максимальную яркость устройства",
				"включи яркость света на минимум",
				"включи минимальную яркость света",
				"включи яркость устройства на минимум",
				"включи минимальную яркость для устройства",
			},
			model.LightDeviceType,
			[]model.ICapability{
				model.NewCapability(model.RangeCapabilityType).
					WithParameters(model.RangeCapabilityParameters{
						Instance:     model.BrightnessRangeInstance,
						Range:        &model.Range{Min: 1, Max: 100, Precision: 1},
						RandomAccess: true,
						Unit:         model.UnitPercent,
					}),
			},
			someDeviceInflection,
		},
		{
			"Test Brightness For LightDeviceType named \"свет\"",
			[]string{
				"прибавь яркость света",
				"прибавь яркость для света",
				"увеличь яркость на свете",
				"убавь яркость света",
				"уменьши яркость для света",
				"убавь яркость на свете",
				"включи яркость света 20 процентов",
				"установи яркость света в 40 процентов",
				"включи яркость света на максимум",
				"включи максимальную яркость света",
				"включи яркость света на минимум",
				"включи минимальную яркость для света",
			},
			model.LightDeviceType,
			[]model.ICapability{
				model.NewCapability(model.RangeCapabilityType).
					WithParameters(model.RangeCapabilityParameters{
						Instance:     model.BrightnessRangeInstance,
						Range:        &model.Range{Min: 1, Max: 100, Precision: 1},
						RandomAccess: true,
						Unit:         model.UnitPercent,
					}),
			},
			lightInflection,
		},
		{
			"Test Volume",
			[]string{
				"сделай устройство погромче",
				"прибавь звук на устройстве",
				"сделай устройство потише",
				"убавь звук на устройстве",
			},
			model.OtherDeviceType,
			[]model.ICapability{
				model.NewCapability(model.RangeCapabilityType).
					WithParameters(model.RangeCapabilityParameters{
						Instance: model.VolumeRangeInstance,
					}),
			},
			someDeviceInflection,
		},
		{
			"Test Channel",
			[]string{
				"включи следующий канал на устройстве",
				"переключи на следующий канал устройства",
				"включи предыдущий канал на устройстве",
				"переключи на предыдущий канал устройства",
			},
			model.TvDeviceDeviceType,
			[]model.ICapability{
				model.NewCapability(model.RangeCapabilityType).
					WithParameters(model.RangeCapabilityParameters{
						Instance: model.ChannelRangeInstance,
					}),
			},
			someDeviceInflection,
		},
		{
			"Test Channel Random Access",
			[]string{
				"включи следующий канал на устройстве",
				"переключи на следующий канал устройства",
				"включи предыдущий канал на устройстве",
				"переключи на предыдущий канал устройства",
				"включи 5 канал на устройстве",
				"переключи на 2 канал на устройстве",
				"включи на устройстве 17 канал",
			},
			model.TvDeviceDeviceType,
			[]model.ICapability{
				model.NewCapability(model.RangeCapabilityType).
					WithParameters(model.RangeCapabilityParameters{
						Instance:     model.ChannelRangeInstance,
						RandomAccess: true,
					}),
			},
			someDeviceInflection,
		},
		{
			"Test Thermostat Mode",
			[]string{
				"включи эко режим на устройстве",
				"поставь эко режим на устройстве",
				"включи авто режим на устройстве",
				"поставь авто режим на устройстве",
				"включи режим вентиляции на устройстве",
				"поставь режим вентиляции на устройстве",
				"включи режим охлаждения на устройстве",
				"поставь режим охлаждения на устройстве",
				"включи режим обогрева на устройстве",
				"поставь режим обогрева на устройстве",
				"включи режим windfree на устройстве",
				"поставь режим windfree на устройстве",
				"включи следующий режим работы устройства",
				"поставь предыдущий режим работы устройства",
			},
			model.OtherDeviceType,
			[]model.ICapability{
				model.NewCapability(model.ModeCapabilityType).
					WithParameters(model.ModeCapabilityParameters{
						Instance: model.ThermostatModeInstance,
						Modes: []model.Mode{
							{Value: model.EcoMode},
							{Value: model.AutoMode},
							{Value: model.FanOnlyMode},
							{Value: model.CoolMode},
							{Value: model.HeatMode},
							{Value: model.WindFreeMode},
						}}),
			},
			someDeviceInflection,
		},
		{
			"Test Multicooker program Mode",
			[]string{
				"включи режим плов на устройстве",
				"поставь режим плов на устройстве",
				"включи режим холодец на устройстве",
				"поставь режим холодец на устройстве",
				"включи режим макароны на устройстве",
				"поставь режим макароны на устройстве",
				"включи предыдущую программу устройства",
				"поставь следующую программу на устройстве",
			},
			model.MulticookerDeviceType,
			[]model.ICapability{
				model.NewCapability(model.ModeCapabilityType).
					WithParameters(model.ModeCapabilityParameters{
						Instance: model.ProgramModeInstance,
						Modes: []model.Mode{
							{Value: model.PilafMode},
							{Value: model.AspicMode},
							{Value: model.MacaroniMode},
						}}),
			},
			someDeviceInflection,
		},
		{
			"Test Temperature",
			[]string{
				"прибавь температуру на устройстве",
				"увеличь температуру для устройства",
				"убавь температуру для устройства",
				"уменьши температуру устройства",
				"включи температуру устройства 30 градусов",
				"установи температуру устройства на 20 градусов",
				"включи максимальную температуру на устройстве",
				"включи температуру устройства на максимум",
				"включи минимальную температуру на устройстве",
				"включи температуру устройства на минимум",
			},
			model.OtherDeviceType,
			[]model.ICapability{
				model.NewCapability(model.RangeCapabilityType).
					WithParameters(model.RangeCapabilityParameters{
						Instance:     model.TemperatureRangeInstance,
						Range:        &model.Range{Min: 11, Max: 30, Precision: 1},
						RandomAccess: true,
						Unit:         model.UnitTemperatureCelsius,
					}),
			},
			someDeviceInflection,
		},
		{
			"Test FanSpeed Mode",
			[]string{
				"включи низкую скорость вентиляции на устройстве",
				"включи скорость вентиляции устройства на минимум",
				"включи среднюю скорость вентиляции на устройстве",
				"включи скорость вентиляции устройства на среднюю",
				"включи высокую скорость вентиляции на устройстве",
				"включи скорость вентиляции устройства на максимум",
				"включи предыдущую скорость вентиляции устройства",
				"поставь следующую скорость вентиляции на устройстве",
			},
			model.OtherDeviceType,
			[]model.ICapability{
				model.NewCapability(model.ModeCapabilityType).
					WithParameters(model.ModeCapabilityParameters{
						Instance: model.FanSpeedModeInstance,
						Modes: []model.Mode{
							{Value: model.AutoMode},
							{Value: model.LowMode},
							{Value: model.MediumMode},
							{Value: model.HighMode},
						},
					}),
			},
			someDeviceInflection,
		},
		{
			"Test Color",
			[]string{
				"сделай свет устройства потеплее",
				"включи свет потеплее для устройства",
				"сделай свет устройства похолоднее",
				"включи свет похолоднее для устройства",
				"измени цвет устройства на белый",
				"включи белый цвет для устройства",
				"измени цвет устройства на белый",
			},
			model.OtherDeviceType,
			[]model.ICapability{
				model.NewCapability(model.ColorSettingCapabilityType).
					WithParameters(model.ColorSettingCapabilityParameters{
						TemperatureK: &model.TemperatureKParameters{
							Min: 0,
							Max: 100,
						},
						ColorModel: &hsvModelType,
					}),
			},
			someDeviceInflection,
		},
		{
			"Test Color For LightDeviceType",
			[]string{
				"сделай свет потеплее",
				"сделай свет устройства потеплее",
				"включи свет потеплее для устройства",
				"сделай свет похолоднее",
				"сделай свет устройства похолоднее",
				"включи свет похолоднее для устройства",
				"включи белый свет",
				"измени цвет устройства на белый",
				"включи белый цвет для устройства",
				"измени цвет устройства на белый",
			},
			model.LightDeviceType,
			[]model.ICapability{
				model.NewCapability(model.ColorSettingCapabilityType).
					WithParameters(model.ColorSettingCapabilityParameters{
						TemperatureK: &model.TemperatureKParameters{
							Min: 0,
							Max: 100,
						},
						ColorModel: &hsvModelType,
					}),
			},
			someDeviceInflection,
		},
		{
			"Test Color For LightDeviceType named \"свет\"",
			[]string{
				"сделай свет потеплее",
				"сделай свет света потеплее",
				"включи свет потеплее для света",
				"сделай свет похолоднее",
				"сделай свет света похолоднее",
				"включи свет похолоднее для света",
				"включи белый свет",
				"измени цвет света на белый",
				"включи белый цвет для света",
				"измени цвет света на белый",
			},
			model.LightDeviceType,
			[]model.ICapability{
				model.NewCapability(model.ColorSettingCapabilityType).
					WithParameters(model.ColorSettingCapabilityParameters{
						TemperatureK: &model.TemperatureKParameters{
							Min: 0,
							Max: 100,
						},
						ColorModel: &hsvModelType,
					}),
			},
			lightInflection,
		},
		{
			"Test Mute",
			[]string{
				"выключи звук на устройстве",
				"выключи звук устройства",
				"включи звук на устройстве",
				"включи звук устройства",
			},
			model.OtherDeviceType,
			[]model.ICapability{
				model.NewCapability(model.ToggleCapabilityType).
					WithParameters(model.ToggleCapabilityParameters{
						Instance: model.MuteToggleCapabilityInstance,
					}),
			},
			someDeviceInflection,
		},
		{
			"Test Kettle Boiling",
			[]string{
				"поставь чайник",
				"вскипяти чайник",
				"нагрей чайник",
				"подогрей чайник",
				"грей чайник",
				"согрей чайник",
			},
			model.KettleDeviceType,
			[]model.ICapability{
				model.MakeCapabilityByType(model.OnOffCapabilityType),
			},
			kettleInflection,
		},
		{
			"Test Thermostat No Target Temperature",
			[]string{
				"прибавь температуру на устройстве",
				"увеличь температуру для устройства",
				"убавь температуру для устройства",
				"уменьши температуру устройства",
				"включи температуру устройства 20 градусов",
				"установи температуру устройства на 10 градусов",
				"включи максимальную температуру на устройстве",
				"включи температуру устройства на максимум",
				"включи минимальную температуру на устройстве",
				"включи температуру устройства на минимум",
				"прибавь температуру",
				"увеличь температуру",
				"убавь температуру",
				"уменьши температуру",
				"включи температуру 20 градусов",
				"установи температуру на 10 градусов",
				"включи максимальную температуру",
				"включи температуру на максимум",
				"включи минимальную температуру",
				"включи температуру на минимум",
				"сделай теплее",
				"сделай холоднее",
				"поставь температуру потеплее",
				"поставь температуру похолоднее",
				"сделай температуру похолоднее",
				"сделай температуру прохладнее",
			},
			model.ThermostatDeviceType,
			[]model.ICapability{
				model.NewCapability(model.RangeCapabilityType).
					WithParameters(model.RangeCapabilityParameters{
						Instance:     model.TemperatureRangeInstance,
						Range:        &model.Range{Min: 1, Max: 28, Precision: 1},
						RandomAccess: true,
						Unit:         model.UnitTemperatureCelsius,
					}),
			},
			someDeviceInflection,
		},
		{
			"Test Thermostat Mode No Target",
			[]string{
				"включи эко режим на устройстве",
				"поставь эко режим на устройстве",
				"включи авто режим на устройстве",
				"поставь авто режим на устройстве",
				"включи режим вентиляции на устройстве",
				"поставь режим вентиляции на устройстве",
				"включи режим охлаждения на устройстве",
				"поставь режим охлаждения на устройстве",
				"включи режим обогрева на устройстве",
				"поставь режим обогрева на устройстве",
				"включи следующий режим работы устройства",
				"поставь предыдущий режим работы устройства",
				"включи эко режим",
				"переключи в эко режим",
				"включи авто режим",
				"переключи в авто режим",
				"включи режим вентиляции",
				"переключи в режим вентиляции",
				"включи режим охлаждения",
				"переключи в режим охлаждения",
				"включи режим обогрева",
				"переключи в режим обогрева",
				"включи режим windfree",
				"переключи в режим windfree",
			},
			model.ThermostatDeviceType,
			[]model.ICapability{
				model.NewCapability(model.ModeCapabilityType).
					WithParameters(model.ModeCapabilityParameters{
						Instance: model.ThermostatModeInstance,
						Modes: []model.Mode{
							{Value: model.EcoMode},
							{Value: model.AutoMode},
							{Value: model.FanOnlyMode},
							{Value: model.CoolMode},
							{Value: model.HeatMode},
							{Value: model.WindFreeMode},
						},
					}),
			},
			someDeviceInflection,
		},
		{
			"Test Thermostat FanSpeed Mode No Target",
			[]string{
				"включи низкую скорость вентиляции на устройстве",
				"включи скорость вентиляции устройства на минимум",
				"включи среднюю скорость вентиляции на устройстве",
				"включи скорость вентиляции устройства на среднюю",
				"включи высокую скорость вентиляции на устройстве",
				"включи скорость вентиляции устройства на максимум",
				"включи предыдущую скорость вентиляции устройства",
				"поставь следующую скорость вентиляции на устройстве",
				"включи скорость вентиляции в авто",
				"включи скорость вентиляции на авто",
				"включи низкую скорость вентиляции",
				"включи скорость вентиляции на минимум",
				"включи среднюю скорость вентиляции",
				"включи скорость вентиляции на среднюю",
				"включи высокую скорость вентиляции",
				"включи скорость вентиляции на максимум",
			},
			model.ThermostatDeviceType,
			[]model.ICapability{
				model.NewCapability(model.ModeCapabilityType).
					WithParameters(model.ModeCapabilityParameters{
						Instance: model.FanSpeedModeInstance,
						Modes: []model.Mode{
							{Value: model.AutoMode},
							{Value: model.LowMode},
							{Value: model.MediumMode},
							{Value: model.HighMode},
						},
					}),
			},
			someDeviceInflection,
		},
		{
			"Test Humidifier",
			[]string{
				"выключи устройство",
				"выключи увлажнитель",
				"включи увлажнитель",
				"включи устройство",
				"включи увлажнитель воздуха",
				"выключи увлажнитель воздуха",
			},
			model.HumidifierDeviceType,
			[]model.ICapability{
				model.NewCapability(model.OnOffCapabilityType),
			},
			someDeviceInflection,
		},
		{
			"Test Purifier",
			[]string{
				"выключи устройство",
				"выключи очиститель",
				"включи очиститель",
				"включи устройство",
				"включи очиститель воздуха",
				"выключи очиститель воздуха",
			},
			model.PurifierDeviceType,
			[]model.ICapability{
				model.NewCapability(model.OnOffCapabilityType),
			},
			someDeviceInflection,
		},
		{
			"Test CoffeeMaker",
			[]string{
				"выключи устройство",
				"выключи кофеварку",
				"включи кофеварку",
				"включи устройство",
				"включи кофемашину",
				"выключи кофемашину",
				"сделай кофе",
				"свари кофе",
			},
			model.CoffeeMakerDeviceType,
			[]model.ICapability{
				model.NewCapability(model.OnOffCapabilityType),
			},
			someDeviceInflection,
		},
		{
			"Test VacuumCleaner",
			[]string{
				"включи устройство",
				"выключи устройство",
				"выключи робот-пылесос",
				"включи робот-пылесос",
				"включи пылесос",
				"выключи пылесос",
				"пропылесось",
				"верни пылесос на зарядку",
				"верни пылесос на базу",
			},
			model.VacuumCleanerDeviceType,
			[]model.ICapability{
				model.NewCapability(model.OnOffCapabilityType),
			},
			someDeviceInflection,
		},
		{
			"Test Backlight toggle",
			[]string{
				"выключи подсветку на устройстве",
				"выключи подсветку устройства",
				"включи подсветку на устройстве",
				"включи подсветку устройства",
			},
			model.OtherDeviceType,
			[]model.ICapability{
				model.NewCapability(model.ToggleCapabilityType).
					WithParameters(model.ToggleCapabilityParameters{
						Instance: model.BacklightToggleCapabilityInstance,
					}),
			},
			someDeviceInflection,
		},
		{
			"Test Keep warm toggle",
			[]string{
				"выключи поддержку температуры на устройстве",
				"выключи поддержку температуры устройства",
				"включи поддержку температуры на устройстве",
				"включи поддержку температуры устройства",
				"выключи поддержание тепла на устройстве",
				"выключи поддержание тепла устройства",
				"включи поддержание тепла на устройстве",
				"включи поддержание тепла устройства",
				"поддерживай тепло на устройстве",
				"не поддерживай тепло на устройстве",
				"поддерживай температуру устройства",
				"не поддерживай температуру устройства",
			},
			model.OtherDeviceType,
			[]model.ICapability{
				model.NewCapability(model.ToggleCapabilityType).
					WithParameters(model.ToggleCapabilityParameters{
						Instance: model.KeepWarmToggleCapabilityInstance,
					}),
			},
			someDeviceInflection,
		},
		{
			"Test Oscillation toggle",
			[]string{
				"выключи вращение на устройстве",
				"выключи вращение устройства",
				"включи вращение на устройстве",
				"включи вращение устройства",
			},
			model.OtherDeviceType,
			[]model.ICapability{
				model.NewCapability(model.ToggleCapabilityType).
					WithParameters(model.ToggleCapabilityParameters{
						Instance: model.OscillationToggleCapabilityInstance,
					}),
			},
			someDeviceInflection,
		},
		{
			"Test Ionization toggle",
			[]string{
				"выключи ионизатор на устройстве",
				"выключи ионизатор устройства",
				"включи ионизатор на устройстве",
				"включи ионизатор устройства",
				"выключи ионизацию на устройстве",
				"выключи ионизацию устройства",
				"включи ионизацию на устройстве",
				"включи ионизацию устройства",
				"выключи ионизирование воздуха на устройстве",
				"выключи ионизирование воздуха устройства",
				"включи ионизирование воздуха на устройстве",
				"включи ионизирование воздуха устройства",
			},
			model.OtherDeviceType,
			[]model.ICapability{
				model.NewCapability(model.ToggleCapabilityType).
					WithParameters(model.ToggleCapabilityParameters{
						Instance: model.IonizationToggleCapabilityInstance,
					}),
			},
			someDeviceInflection,
		},
		{
			"Test Child controls toggle",
			[]string{
				"выключи детский режим на устройстве",
				"выключи детский режим устройства",
				"включи детский режим на устройстве",
				"включи детский режим устройства",
				"заблокируй управление устройства",
				"разблокируй управление устройства",
				"разблокируй управление на устройстве",
				"заблокируй управление на устройстве",
			},
			model.OtherDeviceType,
			[]model.ICapability{
				model.NewCapability(model.ToggleCapabilityType).
					WithParameters(model.ToggleCapabilityParameters{
						Instance: model.ControlsLockedToggleCapabilityInstance,
					}),
			},
			someDeviceInflection,
		},
		{
			"Test Humidity Range",
			[]string{
				"прибавь влажность на устройстве",
				"увеличь влажность на устройстве",
				"убавь влажность на устройстве",
				"уменьши влажность устройства",
				"установи влажность устройства на 99 процентов",
				"поставь максимальную влажность на устройстве",
				"установи влажность устройства на минимум",
			},
			model.HumidifierDeviceType,
			[]model.ICapability{
				model.NewCapability(model.RangeCapabilityType).
					WithParameters(model.RangeCapabilityParameters{
						Instance:     model.HumidityRangeInstance,
						Unit:         model.UnitPercent,
						RandomAccess: true,
						Looped:       false,
						Range: &model.Range{
							Min:       20,
							Max:       99,
							Precision: 1,
						},
					}),
			},
			someDeviceInflection,
		},
		{
			"Test Pause toggle",
			[]string{
				"выключи паузу на устройстве",
				"включи паузу на устройстве",
				"поставь паузу на устройстве",
				"поставь устройство на паузу",
				"сними устройство с паузы",
				"приостанови устройство",
				"поставь паузу на устройстве",
			},
			model.OtherDeviceType,
			[]model.ICapability{
				model.NewCapability(model.ToggleCapabilityType).
					WithParameters(model.ToggleCapabilityParameters{
						Instance: model.PauseToggleCapabilityInstance,
					}),
			},
			someDeviceInflection,
		},
		{
			"Test Coffee Mode Instance",
			[]string{
				"поставь режим латте на кофеварке",
				"включи режим латте на кофеварке",
				"поставь режим эспрессо на кофеварке",
				"включи режим эспрессо на кофеварке",
				"включи предыдущий кофе кофеварки",
				"поставь следующий кофе на кофеварке",
			},
			model.CoffeeMakerDeviceType,
			[]model.ICapability{
				model.NewCapability(model.ModeCapabilityType).
					WithParameters(model.ModeCapabilityParameters{
						Instance: model.CoffeeModeInstance,
						Modes: []model.Mode{
							{Value: model.LatteMode},
							{Value: model.EspressoMode},
						},
					}),
			},
			coffeeMakerInflection,
		},
		{
			"Test Swing Mode Instance",
			[]string{
				"поставь горизонтальное направление воздуха на устройстве",
				"включи горизонтальное направление воздуха на устройстве",
				"поставь вертикальное направление воздуха на устройстве",
				"включи вертикальное направление воздуха на устройстве",
				"включи предыдущее направление воздуха устройства",
				"поставь следующее направление воздуха на устройстве",
			},
			model.ThermostatDeviceType,
			[]model.ICapability{
				model.NewCapability(model.ModeCapabilityType).
					WithParameters(model.ModeCapabilityParameters{
						Instance: model.SwingModeInstance,
						Modes: []model.Mode{
							{Value: model.HorizontalMode},
							{Value: model.VerticalMode},
						},
					}),
			},
			someDeviceInflection,
		},
		{
			"Test AC WindFree Thermostat mode",
			[]string{
				"включи режим windfree",
				"переключи в режим windfree",
			},
			model.AcDeviceType,
			[]model.ICapability{
				model.NewCapability(model.ModeCapabilityType).
					WithParameters(model.ModeCapabilityParameters{
						Instance: model.ThermostatModeInstance,
						Modes: []model.Mode{
							{Value: model.WindFreeMode},
						},
					}),
			},
			someDeviceInflection,
		},
		{
			"Test Work Speed Instance",
			[]string{
				"поставь быструю скорость работы на устройстве",
				"включи быструю скорость работы на устройстве",
				"поставь турбо скорость работы на устройстве",
				"включи турбо скорость работы на устройстве",
				"включи предыдущую скорость работы устройства",
				"поставь следующую скорость работы на устройстве",
			},
			model.ThermostatDeviceType,
			[]model.ICapability{
				model.NewCapability(model.ModeCapabilityType).
					WithParameters(model.ModeCapabilityParameters{
						Instance: model.WorkSpeedModeInstance,
						Modes: []model.Mode{
							{Value: model.FastMode},
							{Value: model.TurboMode},
						},
					}),
			},
			someDeviceInflection,
		},
		{
			"Test Input Source Mode Instance",
			[]string{
				"поставь третий источник сигнала на устройстве",
				"включи третий источник сигнала на устройстве",
				"поставь нормальный источник сигнала на устройстве",
				"включи нормальный источник сигнала на устройстве",
				"включи предыдущий источник сигнала устройства",
				"поставь следующий источник сигнала на устройстве",
			},
			model.TvDeviceDeviceType,
			[]model.ICapability{
				model.NewCapability(model.ModeCapabilityType).
					WithParameters(model.ModeCapabilityParameters{
						Instance: model.InputSourceModeInstance,
						Modes: []model.Mode{
							{Value: model.NormalMode},
							{Value: model.ThreeMode},
						},
					}),
			},
			someDeviceInflection,
		},
		{
			"Test Program Mode Instance",
			[]string{
				"поставь тихую программу на устройстве",
				"включи тихую программу на устройстве",
				"поставь эко программу на устройстве",
				"включи эко программу на устройстве",
				"включи предыдущую программу устройства",
				"поставь следующую программу на устройстве",
			},
			model.TvDeviceDeviceType,
			[]model.ICapability{
				model.NewCapability(model.ModeCapabilityType).
					WithParameters(model.ModeCapabilityParameters{
						Instance: model.ProgramModeInstance,
						Modes: []model.Mode{
							{Value: model.QuietMode},
							{Value: model.EcoMode},
						},
					}),
			},
			someDeviceInflection,
		},
		{
			"Test Clean Up Mode Instance",
			[]string{
				"поставь тихую уборку на устройстве",
				"включи тихую уборку на устройстве",
				"поставь экспресс режим уборки на устройстве",
				"включи экспресс режим уборки на устройстве",
				"поставь экспресс уборку на устройстве",
				"включи экспресс уборку на устройстве",
				"включи предыдущую уборку устройства",
				"поставь следующую уборку на устройстве",
			},
			model.VacuumCleanerDeviceType,
			[]model.ICapability{
				model.NewCapability(model.ModeCapabilityType).
					WithParameters(model.ModeCapabilityParameters{
						Instance: model.CleanUpModeInstance,
						Modes: []model.Mode{
							{Value: model.QuietMode},
							{Value: model.ExpressMode},
						},
					}),
			},
			someDeviceInflection,
		},
		{
			"Test Washing Machine Device Type",
			[]string{
				"запусти стиралку",
				"выруби стиралку",
				"запусти стиральную машинку",
				"выруби стиральную машинку",
				"включи стиральную машину",
				"выключи стиральную машину",
				"включи устройство",
				"выключи устройство",
			},
			model.WashingMachineDeviceType,
			[]model.ICapability{
				model.NewCapability(model.OnOffCapabilityType),
			},
			someDeviceInflection,
		},
		{
			"Test Dishwashing Machine Device Type",
			[]string{
				"запусти посудомойку",
				"выруби посудомойку",
				"включи посудомоечную машину",
				"выключи посудомоечную машину",
				"включи устройство",
				"выключи устройство",
			},
			model.DishwasherDeviceType,
			[]model.ICapability{
				model.NewCapability(model.OnOffCapabilityType),
			},
			someDeviceInflection,
		},
		{
			"Test Open Range Instance",
			[]string{
				"открой шторы на 50 процентов",
				"открой шторы на 50",
				"открой шторы на треть",
				"приоткрой шторы",
				"прикрой шторы",
				"приоткрой шторы на 10 процентов",
				"приоткрой шторы на половину",
				"поставь открытие штор на 10 процентов",
				"установи открытие штор на 10",
			},
			model.OpenableDeviceType,
			[]model.ICapability{
				model.NewCapability(model.RangeCapabilityType).
					WithParameters(
						model.RangeCapabilityParameters{
							Instance:     model.OpenRangeInstance,
							Range:        &model.Range{Min: 1, Max: 100, Precision: 1},
							RandomAccess: true,
							Unit:         model.UnitPercent,
						}),
			},
			curtainInflection,
		},
		{
			"Test Dry Mode Suggests",
			[]string{
				"включи режим осушения на устройстве",
				"поставь режим осушения на устройстве",
				"включи режим осушения",
				"переключи в режим осушения",
				"включи следующий режим работы устройства",
				"поставь предыдущий режим работы устройства",
			},
			model.ThermostatDeviceType,
			[]model.ICapability{
				model.NewCapability(model.ModeCapabilityType).
					WithParameters(
						model.ModeCapabilityParameters{
							Instance: model.ThermostatModeInstance,
							Modes: []model.Mode{
								model.KnownModes[model.DryMode],
							},
						}),
			},
			someDeviceInflection,
		},
		{
			"Test Heat Mode Instance Suggests",
			[]string{
				"поставь максимальный нагрев на теплом поле",
				"включи максимальный нагрев на теплом поле",
				"поставь авто режим нагрева на теплом поле",
				"включи авто режим нагрева на теплом поле",
				"включи минимальный нагрев на теплом поле",
				"поставь минимальный нагрев на теплом поле",
				"включи предыдущий нагрев теплого пола",
				"поставь следующий нагрев на теплом поле",
			},
			model.ThermostatDeviceType,
			[]model.ICapability{
				model.NewCapability(model.ModeCapabilityType).WithParameters(
					model.ModeCapabilityParameters{
						Instance: model.HeatModeInstance,
						Modes: []model.Mode{
							model.KnownModes[model.MaxMode],
							model.KnownModes[model.AutoMode],
							model.KnownModes[model.MinMode],
						},
					}),
			},
			heatedFloorInflection,
		},
		{
			"Test Trunk Toggle Instance Suggests",
			[]string{
				"отопри багажник машины",
				"открой багажник машины",
			},
			model.OtherDeviceType,
			[]model.ICapability{
				model.NewCapability(model.ToggleCapabilityType).WithParameters(
					model.ToggleCapabilityParameters{Instance: model.TrunkToggleCapabilityInstance}),
			},
			carInflection,
		},
		{
			"Test Central Lock Toggle Instance Suggests",
			[]string{
				"открой машину",
				"открой замок машины",
				"разблокируй машину",
				"закрой двери в машине",
				"запри машину",
				"заблокируй машину",
			},
			model.OtherDeviceType,
			[]model.ICapability{
				model.NewCapability(model.ToggleCapabilityType).WithParameters(
					model.ToggleCapabilityParameters{Instance: model.CentralLockCapabilityInstance}),
			},
			carInflection,
		},
		{
			"Test Custom Button Suggests",
			[]string{
				"пекарь сделай пюре",
				"сделай пюре в пекаре",
				"сделай пюре на пекаре",
			},
			model.OtherDeviceType,
			[]model.ICapability{
				model.NewCapability(model.CustomButtonCapabilityType).WithParameters(
					model.CustomButtonCapabilityParameters{
						Instance:      "2131231231",
						InstanceNames: []string{"сделай пюре"},
					}),
			},
			customControlInflection,
		},
		{
			"Test Dishwasher Mode Suggests",
			[]string{
				"поставь быструю мойку посуды на устройстве",
				"поставь тихую мойку посуды на устройстве",
				"включи быструю мойку посуды на устройстве",
				"включи тихую мойку посуды на устройстве",
				"помой посуду в режиме авто",
				"включи мойку посуды в авто режим",
				"поставь авто режим мойки посуды на устройстве",
				"включи авто режим мойки посуды на устройстве",
				"запусти мойку посуды в режиме авто 45 на устройстве",
				"включи мойку посуды в режиме авто 45 градусов на устройстве",
				"поставь мойку посуды в режиме авто 60 на устройстве",
				"помой посуду в режиме авто 60",
				"запусти мойку посуды на 60 градусов",
				"включи мойку посуды в режиме авто 75 на устройстве",
				"помой посуду в режиме авто 75",
				"запусти мойку посуды на 75 градусов",
				"поставь быструю мойку посуды на 45 на устройстве",
				"включи быструю мойку посуды на 60",
				"запусти быструю мойку посуды на 60 на устройстве",
				"поставь быструю мойку посуды на 75 на устройстве",
				"запусти программу ополаскивания на устройстве",
				"прополощи мою посуду",
				"включи мойку посуды в режим ополаскивания",
				"запусти устройство в интенсивном режиме",
				"включи интенсивную программу мойки",
				"поставь мойку посуды в интенсивный режим",
				"запусти устройство в режиме мойки стекла",
				"включи программу мойки стекла",
				"помой стеклянную посуду",
				"включи предыдущую мойку посуды устройства",
				"поставь следующую мойку посуды на устройстве",
			},
			model.OtherDeviceType,
			[]model.ICapability{
				model.NewCapability(model.ModeCapabilityType).WithParameters(
					model.ModeCapabilityParameters{
						Instance: model.DishwashingModeInstance,
						Modes: []model.Mode{
							model.KnownModes[model.AutoMode],
							model.KnownModes[model.Auto45Mode],
							model.KnownModes[model.Auto60Mode],
							model.KnownModes[model.Auto75Mode],
							model.KnownModes[model.FastMode],
							model.KnownModes[model.Fast45Mode],
							model.KnownModes[model.Fast60Mode],
							model.KnownModes[model.Fast75Mode],
							model.KnownModes[model.PreRinseMode],
							model.KnownModes[model.IntensiveMode],
							model.KnownModes[model.QuietMode],
							model.KnownModes[model.GlassMode],
						},
					}),
			},
			someDeviceInflection,
		},
		{
			"Test Multicooker Device Type",
			[]string{
				"включи мультиварку",
				"выключи мультиварку",
				"включи устройство",
				"выключи устройство",
			},
			model.MulticookerDeviceType,
			[]model.ICapability{
				model.NewCapability(model.OnOffCapabilityType),
			},
			someDeviceInflection,
		},
		{
			"Test Refrigerator Device Type",
			[]string{
				"включи холодильник",
				"выключи холодильник",
				"включи устройство",
				"выключи устройство",
			},
			model.RefrigeratorDeviceType,
			[]model.ICapability{
				model.NewCapability(model.OnOffCapabilityType),
			},
			someDeviceInflection,
		},
		{
			"Test Fan Device Type",
			[]string{
				"включи вентилятор",
				"выключи вентилятор",
				"включи устройство",
				"выключи устройство",
			},
			model.FanDeviceType,
			[]model.ICapability{
				model.NewCapability(model.OnOffCapabilityType),
			},
			someDeviceInflection,
		},
		{
			"Test Iron Device Type",
			[]string{
				"выключи утюг",
				"выключи устройство",
			},
			model.IronDeviceType,
			[]model.ICapability{
				model.NewCapability(model.OnOffCapabilityType),
			},
			someDeviceInflection,
		},
		{
			"Test Pet Feeder Device Type",
			[]string{
				"покорми кота",
				"наполни миску",
				"насыпь корм коту",
				"пора кормить щенка",
			},
			model.PetFeederDeviceType,
			[]model.ICapability{
				model.NewCapability(model.OnOffCapabilityType),
			},
			someDeviceInflection,
		},
		{
			"Test Tea Mode Instances",
			[]string{
				"включи предыдущий режим чая устройства",
				"поставь следующий режим чая на устройстве",
				"сделай черный чай",
				"приготовь черный чай",
				"сделай зеленый чай",
				"приготовь зеленый чай",
				"сделай пуэр",
				"приготовь пуэр",
				"сделай белый чай",
				"приготовь белый чай",
				"сделай улун",
				"приготовь улун",
				"сделай красный чай",
				"приготовь красный чай",
				"сделай травяной чай",
				"приготовь травяной чай",
				"сделай цветочный чай",
				"приготовь цветочный чай",
			},
			model.KettleDeviceType,
			[]model.ICapability{
				model.NewCapability(model.ModeCapabilityType).WithParameters(
					model.ModeCapabilityParameters{
						Instance: model.TeaModeInstance,
						Modes: []model.Mode{
							model.KnownModes[model.BlackTeaMode],
							model.KnownModes[model.GreenTeaMode],
							model.KnownModes[model.PuerhTeaMode],
							model.KnownModes[model.WhiteTeaMode],
							model.KnownModes[model.OolongTeaMode],
							model.KnownModes[model.RedTeaMode],
							model.KnownModes[model.HerbalTeaMode],
							model.KnownModes[model.FlowerTeaMode],
						},
					}),
			},
			someDeviceInflection,
		},
		{
			"Test Color Scenes Suggests",
			[]string{
				"включи режим сирена",
				"поставь режим сирена на устройстве",
				"включи режим освещения сирена на устройстве",
			},
			model.LightDeviceType,
			[]model.ICapability{
				model.NewCapability(model.ColorSettingCapabilityType).
					WithParameters(model.ColorSettingCapabilityParameters{
						ColorSceneParameters: &model.ColorSceneParameters{
							Scenes: model.ColorScenes{
								{
									ID:   model.ColorSceneIDSiren,
									Name: model.KnownColorScenes[model.ColorSceneIDSiren].Name,
								},
							},
						},
						ColorModel: &hsvModelType,
					}),
			},
			someDeviceInflection,
		},
	}
	options := model.SuggestionsOptions{}
	for _, tt := range tests {
		t.Run(tt.name, func(t *testing.T) {
			blocks := BuildSuggestionBlocks(tt.inflection, tt.deviceType, tt.capabilities, model.Properties{}, options)
			flattened := make([]string, 0)
			for _, block := range blocks {
				for _, suggest := range block.Suggests {
					flattened = append(flattened, strings.ToLower(suggest))
				}
			}
			for _, suggest := range tt.expectations {
				assert.Containsf(t, flattened, suggest, fmt.Sprintf("suggests for test %s do not contain %s", tt.name, suggest))
			}
		})
	}
}

func TestActionSuggestionsFromCapabilitiesWithHouseholds(t *testing.T) {
	hsvModelType := model.HsvModelType
	someDeviceInflection := inflector.Inflection{
		Im:   "устройство",
		Rod:  "устройства",
		Dat:  "устройству",
		Vin:  "устройство",
		Tvor: "устройством",
		Pr:   "устройстве",
	}
	dachaInflection := inflector.Inflection{
		Im:   "Дача",
		Rod:  "Даче",
		Dat:  "Даче",
		Vin:  "Дачу",
		Tvor: "Дачей",
		Pr:   "Даче",
	}

	tests := []struct {
		name         string
		expectations []string
		deviceType   model.DeviceType
		capabilities []model.ICapability
		inflection   inflector.Inflection
	}{
		{
			"Test OnOff",
			[]string{
				"включи устройство на даче",
				"выключи устройство на даче",
			},
			model.OtherDeviceType,
			[]model.ICapability{
				model.MakeCapabilityByType(model.OnOffCapabilityType),
			},
			someDeviceInflection,
		},
		{
			"Test OnOff For LightDeviceType",
			[]string{
				"включи свет на даче",
				"включи устройство на даче",
				"выключи свет на даче",
				"выключи устройство на даче",
			},
			model.LightDeviceType,
			[]model.ICapability{
				model.MakeCapabilityByType(model.OnOffCapabilityType),
			},
			someDeviceInflection,
		},
		{
			"Test Multicooker Device Type",
			[]string{
				"включи мультиварку на даче",
				"выключи мультиварку на даче",
				"включи устройство на даче",
				"выключи устройство на даче",
			},
			model.MulticookerDeviceType,
			[]model.ICapability{
				model.NewCapability(model.OnOffCapabilityType),
			},
			someDeviceInflection,
		},
		{
			"Test Refrigerator Device Type",
			[]string{
				"включи холодильник на даче",
				"выключи холодильник на даче",
				"включи устройство на даче",
				"выключи устройство на даче",
			},
			model.RefrigeratorDeviceType,
			[]model.ICapability{
				model.NewCapability(model.OnOffCapabilityType),
			},
			someDeviceInflection,
		},
		{
			"Test Fan Device Type",
			[]string{
				"включи вентилятор на даче",
				"выключи вентилятор на даче",
				"включи устройство на даче",
				"выключи устройство на даче",
			},
			model.FanDeviceType,
			[]model.ICapability{
				model.NewCapability(model.OnOffCapabilityType),
			},
			someDeviceInflection,
		},
		{
			"Test Pet Feeder Device Type",
			[]string{
				"покорми кота на даче",
				"наполни миску на даче",
				"насыпь корм коту на даче",
				"пора кормить щенка на даче",
			},
			model.PetFeederDeviceType,
			[]model.ICapability{
				model.NewCapability(model.OnOffCapabilityType),
			},
			someDeviceInflection,
		},
		{
			"Test Tea Mode Instances",
			[]string{
				"включи предыдущий режим чая устройства на даче",
				"поставь следующий режим чая на устройстве на даче",
				"сделай черный чай на даче",
				"приготовь черный чай на даче",
				"сделай зеленый чай на даче",
				"приготовь зеленый чай на даче",
				"сделай пуэр на даче",
				"приготовь пуэр на даче",
				"сделай белый чай на даче",
				"приготовь белый чай на даче",
				"сделай улун на даче",
				"приготовь улун на даче",
				"сделай красный чай на даче",
				"приготовь красный чай на даче",
				"сделай травяной чай на даче",
				"приготовь травяной чай на даче",
				"сделай цветочный чай на даче",
				"приготовь цветочный чай на даче",
			},
			model.KettleDeviceType,
			[]model.ICapability{
				model.NewCapability(model.ModeCapabilityType).WithParameters(
					model.ModeCapabilityParameters{
						Instance: model.TeaModeInstance,
						Modes: []model.Mode{
							model.KnownModes[model.BlackTeaMode],
							model.KnownModes[model.GreenTeaMode],
							model.KnownModes[model.PuerhTeaMode],
							model.KnownModes[model.WhiteTeaMode],
							model.KnownModes[model.OolongTeaMode],
							model.KnownModes[model.RedTeaMode],
							model.KnownModes[model.HerbalTeaMode],
							model.KnownModes[model.FlowerTeaMode],
						},
					}),
			},
			someDeviceInflection,
		},
		{
			"Test Color Scenes Suggests",
			[]string{
				"включи режим сирена на даче",
				"поставь режим сирена на устройстве на даче",
				"включи режим освещения сирена на устройстве на даче",
			},
			model.LightDeviceType,
			[]model.ICapability{
				model.NewCapability(model.ColorSettingCapabilityType).
					WithParameters(model.ColorSettingCapabilityParameters{
						ColorSceneParameters: &model.ColorSceneParameters{
							Scenes: model.ColorScenes{
								{
									ID:   model.ColorSceneIDSiren,
									Name: model.KnownColorScenes[model.ColorSceneIDSiren].Name,
								},
							},
						},
						ColorModel: &hsvModelType,
					}),
			},
			someDeviceInflection,
		},
		{
			"Test Temperature",
			[]string{
				"прибавь температуру на устройстве на даче",
				"увеличь температуру для устройства на даче",
				"убавь температуру для устройства на даче",
				"уменьши температуру устройства на даче",
				"включи температуру устройства 30 градусов на даче",
				"установи температуру устройства на 20 градусов на даче",
				"включи максимальную температуру на устройстве на даче",
				"включи температуру устройства на максимум на даче",
				"включи минимальную температуру на устройстве на даче",
				"включи температуру устройства на минимум на даче",
			},
			model.OtherDeviceType,
			[]model.ICapability{
				model.NewCapability(model.RangeCapabilityType).
					WithParameters(model.RangeCapabilityParameters{
						Instance:     model.TemperatureRangeInstance,
						Range:        &model.Range{Min: 11, Max: 30, Precision: 1},
						RandomAccess: true,
						Unit:         model.UnitTemperatureCelsius,
					}),
			},
			someDeviceInflection,
		},
		{
			"Test Custom Button Suggests",
			[]string{
				"устройство сделай пюре на даче",
				"сделай пюре в устройстве на даче",
				"сделай пюре на устройстве на даче",
			},
			model.OtherDeviceType,
			[]model.ICapability{
				model.NewCapability(model.CustomButtonCapabilityType).WithParameters(
					model.CustomButtonCapabilityParameters{
						Instance:      "2131231231",
						InstanceNames: []string{"сделай пюре"},
					}),
			},
			someDeviceInflection,
		},
	}
	options := model.SuggestionsOptions{
		CurrentHouseholdInflection: &dachaInflection,
		MoreThanOneHousehold:       true,
	}
	for _, tt := range tests {
		t.Run(tt.name, func(t *testing.T) {
			blocks := BuildSuggestionBlocks(tt.inflection, tt.deviceType, tt.capabilities, model.Properties{}, options)
			flattened := make([]string, 0)
			for _, block := range blocks {
				for _, suggest := range block.Suggests {
					flattened = append(flattened, strings.ToLower(suggest))
				}
			}
			for _, suggest := range tt.expectations {
				assert.Containsf(t, flattened, suggest, fmt.Sprintf("suggests for test %s do not contain %s", tt.name, suggest))
			}
		})
	}
}

func TestBuildQuerySuggestions(t *testing.T) {
	someDeviceInflection := inflector.Inflection{
		Im:   "устройство",
		Rod:  "устройства",
		Dat:  "устройству",
		Vin:  "устройство",
		Tvor: "устройством",
		Pr:   "устройстве",
	}

	type args struct {
		capabilities model.Capabilities
		properties   model.Properties
		inflection   inflector.Inflection
	}
	tests := []struct {
		name string
		args args
		want []string
	}{
		{
			name: "Test FloatProperty HumidityInstance suggests",
			args: args{
				properties: model.Properties{
					model.MakePropertyByType(model.FloatPropertyType).
						WithRetrievable(true).
						WithParameters(model.FloatPropertyParameters{
							Instance: model.HumidityPropertyInstance,
						}),
				},
				inflection: someDeviceInflection,
			},
			want: []string{
				"Что с устройством?",
				"Какая влажность в доме?",
				"Какая влажность на устройстве?",
			},
		},
		{
			name: "Test FloatProperty TemperatureInstance suggests",
			args: args{
				properties: model.Properties{
					model.MakePropertyByType(model.FloatPropertyType).
						WithRetrievable(true).
						WithReportable(false).
						WithParameters(model.FloatPropertyParameters{
							Instance: model.TemperaturePropertyInstance,
						}),
				},
				inflection: someDeviceInflection,
			},
			want: []string{
				"Что с устройством?",
				"Какая температура в доме?",
				"Какая температура на устройстве?",
			},
		},
		{
			name: "Test FloatProperty CO2Instance suggests",
			args: args{
				properties: model.Properties{
					model.MakePropertyByType(model.FloatPropertyType).
						WithRetrievable(true).
						WithReportable(true).
						WithParameters(model.FloatPropertyParameters{
							Instance: model.CO2LevelPropertyInstance,
						}),
				},
				inflection: someDeviceInflection,
			},
			want: []string{
				"Что с устройством?",
				"Какой уровень углекислого газа в доме?",
				"Какой уровень углекислого газа на устройстве?",
				"Какой уровень кислорода в доме?",
				"Какой уровень кислорода на устройстве?",
			},
		},
		{
			name: "Test FloatProperty BatteryLevel suggests",
			args: args{
				properties: model.Properties{
					model.MakePropertyByType(model.FloatPropertyType).
						WithRetrievable(false).
						WithReportable(true).
						WithParameters(model.FloatPropertyParameters{
							Instance: model.BatteryLevelPropertyInstance,
						}),
				},
				inflection: someDeviceInflection,
			},
			want: []string{
				"Что с устройством?",
				"Какой уровень заряда батареи на устройстве?",
			},
		},
	}
	for _, tt := range tests {
		t.Run(tt.name, func(t *testing.T) {
			actual, ok := BuildQuerySuggestions(tt.args.inflection, tt.args.capabilities, tt.args.properties, model.SuggestionsOptions{})
			assert.True(t, ok)
			assert.Equal(t, QuerySuggestionBlockType, actual.Type)
			assert.ElementsMatch(t, tt.want, actual.Suggests)
		})
	}
}

func TestBuildQuerySuggestionsWithHousehold(t *testing.T) {
	someDeviceInflection := inflector.Inflection{
		Im:   "устройство",
		Rod:  "устройства",
		Dat:  "устройству",
		Vin:  "устройство",
		Tvor: "устройством",
		Pr:   "устройстве",
	}

	dachaInflection := inflector.Inflection{
		Im:   "Дача",
		Rod:  "Даче",
		Dat:  "Даче",
		Vin:  "Дачу",
		Tvor: "Дачей",
		Pr:   "Даче",
	}

	type args struct {
		capabilities model.Capabilities
		properties   model.Properties
		inflection   inflector.Inflection
	}
	tests := []struct {
		name string
		args args
		want []string
	}{
		{
			name: "Test FloatProperty HumidityInstance suggests",
			args: args{
				properties: model.Properties{
					model.MakePropertyByType(model.FloatPropertyType).
						WithRetrievable(true).
						WithParameters(model.FloatPropertyParameters{
							Instance: model.HumidityPropertyInstance,
						}),
				},
				inflection: someDeviceInflection,
			},
			want: []string{
				"Что с устройством на даче?",
				"Какая влажность на даче?",
				"Какая влажность на устройстве на даче?",
			},
		},
		{
			name: "Test FloatProperty TemperatureInstance suggests",
			args: args{
				properties: model.Properties{
					model.MakePropertyByType(model.FloatPropertyType).
						WithRetrievable(true).
						WithParameters(model.FloatPropertyParameters{
							Instance: model.TemperaturePropertyInstance,
						}),
				},
				inflection: someDeviceInflection,
			},
			want: []string{
				"Что с устройством на даче?",
				"Какая температура на даче?",
				"Какая температура на устройстве на даче?",
			},
		},
		{
			name: "Test FloatProperty CO2Instance suggests",
			args: args{
				properties: model.Properties{
					model.MakePropertyByType(model.FloatPropertyType).
						WithRetrievable(true).
						WithParameters(model.FloatPropertyParameters{
							Instance: model.CO2LevelPropertyInstance,
						}),
				},
				inflection: someDeviceInflection,
			},
			want: []string{
				"Что с устройством на даче?",
				"Какой уровень углекислого газа на даче?",
				"Какой уровень углекислого газа на устройстве на даче?",
				"Какой уровень кислорода на даче?",
				"Какой уровень кислорода на устройстве на даче?",
			},
		},
		{
			name: "Test FloatProperty BatteryLevel suggests",
			args: args{
				properties: model.Properties{
					model.MakePropertyByType(model.FloatPropertyType).
						WithRetrievable(true).
						WithParameters(model.FloatPropertyParameters{
							Instance: model.BatteryLevelPropertyInstance,
						}),
				},
				inflection: someDeviceInflection,
			},
			want: []string{
				"Что с устройством на даче?",
				"Какой уровень заряда батареи на устройстве на даче?",
			},
		},
	}
	options := model.SuggestionsOptions{
		CurrentHouseholdInflection: &dachaInflection,
		MoreThanOneHousehold:       true,
	}
	for _, tt := range tests {
		t.Run(tt.name, func(t *testing.T) {
			actual, ok := BuildQuerySuggestions(tt.args.inflection, tt.args.capabilities, tt.args.properties, options)
			assert.True(t, ok)
			assert.Equal(t, QuerySuggestionBlockType, actual.Type)
			assert.ElementsMatch(t, tt.want, actual.Suggests)
		})
	}
}
