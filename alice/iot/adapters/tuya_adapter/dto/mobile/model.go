package mobile

import (
	"reflect"
	"sort"
	"strconv"
	"strings"

	"a.yandex-team.ru/alice/iot/adapters/tuya_adapter/tuya"
	"a.yandex-team.ru/alice/iot/bulbasaur/model"
	"a.yandex-team.ru/alice/library/go/tools"
	"a.yandex-team.ru/library/go/core/xerrors"
	"a.yandex-team.ru/library/go/valid"
)

type IRCategory struct {
	ID   string              `json:"id"`
	Name tuya.IrCategoryName `json:"name"`
	Type model.DeviceType    `json:"type"`
}

type IRCategoriesResponse struct {
	Status     string       `json:"status"`
	RequestID  string       `json:"request_id"`
	Categories []IRCategory `json:"categories"`
}

type IRBrand struct {
	ID   string `json:"id"`
	Name string `json:"name"`
}

type IRCategoryBrandsResponse struct {
	Status    string    `json:"status"`
	RequestID string    `json:"request_id"`
	Brands    []IRBrand `json:"brands"`
}

type IRCategoryBrandPresetsResponse struct {
	Status    string   `json:"status"`
	RequestID string   `json:"request_id"`
	Presets   []string `json:"presets"`
}

type IRCategoryBrandPresetControlResponse struct {
	Status    string      `json:"status"`
	RequestID string      `json:"request_id"`
	Control   interface{} `json:"control"`
}

type IRMatchedRemotes struct {
	MatchingType         model.DeviceType                         `json:"matching_type"`
	TypeToMatchedPresets map[model.DeviceType]tuya.MatchedPresets `json:"matched_presets"`
}

type IRMatchedRemotesResponse struct {
	Status         string           `json:"status"`
	RequestID      string           `json:"request_id"`
	MatchedRemotes IRMatchedRemotes `json:"matched_remotes"`
}

type InfraredTVControl struct {
	Type model.DeviceType  `json:"type"`
	Keys map[string]string `json:"keys"`
}

type Mode struct {
	Value model.ModeValue `json:"value"`
	Name  string          `json:"name"`
}

type temperatureRange struct {
	Min int `json:"min"`
	Max int `json:"max"`
}

type InfraredACControl struct {
	Type model.DeviceType `json:"type"`
	Keys struct {
		Temperature *temperatureRange `json:"temperature,omitempty"`
		Modes       []Mode            `json:"modes"`
		FanSpeed    []Mode            `json:"fan_speed"`
	} `json:"keys"`
}

func (iac *InfraredACControl) FromInfraredACControlAbilities(controlAbilities tuya.InfraredACControlAbilities) {
	acModes := make([]Mode, 0, len(controlAbilities.AcWorkModes))
	for _, mode := range controlAbilities.AcWorkModes {
		if knownModesMode, exists := model.KnownModes[mode]; exists {
			acModes = append(acModes, Mode{Value: mode, Name: *knownModesMode.Name})
		}
	}

	fanModes := make([]Mode, 0, len(controlAbilities.FanSpeedModes))
	for _, mode := range controlAbilities.FanSpeedModes {
		if knownModesMode, exists := model.KnownModes[mode]; exists {
			fanModes = append(fanModes, Mode{Value: mode, Name: *knownModesMode.Name})
		}
	}

	sort.Sort(ModesSorting(acModes))
	iac.Keys.Modes = acModes

	sort.Sort(ModesSorting(fanModes))
	iac.Keys.FanSpeed = fanModes

	if controlAbilities.TemperatureWorkRange != nil {
		iac.Keys.Temperature = &temperatureRange{
			Min: controlAbilities.TemperatureWorkRange.Min,
			Max: controlAbilities.TemperatureWorkRange.Max,
		}
	}
}

type IrAcAction struct {
	PresetID     string  `json:"preset_id"`
	Power        bool    `json:"power"`
	Mode         *string `json:"mode,omitempty"`
	FanSpeedMode *string `json:"fan_speed,omitempty"`
	Temperature  *int    `json:"temperature,omitempty"`
}

func (iaa *IrAcAction) ToAcStateView() (tuya.AcStateView, error) {
	state := tuya.AcStateView{
		RemoteIndex: iaa.PresetID,
		Power:       tools.AOS("0"),
	}

	// Power
	if iaa.Power {
		state.Power = tools.AOS("1")
	}

	// Fan mode
	if iaa.FanSpeedMode != nil {
		fanSpeedIDMap := map[model.ModeValue]string{
			model.AutoMode:   "0",
			model.LowMode:    "1",
			model.MediumMode: "2",
			model.HighMode:   "3",
		}

		if modeID, found := fanSpeedIDMap[model.ModeValue(*iaa.FanSpeedMode)]; found {
			state.Wind = tools.AOS(modeID)
		} else {
			return tuya.AcStateView{}, xerrors.Errorf("unsupported fan speed mode value %q: %w", *iaa.FanSpeedMode, &model.InvalidValueError{})
		}
	}

	// Ac work mode
	if iaa.Mode != nil {
		workModeIDMap := map[model.ModeValue]string{
			model.CoolMode:    "0",
			model.HeatMode:    "1",
			model.AutoMode:    "2",
			model.FanOnlyMode: "3",
			model.DryMode:     "4",
		}

		mode := model.ModeValue(*iaa.Mode)
		if modeID, found := workModeIDMap[mode]; found {
			state.Mode = tools.AOS(modeID)
		} else {
			return tuya.AcStateView{}, xerrors.Errorf("unsupported ac work mode value %q: %w", mode, &model.InvalidValueError{})
		}
	}

	// Temperature
	if iaa.Temperature != nil {
		state.Temp = tools.AOS(strconv.Itoa(*iaa.Temperature))
	}

	return state, nil
}

type DeviceFirmwareVersionResponse struct {
	Status        string                `json:"status"`
	RequestID     string                `json:"request_id"`
	UpgradeStatus FirmwareUpgradeStatus `json:"upgrade_status"`
}

type UpgradeDeviceFirmwareResponse struct {
	Status    string `json:"status"`
	RequestID string `json:"request_id"`
}

// for adding buttons to custom control
type IRCustomCode struct {
	CustomName IRCustomButtonName `json:"name"`
}

// saving custom control with one button first
type IRSaveCustomControlRequest struct {
	Name       IRCustomControlName `json:"name"`
	DeviceType model.DeviceType    `json:"device_type"`
	Code       IRCustomCode        `json:"code"`
}

type IRSaveCustomControlResponse struct {
	Status    string `json:"status"`
	RequestID string `json:"request_id"`
	DeviceID  string `json:"device_id"`
}

type IRCustomButton struct {
	Key  string `json:"key"`
	Name string `json:"name"`
}

func (ircb *IRCustomButton) FromIRCustomButton(cb tuya.IRCustomButton) {
	ircb.Name = cb.Name
	ircb.Key = cb.Key
}

type IRCustomControlConfigurationView struct {
	ID         string           `json:"id"`
	Name       string           `json:"name"`
	DeviceType model.DeviceType `json:"device_type"`
	Buttons    []IRCustomButton `json:"buttons"`
}

func (cccv *IRCustomControlConfigurationView) FromIRCustomControl(cc tuya.IRCustomControl) {
	cccv.ID = cc.ID
	cccv.Name = cc.Name
	cccv.DeviceType = cc.DeviceType
	cccv.Buttons = make([]IRCustomButton, 0, len(cc.Buttons))
	for _, cpButton := range cc.Buttons {
		var button IRCustomButton
		button.FromIRCustomButton(cpButton)
		cccv.Buttons = append(cccv.Buttons, button)
	}
	sort.Sort(IRCustomButtonSortingByName(cccv.Buttons))
}

type IRCustomButtonSuggestionsResponse struct {
	Status      string   `json:"status"`
	RequestID   string   `json:"request_id"`
	Suggestions []string `json:"suggestions"`
}

type IRCustomControlSuggestionsResponse struct {
	Status      string   `json:"status"`
	RequestID   string   `json:"request_id"`
	Suggestions []string `json:"suggestions"`
}

type IRCustomButtonName string

type IRCustomButtonValidationRequest struct {
	Name IRCustomButtonName `json:"name"`
}

func (ircbn IRCustomButtonName) Validate(vctx *valid.ValidationCtx) (bool, error) {
	var verrs valid.Errors

	if err := tuya.ValidCustomButtonName(string(ircbn), 40); err != nil {
		verrs = append(verrs, err)
	}

	if validator, ok := vctx.Get(IRCustomButtonExistingNameValidator); ok {
		var param string
		if err := validator(reflect.ValueOf(ircbn), param); err != nil {
			verrs = append(verrs, err)
		}
	}

	if len(verrs) > 0 {
		return false, verrs
	}
	return false, nil
}

func IRCustomButtonExistingNameValidatorFunc(buttons []tuya.IRCustomButton) valid.ValidatorFunc {
	return func(value reflect.Value, _ string) error {
		if value.Kind() != reflect.String {
			return valid.ErrBadParams
		}
		for _, button := range buttons {
			otherButtonName := tools.StandardizeSpaces(strings.ToLower(russianNumericRegex.ReplaceAllString(button.Name, "")))
			buttonName := tools.StandardizeSpaces(strings.ToLower(russianNumericRegex.ReplaceAllString(value.String(), "")))
			if buttonName == otherButtonName {
				return &tuya.ErrCustomButtonNameIsTaken{}
			}
		}
		return nil
	}
}

type IRCustomControlName string

type IRCustomControlNameValidationRequest struct {
	Name IRCustomControlName `json:"name"`
}

func (irccn IRCustomControlName) Validate(vctx *valid.ValidationCtx) (bool, error) {
	var verrs valid.Errors

	if err := tuya.ValidCustomControlName(string(irccn), 20); err != nil {
		verrs = append(verrs, err)
	}

	if len(verrs) > 0 {
		return false, verrs
	}
	return false, nil
}

type IRCustomButtonRenameRequest struct {
	Name string `json:"name"`
}

type GetDevicesUnderPairingTokenResponse struct {
	SuccessDevices []tuya.DeviceUnderPairingToken `json:"successDevices"`
	ErrorDevices   []tuya.DeviceUnderPairingToken `json:"errorDevices"`
}

func (gduptr *GetDevicesUnderPairingTokenResponse) FromSuccessAndErrorDevices(successDevices []tuya.DeviceUnderPairingToken, errorDevices []tuya.DeviceUnderPairingToken) {
	gduptr.SuccessDevices = make([]tuya.DeviceUnderPairingToken, 0, len(successDevices))
	gduptr.SuccessDevices = append(gduptr.SuccessDevices, successDevices...)
	gduptr.ErrorDevices = make([]tuya.DeviceUnderPairingToken, 0, len(errorDevices))
	gduptr.ErrorDevices = append(gduptr.ErrorDevices, errorDevices...)
}
