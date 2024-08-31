package frames

import (
	"fmt"
	"math"
	"sort"
	"strings"
	"time"

	"github.com/mitchellh/copystructure"
	"golang.org/x/exp/slices"

	"a.yandex-team.ru/alice/iot/bulbasaur/megamind/common"
	"a.yandex-team.ru/alice/iot/bulbasaur/megamind/sdk"
	"a.yandex-team.ru/alice/iot/bulbasaur/model"
	"a.yandex-team.ru/alice/iot/bulbasaur/nlg"
	libnlg "a.yandex-team.ru/alice/library/go/nlg"
	"a.yandex-team.ru/alice/library/go/timestamp"
	"a.yandex-team.ru/alice/library/go/tools"
	commonpb "a.yandex-team.ru/alice/megamind/protos/common"
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/xerrors"
)

var _ sdk.GranetFrame = &ActionFrameV2{}
var _ sdk.TSF = &ActionFrameV2{}

type ActionFrameV2 struct {
	IntentParameters ActionIntentParametersSlots

	Devices    DeviceSlots
	Groups     GroupSlots
	Rooms      RoomSlots
	Households HouseholdSlots

	RangeValue           *RangeValueSlot
	ColorSettingValue    *ColorSettingValueSlot
	ToggleValue          *ToggleValueSlot
	OnOffValue           *OnOffValueSlot
	ModeValue            *ModeValueSlot
	CustomButtonInstance *CustomButtonInstanceSlot

	RequiredDeviceType RequiredDeviceTypesSlot
	AllDevicesRequired AllDevicesRequestedSlot

	ExactDate        ExactDateSlot
	ExactTime        ExactTimeSlot
	RelativeDateTime RelativeDateTimeSlot
	IntervalDateTime IntervalDateTimeSlot

	// TODO(aaulayev): remove this after updates to action processor are in production
	DeviceType DeviceTypeSlot

	ParsedTime            time.Time // time parsed from ExactDate, ExactTime and RelativeDateTime slots
	ParsedIntervalEndTime time.Time // time when interval action must be stopped
}

func (f *ActionFrameV2) FromTypedSemanticFrame(tsf *commonpb.TTypedSemanticFrame) error {
	deviceActionFrame := tsf.GetIoTDeviceActionSemanticFrame()
	if deviceActionFrame == nil {
		return xerrors.New("failed to get device action frame from")
	}

	request := deviceActionFrame.GetRequest().GetRequestValue()
	if request == nil {
		return xerrors.New("failed to get request value from device action frame")
	}

	protoIntentParameters := request.GetIntentParameters()

	f.IntentParameters = ActionIntentParametersSlots{
		{
			CapabilityType:     protoIntentParameters.CapabilityType,
			CapabilityInstance: protoIntentParameters.CapabilityInstance,
			RelativityType:     protoIntentParameters.CapabilityValue.RelativityType,
		},
	}

	// ActionFrameV2 has different fields for values of different capabilities,
	// while the tsf stores everything in its capability_value field. Here one of f's value slots is filled.
	if err := f.fillValueFromActionIntentParameters(protoIntentParameters); err != nil {
		return xerrors.Errorf("failed to fill value slot from action intent parameters: %w", err)
	}

	// For devices, rooms, households and groups use one slot per id, because that's how they usually come in granet frames.
	for _, id := range request.DeviceIDs {
		f.Devices = append(f.Devices, DeviceSlot{
			DeviceIDs: []string{id},
			SlotType:  string(DeviceSlotType),
		})
	}

	for _, id := range request.RoomIDs {
		f.Rooms = append(f.Rooms, RoomSlot{
			RoomIDs:  []string{id},
			SlotType: string(RoomSlotType),
		})
	}

	for _, id := range request.HouseholdIDs {
		f.Households = append(f.Households, HouseholdSlot{
			HouseholdID: id,
			SlotType:    string(HouseholdSlotType),
		})
	}

	for _, id := range request.GroupIDs {
		f.Groups = append(f.Groups, GroupSlot{IDs: []string{id}})
	}

	for _, deviceType := range request.DeviceTypes {
		f.Devices = append(f.Devices, DeviceSlot{
			DeviceType: deviceType,
			SlotType:   string(DeviceTypeSlotType),
		})
	}

	return nil
}

// ToTypedSemanticFrame is not supported, but necessary to implement TSF interface.
func (f *ActionFrameV2) ToTypedSemanticFrame() *commonpb.TTypedSemanticFrame {
	return nil
}

func (f *ActionFrameV2) SupportedSlots() []sdk.GranetSlot {
	return []sdk.GranetSlot{
		&ActionIntentParametersSlot{},
		&DeviceSlot{},
		&GroupSlot{},
		&RoomSlot{},
		&HouseholdSlot{},
		&RangeValueSlot{},
		&ColorSettingValueSlot{},
		&ToggleValueSlot{},
		&OnOffValueSlot{},
		&ModeValueSlot{},
		&CustomButtonInstanceSlot{},
		&RequiredDeviceTypesSlot{},
		&AllDevicesRequestedSlot{},
		&DeviceTypeSlot{},
		&ExactDateSlot{},
		&ExactTimeSlot{},
		&RelativeDateTimeSlot{},
		&IntervalDateTimeSlot{},
	}
}

func (f *ActionFrameV2) SetSlots(slots []sdk.GranetSlot) error {
	for _, slot := range slots {
		switch typedSlot := slot.(type) {
		case *ActionIntentParametersSlot:
			f.IntentParameters = append(f.IntentParameters, *typedSlot)
		case *DeviceSlot:
			f.Devices = append(f.Devices, *typedSlot)
		case *GroupSlot:
			f.Groups = append(f.Groups, *typedSlot)
		case *RoomSlot:
			f.Rooms = append(f.Rooms, *typedSlot)
		case *HouseholdSlot:
			f.Households = append(f.Households, *typedSlot)
		case *RangeValueSlot:
			f.RangeValue = typedSlot
		case *ColorSettingValueSlot:
			f.ColorSettingValue = typedSlot
		case *ToggleValueSlot:
			f.ToggleValue = typedSlot
		case *OnOffValueSlot:
			f.OnOffValue = typedSlot
		case *ModeValueSlot:
			f.ModeValue = typedSlot
		case *CustomButtonInstanceSlot:
			f.CustomButtonInstance = typedSlot
		case *RequiredDeviceTypesSlot:
			f.RequiredDeviceType = *typedSlot
		case *AllDevicesRequestedSlot:
			f.AllDevicesRequired = *typedSlot
		case *DeviceTypeSlot:
			f.DeviceType = *typedSlot
		case *ExactDateSlot:
			f.ExactDate = *typedSlot
		case *ExactTimeSlot:
			f.ExactTime = *typedSlot
		case *RelativeDateTimeSlot:
			f.RelativeDateTime = *typedSlot
		case *IntervalDateTimeSlot:
			f.IntervalDateTime = *typedSlot
		default:
			return xerrors.Errorf("unsupported slot: slot name: %q, slot type: %q", slot.Name(), slot.Type())
		}
	}

	return nil
}

// FromInput fills the frame from all frames in the input.
// There must be only action frames in the input.
// If there are multiple action frames in the input, they will be merged into one frame,
// because it is easier to deal with one ActionFrameV2 than with several of them.
func (f *ActionFrameV2) FromInput(input sdk.Input) error {
	if input.Type() == sdk.TypedSemanticFrameInputType {
		if err := sdk.UnmarshalTSF(input, f); err != nil {
			return xerrors.Errorf("failed to unmarshal tsf to action frame: %w", err)
		}
		return nil
	}

	inputFrames := input.GetFrames()
	if len(inputFrames) == 0 {
		return xerrors.New("no frames in the input")
	}

	actionFrames := make([]ActionFrameV2, 0, len(inputFrames))
	for _, inputFrame := range inputFrames {
		actionFrame := ActionFrameV2{}
		if err := sdk.UnmarshalSlots(&inputFrame, &actionFrame); err != nil {
			return xerrors.Errorf("failed to unmarshal slots: %w, input frame: %v", err, inputFrame)
		}
		actionFrames = append(actionFrames, actionFrame)
	}

	*f = mergeActionFrames(actionFrames)
	return nil
}

func (f *ActionFrameV2) AppendSlots(slots ...sdk.GranetSlot) error {
	if len(slots) == 0 {
		return nil
	}

	frame := f.Clone()
	for _, slot := range slots {
		switch typedSlot := slot.(type) {
		case *ActionIntentParametersSlot:
			frame.IntentParameters = append(frame.IntentParameters, *typedSlot)
		case *DeviceSlot:
			frame.Devices = append(frame.Devices, *typedSlot)
		case *GroupSlot:
			frame.Groups = append(frame.Groups, *typedSlot)
		case *RoomSlot:
			frame.Rooms = append(frame.Rooms, *typedSlot)
		case *HouseholdSlot:
			frame.Households = append(frame.Households, *typedSlot)
		case *RangeValueSlot:
			frame.RangeValue = typedSlot
		case *ColorSettingValueSlot:
			frame.ColorSettingValue = typedSlot
		case *ToggleValueSlot:
			frame.ToggleValue = typedSlot
		case *OnOffValueSlot:
			frame.OnOffValue = typedSlot
		case *ModeValueSlot:
			frame.ModeValue = typedSlot
		case *CustomButtonInstanceSlot:
			frame.CustomButtonInstance = typedSlot
		case *RequiredDeviceTypesSlot:
			frame.RequiredDeviceType = *typedSlot
		case *AllDevicesRequestedSlot:
			frame.AllDevicesRequired = *typedSlot
		case *DeviceTypeSlot:
			frame.DeviceType = *typedSlot
		case *ExactDateSlot:
			frame.ExactDate = *typedSlot
		case *ExactTimeSlot:
			frame.ExactTime = *typedSlot
		case *RelativeDateTimeSlot:
			frame.RelativeDateTime = *typedSlot
		case *IntervalDateTimeSlot:
			frame.IntervalDateTime = *typedSlot
		default:
			return xerrors.Errorf("unsupported slot: slot name: %q, slot type: %q", slot.Name(), slot.Type())
		}
	}

	*f = *frame
	return nil
}

func (f *ActionFrameV2) ContainsDateTime() bool {
	return !f.ExactTime.IsZero() || !f.ExactDate.IsZero() || !f.RelativeDateTime.IsZero()
}

func (f *ActionFrameV2) ValidateBegemotDateTime() error {
	if !f.ExactDate.IsZero() && f.RelativeDateTime.HasTime() {
		return common.WeirdTimeRelativityValidationError
	}

	// If there is exact date or relative date and no time, ask to specify time
	if (!f.ExactDate.IsZero() || !f.RelativeDateTime.IsZero() && !f.RelativeDateTime.HasTime()) && f.ExactTime.IsZero() {
		return common.TimeIsNotSpecifiedValidationError
	}

	return nil
}

func (f *ActionFrameV2) Clone() *ActionFrameV2 {
	if f == nil {
		return nil
	}

	cloned, err := copystructure.Copy(*f)
	if err != nil {
		return nil
	}

	clonedFrame := cloned.(ActionFrameV2)
	return &clonedFrame
}

func (f *ActionFrameV2) SetParsedTimeIfNotNow(runContext sdk.RunContext) {
	timestamper, err := timestamp.TimestamperFromContext(runContext.Context())
	if err != nil {
		runContext.Logger().Info("no timestamper in the context")
		return
	}

	clientTime := runContext.ClientInfo().LocalTime(timestamper.CreatedTimestamp().AsTime())
	mergedDate := f.ExactDate.MergedDate()
	mergedTime := f.ExactTime.MergedTime()
	runContext.Logger().Info(
		"parsing datetime",
		log.Any("exact_date", mergedDate),
		log.Any("exact_time", mergedTime),
		log.Any("relative_datetime", f.RelativeDateTime),
	)
	parsedTime := common.ParseBegemotDateAndTimeV2(
		clientTime,
		mergedDate,
		mergedTime,
		f.RelativeDateTime.DateTimeRanges,
	)
	runContext.Logger().Info("begemot datetime parsed", log.Time("parsed_time", parsedTime))
	if !parsedTime.Equal(clientTime) {
		f.ParsedTime = parsedTime
	}
}

func (f *ActionFrameV2) ValidateParsedTime(clientTime time.Time) error {
	if f.ParsedTime.IsZero() {
		return nil
	}
	if f.ParsedTime.Before(clientTime) {
		return common.PastActionValidationError
	}
	if f.ParsedTime.Sub(clientTime) > time.Hour*24*7 {
		return common.FarFutureValidationError
	}

	return nil
}

func (f *ActionFrameV2) ValidateHouseholds() error {
	if len(f.Households) > 1 {
		return common.MultipleHouseholdsInRequestValidationError
	}
	return nil
}

func (f *ActionFrameV2) ValidateVideoStreamCapability(runContext sdk.RunContext) error {
	if _, ok := f.IntentParameters.FindByCapability(model.VideoStreamCapabilityType, string(model.GetStreamCapabilityInstance)); !ok ||
		runContext.ClientInfo().IsTandem() {
		return nil
	}

	if runContext.ClientInfo().IsIotApp() {
		// TODO(akastornov): support iot app
		return common.CannotPlayVideoStreamInIotAppValidationError
	}
	if len(runContext.ClientInfo().GetSupportedVideoStreamProtocols()) == 0 {
		return common.CannotPlayVideoOnDeviceValidationError
	}
	if !runContext.Request().GetBaseRequest().GetInterfaces().GetIsTvPlugged() {
		return common.TVIsNotPluggedValidationError
	}

	return nil
}

func (f *ActionFrameV2) ValidateAllDevicesRequired() error {
	// IOT-801: we forbid to turn on all devices without groups.
	// Such frames contain on_off capability with empty devices and groups.
	_, hasOnOff := f.IntentParameters.FindByCapability(model.OnOffCapabilityType, string(model.OnOnOffCapabilityInstance))
	if hasOnOff && len(f.Devices) == 0 && f.RequiredDeviceType.IsZero() && len(f.Groups) == 0 {
		if f.OnOffValue.GetValue() {
			return common.TurnOnEverythingIsForbiddenValidationError
		}
	}

	return nil
}

func (f *ActionFrameV2) ValidateUnknownNumMode() error {
	// There are modes like auto 45 or fast 60.
	// In case user named a number we don't know, special value 'unknown' comes in mode_value slot.
	if f.ModeValue != nil && f.ModeValue.ModeValue == UnknownModeValue {
		return common.UnknownModeValidationError
	}
	return nil
}

// ExtractActionIntent translates the frame into action intent – a unified description of what is requested from devices.
// It also performs filtration of user devices by requested rooms, households, device types and groups and some additional validation.
func (f *ActionFrameV2) ExtractActionIntent(runContext sdk.RunContext) (ActionFrameExtractionResult, error) {
	if f.Rooms.ContainsOnlyDemo() {
		onlyDemoRoomsExtractionResult(f.Rooms)
	}

	gatheredDevices, extractionStatus := f.GatherDevices(runContext)
	if extractionStatus != OkExtractionStatus {
		if f.isRequestToTandem(runContext) {
			return ActionFrameExtractionResult{
				Status: RequestToTandem,
			}, nil
		}
		return ActionFrameExtractionResult{
			Status:     DevicesNotFoundExtractionStatus,
			FailureNLG: devicesNotFoundActionNLG(f.Devices),
		}, nil
	}

	filtrationResult, winnerIntentParameters := f.filterDevices(gatheredDevices)
	if filtrationResult.Reason != common.AllGoodFiltrationReason {
		return ActionFrameExtractionResult{
			Status:     ExtractionStatus(filtrationResult.Reason),
			FailureNLG: filtrationResult.Reason.NLG(),
		}, nil
	}

	survivedDevices, postProcessStatus := f.postProcessActionSurvivedDevices(runContext, filtrationResult.SurvivedDevices, winnerIntentParameters)
	if postProcessStatus == MultipleHouseholdsExtractionStatus {
		return ActionFrameExtractionResult{
			Status:     postProcessStatus,
			FailureNLG: nlg.NoHouseholdSpecifiedAction,
		}, nil
	}

	// https://st.yandex-team.ru/DIALOG-5842
	if f.isIrrelevantShortCommand(survivedDevices, winnerIntentParameters) {
		return ActionFrameExtractionResult{
			Status: ShortCommandWithMultiplePossibleRooms,
		}, nil
	}

	survivedDevices, err := f.updateDevicesFromIntentParametersV2(survivedDevices, winnerIntentParameters, runContext.ClientInfo())
	if err != nil {
		return ActionFrameExtractionResult{}, xerrors.Errorf("failed to update devices from intent parameters: %w", err)
	}

	return ActionFrameExtractionResult{
		Devices:              survivedDevices,
		IntentParametersSlot: winnerIntentParameters,
		ValueSlot:            f.getWinnerCapabilityValue(winnerIntentParameters),
		RequestedTime:        f.ParsedTime,
		IntervalEndTime:      f.ParsedIntervalEndTime,
		CreatedTime:          timestamp.CurrentTimestampCtx(runContext.Context()).AsTime(),
		Status:               OkExtractionStatus,
	}, nil
}

// postProcessActionSurvivedDevices leaves only devices from the current households if the client is iot app,
// or only devices from the same room with a speaker if the client is the speaker.
// Explicitly named devices or groups are preserved.
func (f *ActionFrameV2) postProcessActionSurvivedDevices(runContext sdk.RunContext, devices model.Devices, intentParameters ActionIntentParametersSlot) (model.Devices, ExtractionStatus) {
	explicitlyNamedDevices, gatheredDevices := f.findExplicitlyNamed(devices)

	// If there are only explicitly named devices, return them.
	if len(gatheredDevices) == 0 {
		if len(explicitlyNamedDevices.GroupByHousehold()) > 1 {
			return model.Devices{}, MultipleHouseholdsExtractionStatus
		}
		return explicitlyNamedDevices, OkExtractionStatus
	}

	// If no rooms or households specified, try to guess them from client info.
	if len(devices.GroupByHousehold()) > 1 && len(f.Households.HouseholdIDs()) == 0 {
		filtered, err := filterByClientInfo(gatheredDevices, runContext)
		if err != nil {
			runContext.Logger().Errorf("failed to filter devices by client info: %w", err)
			return nil, MultipleHouseholdsExtractionStatus
		} else if len(filtered.GroupByHousehold()) > 1 {
			// If we couldn't deduce household, ask user to specify it
			return nil, MultipleHouseholdsExtractionStatus
		} else {
			gatheredDevices = filtered
		}
	}

	// If there are suitable devices in multiple rooms, use those from the same room as the speaker.
	// If the request has a room type ("everything"), we must not filter.
	if len(f.Rooms.RoomIDs()) == 0 && len(f.Rooms.RoomTypes()) == 0 {
		gatheredDevices = tryToFilterBySpeakerRoom(gatheredDevices, runContext)
	}

	// Thermostats have higher priority in mode capability requests.
	if intentParameters.CapabilityType == string(model.ModeCapabilityType) {
		thermostatDevices := make(model.Devices, 0, len(gatheredDevices))
		for _, device := range gatheredDevices {
			if device.Type == model.ThermostatDeviceType {
				thermostatDevices = append(thermostatDevices, device)
			}
		}
		if len(thermostatDevices) > 0 {
			gatheredDevices = thermostatDevices
		}
	}

	return append(explicitlyNamedDevices, gatheredDevices...), OkExtractionStatus
}

func tryToFilterBySpeakerRoom(devices model.Devices, runContext sdk.RunContext) model.Devices {
	userInfo, _ := runContext.UserInfo()
	clientDevice, ok := userInfo.Devices.GetDeviceByQuasarExtID(runContext.ClientInfo().DeviceID)
	if !ok {
		return devices
	}

	devicesInTheSpeakerRoom := devices.FilterByRoomIDs([]string{clientDevice.Room.ID})
	// If there is no suitable devices in the speaker room, then don't filter by room.
	if len(devicesInTheSpeakerRoom) == 0 {
		return devices
	}

	return devicesInTheSpeakerRoom
}

func filterByClientInfo(devices model.Devices, runContext sdk.RunContext) (model.Devices, error) {
	devicesByHousehold := devices.GroupByHousehold()
	userInfo, _ := runContext.UserInfo()

	switch {
	case runContext.ClientInfo().IsSmartSpeaker():
		clientDevice, ok := userInfo.Devices.GetDeviceByQuasarExtID(runContext.ClientInfo().DeviceID)
		if !ok {
			return nil, xerrors.New("client device not found")
		}

		// Use devices from the same household as the speaker.
		householdDevices := devicesByHousehold[clientDevice.HouseholdID]
		// If there is no suitable devices in the household, return devices unchanged.
		if len(householdDevices) == 0 {
			return devices, nil
		}

		return householdDevices, nil

	case runContext.ClientInfo().IsIotApp():
		// Use current household id if the client is iot app: IOT-1339.
		householdDevices := devicesByHousehold[userInfo.CurrentHouseholdID]
		// If there is no suitable devices in the household, return devices unchanged.
		if len(householdDevices) == 0 {
			return devices, nil
		}
		return householdDevices, nil
	}

	return devices, nil
}

func deviceHasGroup(device model.Device, groupIDs map[string]bool) bool {
	for _, deviceGroupID := range device.GroupsIDs() {
		if groupIDs[deviceGroupID] {
			return true
		}
	}

	return false
}

func (f *ActionFrameV2) SetIntervalEndTime(runContext sdk.RunContext) {
	if f.IntervalDateTime.IsZero() {
		return
	}

	var endTime time.Time
	if f.ParsedTime.IsZero() {
		timestamper, err := timestamp.TimestamperFromContext(runContext.Context())
		if err != nil {
			runContext.Logger().Info("no timestamper in the context")
		}
		now := runContext.ClientInfo().LocalTime(timestamper.CreatedTimestamp().AsTime())
		endTime = common.AddDateTimeRangesToTime(now, f.IntervalDateTime.DateTimeRanges)
	} else {
		endTime = common.AddDateTimeRangesToTime(f.ParsedTime, f.IntervalDateTime.DateTimeRanges)
	}

	f.ParsedIntervalEndTime = endTime
}

func (f *ActionFrameV2) fillValueFromActionIntentParameters(parameters *commonpb.TIoTActionIntentParameters) error {
	if parameters.GetCapabilityValue().GetValue() == nil {
		return nil
	}

	switch parameters.GetCapabilityType() {
	case string(model.OnOffCapabilityType):
		value := parameters.GetCapabilityValue().GetBoolValue()
		f.OnOffValue = &OnOffValueSlot{
			Value: value,
		}
	case string(model.ToggleCapabilityType):
		value := parameters.GetCapabilityValue().GetBoolValue()
		f.ToggleValue = &ToggleValueSlot{
			Value: value,
		}
	case string(model.ModeCapabilityType):
		value := parameters.GetCapabilityValue().GetModeValue()
		f.ModeValue = &ModeValueSlot{
			ModeValue: model.ModeValue(value),
		}
	default:
		return xerrors.Errorf("unsupported capability type: %q", parameters.CapabilityType)
	}

	return nil
}

func (f *ActionFrameV2) GatherDevices(runContext sdk.RunContext) (model.Devices, ExtractionStatus) {
	userInfo, _ := runContext.UserInfo()

	if len(f.Devices) == 0 {
		return userInfo.Devices, OkExtractionStatus
	}

	gatheredDevices := userInfo.Devices.FilterByIDs(f.Devices.DeviceIDs())
	requestedDevicesByType := gatheredDevices.GroupByType()
	userDevicesByType := userInfo.Devices.GroupByType()

	// TODO(aaulayev): remove this when the processor is in production under exp
	if !f.DeviceType.IsZero() {
		f.Devices = append(f.Devices, DeviceSlot{
			DeviceType: string(f.DeviceType.DeviceType),
			SlotType:   string(DeviceTypeSlotType),
		})
	}

	// Add user devices of the requested device types
	for _, deviceType := range f.Devices.DeviceTypes() {
		// If requested type is light and some light device is explicitly named by user, do not gather light devices
		// "Включи свет в торшере" -> {"floor-lamp-id"}, not {"floor-lamp-id", "table-lamp-id", ...}
		if deviceType == string(model.LightDeviceType) && len(requestedDevicesByType[model.LightDeviceType]) > 0 {
			continue
		}

		gatheredDevices = append(gatheredDevices, userDevicesByType[model.DeviceType(deviceType)]...)
	}

	// Remove duplicates
	gatheredDevices = gatheredDevices.ToMap().Flatten()

	if len(gatheredDevices) == 0 {
		return nil, DevicesNotFoundExtractionStatus
	}

	return gatheredDevices, OkExtractionStatus
}

func (f *ActionFrameV2) filterDevices(devices model.Devices) (common.FrameFiltrationResult, ActionIntentParametersSlot) {
	filtrationResult := common.NewFrameFiltrationResult(devices)

	filtrationResult.Merge(common.FilterByGroups(filtrationResult.SurvivedDevices, f.Groups.GroupIDs()))
	filtrationResult.Merge(common.FilterByRooms(filtrationResult.SurvivedDevices, f.Rooms.RoomIDs()))
	filtrationResult.Merge(common.FilterByHouseholds(filtrationResult.SurvivedDevices, f.Households.HouseholdIDs()))

	intentParametersFiltrationResult, winnerIntentParameters := f.filterByActionIntentParameters(filtrationResult.SurvivedDevices, f.IntentParameters)
	filtrationResult.Merge(intentParametersFiltrationResult)

	// Required device type slot is used in requests like "make some coffee".
	// With its help we can answer properly if user asks us to do something illegal, like "make coffee on the tv".
	// Now the slot is only used with on_off capability.
	if winnerIntentParameters.CapabilityType == string(model.OnOffCapabilityType) && !f.RequiredDeviceType.IsZero() {
		filtrationResult.Merge(filterByRequiredDeviceType(filtrationResult.SurvivedDevices, f.RequiredDeviceType))
	}

	return filtrationResult, winnerIntentParameters
}

func (f *ActionFrameV2) filterByActionIntentParameters(devices model.Devices, intentParametersSlots ActionIntentParametersSlots) (common.FrameFiltrationResult, ActionIntentParametersSlot) {
	filtrationResult := common.NewFrameFiltrationResult(devices)
	if len(devices) == 0 {
		return filtrationResult, intentParametersSlots[0]
	}

	survivedParameters := map[ActionIntentParametersSlot]bool{}
	survivedDevices := map[ActionIntentParametersSlot]model.Devices{}

	// Find all suitable parameters for every device, mark parameters as survived and store devices in survivedDevices.
	for _, device := range devices {
		for _, parametersSlot := range intentParametersSlots {
			if f.isActionIntentApplicable(device, parametersSlot) {
				survivedParameters[parametersSlot] = true
				survivedDevices[parametersSlot] = append(survivedDevices[parametersSlot], device)
			}
		}
	}

	if len(survivedDevices) == 0 {
		filtrationResult.Reason = common.InappropriateCapabilityFiltrationReason
		filtrationResult.SurvivedDevices = model.Devices{}
		return filtrationResult, ActionIntentParametersSlot{}
	}

	winnerParameters := chooseWinnerParameters(survivedParameters)
	filtrationResult.SurvivedDevices = survivedDevices[winnerParameters]

	return filtrationResult, winnerParameters
}

func (f *ActionFrameV2) updateDevicesFromIntentParametersV2(devices model.Devices, intentParameters ActionIntentParametersSlot, clientInfo common.ClientInfo) (model.Devices, error) {
	updatedDevices := make(model.Devices, 0, len(devices))
	for _, device := range devices {
		var updatedDevice model.Device
		capability, err := f.capabilityFromIntentParameters(device, intentParameters, clientInfo)
		if err != nil {
			return updatedDevices, err
		}
		updatedDevice.PopulateAsStateContainer(device, model.Capabilities{capability})
		updatedDevices = append(updatedDevices, updatedDevice)
	}

	return updatedDevices, nil
}

func (f *ActionFrameV2) capabilityFromIntentParameters(device model.Device, intentParameters ActionIntentParametersSlot, clientInfo common.ClientInfo) (model.ICapability, error) {
	capability := model.MakeCapabilityByType(model.CapabilityType(intentParameters.CapabilityType))
	var err error

	switch c := capability.(type) {
	case *model.ColorSettingCapability:
		capability, err = f.fillColorSettingCapability(c, device, intentParameters)
	case *model.CustomButtonCapability:
		capability = f.fillCustomButtonCapability(c)
	case *model.ModeCapability:
		capability = f.fillModeCapability(c, device, intentParameters)
	case *model.OnOffCapability:
		capability = f.fillOnOffCapability(c, device, intentParameters)
	case *model.RangeCapability:
		capability, err = f.fillRangeCapability(c, device, intentParameters)
	case *model.ToggleCapability:
		capability = f.fillToggleCapability(c, device, intentParameters)
	case *model.VideoStreamCapability:
		capability = f.fillVideoStreamCapability(c, clientInfo)
	default:
		return nil, xerrors.New(fmt.Sprintf("unsupported capability: %T", c))
	}

	if err != nil {
		return nil, xerrors.Errorf("failed to fill capability from intent parameters: %w", err)
	}
	return capability, nil
}

func (f *ActionFrameV2) fillColorSettingCapability(c *model.ColorSettingCapability, device model.Device, intentParameters ActionIntentParametersSlot) (*model.ColorSettingCapability, error) {
	switch intentParameters.CapabilityInstance {
	case string(model.TemperatureKCapabilityInstance):
		if intentParameters.RelativityType == "" {
			return nil, xerrors.New("non-relative requests to temperature_k capability are not supported")
		} else {
			// Relativity is not nil, so we need to get Next or previous TemperatureK value from ColorPalette
			capability, _ := device.GetCapabilityByTypeAndInstance(model.ColorSettingCapabilityType, string(model.TemperatureKCapabilityInstance))
			var newColor model.Color

			// If state is not nil, get current color from state
			if capability.State() != nil && capability.State().(model.ColorSettingCapabilityState).Instance == model.TemperatureKCapabilityInstance {
				currentColor, _ := capability.State().(model.ColorSettingCapabilityState).ToColor()

				switch intentParameters.RelativityType {
				case string(common.Increase):
					newColor = model.ColorPalette.FilterType(model.WhiteColor).GetNext(currentColor)
				case string(common.Decrease):
					newColor = model.ColorPalette.FilterType(model.WhiteColor).GetPrevious(currentColor)
				}

				maxValue := capability.Parameters().(model.ColorSettingCapabilityParameters).TemperatureK.Max
				minValue := capability.Parameters().(model.ColorSettingCapabilityParameters).TemperatureK.Min

				if newColor.Temperature > maxValue || newColor.Temperature < minValue {
					newColor = currentColor
				}
			} else {
				// Otherwise, set color to default
				newColor = model.ColorPalette.GetDefaultWhiteColor()
			}
			c.SetState(newColor.ToColorSettingCapabilityState(model.TemperatureKCapabilityInstance))
		}
	case model.HypothesisColorSceneCapabilityInstance:
		if f.ColorSettingValue == nil {
			return nil, xerrors.New("failed to fill color scene capability: color setting value slot is nil")
		}
		colorScene, ok := model.KnownColorScenes[f.ColorSettingValue.ColorScene] // you cannot set unknown color scene via mm
		if !ok {
			return nil, xerrors.Errorf("unknown color scene id: %q", f.ColorSettingValue.ColorScene)
		}
		c.SetState(colorScene.ToColorSettingCapabilityState())
	case model.HypothesisColorCapabilityInstance:
		if f.ColorSettingValue == nil {
			return nil, xerrors.New("failed to fill color capability: color setting value slot is nil")
		}
		color, ok := model.ColorPalette.GetColorByID(f.ColorSettingValue.Color)
		if !ok {
			return nil, xerrors.Errorf("unknown color id: %q", f.ColorSettingValue.Color)
		}
		capability := device.GetCapabilitiesByType(model.ColorSettingCapabilityType)[0]
		c.SetState(color.ToColorSettingCapabilityState(
			capability.Parameters().(model.ColorSettingCapabilityParameters).GetColorSettingCapabilityInstance()),
		)
	default:
		return nil, xerrors.Errorf("unsupported color setting capability instance: %q", intentParameters.CapabilityInstance)
	}

	return c, nil
}

func (f *ActionFrameV2) fillCustomButtonCapability(c *model.CustomButtonCapability) *model.CustomButtonCapability {
	if f.CustomButtonInstance == nil || len(f.CustomButtonInstance.Instances) == 0 {
		return c
	}
	c.SetState(model.CustomButtonCapabilityState{
		Instance: f.CustomButtonInstance.Instances[0],
		Value:    true,
	})
	return c
}

func (f *ActionFrameV2) fillModeCapability(c *model.ModeCapability, device model.Device, intentParameters ActionIntentParametersSlot) *model.ModeCapability {
	if intentParameters.RelativityType == "" {
		c.SetState(model.ModeCapabilityState{
			Instance: model.ModeCapabilityInstance(intentParameters.CapabilityInstance),
			Value:    f.ModeValue.GetModeValue(),
		})
	} else {
		capability, _ := device.GetCapabilityByTypeAndInstance(model.ModeCapabilityType, intentParameters.CapabilityInstance)
		if parameters, ok := capability.Parameters().(model.ModeCapabilityParameters); ok {
			knownCurModes := make([]model.Mode, 0, len(parameters.Modes))
			for _, mode := range parameters.Modes {
				knownCurModes = append(knownCurModes, model.KnownModes[mode.Value])
			}
			sort.Sort(model.ModesSorting(knownCurModes))
			var curMode string
			// need to get current state, if state is nil, its first state of array
			if state, ok := capability.State().(model.ModeCapabilityState); ok {
				curMode = string(state.Value)
			} else {
				curMode = string(knownCurModes[0].Value)
			}
			var modeIndex = 0
			for ind, value := range knownCurModes {
				if string(value.Value) == curMode {
					modeIndex = ind
					break
				}
			}
			switch intentParameters.RelativityType {
			case string(common.Increase):
				if modeIndex+1 < len(knownCurModes) {
					modeIndex++
				} else {
					modeIndex = 0
				}
			case string(common.Decrease):
				if modeIndex-1 >= 0 {
					modeIndex--
				} else {
					modeIndex = len(knownCurModes) - 1
				}
			}
			c.SetState(model.ModeCapabilityState{
				Instance: model.ModeCapabilityInstance(intentParameters.CapabilityInstance),
				Value:    knownCurModes[modeIndex].Value,
			})
		}
	}

	return c
}

func (f *ActionFrameV2) fillOnOffCapability(c *model.OnOffCapability, device model.Device, intentParameters ActionIntentParametersSlot) *model.OnOffCapability {
	if intentParameters.RelativityType != "" {
		capability, _ := device.GetCapabilityByTypeAndInstance(model.OnOffCapabilityType, intentParameters.CapabilityInstance)
		if capability.State() == nil {
			capability.SetState(c.DefaultState())
		}
		newValue := capability.State().(model.OnOffCapabilityState).Value
		switch intentParameters.RelativityType {
		case string(common.Invert):
			newValue = !capability.State().(model.OnOffCapabilityState).Value
		}
		c.SetState(model.OnOffCapabilityState{
			Instance: model.OnOffCapabilityInstance(intentParameters.CapabilityInstance),
			Value:    newValue,
		})
	} else {
		c.SetState(model.OnOffCapabilityState{
			Instance: model.OnOffCapabilityInstance(intentParameters.CapabilityInstance),
			Value:    f.OnOffValue.GetValue(),
		})
	}

	return c
}

func (f *ActionFrameV2) fillRangeCapability(c *model.RangeCapability, device model.Device, intentParameters ActionIntentParametersSlot) (*model.RangeCapability, error) {
	capability, _ := device.GetCapabilityByTypeAndInstance(model.RangeCapabilityType, intentParameters.CapabilityInstance)
	var err error

	switch {
	case intentParameters.RelativityType == "" ||
		(f.RangeValue != nil && slices.Contains([]string{MaxRangeValue, MinRangeValue}, f.RangeValue.StringValue)):
		c, err = fillRangeFromAbsoluteValueV2(c, device, intentParameters.CapabilityInstance, f.RangeValue)
	case capability.Retrievable() && capability.Parameters().(model.RangeCapabilityParameters).Range != nil:
		c, err = fillRangeFromRetrievableRelativeValueV2(c, device, capability, intentParameters.CapabilityInstance,
			common.RelativityType(intentParameters.RelativityType), f.RangeValue)
	case !capability.Retrievable():
		c = fillRangeFromNonRetrievableRelativeValueV2(c, intentParameters.CapabilityInstance, common.RelativityType(intentParameters.RelativityType),
			f.RangeValue)
	default:
		panic(fmt.Sprintf("unexpected device capability: %v", capability))
	}

	if err != nil {
		return nil, xerrors.Errorf("failed to fill range capability: %w", err)
	}
	return c, nil
}

func fillRangeFromAbsoluteValueV2(c *model.RangeCapability, device model.Device, instance string, value *RangeValueSlot) (*model.RangeCapability, error) {
	if value == nil {
		return nil, xerrors.New("no range value slot in non-relative capability intent")
	}

	switch value.Type() {
	case string(StringSlotType):
		capability, _ := device.GetCapabilityByTypeAndInstance(model.RangeCapabilityType, instance)
		switch value.StringValue {
		case MaxRangeValue:
			c.SetState(model.RangeCapabilityState{
				Instance: model.RangeCapabilityInstance(instance),
				Value:    capability.Parameters().(model.RangeCapabilityParameters).Range.Max,
			})
		case MinRangeValue:
			c.SetState(model.RangeCapabilityState{
				Instance: model.RangeCapabilityInstance(instance),
				Value:    capability.Parameters().(model.RangeCapabilityParameters).Range.Min,
			})
		default:
			return nil, xerrors.Errorf("unknown string range value: %q", value.StringValue)
		}
	case string(NumSlotType):
		c.SetState(model.RangeCapabilityState{
			Instance: model.RangeCapabilityInstance(instance),
			Value:    float64(value.NumValue),
		})
	default:
		return nil, xerrors.Errorf("unsupported range value type: %q", value.Type())
	}

	return c, nil
}

func fillRangeFromRetrievableRelativeValueV2(c *model.RangeCapability, device model.Device, capability model.ICapability, instance string,
	relativityType common.RelativityType, value *RangeValueSlot) (*model.RangeCapability, error) {
	maxValue := capability.Parameters().(model.RangeCapabilityParameters).Range.Max
	minValue := capability.Parameters().(model.RangeCapabilityParameters).Range.Min
	precision := capability.Parameters().(model.RangeCapabilityParameters).Range.Precision

	// get currentValue from the state, if the state is nil, use the default one
	var currentValue float64
	if capability.State() == nil {
		currentValue = capability.DefaultState().(model.RangeCapabilityState).Value
	} else {
		currentValue = capability.State().(model.RangeCapabilityState).Value
	}

	// get delta, this is the number of steps divided by the BinNum, so full range can be achieved by BinNum number of deltas
	var delta float64
	if value == nil {
		if model.MultiDeltaRangeInstances.Contains(instance) {
			steps := (maxValue - minValue) / precision
			binNum := model.MultiDeltaRangeInstances.LookupBinNum(instance)
			delta = math.Round(steps/binNum) * precision
			if delta == 0 {
				delta = precision
			}
		} else {
			delta = precision
		}
	} else {
		delta = float64(value.NumValue)
	}

	if relativityType == common.Decrease {
		delta = -delta
	}

	newState := model.RangeCapabilityState{
		Instance: model.RangeCapabilityInstance(instance),
	}
	// IOT-303: old logic, relative flag is banned for retrievable:true devices
	if _, found := common.RelativeFlagNonSupportingSkills[device.SkillID]; found {
		newValue := currentValue + delta
		if newValue > maxValue {
			newValue = maxValue
		}
		if newValue < minValue {
			newValue = minValue
		}
		newState.Value = newValue
	} else {
		// IOT-303: new logic with relative flag in documentation
		newState.Value = delta
		newState.Relative = tools.AOB(true)
	}

	c.SetState(newState)
	return c, nil
}

func fillRangeFromNonRetrievableRelativeValueV2(c *model.RangeCapability, instance string, relativityType common.RelativityType, value *RangeValueSlot) *model.RangeCapability {
	var newValue float64
	switch {
	case value != nil:
		newValue = float64(value.NumValue)
	case model.RangeCapabilityInstance(instance) == model.VolumeRangeInstance:
		// https://st.yandex-team.ru/QUASAR-4167
		newValue = float64(3)
	default:
		newValue = float64(1)
	}

	if relativityType == common.Decrease {
		newValue = -newValue
	}

	c.SetState(model.RangeCapabilityState{
		Instance: model.RangeCapabilityInstance(instance),
		Relative: tools.AOB(true),
		Value:    newValue,
	})

	return c
}

func (f *ActionFrameV2) fillToggleCapability(c *model.ToggleCapability, device model.Device, intentParameters ActionIntentParametersSlot) *model.ToggleCapability {
	if intentParameters.RelativityType != "" {
		capability, _ := device.GetCapabilityByTypeAndInstance(model.ToggleCapabilityType, intentParameters.CapabilityInstance)
		if capability.State() == nil {
			capability.SetState(capability.DefaultState())
		}
		newValue := capability.State().(model.ToggleCapabilityState).Value
		switch intentParameters.RelativityType {
		case string(common.Invert):
			newValue = !capability.State().(model.ToggleCapabilityState).Value
		}
		c.SetState(model.ToggleCapabilityState{
			Instance: model.ToggleCapabilityInstance(intentParameters.CapabilityInstance),
			Value:    newValue,
		})
	} else {
		c.SetState(model.ToggleCapabilityState{
			Instance: model.ToggleCapabilityInstance(intentParameters.CapabilityInstance),
			Value:    f.ToggleValue.GetValue(),
		})
	}

	return c
}

func (f *ActionFrameV2) fillVideoStreamCapability(c *model.VideoStreamCapability, clientInfo common.ClientInfo) *model.VideoStreamCapability {
	c.SetState(model.VideoStreamCapabilityState{
		Instance: model.GetStreamCapabilityInstance,
		Value: model.VideoStreamCapabilityValue{
			Protocols: clientInfo.GetSupportedVideoStreamProtocols(),
		},
	})

	return c
}

func filterByRequiredDeviceType(devices model.Devices, deviceType RequiredDeviceTypesSlot) common.FrameFiltrationResult {
	filtrationResult := common.NewFrameFiltrationResult(devices)
	if len(devices) == 0 {
		return filtrationResult
	}

	filtered := make(model.Devices, 0, len(devices))
	for _, device := range devices {
		if slices.Contains(deviceType.DeviceTypes, string(device.Type)) {
			filtered = append(filtered, device)
		}
	}

	filtrationResult.SurvivedDevices = filtered
	if len(filtrationResult.SurvivedDevices) == 0 {
		filtrationResult.Reason = common.InappropriateCapabilityFiltrationReason
	}

	return filtrationResult
}

func chooseWinnerParameters(survivedParameters map[ActionIntentParametersSlot]bool) ActionIntentParametersSlot {
	var winner ActionIntentParametersSlot
	for parameters := range survivedParameters {
		if actionIntentParametersPriorities[model.CapabilityType(winner.CapabilityType)] < actionIntentParametersPriorities[model.CapabilityType(parameters.CapabilityType)] {
			winner = parameters
		}
	}

	return winner
}

var actionIntentParametersPriorities = map[model.CapabilityType]int{
	model.CustomButtonCapabilityType: 7,
	model.VideoStreamCapabilityType:  6,
	model.OnOffCapabilityType:        5,
	model.ToggleCapabilityType:       4,
	model.ColorSettingCapabilityType: 3,
	model.ModeCapabilityType:         2,
	model.RangeCapabilityType:        1,
}

func (f *ActionFrameV2) isActionIntentApplicable(device model.Device, intentParameters ActionIntentParametersSlot) bool {
	capabilityType := model.CapabilityType(intentParameters.CapabilityType)
	capabilityInstance := intentParameters.CapabilityInstance
	capability, ok := device.GetCapabilityByTypeAndInstance(capabilityType, capabilityInstance)

	// Mode intent parameters usually come from granet without instance
	if !ok && capabilityType == model.ModeCapabilityType {
		modeCapabilities := device.GetCapabilitiesByType(capabilityType)
		if len(modeCapabilities) == 0 {
			return false
		}
		capability = modeCapabilities[0]
		ok = true
	}

	if !ok {
		return false
	}

	if f.isActionApplicable(device, capability, intentParameters) {
		return true
	}
	return false
}

func (f *ActionFrameV2) isActionApplicable(d model.Device, capability model.ICapability, intentParameters ActionIntentParametersSlot) bool {
	switch capability.Type() {
	case model.OnOffCapabilityType:
		return isOnOffCapabilityApplicable(d.Type, f.OnOffValue.GetValue())
	case model.ColorSettingCapabilityType:
		return isColorSettingCapabilityApplicable(capability, f.ColorSettingValue, intentParameters)
	case model.RangeCapabilityType:
		return isRangeCapabilityApplicable(d, capability, f.RangeValue, common.RelativityType(intentParameters.RelativityType))
	case model.ModeCapabilityType:
		return isModeCapabilityApplicable(capability, f.ModeValue)
	default:
		return true
	}
}

func (f *ActionFrameV2) findExplicitlyNamed(devices model.Devices) (model.Devices, model.Devices) {
	frameDevices := f.Devices.DeviceIDs()
	frameDevicesMap := make(map[string]bool, len(frameDevices))
	for _, id := range frameDevices {
		frameDevicesMap[id] = true
	}

	frameGroups := f.Groups.GroupIDs()
	frameGroupsMap := make(map[string]bool, len(frameGroups))
	for _, id := range frameGroups {
		frameGroupsMap[id] = true
	}

	explicitlyNamedDevices := make(model.Devices, 0, len(devices))
	gatheredDevices := make(model.Devices, 0, len(devices))

	// All devices and groups explicitly named by the user must survive.
	for _, device := range devices {
		if frameDevicesMap[device.ID] || deviceHasGroup(device, frameGroupsMap) {
			explicitlyNamedDevices = append(explicitlyNamedDevices, device)
		} else {
			gatheredDevices = append(gatheredDevices, device)
		}
	}

	return explicitlyNamedDevices, gatheredDevices
}

func (f *ActionFrameV2) RequiredDeviceTypes() model.DeviceTypes {
	deviceTypes := make(model.DeviceTypes, 0, len(f.RequiredDeviceType.DeviceTypes))
	for _, dt := range f.RequiredDeviceType.DeviceTypes {
		deviceTypes = append(deviceTypes, model.DeviceType(dt))
	}
	return deviceTypes
}

func (f *ActionFrameV2) getWinnerCapabilityValue(intentParameters ActionIntentParametersSlot) sdk.GranetSlot {
	switch model.CapabilityType(intentParameters.CapabilityType) {
	case model.OnOffCapabilityType:
		return f.OnOffValue
	case model.ToggleCapabilityType:
		return f.ToggleValue
	case model.RangeCapabilityType:
		return f.RangeValue
	case model.ModeCapabilityType:
		return f.ModeValue
	case model.CustomButtonCapabilityType:
		return f.CustomButtonInstance
	case model.ColorSettingCapabilityType:
		return f.ColorSettingValue
	default:
		return nil
	}
}

func (f *ActionFrameV2) isIrrelevantShortCommand(devices model.Devices, intentParameters ActionIntentParametersSlot) bool {
	return len(f.Households.HouseholdIDs()) == 0 && len(f.Rooms.RoomIDs()) == 0 && // location is not specified
		len(devices.GroupByRoom()) > 1 && // survived devices are in different rooms
		intentParameters.IsShortOnOffCommand()
}

func (f *ActionFrameV2) isRequestToTandem(runContext sdk.RunContext) bool {
	if tandemDataSource, ok := runContext.Request().GetDataSources()[int32(commonpb.EDataSourceType_TANDEM_ENVIRONMENT_STATE)]; ok {
		if tandemState := tandemDataSource.GetTandemEnvironmentState(); tandemState != nil {
			for _, tandemGroup := range tandemState.GetGroups() {
				for _, tandemDevice := range tandemGroup.GetDevices() {
					if tandemDevice.GetId() == runContext.ClientDeviceID() {
						return true
					}
				}
			}
		}
	}
	return false
}

func isOnOffCapabilityApplicable(deviceType model.DeviceType, capabilityValue bool) bool {
	// we cannot turn irons on
	if deviceType == model.IronDeviceType && capabilityValue {
		return false
	}
	// we cannot unfeed the dog/cat
	if deviceType == model.PetFeederDeviceType && !capabilityValue {
		return false
	}

	return true
}

func isColorSettingCapabilityApplicable(capability model.ICapability, valueSlot *ColorSettingValueSlot, intentParameters ActionIntentParametersSlot) bool {
	params := capability.Parameters().(model.ColorSettingCapabilityParameters)

	// some lights are white-mode only
	if intentParameters.CapabilityInstance == model.HypothesisColorCapabilityInstance && params.ColorModel == nil {
		if valueSlot == nil {
			return true
		}
		// if color unknown or multicolor request on white-mode lamp
		if color, ok := model.ColorPalette.GetColorByID(valueSlot.Color); !ok || color.Type == model.Multicolor {
			return false
		}
	}
	if intentParameters.CapabilityInstance == string(model.TemperatureKCapabilityInstance) && params.TemperatureK == nil {
		return false
	}
	if intentParameters.CapabilityInstance == model.HypothesisColorSceneCapabilityInstance {
		if _, ok := params.GetAvailableScenes().AsMap()[valueSlot.ColorScene]; !ok {
			return false
		}
	}

	return true
}

func isRangeCapabilityApplicable(
	d model.Device,
	capability model.ICapability,
	valueSlot *RangeValueSlot,
	relativityType common.RelativityType,
) bool {
	params := capability.Parameters().(model.RangeCapabilityParameters)

	if params.Range == nil {
		// if we got an absolute value from mm, but we cannot set it
		if relativityType == "" && !params.RandomAccess {
			return false
		}
		// simple value type check
		if valueSlot != nil && valueSlot.SlotType == string(NumSlotType) {
			// relative values can be positive only
			if relativityType != "" && valueSlot.NumValue < 0 {
				return false
			}
			// Skip relative range action with value over 50 cause IR hub restrictions
			if capability.Instance() == string(model.ChannelRangeInstance) || capability.Instance() == string(model.VolumeRangeInstance) {
				if d.SkillID == model.TUYA && relativityType != "" && valueSlot.NumValue > 50 {
					return false
				}
			}
		}
	} else {
		// if value is an absolute value
		if valueSlot != nil && valueSlot.SlotType == string(NumSlotType) && relativityType == "" {
			// if random_access cannot be used
			if !params.RandomAccess {
				return false
			}
			// if requested value cannot be set
			if float64(valueSlot.NumValue) > params.Range.Max || float64(valueSlot.NumValue) < params.Range.Min {
				return false
			}
		}
	}

	return true
}

func isModeCapabilityApplicable(capability model.ICapability, value *ModeValueSlot) bool {
	params := capability.Parameters().(model.ModeCapabilityParameters)

	if !value.IsZero() {
		if _, ok := params.GetModesMap()[value.GetModeValue()]; !ok {
			return false
		}
	}

	return true
}

type ActionFrameExtractionResult struct {
	// Devices filtered by types, households, rooms and intent parameters that will be queried in apply.
	Devices model.Devices
	// IntentParametersSlot that must be used for sending actions to devices.
	IntentParametersSlot ActionIntentParametersSlot
	// CreatedTime is used in scenario launches for delayed actions.
	CreatedTime time.Time
	// RequestedTime is a time at which delayed actions must be performed.
	RequestedTime time.Time
	// ValueSlot contains value of winner capability.
	ValueSlot sdk.GranetSlot
	// IntervalEndTime is a time at which interval actions must be stopped.
	IntervalEndTime time.Time
	// OkExtractionStatus if extraction is successful and the result can be sent to apply
	Status     ExtractionStatus
	FailureNLG libnlg.NLG // we don't send nlg after extraction if it went fine
}

// mergeActionFrames stores slots from all frames in one frame. Duplicates are not filtered.
func mergeActionFrames(frames []ActionFrameV2) ActionFrameV2 {
	superFrame := ActionFrameV2{}
	for _, frame := range frames {
		superFrame.IntentParameters = append(superFrame.IntentParameters, frame.IntentParameters...)
		superFrame.Devices = append(superFrame.Devices, frame.Devices...)
		superFrame.Groups = append(superFrame.Groups, frame.Groups...)
		superFrame.Rooms = append(superFrame.Rooms, frame.Rooms...)
		superFrame.Households = append(superFrame.Households, frame.Households...)

		if superFrame.RangeValue.IsZero() {
			superFrame.RangeValue = frame.RangeValue
		}
		if superFrame.ColorSettingValue.IsZero() {
			superFrame.ColorSettingValue = frame.ColorSettingValue
		}
		if superFrame.ToggleValue.IsZero() {
			superFrame.ToggleValue = frame.ToggleValue
		}
		if superFrame.OnOffValue.IsZero() {
			superFrame.OnOffValue = frame.OnOffValue
		}
		if superFrame.ModeValue.IsZero() {
			superFrame.ModeValue = frame.ModeValue
		}
		if superFrame.CustomButtonInstance.IsZero() {
			superFrame.CustomButtonInstance = frame.CustomButtonInstance
		}
		if superFrame.RequiredDeviceType.IsZero() {
			superFrame.RequiredDeviceType = frame.RequiredDeviceType
		}
		if superFrame.AllDevicesRequired.IsZero() {
			superFrame.AllDevicesRequired = frame.AllDevicesRequired
		}
		if superFrame.DeviceType.IsZero() {
			superFrame.DeviceType = frame.DeviceType
		}
		if superFrame.ExactDate.IsZero() {
			superFrame.ExactDate = frame.ExactDate
		}
		if superFrame.ExactTime.IsZero() {
			superFrame.ExactTime = frame.ExactTime
		}
		if superFrame.RelativeDateTime.IsZero() {
			superFrame.RelativeDateTime = frame.RelativeDateTime
		}
		if superFrame.IntervalDateTime.IsZero() {
			superFrame.IntervalDateTime = frame.IntervalDateTime
		}
	}

	return superFrame
}

func devicesNotFoundActionNLG(deviceSlots DeviceSlots) libnlg.NLG {
	deviceTypes := deviceSlots.DeviceTypes()
	demoDevices := deviceSlots.DemoDevices()

	if totalLen := len(deviceTypes) + len(demoDevices); totalLen == 0 || totalLen > 1 {
		return nlg.CannotFindDevices
	}

	demoDeviceNames := make([]string, 0, len(demoDevices))
	for _, demoID := range demoDevices {
		demoDeviceNames = append(demoDeviceNames, DemoDeviceIDToName[strings.TrimPrefix(demoID, "demo--")])
	}

	// special NLGs for device types / demo devices
	switch {
	case slices.Contains(deviceTypes, string(model.LightDeviceType)):
		return nlg.CannotFindLightDevices
	case slices.Contains(deviceTypes, string(model.AcDeviceType)) || slices.Contains(demoDeviceNames, "кондиционер"):
		return nlg.CannotFindAC
	case slices.Contains(deviceTypes, string(model.FanDeviceType)) || slices.Contains(demoDeviceNames, "вентилятор"):
		return nlg.CannotFindFan
	case slices.Contains(demoDeviceNames, "обогреватель"):
		return nlg.CannotFindHeater
	case slices.Contains(deviceTypes, string(model.ReceiverDeviceType)) || slices.Contains(demoDeviceNames, "ресивер"):
		return nlg.CannotFindTVBox
	case slices.Contains(deviceTypes, string(model.TvDeviceDeviceType)) || slices.Contains(demoDeviceNames, "телевизор"):
		return nlg.CannotFindTV
	case slices.Contains(demoDeviceNames, "электрокамин"):
		return nlg.CannotFindFireplace
	}

	// common NLGs
	switch {
	case len(deviceTypes) == 0 && len(demoDeviceNames) == 1:
		return nlg.CannotFindDevice(demoDeviceNames[0])
	case len(deviceTypes) == 1 && len(demoDevices) == 0:
		return nlg.CannotFindRequestedDeviceType(DeviceTypeToName[deviceTypes[0]])
	default:
		return nlg.CannotFindDevices
	}
}
