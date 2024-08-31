package tuya

import (
	"context"
	"encoding/json"
	"fmt"
	"sort"
	"strconv"

	"github.com/mitchellh/mapstructure"
	"golang.org/x/exp/slices"

	"a.yandex-team.ru/alice/iot/bulbasaur/dto/adapter"
	"a.yandex-team.ru/alice/iot/bulbasaur/model"
	"a.yandex-team.ru/alice/iot/bulbasaur/model/tuya"
	"a.yandex-team.ru/alice/library/go/tools"
	"a.yandex-team.ru/library/go/core/xerrors"
)

type tuyaResponse struct {
	Msg     string
	Success bool
	Code    int
	T       int64
	Result  interface{}
}

type tuyaTokenResponse struct {
	tuyaResponse
	Result struct {
		UID               string
		Region            string
		Token             string
		Secret            string
		AccessToken       string `json:"access_token"`
		ExpireTimeSeconds int64  `json:"expire_time"`
	}
}

type tuyaCipherResponse struct {
	tuyaResponse
	Result string
}

type UserDevicesResponse struct {
	tuyaResponse
	Result []UserDevice
}

type DeviceUnderPairingToken struct {
	ID        string `json:"id"`
	IP        string `json:"ip"`
	Name      string `json:"name"`
	ProductID string `json:"productId"`
	UUID      string `json:"uuid"`
}

type ModuleFirmwareInfo struct {
	CurrentVersion  string                `json:"current_version"`
	UpgradeVersion  string                `json:"upgrade_version"`
	LastUpgradeTime int64                 `json:"last_upgrade_time"`
	ModuleType      FirmwareModuleType    `json:"module_type"`
	ModuleDesc      string                `json:"module_desc"`
	UpgradeStatus   FirmwareUpgradeStatus `json:"upgrade_status"`
}

type DeviceFirmwareInfo []ModuleFirmwareInfo

func (dfi DeviceFirmwareInfo) GetCurrentVersionByModuleType(moduleType FirmwareModuleType) (string, bool) {
	for _, moduleFirmwareInfo := range dfi {
		if moduleFirmwareInfo.ModuleType == moduleType {
			return moduleFirmwareInfo.CurrentVersion, true
		}
	}
	return "", false
}

type FirmwareInfoKnownModulesStatusMap map[FirmwareModuleType]FirmwareUpgradeStatus

func (dfi DeviceFirmwareInfo) GetStatusAndModuleType() (FirmwareUpgradeStatus, FirmwareModuleType) {
	currentModuleType := FirmwareModuleType(-1)
	currentStatus := FirmwareUnknownUpgradeStatus
	for _, moduleType := range UpgradableModuleTypes {
		for _, firmwareInfo := range dfi {
			if firmwareInfo.ModuleType == moduleType && firmwareInfo.UpgradeStatus > currentStatus {
				currentModuleType = moduleType
				currentStatus = firmwareInfo.UpgradeStatus
			}
		}
	}
	return currentStatus, currentModuleType
}

type TuyaFirmwareInfoResponse struct {
	tuyaResponse
	Result []ModuleFirmwareInfo
}

type TuyaCommand struct {
	Code     string      `json:"code"`
	Value    interface{} `json:"value"`
	Priority int         // using for sorting commands before sending them to Tuya API. Commands with higher value of priority field has higher priority
}

type TuyaDeviceProductID string

func (pi TuyaDeviceProductID) IsSberLamp() bool {
	return slices.Contains(KnownSberLampProductID, string(pi))
}

func (pi TuyaDeviceProductID) IsYandexLamp() bool {
	return slices.Contains(KnownYandexLampProductID, string(pi))
}

func (pi TuyaDeviceProductID) IsYandexHub() bool {
	return slices.Contains(KnownYandexHubProductID, string(pi))
}

func (pi TuyaDeviceProductID) IsYandexSocket() bool {
	return slices.Contains(KnownYandexSocketProductID, string(pi))
}

type DeviceOwner struct {
	TuyaUID string
	SkillID string
}

type UserDevice struct {
	ID           string
	Name         string
	Category     string // TODO: change to TuyaDeviceType
	Online       bool
	Sub          bool
	Status       []TuyaCommand
	ProductID    TuyaDeviceProductID `json:"product_id"`
	FirmwareInfo DeviceFirmwareInfo
	OwnerUID     string `json:"uid"`
}

func (ud *UserDevice) GetStatusItemByCommandName(name TuyaCommandName) (TuyaCommand, bool) {
	for _, command := range ud.Status {
		if command.Code == name.ToString() {
			return command, true
		}
	}

	return TuyaCommand{}, false
}

// Looking for main switch command for current device
func (ud *UserDevice) GetMainSwitchStatusItem() (TuyaCommand, bool) {
	for _, command := range ud.Status {

		switch ud.Category {
		// devices.types.light
		case TuyaLightDeviceType.ToString():
			if tools.Contains(command.Code, []string{LedSwitchCommand.ToString(), SwitchLedCommand.ToString()}) {
				return command, true
			}
		case TuyaSocketDeviceType.ToString():
			// Using if cause `switch` has higher priority than `switch_1`
			switch command.Code {
			case SwitchCommand.ToString():
				return command, true
			case Switch1Command.ToString():
				return command, true
			}
		}

	}

	return TuyaCommand{}, false
}

// Looking for main bright_value command for current device
func (ud *UserDevice) GetBrightValueItem() (TuyaCommand, bool) {
	for _, command := range ud.Status {
		if tools.Contains(command.Code, []string{BrightnessCommand.ToString(), Brightness2Command.ToString()}) {
			return command, true
		}
	}

	return TuyaCommand{}, false
}

// Looking for main temp_value command for current device
func (ud *UserDevice) GetTempValueItem() (TuyaCommand, bool) {
	for _, command := range ud.Status {
		if tools.Contains(command.Code, []string{TempValueCommand.ToString(), TempValue2Command.ToString()}) {
			return command, true
		}
	}

	return TuyaCommand{}, false
}

// Looking for main colour_data command for current device
func (ud *UserDevice) GetColorDataItem() (TuyaCommand, bool) {
	for _, command := range ud.Status {
		if tools.Contains(command.Code, []string{ColorDataCommand.ToString(), ColorData2Command.ToString()}) {
			return command, true
		}
	}

	return TuyaCommand{}, false
}

// Looking for scene_data_v2 command for current device
func (ud *UserDevice) GetSceneDataItem() (TuyaCommand, bool) {
	for _, command := range ud.Status {
		if command.Code == string(SceneData2Command) {
			return command, true
		}
	}

	return TuyaCommand{}, false
}

func (ud *UserDevice) GetDefaultDeviceConfig() DeviceConfig {
	config := DeviceConfig{}
	switch ud.Category {
	case TuyaLightDeviceType.ToString():
		config.DefaultName = LampDefaultName
	case TuyaSocketDeviceType.ToString():
		config.DefaultName = SocketDefaultName
	case TuyaIRDeviceType.ToString(), TuyaIRDeviceType2.ToString():
		config.DefaultName = HubDefaultName
	}
	// we need to provide some model and manufacturer to not cause validation errors
	config.Model = UnknownDeviceModel
	config.Manufacturer = UnknownDeviceManufacturer
	return config
}

func (ud *UserDevice) GetDeviceConfig() DeviceConfig {
	deviceKey := DeviceKey{
		TuyaCategory: ud.Category,
		ProductID:    ud.ProductID,
	}
	if config, ok := KnownDevices[deviceKey]; ok {
		return config
	}
	return ud.GetDefaultDeviceConfig()
}

func (ud *UserDevice) GetFwVersion() *string {
	var moduleType FirmwareModuleType
	switch ud.Category {
	case TuyaLightDeviceType.ToString():
		moduleType = FirmwareInfoWIFIModuleType
	default:
		moduleType = FirmwareInfoMCUModuleType
	}
	if currentModuleVersion, found := ud.FirmwareInfo.GetCurrentVersionByModuleType(moduleType); found {
		return tools.AOS(currentModuleVersion)
	}
	return nil
}

func (ud *UserDevice) CheckIsAllowedForDiscovery() error {
	if _, isAllowedCategory := CategoriesToDeviceTypeMap[ud.Category]; !isAllowedCategory {
		return fmt.Errorf("unallowed category %s", ud.Category)
	}

	// sub - is a sub-device. For example: IR controls. We are getting them via other handler.
	if ud.Sub {
		return fmt.Errorf("sub device")
	}

	return nil
}

func (ud *UserDevice) ToDeviceInfoView() adapter.DeviceInfoView {
	deviceType := CategoriesToDeviceTypeMap[ud.Category]
	// ignoring errors cause we checked key existing within CheckIsAllowedForDiscovery earlier

	deviceConfig := ud.GetDeviceConfig()
	deviceInfoView := adapter.DeviceInfoView{
		ID:         ud.ID,
		Name:       deviceConfig.DefaultName,
		Type:       deviceType,
		DeviceInfo: &model.DeviceInfo{},
	}

	if len(deviceConfig.Manufacturer) > 0 {
		deviceInfoView.DeviceInfo.Manufacturer = tools.AOS(deviceConfig.Manufacturer)
	}

	if len(deviceConfig.Model) > 0 {
		deviceInfoView.DeviceInfo.Model = tools.AOS(deviceConfig.Model)
	}

	if len(deviceConfig.HardwareVersion) > 0 {
		deviceInfoView.DeviceInfo.HwVersion = tools.AOS(deviceConfig.HardwareVersion)
	}

	deviceInfoView.DeviceInfo.SwVersion = ud.GetFwVersion()

	customData := tuya.CustomData{
		DeviceType: deviceType,
		ProductID:  tools.AOS(string(ud.ProductID)),
		FwVersion:  ud.GetFwVersion(),
	}

	capabilities := make([]adapter.CapabilityInfoView, 0, len(ud.Status))
	properties := make([]adapter.PropertyInfoView, 0, len(ud.Status))

	switch deviceType {
	// LIGHT
	case model.LightDeviceType:
		// Parse supported capabilities
		// -- if we has some light work_mode add brightness & color_setting
		lampSpec := LampSpecByPID(ud.ProductID)
		if _, exists := ud.GetStatusItemByCommandName(WorkModeCommand); exists {

			// -- brightness
			if _, exists := ud.GetBrightValueItem(); exists {
				capabilityBri := adapter.CapabilityInfoView{
					Type:        model.RangeCapabilityType,
					Retrievable: true,
					Reportable:  true,
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
				}
				capabilities = append(capabilities, capabilityBri)
			}

			// -- color_setting
			_, tempExists := ud.GetTempValueItem()
			_, colorExists := ud.GetColorDataItem()
			_, sceneExists := ud.GetSceneDataItem()

			if tempExists || colorExists {
				capabilityCol := adapter.CapabilityInfoView{
					Type:        model.ColorSettingCapabilityType,
					Retrievable: true,
					Reportable:  true,
				}
				parameters := model.ColorSettingCapabilityParameters{}

				// -- color_setting white color by default
				if tempExists {
					parameters.TemperatureK = &model.TemperatureKParameters{
						Min: model.TemperatureK(lampSpec.TempKSpec.Min),
						Max: model.TemperatureK(lampSpec.TempKSpec.Max),
					}
				}

				// -- color_setting hsv param
				if colorExists {
					parameters.ColorModel = model.CM(model.HsvModelType)
				}
				// our lamp do not support scenes until 1.0.15 firmware version so we should check it in the hard way
				if sceneExists && ud.ProductID.IsYandexLamp() && lampFirmwareSupportScenes(ud.ProductID, deviceInfoView.DeviceInfo.SwVersion) {
					parameters.ColorSceneParameters = GetColorSceneParameters(getColorScenePool(ud.ProductID, deviceInfoView.DeviceInfo.SwVersion))
				}

				capabilityCol.Parameters = parameters
				capabilities = append(capabilities, capabilityCol)
			}
		}

		// -- on_off
		if switchCommand, exists := ud.GetMainSwitchStatusItem(); exists {
			capabilityOn := adapter.CapabilityInfoView{
				Type:        model.OnOffCapabilityType,
				Retrievable: true,
				Reportable:  true,
			}
			capabilities = append(capabilities, capabilityOn)
			customData.SwitchCommand = switchCommand.Code
		}

	// SOCKET
	case model.SocketDeviceType:
		// -- on_off
		if switchCommand, exists := ud.GetMainSwitchStatusItem(); exists {
			capabilityOn := adapter.CapabilityInfoView{
				Type:        model.OnOffCapabilityType,
				Retrievable: true,
				Reportable:  true,
			}
			capabilities = append(capabilities, capabilityOn)
			customData.SwitchCommand = switchCommand.Code
		}

		// -- voltage
		if _, exists := ud.GetStatusItemByCommandName(VoltageCommand); exists {
			propertyVoltage := adapter.PropertyInfoView{
				Type:        model.FloatPropertyType,
				Retrievable: true,
				Reportable:  true,
				Parameters: model.FloatPropertyParameters{
					Instance: model.VoltagePropertyInstance,
					Unit:     model.UnitVolt,
				},
			}
			properties = append(properties, propertyVoltage)
		}

		// -- power
		if _, exists := ud.GetStatusItemByCommandName(PowerCommand); exists {
			propertyPower := adapter.PropertyInfoView{
				Type:        model.FloatPropertyType,
				Retrievable: true,
				Reportable:  true,
				Parameters: model.FloatPropertyParameters{
					Instance: model.PowerPropertyInstance,
					Unit:     model.UnitWatt,
				},
			}
			properties = append(properties, propertyPower)
		}

		// -- amperage
		if _, exists := ud.GetStatusItemByCommandName(AmperageCommand); exists {
			propertyAmperage := adapter.PropertyInfoView{
				Type:        model.FloatPropertyType,
				Retrievable: true,
				Reportable:  true,
				Parameters: model.FloatPropertyParameters{
					Instance: model.AmperagePropertyInstance,
					Unit:     model.UnitAmpere,
				},
			}
			properties = append(properties, propertyAmperage)
		}
	// IR TX (HUB)
	case model.HubDeviceType: // not sure why hub has capabilities
		// -- on_off
		if switchCommand, exists := ud.GetMainSwitchStatusItem(); exists {
			capabilityOn := adapter.CapabilityInfoView{
				Type:        model.OnOffCapabilityType,
				Retrievable: true,
				Reportable:  true,
			}
			capabilities = append(capabilities, capabilityOn)
			customData.SwitchCommand = switchCommand.Code
		}
	}

	deviceInfoView.Capabilities = capabilities
	deviceInfoView.Properties = properties
	deviceInfoView.CustomData = customData

	return deviceInfoView
}

func (ud *UserDevice) ToDeviceStateView() adapter.DeviceStateView {
	deviceStateView := adapter.DeviceStateView{
		ID: ud.ID,
	}

	if !ud.Online {
		deviceStateView.ErrorCode = adapter.DeviceUnreachable
		return deviceStateView
	}

	capabilities := make([]adapter.CapabilityStateView, 0, len(ud.Status))
	properties := make([]adapter.PropertyStateView, 0, len(ud.Status))

	switch ud.Category {
	// devices.types.light
	case TuyaLightDeviceType.ToString():
		lampSpec := LampSpecByPID(ud.ProductID)
		// Parse supported capabilities
		// -- range.brightness + color_setting
		// ---- get brightness from HSV model (colour_data command) if work_mode - color, else - from brightness command
		if workMode, exists := ud.GetStatusItemByCommandName(WorkModeCommand); exists {
			switch workMode.Value {
			case ColorWorkMode.ToString():
				if colorData, exists := ud.GetColorDataItem(); exists {
					hsvData := tuyaHsvToHsvState(colorData.Value.(model.HSV), ud.ProductID)

					// -- brightness
					capability := adapter.CapabilityStateView{
						Type: model.RangeCapabilityType,
						State: model.RangeCapabilityState{
							Instance: model.BrightnessRangeInstance,
							Value:    float64(hsvData.V),
						},
					}
					capabilities = append(capabilities, capability)

					// -- color_setting
					if c, ok := model.ColorHSVColorIDMap[hsvData]; ok && model.ColorPalette[c].Temperature > 0 {
						capability = adapter.CapabilityStateView{
							Type: model.ColorSettingCapabilityType,
							State: model.ColorSettingCapabilityState{
								Instance: model.TemperatureKCapabilityInstance,
								Value:    model.ColorPalette[c].Temperature,
							},
						}
					} else {
						capability = adapter.CapabilityStateView{
							Type: model.ColorSettingCapabilityType,
							State: model.ColorSettingCapabilityState{
								Instance: model.HsvColorCapabilityInstance,
								Value:    hsvData,
							},
						}
					}
					capabilities = append(capabilities, capability)
				}
			case WhiteWorkMode.ToString():
				// -- brightness
				if brightness, exists := ud.GetBrightValueItem(); exists {
					brightValue := float64(percentFromRange(brightness.Value.(int), lampSpec.BrightValueLocalSpec.Min, lampSpec.BrightValueLocalSpec.Max)) // decrease max brightness (really 255) to 80% (204)
					if brightValue == 0 {
						brightValue = 1
					}
					capability := adapter.CapabilityStateView{
						Type: model.RangeCapabilityType,
						State: model.RangeCapabilityState{
							Instance: model.BrightnessRangeInstance,
							Value:    brightValue,
						},
					}
					capabilities = append(capabilities, capability)

					value := model.ColorPalette.GetDefaultWhiteColor().Temperature // default value
					if tempValue, exists := ud.GetTempValueItem(); exists {
						if temperatureK, ok := lampSpec.tempValueToTemperatureKMap[tempValue.Value.(int)]; ok {
							value = temperatureK
						}
					}

					// -- color_setting
					capability = adapter.CapabilityStateView{
						Type: model.ColorSettingCapabilityType,
						State: model.ColorSettingCapabilityState{
							Instance: model.TemperatureKCapabilityInstance,
							Value:    value,
						},
					}
					capabilities = append(capabilities, capability)
				}
			case SceneWorkMode.ToString():
				// -- scene data
				if sceneDataItem, exists := ud.GetSceneDataItem(); exists {
					if sceneDataValue, ok := sceneDataItem.Value.(SceneDataState); ok {
						if colorSceneID, exists := sceneDataValue.ColorSceneID(); exists {
							capability := adapter.CapabilityStateView{
								Type: model.ColorSettingCapabilityType,
								State: model.ColorSettingCapabilityState{
									Instance: model.SceneCapabilityInstance,
									Value:    colorSceneID,
								},
							}
							capabilities = append(capabilities, capability)
						}
					}
				}
			}
		}

		// -- on_off
		if switchCommand, exists := ud.GetMainSwitchStatusItem(); exists {
			capability := adapter.CapabilityStateView{
				Type: model.OnOffCapabilityType,
				State: model.OnOffCapabilityState{
					Instance: model.OnOnOffCapabilityInstance,
					Value:    switchCommand.Value.(bool),
				},
			}

			capabilities = append(capabilities, capability)
		}

	case TuyaSocketDeviceType.ToString():
		// TODO: work with more than single port sockets (codes: switch_1, switch_2 etc)
		if switchCommand, exists := ud.GetMainSwitchStatusItem(); exists {
			capability := adapter.CapabilityStateView{
				Type: model.OnOffCapabilityType,
				State: model.OnOffCapabilityState{
					Instance: model.OnOnOffCapabilityInstance,
					Value:    switchCommand.Value.(bool),
				},
			}

			capabilities = append(capabilities, capability)
		}

		// -- voltage
		if voltageCommand, exists := ud.GetStatusItemByCommandName(VoltageCommand); exists {
			propertyVoltage := adapter.PropertyStateView{
				Type: model.FloatPropertyType,
				State: model.FloatPropertyState{
					Instance: model.VoltagePropertyInstance,
					Value:    deciVoltToVolt(float64(voltageCommand.Value.(int))), // voltage in dV, 2200 dV = 220 V
				},
			}
			properties = append(properties, propertyVoltage)
		}

		// -- power
		if powerCommand, exists := ud.GetStatusItemByCommandName(PowerCommand); exists {
			propertyPower := adapter.PropertyStateView{
				Type: model.FloatPropertyType,
				State: model.FloatPropertyState{
					Instance: model.PowerPropertyInstance,
					Value:    deciWattToWatt(float64(powerCommand.Value.(int))), // active power in dW, 66 dW = 6.6 W
				},
			}
			properties = append(properties, propertyPower)
		}

		// -- amperage
		if amperageCommand, exists := ud.GetStatusItemByCommandName(AmperageCommand); exists {
			propertyAmperage := adapter.PropertyStateView{
				Type: model.FloatPropertyType,
				State: model.FloatPropertyState{
					Instance: model.AmperagePropertyInstance,
					Value:    milliAmpereToAmpere(float64(amperageCommand.Value.(int))), // amperage in mA, 35 mA = 0.035 A
				},
			}
			properties = append(properties, propertyAmperage)
		}
	}

	deviceStateView.Capabilities = capabilities
	deviceStateView.Properties = properties
	return deviceStateView
}

func (tc *TuyaCommand) UnmarshalJSON(b []byte) (err error) {
	tcRaw := struct {
		Code     string
		Value    string // Tuya stores all values within string...
		priority int
	}{}
	if err := json.Unmarshal(b, &tcRaw); err != nil {
		return err
	}
	tc.Code = tcRaw.Code

	// Dropping into a panic for empty value in the next processing is preferable here
	if !tools.Contains(tcRaw.Code, []string{ColorDataCommand.ToString(), ColorData2Command.ToString()}) && tcRaw.Value == "" {
		tc.Value = nil
		return nil
	}

	switch {
	case tcRaw.Code == WorkModeCommand.ToString():
		tc.Value = tcRaw.Value
	// TODO: work with more than single port sockets (codes: switch_1, switch_2 etc)
	case tools.Contains(tcRaw.Code, []string{SwitchLedCommand.ToString(), LedSwitchCommand.ToString(), SwitchCommand.ToString(), Switch1Command.ToString()}):
		tc.Value = strToBool(tcRaw.Value)
	case tools.Contains(tcRaw.Code, []string{BrightnessCommand.ToString(), Brightness2Command.ToString(), TempValueCommand.ToString(), TempValue2Command.ToString(), AmperageCommand.ToString(), PowerCommand.ToString(), VoltageCommand.ToString()}):
		vint, err := strconv.Atoi(tcRaw.Value)
		if err != nil {
			break
		}
		tc.Value = vint
	case tools.Contains(tcRaw.Code, []string{ColorDataCommand.ToString(), ColorData2Command.ToString()}):
		// Cause new Yeelight lamps has empty colour_data by default: {"code":"colour_data_v2","value":""}
		if tcRaw.Value == "" {
			tc.Value = model.HSV{H: 55, S: 55, V: 55}
			return nil
		}
		// Cause Tuya stores hsv within string... Example: `{"code":"colour_data","value":"{\"h\":0.0,\"s\":0.0,\"v\":151.0}"}`
		var value struct {
			H float64
			S float64
			V float64
		}
		if err = json.Unmarshal([]byte(tcRaw.Value), &value); err != nil {
			break
		}
		tc.Value = model.HSV{H: int(value.H), S: int(value.S), V: int(value.V)}
	case tcRaw.Code == string(SceneData2Command):
		if tcRaw.Value == "" {
			tc.Value = SceneDataState{SceneNum: 1}
			return nil
		}
		var value SceneDataState
		if err = json.Unmarshal([]byte(tcRaw.Value), &value); err != nil {
			break
		}
		tc.Value = value
	default:
		tc.Code = "unknown"
		tc.Value = ""
	}

	return err
}

type UserDeviceActionView struct {
	ID       string
	Commands []TuyaCommand
}

func (udac *UserDeviceActionView) FromDeviceActionView(ctx context.Context, tuyaClient *Client, device adapter.DeviceActionRequestView) error {
	udac.ID = device.ID
	commands := make([]TuyaCommand, 0)
	commandsMap := make(map[TuyaCommandName]TuyaCommand)

	var customData tuya.CustomData
	if err := mapstructure.Decode(device.CustomData, &customData); err != nil {
		return xerrors.Errorf("failed to parse custom data: %w", err)
	}

	if deviceType := customData.DeviceType; len(deviceType) == 0 {
		return fmt.Errorf("device type is not found in custom data")
	}

	var workMode TuyaWorkMode
	switchCommandName := SwitchCommand
	if switchFromCustomData := customData.SwitchCommand; len(switchFromCustomData) != 0 {
		switchCommandName = SwitchCommandName(switchFromCustomData)
	}
	switchCommand := TuyaCommand{Code: switchCommandName.ToString(), Value: true, Priority: 99}

	switch customData.DeviceType {
	// Light device type
	case model.LightDeviceType:
		tuyaDevice, err := tuyaClient.GetDeviceByID(ctx, device.ID)
		if err != nil {
			return err
		}
		lampSpec := LampSpecByPID(tuyaDevice.ProductID)

		workModeCommand, exists := tuyaDevice.GetStatusItemByCommandName(WorkModeCommand)
		if !exists {
			return fmt.Errorf("unable to get device work_mode")
		}
		workMode = TuyaWorkMode(workModeCommand.Value.(string))

	lightCapabilityLoop:
		for _, capability := range device.Capabilities {
			switch capability.Type {
			// OnOff
			case model.OnOffCapabilityType:
				state := capability.State.(model.OnOffCapabilityState)

				// Ignore other actions if power_switch is false
				if onOffValue := state.Value; !onOffValue {
					commandsMap = make(map[TuyaCommandName]TuyaCommand)
					switchCommand.Value = false
					// TODO: return action statuses for other actions
					break lightCapabilityLoop
				}
			// Color
			case model.ColorSettingCapabilityType:
				state := capability.State.(model.ColorSettingCapabilityState)

				switch state.Instance {
				case model.TemperatureKCapabilityInstance:
					commandsMap[WorkModeCommand] = TuyaCommand{Code: WorkModeCommand.ToString(), Value: WhiteWorkMode}
					if tempValue, ok := lampSpec.temperatureKToTempValueMap[state.Value.(model.TemperatureK)]; ok {
						commandsMap[TempValueCommand] = TuyaCommand{Code: lampSpec.tempCommand.ToString(), Value: tempValue}
					} else {
						commandsMap[TempValueCommand] = TuyaCommand{Code: lampSpec.tempCommand.ToString(), Value: temperatureKToTempValue(state.Value.(model.TemperatureK), tuyaDevice.ProductID)}
					}
					// If we switching to white mode from color mode - sync brightness between modes
					if _, ok := commandsMap[BrightnessCommand]; !ok && workMode == ColorWorkMode {
						colourData, exists := tuyaDevice.GetColorDataItem()
						if !exists {
							return fmt.Errorf("unable to get device colour_data for device in `colour` mode")
						}
						colourDataHsvValue := colourData.Value.(model.HSV)
						briPercent := percentFromRange(colourDataHsvValue.V, lampSpec.ColorBrightness.Min, lampSpec.ColorBrightness.Max)
						commandsMap[BrightnessCommand] = TuyaCommand{Code: lampSpec.brightCommand.ToString(), Value: rangeValueFromPercent(briPercent, lampSpec.BrightValueLocalSpec.Min, lampSpec.BrightValueLocalSpec.Max)}
					}

				case model.HsvColorCapabilityInstance:
					colourData, exists := tuyaDevice.GetColorDataItem()
					if !exists {
						return fmt.Errorf("unable to get device colour_data for device in `colour` mode")
					}

					colourDataHsvValue := colourData.Value.(model.HSV)
					commandHsvValue := hsvStateToTuyaHsv(state.Value.(model.HSV), tuyaDevice.ProductID)
					if cdc, ok := commandsMap[ColorDataCommand]; !ok {
						if workMode == ColorWorkMode {
							commandHsvValue.V = colourDataHsvValue.V
							// If we switching to color mode from white mode - sync brightness between modes
						} else if workMode == WhiteWorkMode {
							briValue, exists := tuyaDevice.GetBrightValueItem()
							if !exists {
								return fmt.Errorf("unable to get device bright_value for device in `white` mode")
							}
							briPercent := percentFromRange(briValue.Value.(int), lampSpec.BrightValueLocalSpec.Min, lampSpec.BrightValueLocalSpec.Max)
							commandHsvValue.V = rangeValueFromPercent(briPercent, lampSpec.ColorBrightness.Min, lampSpec.ColorBrightness.Max)
						}
					} else {
						// if we are here, then we have already processed the range brightness capability
						// So get V value from it
						commandHsvValue.V = cdc.Value.(model.HSV).V
					}

					commandsMap[WorkModeCommand] = TuyaCommand{Code: WorkModeCommand.ToString(), Value: ColorWorkMode}
					commandsMap[ColorDataCommand] = TuyaCommand{Code: lampSpec.colorCommand.ToString(), Value: commandHsvValue}
				case model.SceneCapabilityInstance:
					_, exists := tuyaDevice.GetSceneDataItem()
					if !exists {
						return fmt.Errorf("unable to get scene_data_v2 for supported device")
					}
					colorSceneID := state.Value.(model.ColorSceneID)
					colorScenes := GetColorScenes(getColorScenePool(tuyaDevice.ProductID, customData.FwVersion))
					colorScene, exists := colorScenes[colorSceneID]
					if !exists {
						return fmt.Errorf("unable to set scene with id %s", colorSceneID)
					}
					commandsMap[WorkModeCommand] = TuyaCommand{Code: WorkModeCommand.ToString(), Value: SceneWorkMode}
					commandsMap[SceneData2Command] = TuyaCommand{Code: SceneData2Command.ToString(), Value: colorScene}
				}
			// Range
			case model.RangeCapabilityType:
				state := capability.State.(model.RangeCapabilityState)
				switch state.Instance {
				// -- Range.Brightness
				case model.BrightnessRangeInstance:
					// Set Brightness for white mode
					commandsMap[BrightnessCommand] = TuyaCommand{Code: lampSpec.brightCommand.ToString(), Value: rangeValueFromPercent(int(state.Value), lampSpec.BrightValueLocalSpec.Min, lampSpec.BrightValueLocalSpec.Max)} // decrease max brightness (really 255) to 80% (204)
					// Set Brightness for color mode if device has colour_value command
					colourData, exist := tuyaDevice.GetColorDataItem()
					if exist {
						var commandHsvValue model.HSV
						if cdc, ok := commandsMap[ColorDataCommand]; ok {
							// if we are here, then we have already processed the color_setting capability
							// So update only V value within it
							commandHsvValue = cdc.Value.(model.HSV)
						} else {
							commandHsvValue = colourData.Value.(model.HSV)
						}
						commandHsvValue.V = rangeValueFromPercent(int(state.Value), lampSpec.ColorBrightness.Min, lampSpec.ColorBrightness.Max)
						commandsMap[ColorDataCommand] = TuyaCommand{Code: lampSpec.colorCommand.ToString(), Value: commandHsvValue}
					}
				default:
					return fmt.Errorf("unsupported range capability instance: %s", state.Instance)
				}
			}
		}

		// Filter out conflicting commands
		if workModeComand, ok := commandsMap[WorkModeCommand]; ok {
			switch workModeComand.Value {
			case ColorWorkMode:
				delete(commandsMap, BrightnessCommand)
				delete(commandsMap, SceneData2Command)
			case WhiteWorkMode:
				delete(commandsMap, ColorDataCommand)
				delete(commandsMap, SceneData2Command)
			case SceneWorkMode:
				delete(commandsMap, BrightnessCommand)
				delete(commandsMap, ColorDataCommand)
			}
		} else {
			switch workMode {
			case ColorWorkMode:
				delete(commandsMap, BrightnessCommand)
				delete(commandsMap, SceneData2Command)
			case WhiteWorkMode:
				delete(commandsMap, ColorDataCommand)
				delete(commandsMap, SceneData2Command)
			case SceneWorkMode:
				delete(commandsMap, BrightnessCommand)
				delete(commandsMap, ColorDataCommand)
			}
		}

	// Socket device type
	case model.SocketDeviceType:
	socketCapabilityLoop:
		for _, capability := range device.Capabilities {
			switch capability.Type {
			case model.OnOffCapabilityType:
				state := capability.State.(model.OnOffCapabilityState)

				// Ignore other actions if power_switch is false
				if onOffValue := state.Value; !onOffValue {
					commandsMap = make(map[TuyaCommandName]TuyaCommand)
					switchCommand.Value = false
					break socketCapabilityLoop
				}
			}
		}

	//
	default:
		return fmt.Errorf("unsupported device type: %s", customData.DeviceType)
	}

	commandsMap[TuyaCommandName(switchCommand.Code)] = switchCommand

	for _, command := range commandsMap {
		commands = append(commands, command)
	}
	sort.Slice(commands, func(i, j int) bool { return commands[i].Priority < commands[j].Priority })
	udac.Commands = commands

	return nil
}
