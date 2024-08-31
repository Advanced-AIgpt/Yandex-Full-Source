package tuya

import (
	"fmt"
	"math"
	"strconv"
	"strings"

	"a.yandex-team.ru/alice/iot/bulbasaur/dto/adapter"
	"a.yandex-team.ru/alice/iot/bulbasaur/model"
	"a.yandex-team.ru/alice/iot/bulbasaur/model/tuya"
	"a.yandex-team.ru/alice/library/go/timestamp"
	"a.yandex-team.ru/alice/library/go/tools"
	"a.yandex-team.ru/library/go/core/xerrors"
	"a.yandex-team.ru/library/go/ptr"
)

type IRControl struct {
	ID                string            `json:"remote_id"`
	Name              string            `json:"remote_name"`
	CategoryID        IrCategoryID      `json:"category_id"`
	BrandID           string            `json:"brand_id"`
	BrandName         string            `json:"brand_name"`
	RemoteIndex       string            `json:"remote_index"`
	Keys              map[string]string `json:"keys"`
	TransmitterID     string            `json:"transmitter_id"`
	CustomControlData *IRCustomControl  `json:"-"`
}

func (irc *IRControl) IsCustomControl() bool {
	return len(irc.BrandName) == 0 && irc.CategoryID == "999" && irc.BrandID == "0"
}

func (irc *IRControl) GetDeviceConfig(productID TuyaDeviceProductID) DeviceConfig {
	deviceKey := DeviceKey{
		TuyaCategory: TuyaIRDeviceType.ToString(),
		ProductID:    productID,
	}
	if config, ok := KnownDevices[deviceKey]; ok {
		return config
	}
	return DeviceConfig{
		Model:        UnknownDeviceModel,
		Manufacturer: UnknownDeviceManufacturer,
	}
}

func (irc *IRControl) ToDeviceInfoView(productID TuyaDeviceProductID) adapter.DeviceInfoView {
	deviceConfig := irc.GetDeviceConfig(productID)

	deviceInfoView := adapter.DeviceInfoView{
		ID:         irc.ID,
		Name:       irc.Name,
		DeviceInfo: &model.DeviceInfo{},
	}

	if len(deviceConfig.Manufacturer) > 0 {
		deviceInfoView.DeviceInfo.Manufacturer = ptr.String(deviceConfig.Manufacturer)
	}

	if len(deviceConfig.Model) > 0 {
		deviceInfoView.DeviceInfo.Model = ptr.String(deviceConfig.Model)
	}

	customData := tuya.CustomData{
		InfraredData: &tuya.InfraredData{
			BrandName:     irc.BrandName,
			TransmitterID: irc.TransmitterID,
		},
	}
	// check for custom preset
	if irc.CustomControlData != nil {
		// change device visible tuya name for db name assigned by user
		deviceInfoView.Name = irc.CustomControlData.Name
		customData.InfraredData.Learned = true
	} else {
		customData.InfraredData.PresetID = irc.RemoteIndex
	}

	keys := make(map[tuya.IRKeyName]string)
	capabilities := make([]adapter.CapabilityInfoView, 0)

	if irc.CustomControlData == nil {
		switch irc.CategoryID {
		case AcIrCategoryID:
			// TODO: cover by tests
			deviceInfoView.Type = model.AcDeviceType
			customData.DeviceType = model.AcDeviceType

			// POWER BUTTON
			onOffCapability := adapter.CapabilityInfoView{
				Type:        model.OnOffCapabilityType,
				Retrievable: true,
				Reportable:  false,
			}
			capabilities = append(capabilities, onOffCapability)

			var acAbilities InfraredACControlAbilities
			acAbilities.FromKeysMap(irc.Keys)

			// TEMPERATURE RANGE
			if acAbilities.TemperatureWorkRange != nil {
				temperatureRangeCapability := adapter.CapabilityInfoView{
					Type:        model.RangeCapabilityType,
					Retrievable: true,
					Reportable:  false,
					Parameters: model.RangeCapabilityParameters{
						Instance:     model.TemperatureRangeInstance,
						Unit:         model.UnitTemperatureCelsius,
						RandomAccess: true,
						Looped:       false,
						Range: &model.Range{
							Min:       float64(acAbilities.TemperatureWorkRange.Min),
							Max:       float64(acAbilities.TemperatureWorkRange.Max),
							Precision: 1,
						},
					},
				}
				capabilities = append(capabilities, temperatureRangeCapability)
			}

			// AC WORK MODES
			if len(acAbilities.AcWorkModes) > 0 {
				modes := make([]model.Mode, 0, len(acAbilities.AcWorkModes))
				for _, mode := range acAbilities.AcWorkModes {
					modes = append(modes, model.Mode{Value: mode})
				}
				workModeCapability := adapter.CapabilityInfoView{
					Type:        model.ModeCapabilityType,
					Retrievable: true,
					Reportable:  false,
					Parameters: model.ModeCapabilityParameters{
						Instance: model.ThermostatModeInstance,
						Modes:    modes,
					},
				}
				capabilities = append(capabilities, workModeCapability)
			}

			// FAN SPEED MODES
			if len(acAbilities.FanSpeedModes) > 0 {
				modes := make([]model.Mode, 0, len(acAbilities.FanSpeedModes))
				for _, mode := range acAbilities.FanSpeedModes {
					modes = append(modes, model.Mode{Value: mode})
				}
				fanModeCapability := adapter.CapabilityInfoView{
					Type:        model.ModeCapabilityType,
					Retrievable: true,
					Reportable:  false,
					Parameters: model.ModeCapabilityParameters{
						Instance: model.FanSpeedModeInstance,
						Modes:    modes,
					},
				}
				capabilities = append(capabilities, fanModeCapability)
			}
		// TV IR CONTROL
		case TvIrCategoryID, SetTopBoxIrCategoryID, BoxIrCategoryID:
			switch irc.CategoryID {
			case TvIrCategoryID:
				deviceInfoView.Type = model.TvDeviceDeviceType
				customData.DeviceType = model.TvDeviceDeviceType

			case SetTopBoxIrCategoryID:
				deviceInfoView.Type = model.ReceiverDeviceType
				customData.DeviceType = model.ReceiverDeviceType

			case BoxIrCategoryID:
				deviceInfoView.Type = model.TvBoxDeviceType
				customData.DeviceType = model.TvBoxDeviceType
			}

			// POWER BUTTON
			if keyID, exists := irc.Keys[string(PowerKeyName)]; exists {
				keys[PowerKeyName] = keyID
				onOffCapability := adapter.CapabilityInfoView{
					Type:        model.OnOffCapabilityType,
					Retrievable: false,
					Reportable:  false,
				}
				capabilities = append(capabilities, onOffCapability)
			}

			// MUTE BUTTON
			if keyID, exists := irc.Keys[string(MuteKeyName)]; exists {
				keys[MuteKeyName] = keyID
				muteCapability := adapter.CapabilityInfoView{
					Type:        model.ToggleCapabilityType,
					Retrievable: false,
					Reportable:  false,
					Parameters: model.ToggleCapabilityParameters{
						Instance: model.MuteToggleCapabilityInstance,
					},
				}
				capabilities = append(capabilities, muteCapability)
			}

			// PAUSE BUTTON
			if keyID, exists := irc.Keys[string(PauseKeyName)]; exists {
				keys[PauseKeyName] = keyID
				pauseCapability := adapter.CapabilityInfoView{
					Type:        model.ToggleCapabilityType,
					Retrievable: false,
					Reportable:  false,
					Parameters: model.ToggleCapabilityParameters{
						Instance: model.PauseToggleCapabilityInstance,
					},
				}
				capabilities = append(capabilities, pauseCapability)
			}

			// INPUT SOURCE BUTTONS
			if ContainsAny(InputSourceIRKeys, irc.Keys) {
				inputSourcesKeysMap, inputSourcesModes := MaskInputSourceKeysUnderFictionalKeyNames(irc.Keys)
				for key, value := range inputSourcesKeysMap {
					keys[key] = value
				}
				inputSourceCapability := adapter.CapabilityInfoView{
					Retrievable: false,
					Reportable:  false,
					Type:        model.ModeCapabilityType,
					Parameters: model.ModeCapabilityParameters{
						Instance: model.InputSourceModeInstance,
						Modes:    inputSourcesModes,
					},
				}
				capabilities = append(capabilities, inputSourceCapability)
			}

			// VOLUME BUTTONS
			if ContainsAll(VolumeIRKeys, irc.Keys) {
				upKeyID := irc.Keys[string(VolumeUpKeyName)]
				downKeyID := irc.Keys[string(VolumeDownKeyName)]
				keys[VolumeUpKeyName] = upKeyID
				keys[VolumeDownKeyName] = downKeyID
				volumeCapability := adapter.CapabilityInfoView{
					Type:        model.RangeCapabilityType,
					Retrievable: false,
					Reportable:  false,
					Parameters: model.RangeCapabilityParameters{
						Instance:     model.VolumeRangeInstance,
						RandomAccess: false,
						Looped:       false,
					},
				}
				capabilities = append(capabilities, volumeCapability)
			}

			// CHANNEL BUTTONS
			if ContainsAll(ChannelUpDownIRKeys, irc.Keys) || ContainsAll(ChannelDigitsIRKeys, irc.Keys) {
				channelCapability := adapter.CapabilityInfoView{
					Type:        model.RangeCapabilityType,
					Retrievable: false,
					Reportable:  false,
					Parameters: model.RangeCapabilityParameters{
						Instance:     model.ChannelRangeInstance,
						RandomAccess: ContainsAll(ChannelDigitsIRKeys, irc.Keys),
						Looped:       ContainsAll(ChannelUpDownIRKeys, irc.Keys),
					},
				}
				if ContainsAll(ChannelUpDownIRKeys, irc.Keys) {
					upKeyID := irc.Keys[string(ChannelUpKeyName)]
					downKeyID := irc.Keys[string(ChannelDownKeyName)]
					keys[ChannelUpKeyName] = upKeyID
					keys[ChannelDownKeyName] = downKeyID
				}
				if ContainsAll(ChannelDigitsIRKeys, irc.Keys) {
					for _, keyName := range ChannelDigitsIRKeys {
						keyID := irc.Keys[keyName]
						keys[tuya.IRKeyName(keyName)] = keyID
					}
				}
				capabilities = append(capabilities, channelCapability)
			}
		}
	} else {
		// custom control now supports only custom buttons
		deviceInfoView.Type = irc.CustomControlData.DeviceType
		customData.DeviceType = irc.CustomControlData.DeviceType

		for _, button := range irc.CustomControlData.Buttons {
			customButtonCapability := adapter.CapabilityInfoView{
				Retrievable: false,
				Reportable:  false,
				Type:        model.CustomButtonCapabilityType,
				Parameters: model.CustomButtonCapabilityParameters{
					InstanceNames: []string{button.Name},
					Instance:      model.CustomButtonCapabilityInstance(button.Key),
				},
			}

			keys[tuya.IRKeyName(button.Key)] = button.Key
			capabilities = append(capabilities, customButtonCapability)
		}
	}

	customData.InfraredData.Keys = keys
	deviceInfoView.Capabilities = capabilities
	deviceInfoView.Properties = make([]adapter.PropertyInfoView, 0)
	deviceInfoView.CustomData = customData

	return deviceInfoView
}

type IRCommand struct {
	PresetID string `json:"preset_id"`
	KeyID    string `json:"key_id"`
}

type IRCustomCommand struct {
	RemoteID string `json:"remote_id"`
	Key      string `json:"key"`
}

type IRBatchCommand struct {
	InfraredID string     `json:"infrared_id"`
	RemoteID   string     `json:"remote_id"`
	Key        BatchIrKey `json:"key"`
	Value      string     `json:"value"`
}

type IRControlSendKeysView struct {
	ID             string
	Commands       []IRCommand
	CustomCommands []IRCustomCommand
	BatchCommands  []IRBatchCommand
	CustomData     tuya.CustomData
}

func (ircs *IRControlSendKeysView) FromDeviceActionView(device adapter.DeviceActionRequestView) error {
	ircs.ID = device.ID
	commands := make([]IRCommand, 0, len(device.Capabilities))
	customCommands := make([]IRCustomCommand, 0, len(device.Capabilities))
	batchCommands := make([]IRBatchCommand, 0, len(device.Capabilities))

	if ircs.CustomData.InfraredData.Learned {
		for _, capability := range device.Capabilities {
			switch capability.Type {
			case model.CustomButtonCapabilityType:
				if keyID, exists := ircs.CustomData.InfraredData.Keys[tuya.IRKeyName(capability.State.GetInstance())]; exists {
					customCommands = append(customCommands, IRCustomCommand{RemoteID: ircs.ID, Key: keyID})
				} else {
					return fmt.Errorf("unable to find custom button key %s within custom data", capability.State.GetInstance())
				}
			}
		}
	} else {
		switch ircs.CustomData.DeviceType {
		// TV, Receiver, Box
		case model.TvDeviceDeviceType, model.ReceiverDeviceType, model.TvBoxDeviceType:
			for _, capability := range device.Capabilities {
				switch capability.Type {
				// POWER
				case model.OnOffCapabilityType:
					if keyID, exists := ircs.CustomData.InfraredData.Keys[PowerKeyName]; exists {
						commands = append(commands, IRCommand{PresetID: ircs.CustomData.InfraredData.PresetID, KeyID: keyID})
					} else {
						return fmt.Errorf("unable to find power button key_id within custom data")
					}
				case model.ToggleCapabilityType:
					state := capability.State.(model.ToggleCapabilityState)
					switch state.Instance {
					// MUTE
					case model.MuteToggleCapabilityInstance:
						if keyID, exists := ircs.CustomData.InfraredData.Keys[MuteKeyName]; exists {
							commands = append(commands, IRCommand{PresetID: ircs.CustomData.InfraredData.PresetID, KeyID: keyID})
						} else {
							return fmt.Errorf("unable to find mute button key_id within custom data")
						}
					// PAUSE
					case model.PauseToggleCapabilityInstance:
						if keyID, exists := ircs.CustomData.InfraredData.Keys[PauseKeyName]; exists {
							commands = append(commands, IRCommand{PresetID: ircs.CustomData.InfraredData.PresetID, KeyID: keyID})
						} else {
							return fmt.Errorf("unable to find pause button key_id within custom data")
						}
					default:
						return fmt.Errorf("unsupported instance for current device")
					}
				// CUSTOM
				case model.CustomButtonCapabilityType:
					if keyID, exists := ircs.CustomData.InfraredData.Keys[tuya.IRKeyName(capability.State.GetInstance())]; exists {
						commands = append(commands, IRCommand{PresetID: ircs.CustomData.InfraredData.PresetID, KeyID: keyID})
					} else {
						return fmt.Errorf("unable to find custom button key %s within custom data", capability.State.GetInstance())
					}
				// VOLUME & CHANNEL
				case model.RangeCapabilityType:
					state := capability.State.(model.RangeCapabilityState)
					switch state.Instance {
					// VOLUME
					case model.VolumeRangeInstance:
						// batch api
						if state.IsRelative() {
							var (
								volumeKeyName tuya.IRKeyName
								batchIrKey    BatchIrKey
							)
							switch {
							case state.Value > 0:
								volumeKeyName = VolumeUpKeyName
								batchIrKey = BatchVolumeUpKey
							case state.Value < 0:
								volumeKeyName = VolumeDownKeyName
								batchIrKey = BatchVolumeDownKey
							default:
								return fmt.Errorf("expected positive or negative float64 value within state value in relative range action")
							}

							if _, exists := ircs.CustomData.InfraredData.Keys[volumeKeyName]; exists {
								batchCommands = append(batchCommands, IRBatchCommand{
									InfraredID: ircs.CustomData.InfraredData.TransmitterID,
									RemoteID:   device.ID,
									Key:        batchIrKey,
									Value:      strconv.Itoa(int(math.Abs(state.Value))),
								})
							} else {
								return fmt.Errorf("unable to find %s button key_id within custom data", volumeKeyName)
							}
						} else {
							return fmt.Errorf("TV IR Control supports only relative:true volume range action state")
						}
					// CHANNEL
					case model.ChannelRangeInstance:
						if state.IsRelative() {
							switch state.Value {
							case float64(1):
								if keyID, exists := ircs.CustomData.InfraredData.Keys[ChannelUpKeyName]; exists {
									commands = append(commands, IRCommand{PresetID: ircs.CustomData.InfraredData.PresetID, KeyID: keyID})
								} else {
									return fmt.Errorf("unable to find %s button key_id within custom data", ChannelUpKeyName)
								}
							case float64(-1):
								if keyID, exists := ircs.CustomData.InfraredData.Keys[ChannelDownKeyName]; exists {
									commands = append(commands, IRCommand{PresetID: ircs.CustomData.InfraredData.PresetID, KeyID: keyID})
								} else {
									return fmt.Errorf("unable to find %s button key_id within custom data", ChannelDownKeyName)
								}
							default:
								return fmt.Errorf("expected -1 or 1 within state value in relative range action")
							}
						} else {
							if state.Value < 0 {
								return fmt.Errorf("channel value can`t be negative")
							}
							for _, channelDigit := range SplitFloat64ToInts(state.Value) {
								channelKeyName := tuya.IRKeyName(strconv.Itoa(channelDigit))
								if keyID, exists := ircs.CustomData.InfraredData.Keys[channelKeyName]; exists {
									commands = append(commands, IRCommand{PresetID: ircs.CustomData.InfraredData.PresetID, KeyID: keyID})
								} else {
									return fmt.Errorf("unable to find %s button key_id within custom data", channelKeyName)
								}
							}
						}
					default:
						return fmt.Errorf("unsupported instance for current device")
					}
				case model.ModeCapabilityType:
					state := capability.State.(model.ModeCapabilityState)
					modeValue := state.Value
					switch state.Instance {
					case model.InputSourceModeInstance:
						if keyName, exists := NumericModeToInputSourceIRKeyName[modeValue]; exists {
							if keyID, exists := ircs.CustomData.InfraredData.Keys[keyName]; exists {
								commands = append(commands, IRCommand{PresetID: ircs.CustomData.InfraredData.PresetID, KeyID: keyID})
							} else {
								return fmt.Errorf("unable to find %s button key_id within custom data", keyName)
							}
						} else {
							return fmt.Errorf("unable to find %s mode value corresponding button key_id within custom data", modeValue)
						}
					default:
						return fmt.Errorf("unsupported instance for current device")
					}
				}
			}
		}
	}

	ircs.Commands = commands
	ircs.CustomCommands = customCommands
	ircs.BatchCommands = batchCommands
	return nil
}

type temperatureWorkRange struct {
	Min int
	Max int
}

type InfraredACControlAbilities struct {
	TemperatureWorkRange *temperatureWorkRange
	AcWorkModes          []model.ModeValue
	FanSpeedModes        []model.ModeValue
}

func (iaca *InfraredACControlAbilities) FromKeysMap(keysMap map[string]string) {
	acWorkModesMap := make(map[model.ModeValue]bool)
	fanSpeedModesMap := make(map[model.ModeValue]bool)
	temperatures := make([]int, 0, len(keysMap))

	// keySet for example: M0_T20_S0: mode=0, temp=20, wind=0
	for keySet := range keysMap {
		keySet = strings.ToLower(keySet)
		abilities := strings.Split(keySet, "_")
		for _, ability := range abilities {
			// skip ability shorter than 2 symbols cause it cant has correct value
			if len(ability) < 2 {
				continue
			}

			switch {
			// MODE
			case strings.HasPrefix(ability, "m"):
				if mode, found := IrACModesMap[fmt.Sprintf("%c", ability[1])]; found {
					acWorkModesMap[mode] = true
				}
			// TEMP
			case strings.HasPrefix(ability, "t"):
				if temperature, err := strconv.Atoi(strings.Replace(ability, "t", "", 1)); err == nil {
					temperatures = append(temperatures, temperature)
				}
			// FAN_SPEED
			case strings.HasPrefix(ability, "s"):
				if mode, found := IrACFanSpeedMap[fmt.Sprintf("%c", ability[1])]; found {
					fanSpeedModesMap[mode] = true
				}
			}
		}
	}

	iaca.AcWorkModes = make([]model.ModeValue, 0, len(acWorkModesMap))
	for mode := range acWorkModesMap {
		iaca.AcWorkModes = append(iaca.AcWorkModes, mode)
	}

	iaca.FanSpeedModes = make([]model.ModeValue, 0, len(fanSpeedModesMap))
	for mode := range fanSpeedModesMap {
		iaca.FanSpeedModes = append(iaca.FanSpeedModes, mode)
	}

	if len(temperatures) > 0 {
		tempRange := temperatureWorkRange{
			Min: tools.IntSliceMin(temperatures),
			Max: tools.IntSliceMax(temperatures),
		}
		iaca.TemperatureWorkRange = &tempRange
	}
}

type TuyaAcState struct {
	RemoteID string  `json:"remote_id"`
	Power    *string `json:"power,omitempty"`
	Mode     *string `json:"mode,omitempty"`
	Wind     *string `json:"wind,omitempty"`
	Temp     *string `json:"temp,omitempty"`
}

func (tas *TuyaAcState) ToDeviceStateView() adapter.DeviceStateView {
	deviceStateView := adapter.DeviceStateView{
		ID: tas.RemoteID,
	}
	capabilities := make([]adapter.CapabilityStateView, 0)

	// Thermostat mode
	if tas.Mode != nil {
		if mode, ok := IrACModesMap[*tas.Mode]; ok {
			capability := adapter.CapabilityStateView{
				Type: model.ModeCapabilityType,
				State: model.ModeCapabilityState{
					Instance: model.ThermostatModeInstance,
					Value:    mode,
				},
			}
			capabilities = append(capabilities, capability)
		}
	}

	// FanSpeed mode
	if tas.Wind != nil {
		if mode, ok := IrACFanSpeedMap[*tas.Wind]; ok {
			capability := adapter.CapabilityStateView{
				Type: model.ModeCapabilityType,
				State: model.ModeCapabilityState{
					Instance: model.FanSpeedModeInstance,
					Value:    mode,
				},
			}
			capabilities = append(capabilities, capability)
		}
	}

	// Range Temperature
	if tas.Temp != nil {
		if temp, err := strconv.ParseFloat(*tas.Temp, 64); err != nil {
			deviceStateView.ErrorCode = adapter.InternalError
			deviceStateView.ErrorMessage = err.Error()
		} else {
			capability := adapter.CapabilityStateView{
				Type: model.RangeCapabilityType,
				State: model.RangeCapabilityState{
					Instance: model.TemperatureRangeInstance,
					Value:    temp,
				},
			}
			capabilities = append(capabilities, capability)
		}
	}

	// OnOff
	if tas.Power != nil {
		if powerValue, ok := IrAcPowerMap[*tas.Power]; ok {
			capability := adapter.CapabilityStateView{
				Type: model.OnOffCapabilityType,
				State: model.OnOffCapabilityState{
					Instance: model.OnOnOffCapabilityInstance,
					Value:    powerValue,
				},
			}
			capabilities = append(capabilities, capability)
		} else {
			deviceStateView.ErrorCode = adapter.InternalError
			deviceStateView.ErrorMessage = fmt.Sprintf("Get unsupported power value from provider: %s", *tas.Power)
		}
	}

	if deviceStateView.ErrorCode == "" {
		deviceStateView.Capabilities = capabilities
	}
	return deviceStateView
}

type AcStateView struct {
	RemoteID    string  `json:"remote_id"` // ir device id
	RemoteIndex string  `json:"remote_index"`
	Power       *string `json:"power"`
	Mode        *string `json:"mode,omitempty"`
	Wind        *string `json:"wind,omitempty"`
	Temp        *string `json:"temp,omitempty"`
}

func (asv AcStateView) String() string {
	if asv.Power == nil {
		asv.Power = ptr.String("nil")
	}
	if asv.Mode == nil {
		asv.Mode = ptr.String("nil")
	}
	if asv.Wind == nil {
		asv.Wind = ptr.String("nil")
	}
	if asv.Temp == nil {
		asv.Temp = ptr.String("nil")
	}

	return fmt.Sprintf(
		"{remote_id: %s, remote_index:%s power:%s mode:%s wind:%s temp:%s}",
		asv.RemoteID,
		asv.RemoteIndex,
		*asv.Power,
		*asv.Mode,
		*asv.Wind,
		*asv.Temp,
	)
}

type AcIRDeviceStateView struct {
	ID            string  `json:",omitempty"`
	TransmitterID string  `json:",omitempty"`
	RemoteIndex   string  `json:"remote_index"`
	Power         *string `json:"power"`
	Mode          *string `json:"mode,omitempty"`
	Wind          *string `json:"wind,omitempty"`
	Temp          *string `json:"temp,omitempty"`
}

func (aidsv *AcIRDeviceStateView) FromDeviceActionView(device adapter.DeviceActionRequestView) error {

	for _, capability := range device.Capabilities {
		switch capability.Type {
		// POWER
		case model.OnOffCapabilityType:
			aidsv.Power = ptr.String("1")
			if !capability.State.(model.OnOffCapabilityState).Value {
				aidsv.Power = ptr.String("0")
			}
		// MODE & FAN SPEED
		case model.ModeCapabilityType:
			state := capability.State.(model.ModeCapabilityState)
			switch state.Instance {
			case model.ThermostatModeInstance:
				workModeIDMap := map[model.ModeValue]string{
					model.CoolMode:    "0",
					model.HeatMode:    "1",
					model.AutoMode:    "2",
					model.FanOnlyMode: "3",
					model.DryMode:     "4",
				}

				if modeID, found := workModeIDMap[model.ModeValue(state.Value)]; found {
					aidsv.Mode = ptr.String(modeID)
				} else {
					return xerrors.Errorf("unsupported ac work mode value %q: %w", state.Value, &model.InvalidValueError{})
				}
			case model.FanSpeedModeInstance:
				fanSpeedIDMap := map[model.ModeValue]string{
					model.AutoMode:   "0",
					model.LowMode:    "1",
					model.MediumMode: "2",
					model.HighMode:   "3",
				}

				if modeID, found := fanSpeedIDMap[model.ModeValue(state.Value)]; found {
					aidsv.Wind = ptr.String(modeID)
				} else {
					return xerrors.Errorf("unsupported fan speed mode value %q: %w", state.Value, &model.InvalidValueError{})
				}
			default:
				return fmt.Errorf("unsupported mode instance for current device: %s", state.Instance)
			}
		// TEMPERATURE
		case model.RangeCapabilityType:
			state := capability.State.(model.RangeCapabilityState)
			switch state.Instance {
			case model.TemperatureRangeInstance:
				if state.Relative == nil || !*state.Relative {
					aidsv.Temp = ptr.String(strconv.Itoa(int(state.Value)))
				} else {
					return fmt.Errorf("AC IR Control doesn`t supports relative temperature changing")
				}
			default:
				return fmt.Errorf("unsupported range instance for current device: %s", state.Instance)
			}
		}
	}

	return nil
}

func (aidsv *AcIRDeviceStateView) ToAcStateView() AcStateView {
	return AcStateView{
		RemoteID:    aidsv.ID,
		RemoteIndex: aidsv.RemoteIndex,
		Power:       aidsv.Power,
		Mode:        aidsv.Mode,
		Temp:        aidsv.Temp,
		Wind:        aidsv.Wind,
	}
}

type learnedCodeResponse struct {
	Code    string
	Success bool
}

type tuyaLearnedCodeResponse struct {
	tuyaResponse
	Result learnedCodeResponse
}

type MatchedPreset struct {
	BrandID    string       `json:"brand_id"`
	BrandName  string       `json:"brand_name"`
	PresetID   string       `json:"preset_id"`
	CategoryID IrCategoryID `json:"category_id"`
}

type MatchedPresets []MatchedPreset

type matchingRemotesBrand struct {
	BrandID   string `json:"brand_id"`
	BrandName string `json:"brand_name"`
}

type matchingRemotesResultSet struct {
	Brands      []matchingRemotesBrand `json:"brands"`
	RemoteIndex string                 `json:"remote_index"`
}

func (mrrs *matchingRemotesResultSet) toMatchedPreset(categoryID IrCategoryID) (MatchedPreset, bool) {
	// it guaranteed that in resultSet there is at least one brand
	if len(mrrs.Brands) > 0 {
		return MatchedPreset{BrandID: mrrs.Brands[0].BrandID, BrandName: mrrs.Brands[0].BrandName, PresetID: mrrs.RemoteIndex, CategoryID: categoryID}, true
	}
	// but just in case of something
	return MatchedPreset{}, false
}

type matchingRemotesResponse struct {
	tuyaResponse
	Result []matchingRemotesResultSet
}

func (mrr matchingRemotesResponse) toMatchedPresets(categoryID IrCategoryID) MatchedPresets {
	matchedPresetList := make(MatchedPresets, 0)
	for _, mrrs := range mrr.Result {
		if matchedPreset, ok := mrrs.toMatchedPreset(categoryID); ok {
			matchedPresetList = append(matchedPresetList, matchedPreset)
		}
	}
	return matchedPresetList
}

func (first MatchedPresets) Intersect(second MatchedPresets) MatchedPresets {
	firstMap := make(map[string]bool)
	for _, preset := range first {
		firstMap[preset.PresetID] = true
	}
	result := make(MatchedPresets, 0)

	for _, preset := range second {
		if _, found := firstMap[preset.PresetID]; found {
			result = append(result, preset)
		}
	}
	return result
}

type IRCode struct {
	Code string `json:"code"`
	Name string `json:"name"`
}

func GenerateIRCodeName() string {
	return strconv.FormatInt(timestamp.Now().UnixNano(), 10)
}

type irCustomControlResponse struct {
	RemoteID string `json:"remote_id"`
}

type tuyaIRCustomControlResponse struct {
	tuyaResponse
	Result irCustomControlResponse
}

// find any input sources available and mask them under numeric modes
func MaskInputSourceKeysUnderFictionalKeyNames(keysMap map[string]string) (map[tuya.IRKeyName]string, []model.Mode) {
	resultMap := make(map[tuya.IRKeyName]string)
	modes := make([]model.Mode, 0, len(InputSourceIRKeys))
	for _, irKey := range InputSourceIRKeys {
		if len(modes) == len(model.IntToNumericModeValue) {
			break
		}
		if keyID, exists := keysMap[irKey]; exists {
			var irKeyName tuya.IRKeyName
			var numericMode model.ModeValue

			if mapNumericMode, exist := model.IntToNumericModeValue[len(modes)+1]; exist {
				numericMode = mapNumericMode
				if mapKeyName, exist := NumericModeToInputSourceIRKeyName[mapNumericMode]; exist {
					irKeyName = mapKeyName
				} else {
					break
				}
			} else {
				break
			}

			resultMap[irKeyName] = keyID
			modes = append(modes, model.Mode{Value: numericMode})
		}
	}
	return resultMap, modes
}
