package tuya

import (
	"regexp"

	"a.yandex-team.ru/alice/iot/bulbasaur/model"
	"a.yandex-team.ru/alice/iot/bulbasaur/model/tuya"
)

var isAllowedBrandNameRegex = regexp.MustCompile(`^[a-zA-Zа-яА-Я \d_\-\.\&]+$`)
var nameRegex = regexp.MustCompile(`^(([а-яёА-ЯЁ]+)|(\d+))$`)
var onlyRusLettersRegex = regexp.MustCompile(`[а-яёА-ЯЁ]`)

type TuyaDeviceType string

func (t TuyaDeviceType) ToString() string {
	return string(t)
}

var CategoriesToDeviceTypeMap map[string]model.DeviceType
var IRCategoriesToDeviceTypeMap map[IrCategoryID]model.DeviceType
var DeviceTypeToIRCategoriesMap map[model.DeviceType]IrCategoryID

type DeviceKey struct {
	ProductID    TuyaDeviceProductID
	TuyaCategory string
}

type DeviceConfig struct {
	DefaultName     string
	Manufacturer    string
	Model           string
	HardwareVersion string
}

var KnownDevices map[DeviceKey]DeviceConfig

// Dont forget to add new allowed categories to UserDevice.HasAllowedCategory()
const (
	TuyaLightDeviceType  TuyaDeviceType = "dj"
	TuyaSocketDeviceType TuyaDeviceType = "cz"
	TuyaIRDeviceType     TuyaDeviceType = "qt"
	TuyaIRDeviceType2    TuyaDeviceType = "wnykq" // squared black IR gateways
)

const (
	LampDefaultName   string = "Лампочка"
	SocketDefaultName string = "Розетка"
	HubDefaultName    string = "Пульт"
)

// TODO: add socket and light instances (switch, colour_data, work_mode, etc)

type WiFiConnectionType int

const (
	EzMode WiFiConnectionType = 0
	ApMode WiFiConnectionType = 1
)

type TuyaCommandName string

func (t TuyaCommandName) ToString() string {
	return string(t)
}

const (
	// capabilities
	WorkModeCommand    TuyaCommandName = "work_mode"
	ColorDataCommand   TuyaCommandName = "colour_data"
	ColorData2Command  TuyaCommandName = "colour_data_v2"
	BrightnessCommand  TuyaCommandName = "bright_value"
	Brightness2Command TuyaCommandName = "bright_value_v2"
	TempValueCommand   TuyaCommandName = "temp_value"
	TempValue2Command  TuyaCommandName = "temp_value_v2"
	SceneData2Command  TuyaCommandName = "scene_data_v2"

	// properties
	AmperageCommand TuyaCommandName = "cur_current"
	VoltageCommand  TuyaCommandName = "cur_voltage"
	PowerCommand    TuyaCommandName = "cur_power"
)

type TuyaWorkMode string

func (t TuyaWorkMode) ToString() string {
	return string(t)
}

const (
	ColorWorkMode TuyaWorkMode = "colour"
	WhiteWorkMode TuyaWorkMode = "white"
	SceneWorkMode TuyaWorkMode = "scene"
)

type SwitchCommandName string

func (t SwitchCommandName) ToString() string {
	return string(t)
}

const (
	SwitchCommand    SwitchCommandName = "switch"
	Switch1Command   SwitchCommandName = "switch_1"
	LedSwitchCommand SwitchCommandName = "led_switch"
	SwitchLedCommand SwitchCommandName = "switch_led"
)

type IrCategoryName string

const (
	TvIrCategoryName        IrCategoryName = "Телевизор"
	AcIrCategoryName        IrCategoryName = "Кондиционер"
	SetTopBoxIrCategoryName IrCategoryName = "Ресивер"
	BoxIrCategoryName       IrCategoryName = "Приставка"

	// artificial category for custom devices
	CustomIrCategoryName IrCategoryName = "Другое устройство"
)

type IrCategoryID string

const (
	SetTopBoxIrCategoryID IrCategoryID = "1"
	TvIrCategoryID        IrCategoryID = "2"
	BoxIrCategoryID       IrCategoryID = "3"
	AcIrCategoryID        IrCategoryID = "5"
	FanIrCategoryID       IrCategoryID = "8"
	CustomIrCategoryID    IrCategoryID = "999"
)

const (
	PowerKeyName       tuya.IRKeyName = "power"
	MuteKeyName        tuya.IRKeyName = "mute"
	PauseKeyName       tuya.IRKeyName = "pause"
	ChannelUpKeyName   tuya.IRKeyName = "channel_up"
	ChannelDownKeyName tuya.IRKeyName = "channel_down"
	VolumeUpKeyName    tuya.IRKeyName = "volume_up"
	VolumeDownKeyName  tuya.IRKeyName = "volume_down"
	DigitOneKeyName    tuya.IRKeyName = "1"
	DigitTwoKeyName    tuya.IRKeyName = "2"
	DigitThreeKeyName  tuya.IRKeyName = "3"
	DigitFourKeyName   tuya.IRKeyName = "4"
	DigitFiveKeyName   tuya.IRKeyName = "5"
	DigitSixKeyName    tuya.IRKeyName = "6"
	DigitSevenKeyName  tuya.IRKeyName = "7"
	DigitEightKeyName  tuya.IRKeyName = "8"
	DigitNineKeyName   tuya.IRKeyName = "9"
	DigitZeroKeyName   tuya.IRKeyName = "0"

	// input sources
	InputSourceKeyName           tuya.IRKeyName = "input"
	InputSourceHDMI1KeyName      tuya.IRKeyName = "hdmi1"
	InputSourceHDMI2KeyName      tuya.IRKeyName = "hdmi2"
	InputSourceHDMI3KeyName      tuya.IRKeyName = "hdmi3"
	InputSourceHDMI4KeyName      tuya.IRKeyName = "hdmi4"
	InputSourceComponent1KeyName tuya.IRKeyName = "component1"
	InputSourceComponent2KeyName tuya.IRKeyName = "component2"
	InputSourceTVVIDKeyName      tuya.IRKeyName = "tv/vid"
	InputSourceTVKeyName         tuya.IRKeyName = "tv"
	InputSourceVideoKeyName      tuya.IRKeyName = "video"
	InputSourceVideo2KeyName     tuya.IRKeyName = "video2"
	InputSourceVideo3KeyName     tuya.IRKeyName = "video3"
	InputSourceDVIKeyName        tuya.IRKeyName = "dvi"
	InputSourceDVI1KeyName       tuya.IRKeyName = "inputdvi1"
	InputSourceDVI2KeyName       tuya.IRKeyName = "inputdvi2"
)

// fictional tuya.IRKeyName for different input sources
const (
	InputSourceOneKeyName   tuya.IRKeyName = "input_source_1"
	InputSourceTwoKeyName   tuya.IRKeyName = "input_source_2"
	InputSourceThreeKeyName tuya.IRKeyName = "input_source_3"
	InputSourceFourKeyName  tuya.IRKeyName = "input_source_4"
	InputSourceFiveKeyName  tuya.IRKeyName = "input_source_5"
	InputSourceSixKeyName   tuya.IRKeyName = "input_source_6"
	InputSourceSevenKeyName tuya.IRKeyName = "input_source_7"
	InputSourceEightKeyName tuya.IRKeyName = "input_source_8"
	InputSourceNineKeyName  tuya.IRKeyName = "input_source_9"
	InputSourceTenKeyName   tuya.IRKeyName = "input_source_10"
)

type BatchIrKey string

const (
	BatchVolumeUpKey   BatchIrKey = "volumeUp"
	BatchVolumeDownKey BatchIrKey = "volumeDown"
)

const (
	LampYandexProductID          TuyaDeviceProductID = "khpqhrx9jadvbvlq" //0 devices Oo
	Lamp2YandexProductID         TuyaDeviceProductID = "waa9pn2he0ebu3fx" //~1500 devices
	LampE27Lemon2YandexProductID TuyaDeviceProductID = "muushnich7dcbbxi" //first samples of yandex bulbs powered by yeelight, ~50k devices
	LampE27Lemon3YandexProductID TuyaDeviceProductID = "bamlqiywhx6hapfw" // sample of yeelight v3 bulb
	LampE27A60YandexProductID    TuyaDeviceProductID = "p0k9kcgmgiv9kdah" // cheap E27 lamp

	LampE14TestYandexProductID  TuyaDeviceProductID = "d3hrlpwesohbyly5" // yeelight E14 bulb
	LampE14Test2YandexProductID TuyaDeviceProductID = "q77y6breqaip3pj4" // yeelight E14 2 bulb
	LampE14MPYandexProductID    TuyaDeviceProductID = "dptwvxuihrhihfhi" // Mass production E14
	LampGU10TestYandexProductID TuyaDeviceProductID = "6gifdpwsfx6rigss" // GU10
	LampGU10MPYandexProductID   TuyaDeviceProductID = "86hqjbtrchuzudar" // GU10 MP

	LampE27SberProductID  TuyaDeviceProductID = "k4pymp1979gaat5a" // E27
	Lamp2E27SberProductID TuyaDeviceProductID = "qld69nw3svqr5l4o" // E27 v2
	Lamp3E27SberProductID TuyaDeviceProductID = "3acli3t0bmfpxnvu" // E27 v3
	Lamp4E27SberProductID TuyaDeviceProductID = "zawvklzbayshkzby" // E27 v4
	Lamp5E27SberProductID TuyaDeviceProductID = "bxssznt4obhdhrth" // E27 v5

	LampE14SberProductID  TuyaDeviceProductID = "6zxulwf3uojqiyd5" // E14
	Lamp2E14SberProductID TuyaDeviceProductID = "gzbsppvvzphtyi4j" // E14 v2
	Lamp3E14SberProductID TuyaDeviceProductID = "h9aumgzsllc46wgn" // E14 v3
	Lamp4E14SberProductID TuyaDeviceProductID = "ejkcu74tgdawjs6y" // E14 v4

	LampGU10SberProductID TuyaDeviceProductID = "nqlt4b9mzg5kjhcq" // GU10

	HubYandexProductID  TuyaDeviceProductID = "ghxopfgbbchswzir" //~900 devices
	Hub2YandexProductID TuyaDeviceProductID = "yxejnuxiaebrqfxb" //~35k devices

	SocketYandexProductID  TuyaDeviceProductID = "37olv4zlvlucpjfv" //~3500 devices
	Socket2YandexProductID TuyaDeviceProductID = "3nprwiontzyzpzng" //~65k devices
	Socket3YandexProductID TuyaDeviceProductID = "xldbkwenyuwtxmcu" //~1500 devices

	SocketSberProductID TuyaDeviceProductID = "xdfqgvditahqnpj2"
)

const (
	LampYandexModel     string = "YNDX-0005"
	Lamp3YandexModel    string = "YNDX-00010"
	Lamp4YandexModel    string = "YNDX-00018"
	Lamp5YandexModel    string = "YNDX-00501"
	LampE14YandexModel  string = "YNDX-00017-ALPHA"
	Lamp2E14YandexModel string = "YNDX-00017"
	LampGU10YandexModel string = "YNDX-00019"
	HubYandexModel      string = "YNDX-0006"
	SocketYandexModel   string = "YNDX-0007"

	SocketSberModel   string = "SBDV-00018"
	LampGU10SberModel string = "SBDV-00024"
	LampE14SberModel  string = "SBDV-00020"
	LampE27SberModel  string = "SBDV-00019"
)

type FirmwareModuleType int

// used in Tuya Firmware Info/Bulbasaur Firmware Info.
const (
	FirmwareInfoWIFIModuleType FirmwareModuleType = 0
	FirmwareInfoBLEModuleType  FirmwareModuleType = 1
	FirmwareInfoGPRSModuleType FirmwareModuleType = 2
	FirmwareInfoMCUModuleType  FirmwareModuleType = 9
)

type FirmwareUpgradeStatus int

const (
	FirmwareNoNeedUpgradeStatus    FirmwareUpgradeStatus = 0
	FirmwareHardwareIsReadyStatus  FirmwareUpgradeStatus = 1
	FirmwareUpgradingStatus        FirmwareUpgradeStatus = 2
	FirmwareUpgradeCompleteStatus  FirmwareUpgradeStatus = 3
	FirmwareUpgradeExceptionStatus FirmwareUpgradeStatus = 4
	FirmwareUnknownUpgradeStatus   FirmwareUpgradeStatus = -999
)

const (
	UnableToGetFirmwareErrorMessage      string = "Не удалось получить данные о прошивке. Попробуйте позже."
	FirmwareUpgradeExceptionErrorMessage string = "Произошла ошибка во время обновления прошивки. Попробуйте еще раз."
	LastCustomButtonDeletionErrorMessage string = "Обученное устройство должно иметь как минимум одну кнопку."
)

const (
	StaticColorSceneUnitChangeMode   ColorSceneUnitChangeMode = "static"
	JumpColorSceneUnitChangeMode     ColorSceneUnitChangeMode = "jump"
	GradientColorSceneUnitChangeMode ColorSceneUnitChangeMode = "gradient"
)

const (
	TuyaChipsetType     ChipsetType = "tuya"
	YeelightChipsetType ChipsetType = "yeelight"
	SberChipsetType     ChipsetType = "sber"
)

var KnownTVIRKeys []string
var VolumeIRKeys []string
var InputSourceIRKeys []string
var ChannelUpDownIRKeys []string
var ChannelDigitsIRKeys []string
var NumericModeToInputSourceIRKeyName map[model.ModeValue]tuya.IRKeyName
var IrACModesMap map[string]model.ModeValue
var IrACFanSpeedMap map[string]model.ModeValue
var IrAcPowerMap map[string]bool
var KnownIrCategoryNames map[IrCategoryID]IrCategoryName
var MediaIrCategoryIDs []string
var UpgradableModuleTypes []FirmwareModuleType
var KnownYandexLampProductID []string
var KnownYandexHubProductID []string
var KnownYandexSocketProductID []string
var KnownSberLampProductID []string

var YandexDevicesManufacturer = "Yandex Services AG"
var SberDevicesManufacturer = "SberDevices"
var UnknownDeviceManufacturer = "Неизвестно"
var UnknownDeviceModel = "Неизвестно"

type minMaxSimple struct {
	Min int
	Max int
}

type ChipsetType string

type lampSpecs struct {
	TempKSpec                  minMaxSimple
	TempValueSpec              minMaxSimple
	BrightValueAPISpec         minMaxSimple
	BrightValueLocalSpec       minMaxSimple // artificial brightness restriction to prevent burnout
	ColorSaturation            minMaxSimple
	ColorBrightness            minMaxSimple
	tempValueToTemperatureKMap map[int]model.TemperatureK
	temperatureKToTempValueMap map[model.TemperatureK]int
	brightCommand              TuyaCommandName
	tempCommand                TuyaCommandName
	colorCommand               TuyaCommandName
	chipsetType                ChipsetType
}

var oldLampSpec lampSpecs
var newYeelightLampSpec lampSpecs
var newTuyaLampSpec lampSpecs
var sberLampSpec lampSpecs

func LampSpecByPID(pid TuyaDeviceProductID) lampSpecs {
	switch pid {
	case LampYandexProductID, Lamp2YandexProductID:
		return oldLampSpec
	case LampE27Lemon2YandexProductID, LampE27Lemon3YandexProductID:
		return newYeelightLampSpec
	case LampE14TestYandexProductID, LampE14Test2YandexProductID, LampE14MPYandexProductID, LampGU10TestYandexProductID, LampGU10MPYandexProductID, LampE27A60YandexProductID:
		return newTuyaLampSpec
	case LampE27SberProductID, LampE14SberProductID, Lamp2E14SberProductID,
		LampGU10SberProductID, Lamp2E27SberProductID, Lamp3E27SberProductID,
		Lamp4E27SberProductID, Lamp3E14SberProductID, Lamp5E27SberProductID,
		Lamp4E14SberProductID:
		return sberLampSpec
	default:
		return oldLampSpec
	}
}

func init() {
	VolumeIRKeys = []string{
		string(VolumeUpKeyName),
		string(VolumeDownKeyName),
	}

	ChannelUpDownIRKeys = []string{
		string(ChannelUpKeyName),
		string(ChannelDownKeyName),
	}

	ChannelDigitsIRKeys = []string{
		string(DigitOneKeyName),
		string(DigitTwoKeyName),
		string(DigitThreeKeyName),
		string(DigitFourKeyName),
		string(DigitFiveKeyName),
		string(DigitSixKeyName),
		string(DigitSevenKeyName),
		string(DigitEightKeyName),
		string(DigitNineKeyName),
		string(DigitZeroKeyName),
	}

	InputSourceIRKeys = []string{
		string(InputSourceKeyName),
		string(InputSourceHDMI1KeyName),
		string(InputSourceHDMI2KeyName),
		string(InputSourceHDMI3KeyName),
		string(InputSourceHDMI4KeyName),
		string(InputSourceComponent1KeyName),
		string(InputSourceComponent2KeyName),
		string(InputSourceTVVIDKeyName),
		string(InputSourceTVKeyName),
		string(InputSourceVideoKeyName),
		string(InputSourceVideo2KeyName),
		string(InputSourceVideo3KeyName),
		string(InputSourceDVIKeyName),
		string(InputSourceDVI1KeyName),
		string(InputSourceDVI2KeyName),
	}

	KnownTVIRKeys = []string{
		string(PowerKeyName),
		string(MuteKeyName),
		string(PauseKeyName),
	}

	KnownTVIRKeys = append(KnownTVIRKeys, VolumeIRKeys...)
	KnownTVIRKeys = append(KnownTVIRKeys, ChannelUpDownIRKeys...)
	KnownTVIRKeys = append(KnownTVIRKeys, ChannelDigitsIRKeys...)
	KnownTVIRKeys = append(KnownTVIRKeys, InputSourceIRKeys...)

	NumericModeToInputSourceIRKeyName = map[model.ModeValue]tuya.IRKeyName{
		model.OneMode:   InputSourceOneKeyName,
		model.TwoMode:   InputSourceTwoKeyName,
		model.ThreeMode: InputSourceThreeKeyName,
		model.FourMode:  InputSourceFourKeyName,
		model.FiveMode:  InputSourceFiveKeyName,
		model.SixMode:   InputSourceSixKeyName,
		model.SevenMode: InputSourceSevenKeyName,
		model.EightMode: InputSourceEightKeyName,
		model.NineMode:  InputSourceNineKeyName,
		model.TenMode:   InputSourceTenKeyName,
	}

	IrACModesMap = map[string]model.ModeValue{
		"0": model.CoolMode,
		"1": model.HeatMode,
		"2": model.AutoMode,
		"3": model.FanOnlyMode,
		"4": model.DryMode,
	}

	IrACFanSpeedMap = map[string]model.ModeValue{
		"0": model.AutoMode,
		"1": model.LowMode,
		"2": model.MediumMode,
		"3": model.HighMode,
	}

	IrAcPowerMap = map[string]bool{
		"0": false,
		"1": true,
	}

	// do not forget to add name in another map in dto/mobile/const.go
	KnownIrCategoryNames = map[IrCategoryID]IrCategoryName{
		SetTopBoxIrCategoryID: SetTopBoxIrCategoryName,
		TvIrCategoryID:        TvIrCategoryName,
		BoxIrCategoryID:       BoxIrCategoryName,
		AcIrCategoryID:        AcIrCategoryName,
	}

	MediaIrCategoryIDs = []string{
		string(SetTopBoxIrCategoryID),
		string(TvIrCategoryID),
		string(BoxIrCategoryID),
	}

	UpgradableModuleTypes = []FirmwareModuleType{
		FirmwareInfoMCUModuleType,
		FirmwareInfoWIFIModuleType,
	}

	CategoriesToDeviceTypeMap = map[string]model.DeviceType{
		TuyaLightDeviceType.ToString():  model.LightDeviceType,
		TuyaSocketDeviceType.ToString(): model.SocketDeviceType,
		TuyaIRDeviceType.ToString():     model.HubDeviceType,
		TuyaIRDeviceType2.ToString():    model.HubDeviceType,
	}

	IRCategoriesToDeviceTypeMap = map[IrCategoryID]model.DeviceType{
		SetTopBoxIrCategoryID: model.ReceiverDeviceType,
		BoxIrCategoryID:       model.TvBoxDeviceType,
		AcIrCategoryID:        model.AcDeviceType,
		TvIrCategoryID:        model.TvDeviceDeviceType,
	}

	DeviceTypeToIRCategoriesMap = map[model.DeviceType]IrCategoryID{
		model.ReceiverDeviceType: SetTopBoxIrCategoryID,
		model.TvBoxDeviceType:    BoxIrCategoryID,
		model.AcDeviceType:       AcIrCategoryID,
		model.TvDeviceDeviceType: TvIrCategoryID,
	}

	// product IDs from iot.tuya.com
	// TODO: Do not forget to add here new products ID for Yandex Devices
	KnownDevices = map[DeviceKey]DeviceConfig{
		{TuyaCategory: TuyaLightDeviceType.ToString(), ProductID: LampYandexProductID}:          {DefaultName: LampDefaultName, Model: LampYandexModel, Manufacturer: YandexDevicesManufacturer, HardwareVersion: "1.0"},
		{TuyaCategory: TuyaLightDeviceType.ToString(), ProductID: Lamp2YandexProductID}:         {DefaultName: LampDefaultName, Model: LampYandexModel, Manufacturer: YandexDevicesManufacturer, HardwareVersion: "1.0"},
		{TuyaCategory: TuyaLightDeviceType.ToString(), ProductID: LampE27Lemon2YandexProductID}: {DefaultName: LampDefaultName, Model: Lamp3YandexModel, Manufacturer: YandexDevicesManufacturer, HardwareVersion: "2.0"},
		{TuyaCategory: TuyaLightDeviceType.ToString(), ProductID: LampE27Lemon3YandexProductID}: {DefaultName: LampDefaultName, Model: Lamp4YandexModel, Manufacturer: YandexDevicesManufacturer, HardwareVersion: "3.0"},
		{TuyaCategory: TuyaLightDeviceType.ToString(), ProductID: LampE27A60YandexProductID}:    {DefaultName: LampDefaultName, Model: Lamp5YandexModel, Manufacturer: YandexDevicesManufacturer, HardwareVersion: "4.0"},
		{TuyaCategory: TuyaLightDeviceType.ToString(), ProductID: LampE14TestYandexProductID}:   {DefaultName: LampDefaultName, Model: LampE14YandexModel, Manufacturer: YandexDevicesManufacturer, HardwareVersion: "1.0"},
		{TuyaCategory: TuyaLightDeviceType.ToString(), ProductID: LampE14Test2YandexProductID}:  {DefaultName: LampDefaultName, Model: Lamp2E14YandexModel, Manufacturer: YandexDevicesManufacturer, HardwareVersion: "2.0"},
		{TuyaCategory: TuyaLightDeviceType.ToString(), ProductID: LampE14MPYandexProductID}:     {DefaultName: LampDefaultName, Model: Lamp2E14YandexModel, Manufacturer: YandexDevicesManufacturer, HardwareVersion: "3.0"},
		{TuyaCategory: TuyaLightDeviceType.ToString(), ProductID: LampGU10TestYandexProductID}:  {DefaultName: LampDefaultName, Model: LampGU10YandexModel, Manufacturer: YandexDevicesManufacturer, HardwareVersion: "1.0"},
		{TuyaCategory: TuyaLightDeviceType.ToString(), ProductID: LampGU10MPYandexProductID}:    {DefaultName: LampDefaultName, Model: LampGU10YandexModel, Manufacturer: YandexDevicesManufacturer, HardwareVersion: "2.0"},
		{TuyaCategory: TuyaLightDeviceType.ToString(), ProductID: LampE27SberProductID}:         {DefaultName: LampDefaultName, Model: LampE27SberModel, Manufacturer: SberDevicesManufacturer, HardwareVersion: "1.0"},
		{TuyaCategory: TuyaLightDeviceType.ToString(), ProductID: Lamp2E27SberProductID}:        {DefaultName: LampDefaultName, Model: LampE27SberModel, Manufacturer: SberDevicesManufacturer, HardwareVersion: "2.0"},
		{TuyaCategory: TuyaLightDeviceType.ToString(), ProductID: Lamp3E27SberProductID}:        {DefaultName: LampDefaultName, Model: LampE27SberModel, Manufacturer: SberDevicesManufacturer, HardwareVersion: "2.0"},
		{TuyaCategory: TuyaLightDeviceType.ToString(), ProductID: Lamp4E27SberProductID}:        {DefaultName: LampDefaultName, Model: LampE27SberModel, Manufacturer: SberDevicesManufacturer, HardwareVersion: "2.0"},
		{TuyaCategory: TuyaLightDeviceType.ToString(), ProductID: Lamp5E27SberProductID}:        {DefaultName: LampDefaultName, Model: LampE27SberModel, Manufacturer: SberDevicesManufacturer, HardwareVersion: "3.0"},
		{TuyaCategory: TuyaLightDeviceType.ToString(), ProductID: LampE14SberProductID}:         {DefaultName: LampDefaultName, Model: LampE14SberModel, Manufacturer: SberDevicesManufacturer, HardwareVersion: "1.0"},
		{TuyaCategory: TuyaLightDeviceType.ToString(), ProductID: Lamp2E14SberProductID}:        {DefaultName: LampDefaultName, Model: LampE14SberModel, Manufacturer: SberDevicesManufacturer, HardwareVersion: "2.0"},
		{TuyaCategory: TuyaLightDeviceType.ToString(), ProductID: Lamp3E14SberProductID}:        {DefaultName: LampDefaultName, Model: LampE14SberModel, Manufacturer: SberDevicesManufacturer, HardwareVersion: "2.0"},
		{TuyaCategory: TuyaLightDeviceType.ToString(), ProductID: Lamp4E14SberProductID}:        {DefaultName: LampDefaultName, Model: LampE14SberModel, Manufacturer: SberDevicesManufacturer, HardwareVersion: "3.0"},
		{TuyaCategory: TuyaLightDeviceType.ToString(), ProductID: LampGU10SberProductID}:        {DefaultName: LampDefaultName, Model: LampGU10SberModel, Manufacturer: SberDevicesManufacturer, HardwareVersion: "2.0"},

		{TuyaCategory: TuyaIRDeviceType.ToString(), ProductID: HubYandexProductID}:   {DefaultName: HubDefaultName, Model: HubYandexModel, Manufacturer: YandexDevicesManufacturer, HardwareVersion: "1.0"},
		{TuyaCategory: TuyaIRDeviceType.ToString(), ProductID: Hub2YandexProductID}:  {DefaultName: HubDefaultName, Model: HubYandexModel, Manufacturer: YandexDevicesManufacturer, HardwareVersion: "1.0"},
		{TuyaCategory: TuyaIRDeviceType2.ToString(), ProductID: HubYandexProductID}:  {DefaultName: HubDefaultName, Model: HubYandexModel, Manufacturer: YandexDevicesManufacturer, HardwareVersion: "1.0"},
		{TuyaCategory: TuyaIRDeviceType2.ToString(), ProductID: Hub2YandexProductID}: {DefaultName: HubDefaultName, Model: HubYandexModel, Manufacturer: YandexDevicesManufacturer, HardwareVersion: "1.0"},

		{TuyaCategory: TuyaSocketDeviceType.ToString(), ProductID: SocketYandexProductID}:  {DefaultName: SocketDefaultName, Model: SocketYandexModel, Manufacturer: YandexDevicesManufacturer, HardwareVersion: "1.0"},
		{TuyaCategory: TuyaSocketDeviceType.ToString(), ProductID: Socket2YandexProductID}: {DefaultName: SocketDefaultName, Model: SocketYandexModel, Manufacturer: YandexDevicesManufacturer, HardwareVersion: "1.0"},
		{TuyaCategory: TuyaSocketDeviceType.ToString(), ProductID: Socket3YandexProductID}: {DefaultName: SocketDefaultName, Model: SocketYandexModel, Manufacturer: YandexDevicesManufacturer, HardwareVersion: "1.0"},
		{TuyaCategory: TuyaSocketDeviceType.ToString(), ProductID: SocketSberProductID}:    {DefaultName: SocketDefaultName, Model: SocketSberModel, Manufacturer: SberDevicesManufacturer, HardwareVersion: "1.0"},
	}

	KnownYandexLampProductID = []string{string(LampYandexProductID), string(Lamp2YandexProductID),
		string(LampE27Lemon2YandexProductID), string(LampE27Lemon3YandexProductID), string(LampE27A60YandexProductID),
		string(LampE14TestYandexProductID), string(LampE14Test2YandexProductID), string(LampE14MPYandexProductID),
		string(LampGU10TestYandexProductID), string(LampGU10MPYandexProductID)}
	KnownYandexHubProductID = []string{string(HubYandexProductID), string(Hub2YandexProductID)}
	KnownYandexSocketProductID = []string{string(SocketYandexProductID), string(Socket2YandexProductID), string(Socket3YandexProductID)}
	KnownSberLampProductID = []string{string(LampE27SberProductID), string(Lamp2E27SberProductID), string(Lamp3E27SberProductID),
		string(Lamp4E27SberProductID), string(Lamp5E27SberProductID), string(LampE14SberProductID), string(Lamp2E14SberProductID),
		string(Lamp3E14SberProductID), string(Lamp4E14SberProductID), string(LampGU10SberProductID)}

	temperatureKToTempValueMap := map[model.TemperatureK]int{
		model.TemperatureK(2700): 0,
		model.TemperatureK(3400): 60,
		model.TemperatureK(4500): 120,
		model.TemperatureK(5600): 190,
		model.TemperatureK(6500): 255,
	}

	tempValueToTemperatureKMap := make(map[int]model.TemperatureK)
	for temperatureK, tempValue := range temperatureKToTempValueMap {
		tempValueToTemperatureKMap[tempValue] = temperatureK
	}

	yeelightTemperatureKToTempValueMap := map[model.TemperatureK]int{
		model.TemperatureK(1500): 150,
		model.TemperatureK(2700): 300,
		model.TemperatureK(3400): 400,
		model.TemperatureK(4500): 600,
		model.TemperatureK(5600): 800,
		model.TemperatureK(6500): 1000,
	}

	yeelightTempValueToTemperatureKMap := make(map[int]model.TemperatureK)
	for temperatureK, tempValue := range yeelightTemperatureKToTempValueMap {
		yeelightTempValueToTemperatureKMap[tempValue] = temperatureK
	}

	yeelightAndTuyaTemperatureKToTempValueMap := map[model.TemperatureK]int{
		model.TemperatureK(1500): 0,
		model.TemperatureK(2700): 150,
		model.TemperatureK(3400): 370,
		model.TemperatureK(4500): 650,
		model.TemperatureK(5600): 900,
		model.TemperatureK(6500): 1000,
	}

	yeelightAndTuyaTempValueToTemperatureKMap := make(map[int]model.TemperatureK)
	for temperatureK, tempValue := range yeelightAndTuyaTemperatureKToTempValueMap {
		yeelightAndTuyaTempValueToTemperatureKMap[tempValue] = temperatureK
	}

	sberTemperatureKToTempValueMap := map[model.TemperatureK]int{
		model.TemperatureK(1500): 150,
		model.TemperatureK(2700): 300,
		model.TemperatureK(3400): 400,
		model.TemperatureK(4500): 600,
		model.TemperatureK(5600): 800,
		model.TemperatureK(6500): 1000,
	}
	sberTempValueToTemperatureKMap := make(map[int]model.TemperatureK)
	for temperatureK, tempValue := range sberTemperatureKToTempValueMap {
		sberTempValueToTemperatureKMap[tempValue] = temperatureK
	}

	oldLampSpec = lampSpecs{
		BrightValueAPISpec: minMaxSimple{
			Min: 25,
			Max: 255,
		},
		BrightValueLocalSpec: minMaxSimple{
			Min: 25,
			Max: 204,
		},
		TempKSpec: minMaxSimple{
			Min: 2700,
			Max: 6500,
		},
		TempValueSpec: minMaxSimple{
			Min: 0,
			Max: 255,
		},
		ColorSaturation: minMaxSimple{
			Min: 1,
			Max: 255,
		},
		ColorBrightness: minMaxSimple{
			Min: 1,
			Max: 255,
		},
		temperatureKToTempValueMap: temperatureKToTempValueMap,
		tempValueToTemperatureKMap: tempValueToTemperatureKMap,
		brightCommand:              BrightnessCommand,
		tempCommand:                TempValueCommand,
		colorCommand:               ColorDataCommand,
		chipsetType:                TuyaChipsetType,
	}

	newYeelightLampSpec = lampSpecs{
		BrightValueAPISpec: minMaxSimple{
			Min: 10,
			Max: 1000,
		},
		BrightValueLocalSpec: minMaxSimple{
			Min: 10,
			Max: 1000,
		},
		TempKSpec: minMaxSimple{
			Min: 1500,
			Max: 6500,
		},
		TempValueSpec: minMaxSimple{
			Min: 0,
			Max: 1000,
		},
		ColorSaturation: minMaxSimple{
			Min: 0,
			Max: 1000,
		},
		ColorBrightness: minMaxSimple{
			Min: 0,
			Max: 1000,
		},
		temperatureKToTempValueMap: yeelightTemperatureKToTempValueMap,
		tempValueToTemperatureKMap: yeelightTempValueToTemperatureKMap,
		brightCommand:              Brightness2Command,
		tempCommand:                TempValue2Command,
		colorCommand:               ColorData2Command,
		chipsetType:                YeelightChipsetType,
	}

	newTuyaLampSpec = lampSpecs{
		BrightValueAPISpec: minMaxSimple{
			Min: 10,
			Max: 1000,
		},
		BrightValueLocalSpec: minMaxSimple{
			Min: 10,
			Max: 1000,
		},
		TempKSpec: minMaxSimple{
			Min: 1500,
			Max: 6500,
		},
		TempValueSpec: minMaxSimple{
			Min: 0,
			Max: 1000,
		},
		ColorSaturation: minMaxSimple{
			Min: 0,
			Max: 1000,
		},
		ColorBrightness: minMaxSimple{
			Min: 0,
			Max: 1000,
		},
		temperatureKToTempValueMap: yeelightAndTuyaTemperatureKToTempValueMap,
		tempValueToTemperatureKMap: yeelightAndTuyaTempValueToTemperatureKMap,
		brightCommand:              Brightness2Command,
		tempCommand:                TempValue2Command,
		colorCommand:               ColorData2Command,
		chipsetType:                TuyaChipsetType,
	}

	sberLampSpec = lampSpecs{
		BrightValueAPISpec: minMaxSimple{
			Min: 10,
			Max: 1000,
		},
		BrightValueLocalSpec: minMaxSimple{
			Min: 10,
			Max: 1000,
		},
		TempKSpec: minMaxSimple{
			Min: 1500,
			Max: 6500,
		},
		TempValueSpec: minMaxSimple{
			Min: 0,
			Max: 1000,
		},
		ColorSaturation: minMaxSimple{
			Min: 0,
			Max: 1000,
		},
		ColorBrightness: minMaxSimple{
			Min: 0,
			Max: 1000,
		},
		temperatureKToTempValueMap: sberTemperatureKToTempValueMap,
		tempValueToTemperatureKMap: sberTempValueToTemperatureKMap,
		brightCommand:              Brightness2Command,
		tempCommand:                TempValue2Command,
		colorCommand:               ColorData2Command,
		chipsetType:                SberChipsetType,
	}

	colorSceneNumToSceneID = make(map[int]model.ColorSceneID)
	for _, scenePool := range knownScenePools {
		for sceneID, scene := range scenePool {
			colorSceneNumToSceneID[scene.SceneNum] = sceneID
		}
		break
	}
}

//property values conversion functions
func milliAmpereToAmpere(value float64) float64 {
	return value * 0.001
}

func deciWattToWatt(value float64) float64 {
	return value * 0.1
}

func deciVoltToVolt(value float64) float64 {
	return value * 0.1
}
