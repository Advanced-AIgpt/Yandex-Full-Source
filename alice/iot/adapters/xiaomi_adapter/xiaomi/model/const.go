package model

import (
	"a.yandex-team.ru/alice/iot/adapters/xiaomi_adapter/xiaomi/iotapi"
	"a.yandex-team.ru/alice/iot/bulbasaur/model"
)

var deviceInfoProperties = []string{
	manufacturerProperty, modelProperty, firmwareVersionProperty,
}
var stateProperties = []string{
	onProperty, brightnessProperty, colorProperty,
	colorTemperatureProperty, modeProperty, targetTemperatureProperty,
	statusProperty, motorControlProperty, fanLevelProperty, speedLevelProperty,
	currentPositionProperty, temperatureProperty, waterLevelProperty, batteryLevelProperty,
	relativeHumidityProperty, atmosphericPressureProperty, motionStateProperty, illuminationProperty,
	contactStateProperty, gasConcentrationProperty, smokeConcentrationProperty,
	pm1DensityProperty, pm2p5DensityProperty, pm10DensityProperty,
}
var buttonEvents = []string{clickEvent, doubleClickEvent, longPressEvent}
var waterLeakEvents = []string{submersionDetectedEvent, noSubmersionDetectedEvent}
var vibrationEvents = []string{tiltEvent, fallEvent, vibrationEvent}
var contactEvents = []string{openEvent, closeEvent}

var yandexAcModesMap map[float64]model.ModeValue
var yandexFanModesMap map[float64]model.ModeValue
var yandexVacuumModesMap map[float64]model.ModeValue
var yandexVacuumModes100Map map[float64]model.ModeValue
var yandexHeaterModesMap map[float64]model.ModeValue

var xiaomiAcModesMap map[model.ModeValue]float64
var xiaomiFanModesMap map[model.ModeValue]float64
var xiaomiVacuumModesMap map[model.ModeValue]float64
var xiaomiVacuumModes100Map map[model.ModeValue]float64 // xiaomi values starts from 101, 102, etc
var xiaomiHeaterModesMap map[model.ModeValue]float64
var xiaomiBatteryLevelEvents = map[float64]model.EventValue{
	1: model.NormalEvent,
	2: model.LowEvent,
}

type AccessType string

const ReadAccess AccessType = "read"
const NotifyAccess AccessType = "notify"

const SonoffCloudID int = 438

//categories
const (
	airconCategory string = "air-conditioner"
	//irCategory     string = "remote-control"
	//kettleCategory string = "kettle"
	gatewayCategory                   string = "gateway"
	lightCategory                     string = "light"
	socketCategory                    string = "outlet"
	switchCategory                    string = "switch"
	tvCategory                        string = "television"
	tvBoxCategory                     string = "tv-box"
	vacuumCategory                    string = "vacuum"
	temperatureHumiditySensorCategory string = "temperature-humidity-sensor"
	airPurifierCategory               string = "air-purifier"
	curtainCategory                   string = "curtain"
	heaterCategory                    string = "heater"
	humidifierCategory                string = "humidifier"
	motionSensorCategory              string = "motion-sensor"
	illuminationSensorCategory        string = "illumination-sensor"
	magnetSensorCategory              string = "magnet-sensor"
	remoteControlCategory             string = "remote-control"
	submersionSensorCategory          string = "submersion-sensor"
	gasSensorCategory                 string = "gas-sensor"
	smokeSensorCategory               string = "smoke-sensor"
	vibrationSensorCategory           string = "vibration-sensor"
)

type Categories map[string]model.DeviceType

func (c Categories) GetDeviceType(category string) model.DeviceType {
	if deviceType, known := c[category]; known {
		return deviceType
	}
	return model.OtherDeviceType
}

func (c Categories) IsKnown(category string) bool {
	_, known := c[category]
	return known
}

var KnownDeviceCategories Categories

//types
const (
	vacuumType  string = "urn:miot-spec-v2:device:vacuum:"
	curtainType string = "urn:miot-spec-v2:device:curtain:"
	tvType      string = "urn:miot-spec-v2:device:television:"
	tvBoxType   string = "urn:miot-spec-v2:device:tv-box:"
	acType      string = "urn:miot-spec-v2:device:air-conditioner:"
)

//services
const (
	deviceInfoService                string = "urn:miot-spec-v2:service:device-information:"
	acService                        string = "urn:miot-spec-v2:service:air-conditioner:"
	fanService                       string = "urn:miot-spec-v2:service:fan-control:"
	lightService                     string = "urn:miot-spec-v2:service:light:"
	switchService                    string = "urn:miot-spec-v2:service:switch:"
	vacuumService                    string = "urn:miot-spec-v2:service:vacuum:"
	temperatureHumiditySensorService string = "urn:miot-spec-v2:service:temperature-humidity-sensor:"
	kettleService                    string = "urn:miot-spec-v2:service:kettle:"
	airPurifierService               string = "urn:miot-spec-v2:service:air-purifier:"
	curtainService                   string = "urn:miot-spec-v2:service:curtain:"
	heaterService                    string = "urn:miot-spec-v2:service:heater:"
	humidifierService                string = "urn:miot-spec-v2:service:humidifier:"
	environmentService               string = "urn:miot-spec-v2:service:environment:"
	batteryService                   string = "urn:miot-spec-v2:service:battery:"
	motionSensorService              string = "urn:miot-spec-v2:service:motion-sensor:"
	illuminationSensorService        string = "urn:miot-spec-v2:service:illumination-sensor:"
	magnetSensorService              string = "urn:miot-spec-v2:service:magnet-sensor:"
	switchSensorService              string = "urn:miot-spec-v2:service:switch-sensor:"
	submersionSensorService          string = "urn:miot-spec-v2:service:submersion-sensor:"
	gasSensorService                 string = "urn:miot-spec-v2:service:gas-sensor:"
	smokeSensorService               string = "urn:miot-spec-v2:service:smoke-sensor:"
	vibrationSensorService           string = "urn:miot-spec-v2:service:vibration-sensor:"

	irAcService    string = "urn:miot-spec-v2:service:ir-aircondition-control:"
	irTvService    string = "urn:miot-spec-v2:service:ir-tv-control:"
	irTvBoxService string = "urn:miot-spec-v2:service:ir-box-control:"
	//irFanService string = "urn:miot-spec-v2:service:ir-fan-control:" TODO: check if is used. 20.01.2020 no one had one
)

//properties
const (
	manufacturerProperty    string = "urn:miot-spec-v2:property:manufacturer:"
	modelProperty           string = "urn:miot-spec-v2:property:model:"
	firmwareVersionProperty string = "urn:miot-spec-v2:property:firmware-revision:"

	onProperty                  string = "urn:miot-spec-v2:property:on:"
	brightnessProperty          string = "urn:miot-spec-v2:property:brightness:"
	colorProperty               string = "urn:miot-spec-v2:property:color:"
	colorTemperatureProperty    string = "urn:miot-spec-v2:property:color-temperature:"
	modeProperty                string = "urn:miot-spec-v2:property:mode:"
	temperatureProperty         string = "urn:miot-spec-v2:property:temperature:"
	relativeHumidityProperty    string = "urn:miot-spec-v2:property:relative-humidity:"
	atmosphericPressureProperty string = "urn:miot-spec-v2:property:atmospheric-pressure:"
	waterLevelProperty          string = "urn:miot-spec-v2:property:water-level:"
	batteryLevelProperty        string = "urn:miot-spec-v2:property:battery-level:"
	targetTemperatureProperty   string = "urn:miot-spec-v2:property:target-temperature:"
	verticalSwingProperty       string = "urn:miot-spec-v2:property:vertical-swing:"
	fanLevelProperty            string = "urn:miot-spec-v2:property:fan-level:"
	statusProperty              string = "urn:miot-spec-v2:property:status:"
	speedLevelProperty          string = "urn:miot-spec-v2:property:speed-level:"
	motorControlProperty        string = "urn:miot-spec-v2:property:motor-control:"
	currentPositionProperty     string = "urn:miot-spec-v2:property:current-position:"
	targetPositionProperty      string = "urn:miot-spec-v2:property:target-position:"
	motionStateProperty         string = "urn:miot-spec-v2:property:motion-state:"
	illuminationProperty        string = "urn:miot-spec-v2:property:illumination:"
	contactStateProperty        string = "urn:miot-spec-v2:property:contact-state:"
	gasConcentrationProperty    string = "urn:miot-spec-v2:property:gas-concentration:"
	smokeConcentrationProperty  string = "urn:miot-spec-v2:property:smoke-concentration:"
	submersionStateProperty     string = "urn:miot-spec-v2:property:submersion-state:"
	pm1DensityProperty          string = "urn:miot-spec-v2:property:pm1-density:"
	pm2p5DensityProperty        string = "urn:miot-spec-v2:property:pm2.5-density:"
	pm10DensityProperty         string = "urn:miot-spec-v2:property:pm10-density:"

	irModeProperty string = "urn:miot-spec-v2:property:ir-mode:"
	// urn:miot-spec-v2:property:ir-temperature: conflicts with actions
)

//actions
const (
	startSweepAction   string = "urn:miot-spec-v2:action:start-sweep:"
	stopSweepAction    string = "urn:miot-spec-v2:action:stop-sweep:"
	stopSweepingAction string = "urn:miot-spec-v2:action:stop-sweeping:"
	startChargeAction  string = "urn:miot-spec-v2:action:start-charge:"

	turnOnAction      string = "urn:miot-spec-v2:action:turn-on:"
	turnOffAction     string = "urn:miot-spec-v2:action:turn-off:"
	channelUpAction   string = "urn:miot-spec-v2:action:channel-up:"
	channelDownAction string = "urn:miot-spec-v2:action:channel-down:"
	volumeUpAction    string = "urn:miot-spec-v2:action:volume-up:"
	volumeDownAction  string = "urn:miot-spec-v2:action:volume-down:"
	muteOnAction      string = "urn:miot-spec-v2:action:mute-on:"
	muteOffAction     string = "urn:miot-spec-v2:action:mute-off:"
	//inputSourceSwitchAction string = "urn:miot-spec-v2:action:input-source-switch:" unsure about how this one works

	temperatureUpAction   string = "urn:miot-spec-v2:action:temperature-up:"
	temperatureDownAction string = "urn:miot-spec-v2:action:temperature-down:"
	//fanSpeedUpAction      string = "urn:miot-spec-v2:action:fan-speed-up:" TODO: add range for unretrievable fanspeed
	//fanSpeedDownAction    string = "urn:miot-spec-v2:action:temperature-down:"
	//horizontal-swing
)

//events
const (
	clickEvent       string = "urn:miot-spec-v2:event:click:"
	doubleClickEvent string = "urn:miot-spec-v2:event:double-click:"
	longPressEvent   string = "urn:miot-spec-v2:event:long-press:"

	submersionDetectedEvent   string = "urn:miot-spec-v2:event:submersion-detected:"
	noSubmersionDetectedEvent string = "urn:miot-spec-v2:event:no-submersion:"

	tiltEvent      string = "urn:miot-spec-v2:event:tilt:"
	fallEvent      string = "urn:miot-spec-v2:event:fall:"
	vibrationEvent string = "urn:miot-spec-v2:event:vibration:"

	openEvent  string = "urn:miot-spec-v2:event:open:"
	closeEvent string = "urn:miot-spec-v2:event:close:"
)

//modes
const (
	colorMode float64 = 1
	dayMode   float64 = 2
)

const (
	autoMode float64 = 0
	coolMode float64 = 1
	dryMode  float64 = 2
	heatMode float64 = 3
	fanMode  float64 = 4
)

const (
	autoHeaterMode   float64 = 0
	maxHeaterMode    float64 = 1
	normalHeaterMode float64 = 2
	minHeaterMode    float64 = 3
)

const (
	silentMode    float64 = 1
	basicMode     float64 = 2
	strongMode    float64 = 3
	fullSpeedMode float64 = 4
)

// values goes from 101, 102, etc
const (
	silentMode101    float64 = 101
	basicMode102     float64 = 102
	strongMode103    float64 = 103
	fullSpeedMode104 float64 = 104
)

const (
	autoFanMode   float64 = 0
	lowFanMode    float64 = 1
	mediumFanMode float64 = 2
	highFanMode   float64 = 3
)

const (
	openCurtainMode  float64 = 1
	closeCurtainMode float64 = 2
)

const (
	idleStatus     float64 = 1
	sweepingStatus float64 = 2
	chargingStatus float64 = 3
	pausedStatus   float64 = 4
)

//units
const (
	kelvinUnit  string = "kelvin"
	percentUnit string = "percentage"
	pascalUnit  string = "pascal"
	noneUnit    string = "none"
)
const (
	// https://iot.mi.com/new/doc/guidelines-for-access/other-platform-access/control-api
	OkStatus                        iotapi.Status = 0
	DeviceOfflineStatus             iotapi.Status = -704042011
	NotSupportedInCurrentModeStatus iotapi.Status = -704053100
	OperationTimeoutStatus          iotapi.Status = -704083036
	UnauthorizedStatus              iotapi.Status = -704010000
)

func init() {
	yandexAcModesMap = make(map[float64]model.ModeValue)
	yandexAcModesMap[autoMode] = model.AutoMode
	yandexAcModesMap[coolMode] = model.CoolMode
	yandexAcModesMap[dryMode] = model.DryMode
	yandexAcModesMap[heatMode] = model.HeatMode
	yandexAcModesMap[fanMode] = model.FanOnlyMode

	yandexFanModesMap = make(map[float64]model.ModeValue)
	yandexFanModesMap[autoFanMode] = model.AutoMode
	yandexFanModesMap[lowFanMode] = model.LowMode
	yandexFanModesMap[mediumFanMode] = model.MediumMode
	yandexFanModesMap[highFanMode] = model.HighMode

	yandexVacuumModesMap = make(map[float64]model.ModeValue)
	yandexVacuumModesMap[silentMode] = model.QuietMode
	yandexVacuumModesMap[basicMode] = model.NormalMode
	yandexVacuumModesMap[strongMode] = model.FastMode
	yandexVacuumModesMap[fullSpeedMode] = model.TurboMode

	yandexHeaterModesMap = make(map[float64]model.ModeValue)
	yandexHeaterModesMap[autoHeaterMode] = model.AutoMode
	yandexHeaterModesMap[maxHeaterMode] = model.MaxMode
	yandexHeaterModesMap[normalHeaterMode] = model.NormalMode
	yandexHeaterModesMap[minHeaterMode] = model.MinMode

	yandexVacuumModes100Map = make(map[float64]model.ModeValue)
	yandexVacuumModes100Map[silentMode101] = model.QuietMode
	yandexVacuumModes100Map[basicMode102] = model.NormalMode
	yandexVacuumModes100Map[strongMode103] = model.FastMode
	yandexVacuumModes100Map[fullSpeedMode104] = model.TurboMode

	xiaomiAcModesMap = make(map[model.ModeValue]float64)
	xiaomiAcModesMap[model.AutoMode] = autoMode
	xiaomiAcModesMap[model.CoolMode] = coolMode
	xiaomiAcModesMap[model.DryMode] = dryMode
	xiaomiAcModesMap[model.HeatMode] = heatMode
	xiaomiAcModesMap[model.FanOnlyMode] = fanMode

	xiaomiFanModesMap = make(map[model.ModeValue]float64)
	xiaomiFanModesMap[model.AutoMode] = autoFanMode
	xiaomiFanModesMap[model.LowMode] = lowFanMode
	xiaomiFanModesMap[model.MediumMode] = mediumFanMode
	xiaomiFanModesMap[model.HighMode] = highFanMode

	xiaomiHeaterModesMap = make(map[model.ModeValue]float64)
	xiaomiHeaterModesMap[model.AutoMode] = autoHeaterMode
	xiaomiHeaterModesMap[model.MinMode] = minHeaterMode
	xiaomiHeaterModesMap[model.NormalMode] = normalHeaterMode
	xiaomiHeaterModesMap[model.MaxMode] = maxHeaterMode

	xiaomiVacuumModesMap = make(map[model.ModeValue]float64)
	xiaomiVacuumModesMap[model.QuietMode] = silentMode
	xiaomiVacuumModesMap[model.NormalMode] = basicMode
	xiaomiVacuumModesMap[model.FastMode] = strongMode
	xiaomiVacuumModesMap[model.TurboMode] = fullSpeedMode

	xiaomiVacuumModes100Map = make(map[model.ModeValue]float64)
	xiaomiVacuumModes100Map[model.QuietMode] = silentMode101
	xiaomiVacuumModes100Map[model.NormalMode] = basicMode102
	xiaomiVacuumModes100Map[model.FastMode] = strongMode103
	xiaomiVacuumModes100Map[model.TurboMode] = fullSpeedMode104

	KnownDeviceCategories = make(map[string]model.DeviceType)
	KnownDeviceCategories[lightCategory] = model.LightDeviceType
	KnownDeviceCategories[gatewayCategory] = model.LightDeviceType
	KnownDeviceCategories[socketCategory] = model.SocketDeviceType
	KnownDeviceCategories[switchCategory] = model.SwitchDeviceType
	KnownDeviceCategories[airconCategory] = model.AcDeviceType
	KnownDeviceCategories[curtainCategory] = model.CurtainDeviceType
	KnownDeviceCategories[humidifierCategory] = model.HumidifierDeviceType
	KnownDeviceCategories[vacuumCategory] = model.VacuumCleanerDeviceType
	KnownDeviceCategories[airPurifierCategory] = model.PurifierDeviceType
	KnownDeviceCategories[heaterCategory] = model.ThermostatDeviceType
	//KnownDeviceCategories[tvCategory] = model.TvDeviceDeviceType
	//KnownDeviceCategories[tvBoxCategory] = model.MediaDeviceDeviceType

	KnownDeviceCategories[temperatureHumiditySensorCategory] = model.SensorDeviceType
	KnownDeviceCategories[motionSensorCategory] = model.SensorDeviceType
	KnownDeviceCategories[illuminationSensorCategory] = model.SensorDeviceType
	KnownDeviceCategories[magnetSensorCategory] = model.SensorDeviceType
	KnownDeviceCategories[remoteControlCategory] = model.SensorDeviceType
	KnownDeviceCategories[submersionSensorCategory] = model.SensorDeviceType
	KnownDeviceCategories[gasSensorCategory] = model.SensorDeviceType
	KnownDeviceCategories[smokeSensorCategory] = model.SensorDeviceType
	KnownDeviceCategories[vibrationSensorCategory] = model.SensorDeviceType
}
