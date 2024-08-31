package model

import (
	"fmt"
	"math"
	"strings"

	"a.yandex-team.ru/alice/library/go/inflector"
	"a.yandex-team.ru/alice/library/go/tools"
)

type TargetNLGOptions struct {
	UseDeviceTarget  bool
	DeviceName       string
	DeviceInflection inflector.Inflection

	UseRoomTarget  bool
	RoomName       string
	RoomInflection inflector.Inflection
}

func (opts TargetNLGOptions) InRoomNLG() string {
	switch strings.ToLower(opts.RoomName) {
	case "кухня", "балкон", "улица":
		return fmt.Sprintf("на %s", opts.RoomInflection.Pr)
	}
	return fmt.Sprintf("в %s", opts.RoomInflection.Pr)
}

func GetModeNameForInstance(_ ModeCapabilityInstance, value ModeValue) string {
	mode, ok := KnownModes[value]
	if !ok {
		return "значение неизвестно"
	}
	return strings.ToLower(*mode.Name)
}

func GetUnitFloatValueForVoiceStatus(value float64, unit Unit) string {
	var msgBuilder strings.Builder
	var roundedValue float64

	if unit == UnitAmpere {
		roundedValue = math.Round(value*100) / 100
		msgBuilder.WriteString(fmt.Sprintf("%.2f ", roundedValue))
	} else {
		roundedValue = math.Round(value)
		msgBuilder.WriteString(fmt.Sprintf("%d ", int64(roundedValue)))
	}

	remainder := int64(roundedValue) % 100
	switch unit {
	case UnitPercent:
		switch remainder {
		case 1, 21, 31, 41, 51, 61, 71, 81, 91:
			msgBuilder.WriteString("процент")
		case 2, 22, 32, 42, 52, 62, 72, 82, 92, 3, 23, 33, 43, 53, 63, 73, 83, 93, 4, 24, 34, 44, 54, 64, 74, 84, 94:
			msgBuilder.WriteString("процента")
		default:
			msgBuilder.WriteString("процентов")
		}
	case UnitTemperatureCelsius:
		switch remainder {
		case 1, 21, 31, 41, 51, 61, 71, 81, 91:
			msgBuilder.WriteString("градус Цельсия")
		case 2, 22, 32, 42, 52, 62, 72, 82, 92, 3, 23, 33, 43, 53, 63, 73, 83, 93, 4, 24, 34, 44, 54, 64, 74, 84, 94:
			msgBuilder.WriteString("градуса Цельсия")
		default:
			msgBuilder.WriteString("градусов Цельсия")
		}
	case UnitTemperatureKelvin:
		switch remainder {
		case 1, 21, 31, 41, 51, 61, 71, 81, 91:
			msgBuilder.WriteString("Кельвин")
		case 2, 22, 32, 42, 52, 62, 72, 82, 92, 3, 23, 33, 43, 53, 63, 73, 83, 93, 4, 24, 34, 44, 54, 64, 74, 84, 94:
			msgBuilder.WriteString("Кельвина")
		default:
			msgBuilder.WriteString("Кельвинов")
		}
	case UnitPPM:
		switch remainder {
		case 1, 21, 31, 41, 51, 61, 71, 81, 91:
			msgBuilder.WriteString("миллионная доля")
		case 2, 22, 32, 42, 52, 62, 72, 82, 92, 3, 23, 33, 43, 53, 63, 73, 83, 93, 4, 24, 34, 44, 54, 64, 74, 84, 94:
			msgBuilder.WriteString("миллионные доли")
		default:
			msgBuilder.WriteString("миллионных долей")
		}
	case UnitAmpere:
		if roundedValue < 1 {
			msgBuilder.WriteString("ампера")
		} else {
			switch remainder {
			case 2, 22, 32, 42, 52, 62, 72, 82, 92, 3, 23, 33, 43, 53, 63, 73, 83, 93, 4, 24, 34, 44, 54, 64, 74, 84, 94:
				msgBuilder.WriteString("ампера")
			default:
				msgBuilder.WriteString("ампер")
			}
		}
	case UnitVolt:
		switch remainder {
		case 2, 22, 32, 42, 52, 62, 72, 82, 92, 3, 23, 33, 43, 53, 63, 73, 83, 93, 4, 24, 34, 44, 54, 64, 74, 84, 94:
			msgBuilder.WriteString("вольта")
		default:
			msgBuilder.WriteString("вольт")
		}
	case UnitWatt:
		switch remainder {
		case 2, 22, 32, 42, 52, 62, 72, 82, 92, 3, 23, 33, 43, 53, 63, 73, 83, 93, 4, 24, 34, 44, 54, 64, 74, 84, 94:
			msgBuilder.WriteString("ватта")
		default:
			msgBuilder.WriteString("ватт")
		}
	case UnitDensityMcgM3:
		switch remainder {
		case 2, 22, 32, 42, 52, 62, 72, 82, 92, 3, 23, 33, 43, 53, 63, 73, 83, 93, 4, 24, 34, 44, 54, 64, 74, 84, 94:
			msgBuilder.WriteString("микрограмма на метр кубический")
		default:
			msgBuilder.WriteString("микрограмм на метр кубический")
		}
	case UnitPressureAtm:
		switch remainder {
		case 1, 21, 31, 41, 51, 61, 71, 81, 91:
			msgBuilder.WriteString("атмосфера")
		case 2, 22, 32, 42, 52, 62, 72, 82, 92, 3, 23, 33, 43, 53, 63, 73, 83, 93, 4, 24, 34, 44, 54, 64, 74, 84, 94:
			msgBuilder.WriteString("атмосферы")
		default:
			msgBuilder.WriteString("атмосфер")
		}
	case UnitPressurePascal:
		switch remainder {
		case 0, 1, 21, 31, 41, 51, 61, 71, 81, 91:
			msgBuilder.WriteString("паскаль")
		case 2, 22, 32, 42, 52, 62, 72, 82, 92, 3, 23, 33, 43, 53, 63, 73, 83, 93, 4, 24, 34, 44, 54, 64, 74, 84, 94:
			msgBuilder.WriteString("паскаля")
		default:
			msgBuilder.WriteString("паскалей")
		}
	case UnitPressureBar:
		switch remainder {
		case 2, 22, 32, 42, 52, 62, 72, 82, 92, 3, 23, 33, 43, 53, 63, 73, 83, 93, 4, 24, 34, 44, 54, 64, 74, 84, 94:
			msgBuilder.WriteString("бара")
		default:
			msgBuilder.WriteString("бар")
		}
	case UnitPressureMmHg:
		switch remainder {
		case 1, 21, 31, 41, 51, 61, 71, 81, 91:
			msgBuilder.WriteString("миллиметр ртутного столба")
		case 2, 22, 32, 42, 52, 62, 72, 82, 92, 3, 23, 33, 43, 53, 63, 73, 83, 93, 4, 24, 34, 44, 54, 64, 74, 84, 94:
			msgBuilder.WriteString("миллиметра ртутного столба")
		default:
			msgBuilder.WriteString("миллиметров ртутного столба")
		}
	case UnitTimeSeconds:
		switch remainder {
		case 1, 21, 31, 41, 51, 61, 71, 81, 91:
			msgBuilder.WriteString("секунда")
		case 2, 22, 32, 42, 52, 62, 72, 82, 92, 3, 23, 33, 43, 53, 63, 73, 83, 93, 4, 24, 34, 44, 54, 64, 74, 84, 94:
			msgBuilder.WriteString("секунды")
		default:
			msgBuilder.WriteString("секунд")
		}
	case UnitIlluminationLux:
		switch remainder {
		case 1, 21, 31, 41, 51, 61, 71, 81, 91:
			msgBuilder.WriteString("люкс")
		case 2, 22, 32, 42, 52, 62, 72, 82, 92, 3, 23, 33, 43, 53, 63, 73, 83, 93, 4, 24, 34, 44, 54, 64, 74, 84, 94:
			msgBuilder.WriteString("люкса")
		default:
			msgBuilder.WriteString("люксов")
		}
	}
	return strings.TrimSpace(msgBuilder.String())
}

func GetFloatPropertyVoiceStatus(state FloatPropertyState, parameters FloatPropertyParameters, nlgOptions TargetNLGOptions) string {
	instanceName := KnownPropertyVoiceInstanceNames[parameters.Instance]
	value := GetUnitFloatValueForVoiceStatus(state.Value, parameters.Unit)
	var msg string
	if nlgOptions.UseDeviceTarget && nlgOptions.UseRoomTarget {
		msg = fmt.Sprintf("%s на %s %s %s", instanceName, nlgOptions.DeviceInflection.Pr, nlgOptions.InRoomNLG(), value)
	} else if nlgOptions.UseDeviceTarget {
		msg = fmt.Sprintf("%s на %s %s", instanceName, nlgOptions.DeviceInflection.Pr, value)
	} else {
		msg = fmt.Sprintf("%s %s", instanceName, value)
	}
	return tools.FormatNLGText(msg)
}

func GetDeviceOnOffCapabilityVoiceStatus(state OnOffCapabilityState, deviceType DeviceType, nlgOptions TargetNLGOptions) string {
	isOn := state.Value
	deviceName := nlgOptions.DeviceName

	var target string
	var targetVerb string

	// first we try to find target verb and name among well-known names
	type defaultNameAndShortAnswer struct {
		Name      string
		AnswerOn  string
		AnswerOff string
	}
	defaultNamesAndShortAnswers := map[DeviceType][]defaultNameAndShortAnswer{
		LightDeviceType: {
			{
				Name:      "Лампочка",
				AnswerOn:  "Включена",
				AnswerOff: "Выключена",
			},
			{
				Name:      "Люстра",
				AnswerOn:  "Включена",
				AnswerOff: "Выключена",
			},
			{
				Name:      "Лампа",
				AnswerOn:  "Включена",
				AnswerOff: "Выключена",
			},
			{
				Name:      "Торшер",
				AnswerOn:  "Включен",
				AnswerOff: "Выключен",
			},
			{
				Name:      "Свет",
				AnswerOn:  "Включен",
				AnswerOff: "Выключен",
			},
			{
				Name:      "Ночник",
				AnswerOn:  "Включен",
				AnswerOff: "Выключен",
			},
			{
				Name:      "Светильник",
				AnswerOn:  "Включен",
				AnswerOff: "Выключен",
			},
			{
				Name:      "Лента",
				AnswerOn:  "Включена",
				AnswerOff: "Выключена",
			},
			{
				Name:      "Гирлянда",
				AnswerOn:  "Включена",
				AnswerOff: "Выключена",
			},
		},
		LightCeilingDeviceType: {
			{
				Name:      "Люстра",
				AnswerOn:  "Включена",
				AnswerOff: "Выключена",
			},
			{
				Name:      "Светильник",
				AnswerOn:  "Включен",
				AnswerOff: "Выключен",
			},
			{
				Name:      "Спот",
				AnswerOn:  "Включен",
				AnswerOff: "Выключен",
			},
			{
				Name:      "Ночник",
				AnswerOn:  "Включен",
				AnswerOff: "Выключен",
			},
		},
		LightLampDeviceType: {
			{
				Name:      "Лампа",
				AnswerOn:  "Включена",
				AnswerOff: "Выключена",
			},
			{
				Name:      "Светильник",
				AnswerOn:  "Включен",
				AnswerOff: "Выключен",
			},
			{
				Name:      "Настольная лампа",
				AnswerOn:  "Включена",
				AnswerOff: "Выключена",
			},
			{
				Name:      "Ночник",
				AnswerOn:  "Включен",
				AnswerOff: "Выключен",
			},
		},
		LightStripDeviceType: {
			{
				Name:      "Лента",
				AnswerOn:  "Включена",
				AnswerOff: "Выключена",
			},
			{
				Name:      "Диодная лента",
				AnswerOn:  "Включена",
				AnswerOff: "Выключена",
			},
			{
				Name:      "Подсветка",
				AnswerOn:  "Включена",
				AnswerOff: "Выключена",
			},
			{
				Name:      "Фартук",
				AnswerOn:  "Включен",
				AnswerOff: "Выключен",
			},
		},
		SocketDeviceType: {
			{
				Name:      "Розетка",
				AnswerOn:  "Включена",
				AnswerOff: "Выключена",
			},
			{
				Name:      "Гирлянда",
				AnswerOn:  "Включена",
				AnswerOff: "Выключена",
			},
			{
				Name:      "Телевизор",
				AnswerOn:  "Включен",
				AnswerOff: "Выключен",
			},
			{
				Name:      "Свет",
				AnswerOn:  "Включен",
				AnswerOff: "Выключен",
			},
			{
				Name:      "Обогреватель",
				AnswerOn:  "Включен",
				AnswerOff: "Выключен",
			},
			{
				Name:      "Вентилятор",
				AnswerOn:  "Включен",
				AnswerOff: "Выключен",
			},
			{
				Name:      "Удлинитель",
				AnswerOn:  "Включен",
				AnswerOff: "Выключен",
			},
			{
				Name:      "Чайник",
				AnswerOn:  "Включен",
				AnswerOff: "Выключен",
			},
		},
		SwitchDeviceType: {
			{
				Name:      "Свет",
				AnswerOn:  "Включен",
				AnswerOff: "Выключен",
			},
			{
				Name:      "Выключатель",
				AnswerOn:  "Включен",
				AnswerOff: "Выключен",
			},
			{
				Name:      "Люстра",
				AnswerOn:  "Включена",
				AnswerOff: "Выключена",
			},
		},
		HumidifierDeviceType: {
			{
				Name:      "Увлажнитель",
				AnswerOn:  "Включен",
				AnswerOff: "Выключен",
			},
		},
		PurifierDeviceType: {
			{
				Name:      "Очиститель",
				AnswerOn:  "Включен",
				AnswerOff: "Выключен",
			},
		},
		VacuumCleanerDeviceType: {
			{
				Name:      "Пылесос",
				AnswerOn:  "Включен",
				AnswerOff: "Выключен",
			},
		},
		CookingDeviceType: {
			{
				Name:      "Мультиварка",
				AnswerOn:  "Включена",
				AnswerOff: "Выключена",
			},
			{
				Name:      "Мультипекарь",
				AnswerOn:  "Включен",
				AnswerOff: "Выключен",
			},
		},
		KettleDeviceType: {
			{
				Name:      "Чайник",
				AnswerOn:  "Включен",
				AnswerOff: "Выключен",
			},
			{
				Name:      "Самовар",
				AnswerOn:  "Включен",
				AnswerOff: "Выключен",
			},
		},
		CoffeeMakerDeviceType: {
			{
				Name:      "Кофеварка",
				AnswerOn:  "Включена",
				AnswerOff: "Выключена",
			},
			{
				Name:      "Кофемашина",
				AnswerOn:  "Включена",
				AnswerOff: "Выключена",
			},
		},
		MulticookerDeviceType: {
			{
				Name:      "Мультиварка",
				AnswerOn:  "Включена",
				AnswerOff: "Выключена",
			},
		},
		ThermostatDeviceType: {
			{
				Name:      "Термостат",
				AnswerOn:  "Включен",
				AnswerOff: "Выключен",
			},
			{
				Name:      "Кондиционер",
				AnswerOn:  "Включен",
				AnswerOff: "Выключен",
			},
			{
				Name:      "Теплый пол",
				AnswerOn:  "Включен",
				AnswerOff: "Выключен",
			},
			{
				Name:      "Обогреватель",
				AnswerOn:  "Включен",
				AnswerOff: "Выключен",
			},
			{
				Name:      "Конвектор",
				AnswerOn:  "Включен",
				AnswerOff: "Выключен",
			},
		},
		AcDeviceType: {
			{
				Name:      "Кондиционер",
				AnswerOn:  "Включен",
				AnswerOff: "Выключен",
			},
			{
				Name:      "Кондей",
				AnswerOn:  "Включен",
				AnswerOff: "Выключен",
			},
		},
		MediaDeviceDeviceType: {
			{
				Name:      "Телевизор",
				AnswerOn:  "Включен",
				AnswerOff: "Выключен",
			},
			{
				Name:      "Приставка",
				AnswerOn:  "Включена",
				AnswerOff: "Выключена",
			},
		},
		TvDeviceDeviceType: {
			{
				Name:      "Телевизор",
				AnswerOn:  "Включен",
				AnswerOff: "Выключен",
			},
			{
				Name:      "Ящик",
				AnswerOn:  "Включен",
				AnswerOff: "Выключен",
			},
			{
				Name:      "Телек",
				AnswerOn:  "Включен",
				AnswerOff: "Выключен",
			},
			{
				Name:      "Телик",
				AnswerOn:  "Включен",
				AnswerOff: "Выключен",
			},
			{
				Name:      "Приставка",
				AnswerOn:  "Включена",
				AnswerOff: "Выключена",
			},
		},
		ReceiverDeviceType: {
			{
				Name:      "Ресивер",
				AnswerOn:  "Включен",
				AnswerOff: "Выключен",
			},
			{
				Name:      "Приставка",
				AnswerOn:  "Включена",
				AnswerOff: "Выключена",
			},
		},
		TvBoxDeviceType: {
			{
				Name:      "Приставка",
				AnswerOn:  "Включена",
				AnswerOff: "Выключена",
			},
		},
		WashingMachineDeviceType: {
			{
				Name:      "Стиральная машина",
				AnswerOn:  "Включена",
				AnswerOff: "Выключена",
			},
			{
				Name:      "Стиралка",
				AnswerOn:  "Включена",
				AnswerOff: "Выключена",
			},
		},
		DishwasherDeviceType: {
			{
				Name:      "Посудомоечная машина",
				AnswerOn:  "Включена",
				AnswerOff: "Выключена",
			},
			{
				Name:      "Посудомойка",
				AnswerOn:  "Включена",
				AnswerOff: "Выключена",
			},
		},
		RefrigeratorDeviceType: {
			{
				Name:      "Холодильник",
				AnswerOn:  "Включен",
				AnswerOff: "Выключен",
			},
			{
				Name:      "Холодос",
				AnswerOn:  "Включен",
				AnswerOff: "Выключен",
			},
		},
		FanDeviceType: {
			{
				Name:      "Вентилятор",
				AnswerOn:  "Включен",
				AnswerOff: "Выключен",
			},
			{
				Name:      "Вытяжка",
				AnswerOn:  "Включена",
				AnswerOff: "Выключена",
			},
		},
		IronDeviceType: {
			{
				Name:      "Утюг",
				AnswerOn:  "Включен",
				AnswerOff: "Выключен",
			},
		},
		PetFeederDeviceType: {
			{
				Name:      "Кормушка",
				AnswerOn:  "Включена",
				AnswerOff: "Выключена",
			},
		},
		OpenableDeviceType: {
			{
				Name:      "Дверь",
				AnswerOn:  "Открыта",
				AnswerOff: "Закрыта",
			},
			{
				Name:      "Калитка",
				AnswerOn:  "Открыта",
				AnswerOff: "Закрыта",
			},
			{
				Name:      "Замок",
				AnswerOn:  "Открыт",
				AnswerOff: "Закрыт",
			},
			{
				Name:      "Ворота",
				AnswerOn:  "Открыты",
				AnswerOff: "Закрыты",
			},
		},
		CurtainDeviceType: {
			{
				Name:      "Шторы",
				AnswerOn:  "Открыты",
				AnswerOff: "Закрыты",
			},
			{
				Name:      "Штора",
				AnswerOn:  "Открыта",
				AnswerOff: "Закрыта",
			},
			{
				Name:      "Жалюзи",
				AnswerOn:  "Открыты",
				AnswerOff: "Закрыты",
			},
		},
		SensorDeviceType: {
			{
				Name:      "Датчик",
				AnswerOn:  "Включен",
				AnswerOff: "Выключен",
			},
		},
		CameraDeviceType: {
			{
				Name:      "Камера",
				AnswerOn:  "Включена",
				AnswerOff: "Выключена",
			},
		},
	}
	if namesAndAnswers, ok := defaultNamesAndShortAnswers[deviceType]; ok {
		for _, nameAndAnswers := range namesAndAnswers {
			if strings.Contains(strings.ToLower(deviceName), strings.ToLower(nameAndAnswers.Name)) {
				if isOn {
					targetVerb = nameAndAnswers.AnswerOn
				} else {
					targetVerb = nameAndAnswers.AnswerOff
				}
				target = deviceName
				break
			}
		}
	}

	// if it is not there - try to match it with generated device type name and verb
	generatedDeviceTypeName := deviceType.GenerateDeviceName()
	var deviceTypeVerb string
	switch deviceType {
	case LightDeviceType:
		if isOn {
			deviceTypeVerb = "включен"
		} else {
			deviceTypeVerb = "выключен"
		}
	case LightLampDeviceType, LightCeilingDeviceType, LightStripDeviceType:
		if isOn {
			deviceTypeVerb = "включена"
		} else {
			deviceTypeVerb = "выключена"
		}
	case SocketDeviceType:
		if isOn {
			deviceTypeVerb = "включена"
		} else {
			deviceTypeVerb = "выключена"
		}
	case SwitchDeviceType:
		if isOn {
			deviceTypeVerb = "включен"
		} else {
			deviceTypeVerb = "выключен"
		}
	case HubDeviceType:
		if isOn {
			deviceTypeVerb = "включен"
		} else {
			deviceTypeVerb = "выключен"
		}
	case PurifierDeviceType:
		if isOn {
			deviceTypeVerb = "включен"
		} else {
			deviceTypeVerb = "выключен"
		}
	case HumidifierDeviceType:
		if isOn {
			deviceTypeVerb = "включен"
		} else {
			deviceTypeVerb = "выключен"
		}
	case VacuumCleanerDeviceType:
		if isOn {
			deviceTypeVerb = "включен"
		} else {
			deviceTypeVerb = "выключен"
		}
	case CookingDeviceType:
		if isOn {
			deviceTypeVerb = "включен"
		} else {
			deviceTypeVerb = "выключен"
		}
	case KettleDeviceType:
		if isOn {
			deviceTypeVerb = "включен"
		} else {
			deviceTypeVerb = "выключен"
		}
	case CoffeeMakerDeviceType:
		if isOn {
			deviceTypeVerb = "включена"
		} else {
			deviceTypeVerb = "выключена"
		}
	case MulticookerDeviceType:
		if isOn {
			deviceTypeVerb = "включена"
		} else {
			deviceTypeVerb = "выключена"
		}
	case ThermostatDeviceType:
		if isOn {
			deviceTypeVerb = "включен"
		} else {
			deviceTypeVerb = "выключен"
		}
	case AcDeviceType:
		if isOn {
			deviceTypeVerb = "включен"
		} else {
			deviceTypeVerb = "выключен"
		}
	case MediaDeviceDeviceType:
		if isOn {
			deviceTypeVerb = "включено"
		} else {
			deviceTypeVerb = "выключено"
		}
	case TvDeviceDeviceType:
		if isOn {
			deviceTypeVerb = "включен"
		} else {
			deviceTypeVerb = "выключен"
		}
	case ReceiverDeviceType:
		if isOn {
			deviceTypeVerb = "включен"
		} else {
			deviceTypeVerb = "выключен"
		}
	case TvBoxDeviceType:
		if isOn {
			deviceTypeVerb = "включена"
		} else {
			deviceTypeVerb = "выключена"
		}
	case WashingMachineDeviceType:
		if isOn {
			deviceTypeVerb = "включена"
		} else {
			deviceTypeVerb = "выключена"
		}
	case DishwasherDeviceType:
		if isOn {
			deviceTypeVerb = "включена"
		} else {
			deviceTypeVerb = "выключена"
		}
	case RefrigeratorDeviceType:
		if isOn {
			deviceTypeVerb = "включен"
		} else {
			deviceTypeVerb = "выключен"
		}
	case FanDeviceType:
		if isOn {
			deviceTypeVerb = "включен"
		} else {
			deviceTypeVerb = "выключен"
		}
	case IronDeviceType:
		if isOn {
			deviceTypeVerb = "включен"
		} else {
			deviceTypeVerb = "выключен"
		}
	case PetFeederDeviceType:
		if isOn {
			deviceTypeVerb = "включена"
		} else {
			deviceTypeVerb = "выключена"
		}
	case OpenableDeviceType:
		if isOn {
			deviceTypeVerb = "открыто"
		} else {
			deviceTypeVerb = "закрыто"
		}
	case CurtainDeviceType:
		if isOn {
			deviceTypeVerb = "открыты"
		} else {
			deviceTypeVerb = "закрыты"
		}
	case SensorDeviceType:
		if isOn {
			deviceTypeVerb = "включен"
		} else {
			deviceTypeVerb = "выключен"
		}
	case CameraDeviceType:
		if isOn {
			deviceTypeVerb = "включена"
		} else {
			deviceTypeVerb = "выключена"
		}
	case OtherDeviceType:
		if isOn {
			deviceTypeVerb = "включено"
		} else {
			deviceTypeVerb = "выключено"
		}
	}

	if strings.Contains(strings.ToLower(deviceName), strings.ToLower(generatedDeviceTypeName)) {
		target = deviceName
		targetVerb = deviceTypeVerb
	}

	// if we are still not sure about grammatical gender of target - generate it using device type name
	if len(target) == 0 && len(targetVerb) == 0 {
		target = fmt.Sprintf("%s %s", generatedDeviceTypeName, deviceName)
		targetVerb = deviceTypeVerb
	}

	if nlgOptions.UseRoomTarget {
		target = fmt.Sprintf("%s %s", target, nlgOptions.InRoomNLG())
	}

	if nlgOptions.UseDeviceTarget {
		return tools.FormatNLGText(fmt.Sprintf("%s %s", target, targetVerb))
	}
	return tools.FormatNLGText(targetVerb)
}

func GetModeCapabilityVoiceStatus(state ModeCapabilityState, nlgOptions TargetNLGOptions) string {
	var instanceName string
	switch state.Instance {
	case ThermostatModeInstance:
		instanceName = "Режим работы термостата"
	case FanSpeedModeInstance:
		instanceName = "Скорость вентиляции"
	case WorkSpeedModeInstance:
		instanceName = "Скорость работы"
	case CleanUpModeInstance:
		instanceName = "Режим уборки"
	case ProgramModeInstance:
		instanceName = "Программа"
	case InputSourceModeInstance:
		instanceName = "Источник сигнала"
	case CoffeeModeInstance:
		instanceName = "Режим приготовления кофе"
	case SwingModeInstance:
		instanceName = "Направление воздуха"
	case HeatModeInstance:
		instanceName = "Режим нагрева"
	case DishwashingModeInstance:
		instanceName = "Режим мойки посуды"
	case TeaModeInstance:
		instanceName = "Режим приготовления чая"
	default:
		instanceName = "Режим"
	}
	modeValueName := GetModeNameForInstance(state.Instance, state.Value)

	var msg string
	if nlgOptions.UseDeviceTarget && nlgOptions.UseRoomTarget {
		msg = fmt.Sprintf("%s на %s %s - %s", instanceName, nlgOptions.DeviceInflection.Pr, nlgOptions.InRoomNLG(), modeValueName)
	} else if nlgOptions.UseDeviceTarget {
		msg = fmt.Sprintf("%s на %s - %s", instanceName, nlgOptions.DeviceInflection.Pr, modeValueName)
	} else {
		msg = fmt.Sprintf("%s - %s", instanceName, modeValueName)
	}
	return tools.FormatNLGText(msg)
}

func GetRangeCapabilityVoiceStatus(state RangeCapabilityState, parameters RangeCapabilityParameters, nlgOptions TargetNLGOptions) string {
	var msg string
	switch state.Instance {
	case BrightnessRangeInstance:
		instanceName := "Яркость"
		value := GetUnitFloatValueForVoiceStatus(state.Value, parameters.Unit)
		if nlgOptions.UseDeviceTarget && nlgOptions.UseRoomTarget {
			msg = fmt.Sprintf("%s на %s %s - %s", instanceName, nlgOptions.DeviceInflection.Pr, nlgOptions.InRoomNLG(), value)
		} else if nlgOptions.UseDeviceTarget {
			msg = fmt.Sprintf("%s на %s - %s", instanceName, nlgOptions.DeviceInflection.Pr, value)
		} else {
			msg = fmt.Sprintf("%s - %s", instanceName, value)
		}
	case TemperatureRangeInstance:
		instanceName := "Целевая температура"
		value := GetUnitFloatValueForVoiceStatus(state.Value, parameters.Unit)
		if nlgOptions.UseDeviceTarget && nlgOptions.UseRoomTarget {
			msg = fmt.Sprintf("%s на %s %s %s", instanceName, nlgOptions.DeviceInflection.Pr, nlgOptions.InRoomNLG(), value)
		} else if nlgOptions.UseDeviceTarget {
			msg = fmt.Sprintf("%s на %s %s", instanceName, nlgOptions.DeviceInflection.Pr, value)
		} else {
			msg = fmt.Sprintf("%s %s", instanceName, value)
		}
	case VolumeRangeInstance:
		instanceName := "Громкость"
		value := int64(math.Round(state.Value))
		if nlgOptions.UseDeviceTarget && nlgOptions.UseRoomTarget {
			msg = fmt.Sprintf("%s на %s %s %d", instanceName, nlgOptions.DeviceInflection.Pr, nlgOptions.InRoomNLG(), value)
		} else if nlgOptions.UseDeviceTarget {
			msg = fmt.Sprintf("%s на %s %d", instanceName, nlgOptions.DeviceInflection.Pr, value)
		} else {
			msg = fmt.Sprintf("%s %d", instanceName, value)
		}
	case ChannelRangeInstance:
		value := int64(math.Round(state.Value))
		if nlgOptions.UseDeviceTarget && nlgOptions.UseRoomTarget {
			msg = fmt.Sprintf("На %s %s работает %d канал", nlgOptions.DeviceInflection.Pr, nlgOptions.InRoomNLG(), value)
		} else if nlgOptions.UseDeviceTarget {
			msg = fmt.Sprintf("На %s работает %d канал", nlgOptions.DeviceInflection.Pr, value)
		} else {
			msg = fmt.Sprintf("Работает %d канал", value)
		}
	case HumidityRangeInstance:
		instanceName := "Целевая влажность"
		value := GetUnitFloatValueForVoiceStatus(state.Value, parameters.Unit)
		if nlgOptions.UseDeviceTarget && nlgOptions.UseRoomTarget {
			msg = fmt.Sprintf("%s на %s %s %s", instanceName, nlgOptions.DeviceInflection.Pr, nlgOptions.InRoomNLG(), value)
		} else if nlgOptions.UseDeviceTarget {
			msg = fmt.Sprintf("%s на %s %s", instanceName, nlgOptions.DeviceInflection.Pr, value)
		} else {
			msg = fmt.Sprintf("%s %s", instanceName, value)
		}
	case OpenRangeInstance:
		value := GetUnitFloatValueForVoiceStatus(state.Value, parameters.Unit)
		if nlgOptions.UseDeviceTarget && nlgOptions.UseRoomTarget {
			switch target := strings.ToLower(nlgOptions.DeviceName); target {
			case "штора", "дверь":
				msg = fmt.Sprintf("%s %s открыта на %s", target, nlgOptions.InRoomNLG(), value)
			case "шторы", "жалюзи", "гардины", "занавески":
				msg = fmt.Sprintf("%s %s открыты на %s", target, nlgOptions.InRoomNLG(), value)
			default:
				msg = fmt.Sprintf("%s %s открыто на %s", target, nlgOptions.InRoomNLG(), value)
			}
		} else if nlgOptions.UseDeviceTarget {
			switch target := strings.ToLower(nlgOptions.DeviceName); target {
			case "штора", "дверь":
				msg = fmt.Sprintf("%s открыта на %s", target, value)
			case "шторы", "жалюзи", "гардины", "занавески":
				msg = fmt.Sprintf("%s открыты на %s", target, value)
			default:
				msg = fmt.Sprintf("%s открыто на %s", target, value)
			}
		} else {
			msg = fmt.Sprintf("Открыто на %s", value)
		}
	}
	return tools.FormatNLGText(msg)
}

func GetToggleCapabilityVoiceStatus(state ToggleCapabilityState, nlgOptions TargetNLGOptions) string {
	var instanceName string
	var verb string
	switch state.Instance {
	case MuteToggleCapabilityInstance:
		instanceName = "Звук"
		if state.Value {
			verb = "выключен"
		} else {
			verb = "включен"
		}
	case BacklightToggleCapabilityInstance:
		instanceName = "Подсветка"
		if state.Value {
			verb = "включена"
		} else {
			verb = "выключена"
		}
	case ControlsLockedToggleCapabilityInstance:
		instanceName = "Управление"
		if state.Value {
			verb = "заблокировано"
		} else {
			verb = "разблокировано"
		}
	case IonizationToggleCapabilityInstance:
		instanceName = "Ионизация"
		if state.Value {
			verb = "включена"
		} else {
			verb = "выключена"
		}
	case OscillationToggleCapabilityInstance:
		instanceName = "Вращение"
		if state.Value {
			verb = "включено"
		} else {
			verb = "выключено"
		}
	case KeepWarmToggleCapabilityInstance:
		instanceName = "Поддержание тепла"
		if state.Value {
			verb = "включено"
		} else {
			verb = "выключено"
		}
	case PauseToggleCapabilityInstance:
		instanceName = "Пауза"
		if state.Value {
			verb = "включена"
		} else {
			verb = "выключена"
		}
	case TrunkToggleCapabilityInstance:
		instanceName = "Багажник"
		if state.Value {
			verb = "закрыт"
		} else {
			verb = "открыт"
		}
	case CentralLockCapabilityInstance:
		instanceName = "Центральный замок"
		if state.Value {
			verb = "закрыт"
		} else {
			verb = "открыт"
		}
	}

	var msg string
	if nlgOptions.UseDeviceTarget && nlgOptions.UseRoomTarget {
		msg = fmt.Sprintf("%s на %s %s %s", instanceName, nlgOptions.DeviceInflection.Pr, nlgOptions.InRoomNLG(), verb)
	} else if nlgOptions.UseDeviceTarget {
		msg = fmt.Sprintf("%s на %s %s", instanceName, nlgOptions.DeviceInflection.Pr, verb)
	} else {
		msg = fmt.Sprintf("%s %s", instanceName, verb)
	}
	return tools.FormatNLGText(msg)
}

func GetColorCapabilityVoiceStatus(state ColorSettingCapabilityState, nlgOptions TargetNLGOptions) (string, bool) {
	switch state.Instance {
	case SceneCapabilityInstance:
		colorSceneID := state.Value.(ColorSceneID)
		if colorScene, ok := KnownColorScenes[colorSceneID]; ok {
			var msg string
			if nlgOptions.UseDeviceTarget && nlgOptions.UseRoomTarget {
				msg = fmt.Sprintf("На %s %s выбран режим %s", nlgOptions.DeviceInflection.Pr, nlgOptions.InRoomNLG(), colorScene.Name)
			} else if nlgOptions.UseDeviceTarget {
				msg = fmt.Sprintf("На %s выбран режим %s", nlgOptions.DeviceInflection.Pr, colorScene.Name)
			} else {
				msg = fmt.Sprintf("Выбран режим %s", colorScene.Name)
			}
			return tools.FormatNLGText(msg), true
		}
		return "На устройстве выбран неизвестный режим", false
	default:
		color, ok := state.ToColor()
		if !ok {
			return "Даже не знаю, как назвать этот оттенок. Кажется, это серо-буро-малиновый.", false
		}
		if color.Name == "" {
			switch color.Type {
			case Multicolor:
				return "Кажется, ваше устройство сейчас в цветном режиме.", false
			case WhiteColor:
				return "Кажется, ваше устройство сейчас в белом режиме.", false
			default:
				return "Даже не знаю, как назвать этот оттенок. Кажется, это серо-буро-малиновый.", false
			}
		}
		var msg string
		if nlgOptions.UseDeviceTarget && nlgOptions.UseRoomTarget {
			msg = fmt.Sprintf("На %s %s горит %s цвет", nlgOptions.DeviceInflection.Pr, nlgOptions.InRoomNLG(), color.Name)
		} else if nlgOptions.UseDeviceTarget {
			msg = fmt.Sprintf("На %s горит %s цвет", nlgOptions.DeviceInflection.Pr, color.Name)
		} else {
			msg = fmt.Sprintf("Горит %s цвет", color.Name)
		}
		return tools.FormatNLGText(msg), true
	}
}

func GetDeviceStateVoiceStatus(device Device, nlgOptions TargetNLGOptions) string {
	deviceMessages := make([]string, 0, len(device.Capabilities)+len(device.Properties))
	onOffCapability, hasOnOff := device.GetCapabilityByTypeAndInstance(OnOffCapabilityType, string(OnOnOffCapabilityInstance))

	var onOffMsg string
	if hasOnOff && onOffCapability.State() != nil {
		state := onOffCapability.State().(OnOffCapabilityState)
		onOffMsg = GetDeviceOnOffCapabilityVoiceStatus(state, device.Type, nlgOptions)
		if isOn := state.Value; !isOn {
			return onOffMsg
		}
	} else {
		if nlgOptions.UseRoomTarget {
			onOffMsg = fmt.Sprintf("%s %s сейчас в сети", device.Name, nlgOptions.InRoomNLG())
		} else {
			onOffMsg = fmt.Sprintf("%s сейчас в сети", device.Name)
		}
	}
	deviceMessages = append(deviceMessages, onOffMsg)

	for _, c := range device.Capabilities {
		if c.State() == nil {
			continue
		}
		switch c.Type() {
		case ColorSettingCapabilityType:
			state := c.State().(ColorSettingCapabilityState)
			colorMsg, hasColorStatus := GetColorCapabilityVoiceStatus(state, TargetNLGOptions{UseDeviceTarget: false})
			// sometimes color is not inside our palette and we can't name it - so we skip this status
			if hasColorStatus {
				deviceMessages = append(deviceMessages, strings.ToLower(colorMsg))
			}
		case RangeCapabilityType:
			state := c.State().(RangeCapabilityState)
			parameters := c.Parameters().(RangeCapabilityParameters)
			rangeMsg := GetRangeCapabilityVoiceStatus(state, parameters, TargetNLGOptions{UseDeviceTarget: false})
			deviceMessages = append(deviceMessages, strings.ToLower(rangeMsg))
		case ModeCapabilityType:
			state := c.State().(ModeCapabilityState)
			modeMsg := GetModeCapabilityVoiceStatus(state, TargetNLGOptions{UseDeviceTarget: false})
			deviceMessages = append(deviceMessages, strings.ToLower(modeMsg))
		case ToggleCapabilityType:
			state := c.State().(ToggleCapabilityState)
			isOff := !state.Value
			isPause := state.Instance == PauseToggleCapabilityInstance
			isMute := state.Instance == MuteToggleCapabilityInstance
			if shouldSkip := isOff && (isMute || isPause); shouldSkip {
				// pause and mute are off by default, no reason to voice their status
				continue
			}
			toggleMsg := GetToggleCapabilityVoiceStatus(state, TargetNLGOptions{UseDeviceTarget: false})
			deviceMessages = append(deviceMessages, strings.ToLower(toggleMsg))
		}
	}

	unknownStateProperties := make(Properties, 0, len(device.Properties))
	for _, p := range device.Properties {
		switch p.Type() {
		case FloatPropertyType:
			if p.State() == nil {
				unknownStateProperties = append(unknownStateProperties, p)
				continue
			}
			parameters := p.Parameters().(FloatPropertyParameters)
			state := p.State().(FloatPropertyState)

			propertyMsg := GetFloatPropertyVoiceStatus(state, parameters, TargetNLGOptions{UseDeviceTarget: false})
			deviceMessages = append(deviceMessages, propertyMsg)
		}
	}

	// https://st.yandex-team.ru/IOT-1445
	// instead of skipping unknown property states we want to explicitly state this fact
	if len(unknownStateProperties) > 0 && len(unknownStateProperties) == len(device.Properties) {
		// we know nothing, as jon snow did
		deviceMessages = append(deviceMessages, "данные по датчикам будут позже, но всё работает в штатном режиме!")
	} else if len(unknownStateProperties) > 0 {
		unknownStateInstanceNames := make([]string, 0, len(unknownStateProperties))
		for _, p := range unknownStateProperties {
			instanceName := KnownPropertyInstanceNames[PropertyInstance(p.Parameters().GetInstance())]
			unknownStateInstanceNames = append(unknownStateInstanceNames, instanceName)
		}
		if len(unknownStateInstanceNames) > 1 {
			// best case scenario - no grammatical AGR in gender between instance name and last part
			// example: "Температура, уровень углекислого газа, <че угодно еще в именительном падеже> мне пока неизвестны"
			deviceMessages = append(deviceMessages, fmt.Sprintf("%s мне пока неизвестны", strings.Join(unknownStateInstanceNames, ", ")))
		} else {
			// hell on earth - grammatical AGR is required
			// example:
			// Температура мне пока неизвестна
			// Уровень углекислого газа мне пока неивестен
			// Давление мне пока неизвестно
			// russian language makes me sad
			switch instance := unknownStateProperties[0].Parameters().GetInstance(); {
			case KnownFemalePropertyInstanceNames[instance]:
				deviceMessages = append(deviceMessages, fmt.Sprintf("%s мне пока неизвестна", unknownStateInstanceNames[0]))
			case KnownMalePropertyInstanceNames[instance]:
				deviceMessages = append(deviceMessages, fmt.Sprintf("%s мне пока неизвестен", unknownStateInstanceNames[0]))
			case KnownNeuterPropertyInstanceNames[instance]:
				deviceMessages = append(deviceMessages, fmt.Sprintf("%s мне пока неизвестно", unknownStateInstanceNames[0]))
			}
		}
	}
	return tools.FormatNLGText(strings.Join(deviceMessages, ", "))
}

func GetHouseholdSpecifiedNLG(householdInflection inflector.Inflection) NLGStruct {
	householdAddition := GetHouseholdAddition(householdInflection, false)
	variants := []string{
		"Окей, сделала",
		"Сделала",
		"Выполнила",
	}
	for i := range variants {
		variants[i] = fmt.Sprintf("%s %s", variants[i], householdAddition)
	}
	return NLGStruct{Variants: variants}
}

func GetHouseholdAddition(householdInflection inflector.Inflection, forSuggests bool) string {
	switch {
	case tools.IsAlphanumericEqual(householdInflection.Im, "Дача"):
		return "на даче"
	case tools.IsAlphanumericEqual(householdInflection.Im, "Мой дом"):
		if forSuggests {
			return "в моем доме"
		}
		return "в вашем доме"
	case tools.IsAlphanumericEqual(householdInflection.Im, "Работа"):
		return "на работе"
	case tools.IsAlphanumericEqual(householdInflection.Im, "Родители"):
		return "у родителей"
	default:

		return fmt.Sprintf("в %s", tools.Standardize(householdInflection.Pr))
	}
}
