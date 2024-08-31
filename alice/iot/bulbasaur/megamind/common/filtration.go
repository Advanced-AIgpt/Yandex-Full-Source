package common

import (
	"a.yandex-team.ru/alice/iot/bulbasaur/model"
	"a.yandex-team.ru/alice/iot/bulbasaur/nlg"
	libnlg "a.yandex-team.ru/alice/library/go/nlg"
)

type FrameFiltrationResult struct {
	Reason          FrameFiltrationReason
	SurvivedDevices model.Devices
}

func NewFrameFiltrationResult(devices model.Devices) FrameFiltrationResult {
	return FrameFiltrationResult{
		Reason:          AllGoodFiltrationReason,
		SurvivedDevices: devices,
	}
}

func (fr *FrameFiltrationResult) Merge(other FrameFiltrationResult) {
	if other.Reason != AllGoodFiltrationReason {
		fr.Reason = other.Reason
	}
	fr.SurvivedDevices = other.SurvivedDevices
}

type FrameFiltrationReason string

func (fr FrameFiltrationReason) NLG() libnlg.NLG {
	if filterNLG, ok := FiltrationReasonNLGs[fr]; ok {
		return filterNLG
	}
	return nlg.CannotFindDevices
}

var FiltrationReasonNLGs = map[FrameFiltrationReason]libnlg.NLG{
	InappropriateDevicesFiltrationReason:     nlg.CannotFindDevices,
	InappropriateHouseholdFiltrationReason:   nlg.InappropriateHousehold,
	InappropriateRoomFiltrationReason:        nlg.InappropriateRoom,
	InappropriateGroupFiltrationReason:       nlg.InappropriateGroup,
	InappropriateCapabilityFiltrationReason:  nlg.InvalidAction,
	InappropriateQueryIntentFiltrationReason: nlg.CannotFindDevices,
}

const (
	InappropriateHouseholdFiltrationReason   FrameFiltrationReason = "INAPPROPRIATE_HOUSEHOLD"
	InappropriateRoomFiltrationReason        FrameFiltrationReason = "INAPPROPRIATE_ROOM"
	InappropriateGroupFiltrationReason       FrameFiltrationReason = "INAPPROPRIATE_GROUP"
	InappropriateDevicesFiltrationReason     FrameFiltrationReason = "INAPPROPRIATE_DEVICES"
	TandemTVFiltrationReason                 FrameFiltrationReason = "TANDEM_TV_HYPOTHESIS"
	InappropriateCapabilityFiltrationReason  FrameFiltrationReason = "INAPPROPRIATE_CAPABILITY"
	InappropriateQueryIntentFiltrationReason FrameFiltrationReason = "INAPPROPRIATE_QUERY_INTENT"
	AllGoodFiltrationReason                  FrameFiltrationReason = "ALL_GOOD"
)

func FilterByGroups(devices model.Devices, groupIDs []string) FrameFiltrationResult {
	filtrationResult := NewFrameFiltrationResult(devices)
	if len(groupIDs) == 0 || len(devices) == 0 {
		return filtrationResult
	}

	survived := devices.FilterByGroupIDs(groupIDs)
	if len(survived) == 0 {
		filtrationResult.Reason = InappropriateGroupFiltrationReason
	}
	filtrationResult.SurvivedDevices = survived

	return filtrationResult
}

func FilterByRooms(devices model.Devices, roomIDs []string) FrameFiltrationResult {
	filtrationResult := NewFrameFiltrationResult(devices)
	if len(roomIDs) == 0 || len(devices) == 0 {
		return filtrationResult
	}

	survived := devices.FilterByRoomIDs(roomIDs)
	if len(survived) == 0 {
		filtrationResult.Reason = InappropriateRoomFiltrationReason
	}
	filtrationResult.SurvivedDevices = survived

	return filtrationResult
}

func FilterByHouseholds(devices model.Devices, householdIDs []string) FrameFiltrationResult {
	filtrationResult := NewFrameFiltrationResult(devices)
	if len(householdIDs) == 0 || len(devices) == 0 {
		return filtrationResult
	}

	survived := devices.FilterByHouseholdIDs(householdIDs)
	if len(survived) == 0 {
		filtrationResult.Reason = InappropriateHouseholdFiltrationReason
	}
	filtrationResult.SurvivedDevices = survived

	return filtrationResult
}

func FilterByIDs(devices model.Devices, deviceIDs []string) FrameFiltrationResult {
	filtrationResult := NewFrameFiltrationResult(devices)
	if len(devices) == 0 || len(deviceIDs) == 0 {
		return filtrationResult
	}

	survived := devices.FilterByIDs(deviceIDs)
	if len(survived) == 0 {
		filtrationResult.Reason = InappropriateDevicesFiltrationReason
	}
	filtrationResult.SurvivedDevices = survived

	return filtrationResult
}

// FilterByTandem returns empty devices slice and TandemTVFiltrationReason if there is at least one tv in devices
func FilterByTandem(devices model.Devices, isTandem bool) FrameFiltrationResult {
	filtrationResult := NewFrameFiltrationResult(devices)
	if !isTandem || len(devices) == 0 {
		return filtrationResult
	}

	var survived model.Devices
	for _, d := range devices {
		if d.Type == model.TvDeviceDeviceType {
			filtrationResult.Reason = TandemTVFiltrationReason
			filtrationResult.SurvivedDevices = model.Devices{}
			return filtrationResult
		}
		survived = append(survived, d)
	}
	filtrationResult.SurvivedDevices = survived
	return filtrationResult
}

func FilterByActionIntentParameters(devices model.Devices, intentParameters ActionIntentParameters) FrameFiltrationResult {
	var survived model.Devices
	filtrationResult := NewFrameFiltrationResult(devices)
	if len(devices) == 0 {
		return filtrationResult
	}

	for _, d := range devices {
		if isActionIntentApplicable(d, intentParameters) {
			survived = append(survived, d)
		}
	}

	if len(survived) == 0 {
		filtrationResult.Reason = InappropriateCapabilityFiltrationReason
	}
	filtrationResult.SurvivedDevices = survived
	return filtrationResult
}

func isActionIntentApplicable(device model.Device, intentParameters ActionIntentParameters) bool {
	capabilityType := intentParameters.CapabilityType
	capabilityInstance := intentParameters.CapabilityInstance
	capability, ok := device.GetCapabilityByTypeAndInstance(capabilityType, capabilityInstance)
	if !ok {
		return false
	}
	if IsActionApplicable(device, capability, intentParameters.CapabilityValue, intentParameters.CapabilityUnit, intentParameters.RelativityType) {
		return true
	}
	return false
}

func IsActionApplicable(d model.Device, capability model.ICapability, value interface{}, unit model.Unit,
	relativityType RelativityType) bool {

	switch capability.Type() {
	case model.OnOffCapabilityType:
		return isOnOffCapabilityApplicable(d.Type, value)
	case model.ColorSettingCapabilityType:
		return isColorSettingCapabilityApplicable(capability, value)
	case model.RangeCapabilityType:
		return isRangeCapabilityApplicable(d, capability, value, relativityType, unit)
	default:
		return true
	}
}

func isOnOffCapabilityApplicable(deviceType model.DeviceType, capabilityValue interface{}) bool {
	// we cannot turn irons on
	if deviceType == model.IronDeviceType && capabilityValue.(bool) {
		return false
	}
	// we cannot unfeed the dog/cat
	if deviceType == model.PetFeederDeviceType && !(capabilityValue.(bool)) {
		return false
	}

	return true
}

func isColorSettingCapabilityApplicable(capability model.ICapability, value interface{}) bool {
	params := capability.Parameters().(model.ColorSettingCapabilityParameters)

	// some lights are white-mode only
	if capability.Instance() == model.HypothesisColorCapabilityInstance && params.ColorModel == nil {
		colorID := model.ColorID((value).(string))
		color, ok := model.ColorPalette.GetColorByID(colorID)
		// color unknown or multicolor request on white-mode lamp
		if !ok || color.Type == model.Multicolor {
			return false
		}
	}
	// white is applicable to color-only lamps
	if capability.Instance() == string(model.TemperatureKCapabilityInstance) && params.TemperatureK == nil {
		return false
	}
	if capability.Instance() == model.HypothesisColorSceneCapabilityInstance && params.ColorSceneParameters == nil {
		colorSceneID := model.ColorSceneID((value).(string))
		_, ok := params.GetAvailableScenes().AsMap()[colorSceneID]
		// scene unknown
		if !ok {
			return false
		}
	}

	return true
}

func isRangeCapabilityApplicable(d model.Device, capability model.ICapability, value interface{},
	relativityType RelativityType, unit model.Unit) bool {
	params := capability.Parameters().(model.RangeCapabilityParameters)

	if params.Range == nil {
		// if we got an absolute value from mm, but we cannot set it
		if relativityType == "" && !params.RandomAccess {
			return false
		}
		// if we got a relative percent value from mm, but we cannot set it cause range is nil
		if relativityType != "" && unit == model.UnitPercent {
			return false
		}
		// simple value type check
		if v, ok := value.(float64); value != nil && !ok {
			return false
		} else {
			// relative values can be positive only
			if relativityType != "" && v < 0 {
				return false
			}
			// Skip relative range action with value over 50 cause IR hub restrictions
			if capability.Instance() == string(model.ChannelRangeInstance) || capability.Instance() == string(model.VolumeRangeInstance) {
				if d.SkillID == model.TUYA && relativityType != "" && v > 50 {
					return false
				}
			}
		}
	} else {
		// if value is an absolute value
		if v, ok := value.(float64); ok && relativityType == "" {
			// if random_access cannot be used
			if !params.RandomAccess {
				return false
			}
			// if requested value cannot be set
			if v > params.Range.Max || v < params.Range.Min {
				return false
			}
		}
	}

	return true
}
