package frames

import (
	"time"

	"a.yandex-team.ru/alice/iot/bulbasaur/megamind/arguments"
	"a.yandex-team.ru/alice/iot/bulbasaur/megamind/common"
	"a.yandex-team.ru/alice/iot/bulbasaur/model"
	"a.yandex-team.ru/alice/library/go/libmegamind"
	"a.yandex-team.ru/alice/library/go/timestamp"
	"a.yandex-team.ru/alice/library/go/tools"
	megamindcommonpb "a.yandex-team.ru/alice/megamind/protos/common"
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/log/ctxlog"
	"a.yandex-team.ru/library/go/core/xerrors"
)

var _ common.FrameWithGranetSlots = &ActionFrame{}

// ActionFrame contains all possible slots that can come from iot.action.* granets.
// Feel free to add new fields for entities not used before.
type ActionFrame struct {
	IntentParameters common.ActionIntentParameters

	DeviceIDs    []string
	RoomIDs      []string
	GroupIDs     []string
	HouseholdIDs []string

	DeviceTypes []string

	Date *common.BegemotDate
	Time *common.BegemotTime

	ParsedTime    time.Time
	SemanticFrame libmegamind.SemanticFrame
}

// NewActionFrameWithDeduction is a constructor that does all sorts of wonderful things.
// First, it constructs ActionFrame from libmegamind.SemanticFrame.
// Also, it populates frame Devices from device types and groups.
// Finally, it extends the frame from specified slots.
func NewActionFrameWithDeduction(processorContext common.RunProcessorContext, logger log.Logger, specifiedSlots ...libmegamind.Slot) (*ActionFrame, error) {
	frame := processorContext.SemanticFrame
	if frame.Frame == nil {
		return nil, xerrors.New("semantic frame is nil")
	}

	actionFrame := ActionFrame{
		IntentParameters: common.ActionIntentParameters{},
	}
	actionFrame.SemanticFrame = frame

	if frame.Frame.GetTypedSemanticFrame() != nil {
		if err := actionFrame.populateFromTypedSemanticFrame(frame.Frame.GetTypedSemanticFrame()); err != nil {
			return nil, err
		}
	} else {
		if err := common.PopulateFromSemanticFrame(&actionFrame, frame); err != nil {
			return nil, err
		}
	}

	if err := actionFrame.populateFromRequestContext(processorContext); err != nil {
		return nil, xerrors.Errorf("failed to populate from request context: %w", err)
	}

	if err := actionFrame.extendFromSpecifiedSlots(specifiedSlots...); err != nil {
		return nil, xerrors.Errorf("failed to extend frame from specified slots: %w", err)
	}

	actionFrame.gatherDevices(processorContext, logger)
	return &actionFrame, nil
}

func (f *ActionFrame) ExtractActionIntent(processorContext common.RunProcessorContext) (arguments.ExtractedActionIntent, common.FrameFiltrationResult, error) {
	var extractedActionIntent arguments.ExtractedActionIntent

	timestamper, err := timestamp.TimestamperFromContext(processorContext.Context)
	if err != nil {
		return extractedActionIntent, common.FrameFiltrationResult{}, xerrors.New("failed to get timestamper from the context")
	}
	extractedActionIntent.CreatedTime = processorContext.ClientInfo.LocalTime(timestamper.CreatedTimestamp().AsTime())
	extractedActionIntent.RequestedTime = f.ParsedTime
	userDevices := processorContext.UserInfo.Devices

	filtrationResult := f.filterDevicesByActionIntent(userDevices, processorContext.ClientInfo)

	updatedDevices, err := updateDevicesFromIntentParameters(filtrationResult.SurvivedDevices, f.IntentParameters)
	if err != nil {
		return extractedActionIntent, filtrationResult,
			xerrors.Errorf("failed to update devices from intent parameters: %w", err)
	}
	extractedActionIntent.Devices = updatedDevices

	return extractedActionIntent, filtrationResult, nil
}

// ValidateBegemotDateTime performs validation of the begemot datetime that must happen before parsing it to time.Time
func (f ActionFrame) ValidateBegemotDateTime() error {
	if !f.Date.IsZero() && f.Time.IsZero() {
		return common.TimeIsNotSpecifiedValidationError
	}
	if !f.Time.IsZero() && f.Time.IsRelative() && !f.Date.IsZero() && !f.Date.IsRelative() {
		return common.WeirdTimeRelativityValidationError
	}

	return nil
}

// ValidateParsedDateTime performs validation of the parsed datetime
func (f ActionFrame) ValidateParsedDateTime(clientTime time.Time) error {
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

func (f ActionFrame) ValidateHouseholds(userInfo model.UserInfo) error {
	if len(f.HouseholdIDs) > 1 {
		return common.MultipleHouseholdsInRequestValidationError
	}
	if len(f.HouseholdIDs) == 1 {
		return nil
	}

	frameDevices := userInfo.Devices.FilterByIDs(f.DeviceIDs)
	devicesByHousehold := frameDevices.GroupByHousehold()
	if len(devicesByHousehold) > 1 {
		return common.MultipleSuitableHouseholdsValidationError
	}

	return nil
}

func (f *ActionFrame) SetParsedTime(t time.Time) {
	f.ParsedTime = t
}

// extendFromSpecifiedSlots appends data from time specify frame and household specify info
func (f *ActionFrame) extendFromSpecifiedSlots(specifiedSlots ...libmegamind.Slot) error {
	for _, slot := range specifiedSlots {
		switch common.SlotName(slot.Name) {
		case common.HouseholdSlotName:
			f.HouseholdIDs = append(f.HouseholdIDs, slot.Value)
		case common.TimeSlotName:
			var begemotTime common.BegemotTime
			if err := begemotTime.FromValueString(slot.Value); err != nil {
				return xerrors.Errorf("failed to parse specified time value: %q, error: %w", slot.Value, err)
			}
			f.Time = &begemotTime
		}
		f.SemanticFrame.AddSlots(slot)
	}
	return nil
}

func (f *ActionFrame) householdIDFromClientInfo(userInfo model.UserInfo, clientInfo common.ClientInfo) (string, error) {
	if device, found := userInfo.Devices.GetDeviceByQuasarExtID(clientInfo.DeviceID); found {
		return device.HouseholdID, nil
	}
	return "", xerrors.Errorf("client device not found in user devices, deviceID: %s", clientInfo.DeviceID)
}

// gatherDevices fills DeviceIDs with all devices that can be deduced from device types, groups and devices specified in the frame
func (f *ActionFrame) gatherDevices(processorContext common.RunProcessorContext, logger log.Logger) {
	userDevicesByID := processorContext.UserInfo.Devices.ToMap()
	userGroupsByID := processorContext.UserInfo.Groups.ToMap()
	groupDeviceIDs := f.gatherDeviceIDsFromFrameGroups(userDevicesByID, userGroupsByID)

	// if the client is a speaker and the household is not specified in the frame,
	// only devices in the same household with the speaker are gathered from frame device types
	var clientHouseholdID string
	if len(f.HouseholdIDs) == 0 {
		var err error
		clientHouseholdID, err = f.householdIDFromClientInfo(processorContext.UserInfo, processorContext.ClientInfo)
		if err != nil {
			ctxlog.Debugf(processorContext.Context, logger, "failed to deduce household from client info: %v", err)
		} else {
			ctxlog.Info(processorContext.Context, logger, "householdIDs slot has been filled with speaker's householdId",
				log.String("household_id", clientHouseholdID))
		}
	}

	deviceTypeDeviceIDs := f.gatherDeviceIDsFromFrameDeviceTypes(processorContext.UserInfo.Devices, processorContext.ClientInfo, clientHouseholdID)
	f.DeviceIDs = append(f.DeviceIDs, groupDeviceIDs...)
	f.DeviceIDs = append(f.DeviceIDs, deviceTypeDeviceIDs...)
	f.DeviceIDs = tools.RemoveDuplicates(f.DeviceIDs)
}

// gatherDeviceIDsFromFrameDeviceTypes finds user devices of types specified in frame with some additional logic.
// If all the following conditions are fulfilled,
// then only the devices from the same room as the smart speaker will be taken.
//   1. The request comes from a smart speaker;
//   2. The household is taken from speaker's clientInfo;
//   3. The room is not specified;
//   4. There is a device of the type in the room with the speaker
func (f *ActionFrame) gatherDeviceIDsFromFrameDeviceTypes(userDevices model.Devices, clientInfo common.ClientInfo, householdIDFromClientInfo string) (deviceIDs []string) {
	userDevicesByType := userDevices.GroupByType()
	clientDevice, clientDeviceFound := userDevices.GetDeviceByExtID(clientInfo.DeviceID)
	deviceInTheSpeakerRoomFound := false
	filledDevices := make([]string, 0, len(f.DeviceIDs))

	for _, deviceType := range f.DeviceTypes {
		// Pet feeder devices should not be gathered by type if at least one pet feeder is requested explicitly by its id
		// TODO(aaulayev): Think on how to write this better
		if !devicesShouldBeGatheredByType(deviceType, f.DeviceIDs, userDevices) {
			continue
		}

		devicesOfType, ok := userDevicesByType[model.DeviceType(deviceType)]
		if !ok {
			continue
		}

		for _, device := range devicesOfType {
			filledDevices = append(filledDevices, device.ID)
			if clientDeviceFound && clientDevice.RoomID() == device.RoomID() {
				deviceInTheSpeakerRoomFound = true
			}
		}
	}

	shouldFilterDevicesByRoom := clientInfo.IsSmartSpeaker() && householdIDFromClientInfo != "" &&
		len(f.RoomIDs) == 0 && deviceInTheSpeakerRoomFound

	userDevicesByID := userDevices.ToMap()
	for _, filledDeviceID := range filledDevices {
		device := userDevicesByID[filledDeviceID]
		if shouldFilterDevicesByRoom && device.RoomID() != clientDevice.RoomID() {
			continue
		}

		deviceIDs = append(deviceIDs, filledDeviceID)
	}

	return
}

// TODO(aaulayev): get rid of this later
func devicesShouldBeGatheredByType(deviceType string, frameDeviceIDs []string, userDevices model.Devices) bool {
	// I'll fix this later, I promise
	if (deviceType == string(model.PetFeederDeviceType) || deviceType == string(model.CameraDeviceType)) && len(frameDeviceIDs) > 0 {
		if petFeederDevices, ok := userDevices.GroupByType()[model.DeviceType(deviceType)]; ok {
			if len(petFeederDevices.FilterByIDs(frameDeviceIDs)) > 0 {
				return false
			}
		}
	}
	return true
}

func (f *ActionFrame) gatherDeviceIDsFromFrameGroups(userDevices model.DevicesMapByID, userGroups map[string]model.Group) (deviceIDs []string) {
	for _, groupID := range f.GroupIDs {
		group, ok := userGroups[groupID]
		if !ok {
			continue
		}

		for _, deviceID := range group.Devices {
			if _, ok = userDevices[deviceID]; ok {
				deviceIDs = append(deviceIDs, deviceID)
			}
		}
	}

	return
}

func (f *ActionFrame) filterDevicesByActionIntent(userDevices model.Devices, _ common.ClientInfo) common.FrameFiltrationResult {
	filtrationResult := common.NewFrameFiltrationResult(userDevices)

	filtrationResult.Merge(common.FilterByIDs(filtrationResult.SurvivedDevices, f.DeviceIDs))
	filtrationResult.Merge(common.FilterByHouseholds(filtrationResult.SurvivedDevices, f.HouseholdIDs))
	filtrationResult.Merge(common.FilterByRooms(filtrationResult.SurvivedDevices, f.RoomIDs))
	filtrationResult.Merge(common.FilterByActionIntentParameters(filtrationResult.SurvivedDevices, f.IntentParameters))
	// TODO: remove after https://st.yandex-team.ru/IOT-1276 continues
	//filtrationResult.Merge(common.FilterByTandem(filtrationResult.SurvivedDevices, clientInfo.IsTandem()))

	return filtrationResult
}

func (f *ActionFrame) populateFromRequestContext(processorContext common.RunProcessorContext) error {
	if f.IntentParameters.CapabilityType == model.VideoStreamCapabilityType && f.IntentParameters.CapabilityInstance == string(model.GetStreamCapabilityInstance) {
		f.IntentParameters.CapabilityValue = model.VideoStreamCapabilityValue{
			Protocols: processorContext.ClientInfo.GetSupportedVideoStreamProtocols(),
		}
	}

	return nil
}

func (f *ActionFrame) populateFromTypedSemanticFrame(typedSemanticFrame *megamindcommonpb.TTypedSemanticFrame) error {
	deviceActionFrame := typedSemanticFrame.GetIoTDeviceActionSemanticFrame()
	if deviceActionFrame == nil {
		return xerrors.New("failed to get device action frame from")
	}

	request := deviceActionFrame.GetRequest().GetRequestValue()
	if request == nil {
		return xerrors.New("failed to get request value from device action frame")
	}

	intentParameters := common.ActionIntentParameters{}
	if err := intentParameters.FromProto(request.GetIntentParameters()); err != nil {
		return xerrors.Errorf("failed to parse intent parameters from proto: %w", err)
	}

	f.IntentParameters = intentParameters

	f.RoomIDs = append(f.RoomIDs, request.GetRoomIDs()...)
	f.HouseholdIDs = append(f.HouseholdIDs, request.GetHouseholdIDs()...)
	f.GroupIDs = append(f.GroupIDs, request.GetGroupIDs()...)
	f.DeviceIDs = append(f.DeviceIDs, request.GetDeviceIDs()...)
	f.DeviceTypes = append(f.DeviceTypes, request.GetDeviceTypes()...)

	f.Time = common.NewBegemotTimeFromTimeStamp(timestamp.PastTimestamp(request.GetAtTimestamp()))
	f.Date = common.NewBegemotDateFromTimeStamp(timestamp.PastTimestamp(request.GetAtTimestamp()))

	return nil
}

func (f *ActionFrame) SupportsTimeSpecification() bool {
	if _, ok := SupportsTimeSpecification[libmegamind.SemanticFrameName(f.SemanticFrame.Name())]; ok {
		return true
	}
	return false
}

func (f *ActionFrame) PopulateFromGranetSlots(slots common.GranetSlots) {
	f.IntentParameters = slots.ActionIntentParameters
	f.DeviceIDs = slots.DeviceIDs
	f.RoomIDs = slots.RoomIDs
	f.GroupIDs = slots.GroupIDs
	f.HouseholdIDs = slots.HouseholdIDs

	f.DeviceTypes = slots.DeviceTypes

	f.Date = slots.Date
	f.Time = slots.Time
}

func updateDevicesFromIntentParameters(devices model.Devices, intentParameters common.ActionIntentParameters) (model.Devices, error) {
	updatedDevices := make(model.Devices, 0, len(devices))
	for _, device := range devices {
		var updatedDevice model.Device
		capability, err := common.CapabilityFromIntentParameters(device, intentParameters)
		if err != nil {
			return updatedDevices, err
		}
		updatedDevice.PopulateAsStateContainer(device, model.Capabilities{capability})
		updatedDevices = append(updatedDevices, updatedDevice)
	}

	return updatedDevices, nil
}

type ActionFrameBuilder struct {
	actionFrame ActionFrame
}

func (f *ActionFrameBuilder) Build() ActionFrame {
	return f.actionFrame
}

func (f *ActionFrameBuilder) WithIntentParameters(intentParameters common.ActionIntentParameters) *ActionFrameBuilder {
	f.actionFrame.IntentParameters = intentParameters
	return f
}

func (f *ActionFrameBuilder) WithDeviceIDs(deviceIDs ...string) *ActionFrameBuilder {
	f.actionFrame.DeviceIDs = append(f.actionFrame.DeviceIDs, deviceIDs...)
	return f
}

func (f *ActionFrameBuilder) WithRoomIDs(roomIDs ...string) *ActionFrameBuilder {
	f.actionFrame.RoomIDs = append(f.actionFrame.RoomIDs, roomIDs...)
	return f
}

func (f *ActionFrameBuilder) WithGroupIDs(groupIDs ...string) *ActionFrameBuilder {
	f.actionFrame.GroupIDs = append(f.actionFrame.GroupIDs, groupIDs...)
	return f
}

func (f *ActionFrameBuilder) WithHouseholdIDs(householdIDs ...string) *ActionFrameBuilder {
	f.actionFrame.HouseholdIDs = append(f.actionFrame.HouseholdIDs, householdIDs...)
	return f
}

func (f *ActionFrameBuilder) WithDeviceTypes(deviceTypes ...string) *ActionFrameBuilder {
	f.actionFrame.DeviceTypes = append(f.actionFrame.DeviceTypes, deviceTypes...)
	return f
}

func (f *ActionFrameBuilder) WithDate(begemotDate common.BegemotDate) *ActionFrameBuilder {
	f.actionFrame.Date = &begemotDate
	return f
}

func (f *ActionFrameBuilder) WithTime(begemotTime common.BegemotTime) *ActionFrameBuilder {
	f.actionFrame.Time = &begemotTime
	return f
}

func (f *ActionFrameBuilder) WithParsedTime(parsedTime time.Time) *ActionFrameBuilder {
	f.actionFrame.ParsedTime = parsedTime
	return f
}

func (f *ActionFrameBuilder) WithSemanticFrame(semanticFrame libmegamind.SemanticFrame) *ActionFrameBuilder {
	f.actionFrame.SemanticFrame = semanticFrame
	return f
}
