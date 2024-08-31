package frames

import (
	"strings"

	"a.yandex-team.ru/alice/iot/bulbasaur/megamind/common"
	"a.yandex-team.ru/alice/iot/bulbasaur/megamind/sdk"
	"a.yandex-team.ru/alice/iot/bulbasaur/model"
	"a.yandex-team.ru/alice/iot/bulbasaur/nlg"
	libnlg "a.yandex-team.ru/alice/library/go/nlg"
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/xerrors"
)

var _ sdk.GranetFrame = &QueryFrame{}

type QueryFrame struct {
	// Frame having multiple intent parameters slots is possible. Filtration and validation of the frame is in query.Processor
	IntentParameters QueryIntentParametersSlots

	Devices    DeviceSlots
	Groups     GroupSlots
	Rooms      RoomSlots
	Households HouseholdSlots
}

func (q *QueryFrame) SupportedSlots() []sdk.GranetSlot {
	return []sdk.GranetSlot{
		&QueryIntentParametersSlot{},
		&DeviceSlot{},
		&GroupSlot{},
		&RoomSlot{},
		&HouseholdSlot{},
	}
}

func (q *QueryFrame) SetSlots(slots []sdk.GranetSlot) error {
	for _, slot := range slots {
		switch typedSlot := slot.(type) {
		case *QueryIntentParametersSlot:
			q.IntentParameters = append(q.IntentParameters, *typedSlot)
		case *DeviceSlot:
			q.Devices = append(q.Devices, *typedSlot)
		case *GroupSlot:
			q.Groups = append(q.Groups, *typedSlot)
		case *RoomSlot:
			q.Rooms = append(q.Rooms, *typedSlot)
		case *HouseholdSlot:
			q.Households = append(q.Households, *typedSlot)
		default:
			return xerrors.Errorf("unsupported slot: slot name: %q, slot type: %q", slot.Name(), slot.Type())
		}
	}
	return nil
}

func (q *QueryFrame) AppendSlots(slots ...sdk.GranetSlot) error {
	if len(slots) == 0 {
		return nil
	}

	intentParameters := make([]QueryIntentParametersSlot, 0)
	devices := make([]DeviceSlot, 0)
	groups := make([]GroupSlot, 0)
	rooms := make([]RoomSlot, 0)
	households := make([]HouseholdSlot, 0)

	for _, slot := range slots {
		switch typedSlot := slot.(type) {
		case *QueryIntentParametersSlot:
			intentParameters = append(intentParameters, *typedSlot)
		case *DeviceSlot:
			devices = append(devices, *typedSlot)
		case *GroupSlot:
			groups = append(groups, *typedSlot)
		case *RoomSlot:
			rooms = append(rooms, *typedSlot)
		case *HouseholdSlot:
			households = append(households, *typedSlot)
		default:
			return xerrors.Errorf("unsupported slot: slot name: %q, slot type: %q", slot.Name(), slot.Type())
		}
	}

	q.IntentParameters = append(q.IntentParameters, intentParameters...)
	q.Devices = append(q.Devices, devices...)
	q.Groups = append(q.Groups, groups...)
	q.Rooms = append(q.Rooms, rooms...)
	q.Households = append(q.Households, households...)

	return nil
}

func (q *QueryFrame) DropEqualIntentParameters() {
	filteredIntentParameters := make([]QueryIntentParametersSlot, 0, len(q.IntentParameters))

	seenIntentParameters := make(map[QueryIntentParametersSlot]bool)
	for _, slot := range q.IntentParameters {
		if !seenIntentParameters[slot] {
			filteredIntentParameters = append(filteredIntentParameters, slot)
			seenIntentParameters[slot] = true
		}
	}

	q.IntentParameters = filteredIntentParameters
}

// ExtractQueryIntent translates the frame into query intent – a unified description of what is requested from devices.
// It also performs filtration of user devices by requested rooms, households, device types and groups and some additional validation.
func (q *QueryFrame) ExtractQueryIntent(runContext sdk.RunContext) (QueryFrameExtractionResult, error) {
	// If the frame contains only demo rooms, we know for sure user requested a room he doesn't have.
	// In this case demo response is sent.
	if q.Rooms.ContainsOnlyDemo() {
		return onlyDemoRoomsExtractionResult(q.Rooms), nil
	}

	// Gather devices from requested devices, device types and groups
	gatheredDevices, extractionStatus := q.gatherDevices(runContext)
	if extractionStatus != OkExtractionStatus {
		return QueryFrameExtractionResult{
			Status:     DevicesNotFoundExtractionStatus,
			FailureNLG: devicesNotFoundNLG(q.Devices),
		}, nil
	}

	preparedIntentParameters, err := q.prepareIntentParameters(gatheredDevices)
	if err != nil {
		return QueryFrameExtractionResult{}, xerrors.Errorf("failed to prepare intent parameters: %w", err)
	}

	filtrationResult := q.filterDevices(gatheredDevices, preparedIntentParameters)
	if filtrationResult.Reason != common.AllGoodFiltrationReason {
		return QueryFrameExtractionResult{
			Status:     ExtractionStatus(filtrationResult.Reason),
			FailureNLG: filtrationResult.Reason.NLG(),
		}, nil
	}

	survivedDevices, postProcessStatus := postProcessSurvivedDevices(runContext, filtrationResult.SurvivedDevices)
	if postProcessStatus != OkExtractionStatus {
		return QueryFrameExtractionResult{
			Devices:          survivedDevices,
			IntentParameters: nil,
			Status:           postProcessStatus,
			FailureNLG:       nlg.NoHouseholdSpecifiedQuery,
		}, nil
	}

	return QueryFrameExtractionResult{
		Devices:          survivedDevices,
		IntentParameters: preparedIntentParameters,
		Status:           OkExtractionStatus,
	}, nil
}

// gatherDevices returns a slice containing devices:
//   1) explicitly named by the user,
//   2) deduced from device types, specified in the frame,
//   3) gathered from device groups, specified in the frame.
func (q *QueryFrame) gatherDevices(runContext sdk.RunContext) (model.Devices, ExtractionStatus) {
	userInfo, _ := runContext.UserInfo()

	// There might be no device slots in the frame. It is fine. For example: "Какая влажность в гостиной?"
	if len(q.Devices) == 0 && len(q.Groups) == 0 {
		return userInfo.Devices, OkExtractionStatus
	}

	gatheredDevices := userInfo.Devices.FilterByIDs(q.Devices.DeviceIDs())
	requestedDevicesByType := gatheredDevices.GroupByType()
	requestedDeviceTypes := q.Devices.DeviceTypes()
	userDevicesByType := userInfo.Devices.GroupByType()

	// Add user devices of the requested device types
	for _, requestedType := range requestedDeviceTypes {
		// If requested type is light and some light device is explicitly named by user, do not gather light devices
		// "Включи свет в торшере" -> {"floor-lamp-id"}, not {"floor-lamp-id", "table-lamp-id", ...}
		if requestedType == string(model.LightDeviceType) && len(requestedDevicesByType[model.LightDeviceType]) > 0 {
			continue
		}

		// If no households are specified in the frame and the current client is a smart speaker,
		// only gather devices from the client's household.
		if len(q.Households) == 0 && runContext.ClientInfo().IsSmartSpeaker() {
			if clientHouseholdID, _, ok := runContext.ClientInfo().GetIotLocation(userInfo); !ok {
				runContext.Logger().Infof("failed to deduce household from client info")
			} else {
				runContext.Logger().Info(
					"householdIDs slot has been filled with speaker's householdID",
					log.String("household_id", clientHouseholdID),
				)
				devicesInCurrentHousehold := userDevicesByType[model.DeviceType(requestedType)].FilterByHouseholdIDs([]string{clientHouseholdID})
				gatheredDevices = append(gatheredDevices, devicesInCurrentHousehold...)
				continue
			}
		}

		gatheredDevices = append(gatheredDevices, userDevicesByType[model.DeviceType(requestedType)]...)
	}

	// Add user devices from the requested groups
	requestedGroups := q.Groups.GroupIDs()
	userDevicesByGroup := userInfo.Devices.GroupByGroupID()
	for _, requestedGroup := range requestedGroups {
		gatheredDevices = append(gatheredDevices, userDevicesByGroup[requestedGroup]...)
	}

	// Remove duplicates
	gatheredDevices = gatheredDevices.ToMap().Flatten()

	if len(gatheredDevices) == 0 {
		return nil, DevicesNotFoundExtractionStatus
	}

	return gatheredDevices, OkExtractionStatus
}

// resolveConflictsInParameters resolves conflicts between query granet forms.
// Some capabilities and properties forms can react on the same phrases. In this case two semantic frames will come
// from megamind, but only one will be passed to the processor. We need to determine what user really meant.
// Conflicting entities:
//   1. model.TemperatureRangeInstance and model.TemperaturePropertyInstance
//   2. model.HumidityRangeInstance and model.HumidityPropertyInstance
// resolveConflictsInParameters takes in account what devices user named and what devices the user has
// and saves one of the conflicting entities that should be used.
func (q *QueryFrame) resolveConflictsInParameters(gatheredDevices model.Devices) (common.QueryIntentParametersSlice, error) {
	result := make(common.QueryIntentParametersSlice, 0, len(q.IntentParameters))

	for _, parametersSlot := range q.IntentParameters {
		// If parametersSlot doesn't contain conflicting entities, save it untouched.
		frameIntentParameters := parametersSlot.ToQueryIntentParameters()
		if !frameIntentParameters.ContainsConflictingTarget() {
			result = append(result, frameIntentParameters)
			continue
		}

		// Conflicting property and capability from the slot
		var (
			propertyInstance   model.PropertyInstance
			capabilityInstance model.RangeCapabilityInstance
		)

		switch {
		case frameIntentParameters.IsTemperatureCapabilityOrProperty():
			propertyInstance = model.TemperaturePropertyInstance
			capabilityInstance = model.TemperatureRangeInstance
		case frameIntentParameters.IsHumidityCapabilityOrProperty():
			propertyInstance = model.HumidityPropertyInstance
			capabilityInstance = model.HumidityRangeInstance
		default:
			return common.QueryIntentParametersSlice{}, xerrors.Errorf("unknown conflicting target in intent parameters")
		}

		// If there are no device or group slots in the frame, return property
		// Ex. "Какая влажность в зале?"
		if len(q.Devices)+len(q.Groups) == 0 {
			result = append(result, common.QueryIntentParameters{
				Target:           common.PropertyTarget,
				PropertyType:     string(model.FloatPropertyType),
				PropertyInstance: string(propertyInstance),
			})
			continue
		}

		// If there are device or group slots in the frame, then the corresponding devices are in gatheredDevices.
		// Try to find a device with the property.
		deviceWithPropertyFound := false
		for _, device := range gatheredDevices {
			if _, ok := device.GetPropertyByTypeAndInstance(model.FloatPropertyType, string(propertyInstance)); ok {
				// No need to find all devices with required property. They will be found during filtration.
				deviceWithPropertyFound = true
				break
			}
		}

		// If there is at least one such device, return property parameters.
		if deviceWithPropertyFound {
			result = append(result, common.QueryIntentParameters{
				Target:           common.PropertyTarget,
				PropertyType:     string(model.FloatPropertyType),
				PropertyInstance: string(propertyInstance),
			})
			continue
		}

		// If there are no devices with the property, use capability parameters.
		result = append(result, common.QueryIntentParameters{
			Target:             common.CapabilityTarget,
			CapabilityType:     string(model.RangeCapabilityType),
			CapabilityInstance: string(capabilityInstance),
		})
	}

	return result, nil
}

// filterDevices filters user devices by intent parameters, rooms and households, explicitly specified in the frame.
func (q *QueryFrame) filterDevices(devices model.Devices, intentParameters common.QueryIntentParametersSlice) common.FrameFiltrationResult {
	filtrationResult := common.NewFrameFiltrationResult(devices)

	filtrationResult.Merge(filterByQueryIntentParameters(filtrationResult.SurvivedDevices, intentParameters))
	filtrationResult.Merge(common.FilterByRooms(filtrationResult.SurvivedDevices, q.Rooms.RoomIDs()))
	filtrationResult.Merge(common.FilterByHouseholds(filtrationResult.SurvivedDevices, q.Households.HouseholdIDs()))

	return filtrationResult
}

// filterByQueryIntentParameters returns devices that have at least one capability or property from the intent parameters.
// If there is none, it returns InappropriateQueryIntentFiltrationReason.
func filterByQueryIntentParameters(devices model.Devices, intentParametersSlice common.QueryIntentParametersSlice) common.FrameFiltrationResult {
	survivedDevicesByID := make(model.DevicesMapByID)
	filtrationResult := common.NewFrameFiltrationResult(devices)
	if len(devices) == 0 {
		return filtrationResult
	}

	for _, device := range devices {
		for _, intentParameters := range intentParametersSlice {
			switch intentParameters.Target {
			case common.StateTarget:
				if device.CanPersistState() {
					survivedDevicesByID[device.ID] = device
				}
			case common.CapabilityTarget:
				capabilityType, capabilityInstance := model.CapabilityType(intentParameters.CapabilityType), intentParameters.CapabilityInstance
				capability, ok := device.GetCapabilityByTypeAndInstance(capabilityType, capabilityInstance)
				if capability.IsInternal() {
					continue
				}
				if ok && (capability.Retrievable() || capability.Reportable()) {
					survivedDevicesByID[device.ID] = device
				}
			case common.PropertyTarget:
				propertyType, propertyInstance := model.PropertyType(intentParameters.PropertyType), intentParameters.PropertyInstance
				if propertyType != model.FloatPropertyType {
					continue
				}
				property, ok := device.GetPropertyByTypeAndInstance(propertyType, propertyInstance)
				if ok && (property.Retrievable() || property.Reportable()) {
					survivedDevicesByID[device.ID] = device
				}
			}
		}
	}

	survivedDevices := survivedDevicesByID.Flatten()
	if len(survivedDevices) == 0 {
		filtrationResult.Reason = common.InappropriateQueryIntentFiltrationReason
	}
	filtrationResult.SurvivedDevices = survivedDevices
	return filtrationResult
}

// prepareIntentParameters performs preprocessing on intent parameters:
// resolves conflicts between some capabilities and properties and extends tvoc property.
func (q *QueryFrame) prepareIntentParameters(gatheredDevices model.Devices) (common.QueryIntentParametersSlice, error) {
	// resolve conflicts between capabilities and properties
	intentParameters, err := q.resolveConflictsInParameters(gatheredDevices)
	if err != nil {
		return nil, xerrors.Errorf("failed to resolve conflicts in intent parameters: %w", err)
	}

	return intentParameters.ExtendProperties(), nil
}

// postProcessSurvivedDevices tries to deduce household if there are devices from multiple households after filtration.
// It returns OkExtractionStatus if succeeds and MultipleHouseholdsExtractionStatus if fails.
func postProcessSurvivedDevices(runContext sdk.RunContext, devices model.Devices) (model.Devices, ExtractionStatus) {
	devicesByHousehold := devices.GroupByHousehold()
	if len(devicesByHousehold) <= 1 {
		return devices, OkExtractionStatus
	}

	userInfo, _ := runContext.UserInfo()

	switch {
	case runContext.ClientInfo().IsIotApp():
		// use current household id if the client is iot app: IOT-1339
		currentHouseholdDevices := devicesByHousehold[userInfo.CurrentHouseholdID]
		// if there are no suitable devices in the current household, ask user to specify it
		if len(currentHouseholdDevices) == 0 {
			return devices, MultipleHouseholdsExtractionStatus
		}
		return currentHouseholdDevices, OkExtractionStatus
	case runContext.ClientInfo().IsSmartSpeaker():
		// if the client is a speaker, use its household
		clientHouseholdID, _, ok := runContext.ClientInfo().GetIotLocation(userInfo)
		if !ok {
			runContext.Logger().Infof("failed to deduce household from client info")
			break
		} else {
			runContext.Logger().Info(
				"householdIDs slot has been filled with speaker's householdID",
				log.String("household_id", clientHouseholdID),
			)
		}
		speakerHouseholdDevices := devicesByHousehold[clientHouseholdID]
		// if there are no suitable devices in the speaker's household, ask user to specify it
		if len(speakerHouseholdDevices) == 0 {
			return devices, MultipleHouseholdsExtractionStatus
		}
		return speakerHouseholdDevices, OkExtractionStatus
	}

	return devices, MultipleHouseholdsExtractionStatus
}

type QueryFrameExtractionResult struct {
	// Devices filtered by types, households, rooms and intent parameters that will be queried in apply
	Devices          model.Devices
	IntentParameters common.QueryIntentParametersSlice
	Status           ExtractionStatus // OkExtractionStatus if extraction is successful and the result can be sent to apply
	FailureNLG       libnlg.NLG       // we don't send nlg after extraction if it went fine
}

// onlyDemoRoomsExtractionResult constructs QueryFrameExtractionResult with "ONLY_DEMO_ROOMS" status and appropriate NLG
func onlyDemoRoomsExtractionResult(rooms RoomSlots) QueryFrameExtractionResult {
	demoRoomIDs := make([]string, 0, len(rooms))
	for _, slot := range rooms {
		if slot.DemoRoomID != "" {
			demoRoomIDs = append(demoRoomIDs, slot.DemoRoomID)
		}
	}

	var roomNLG libnlg.NLG
	switch {
	case len(demoRoomIDs) > 1:
		roomNLG = nlg.CannotFindRooms
	case len(demoRoomIDs) == 1:
		roomNLG = nlg.CannotFindRequestedRoom(DemoRoomIDToName[demoRoomIDs[0]])
	default:
		// something's wrong: there must be at least one demo room in rooms
		roomNLG = nlg.CommonError
	}

	return QueryFrameExtractionResult{
		Status:     OnlyDemoRoomsExtractionStatus,
		FailureNLG: roomNLG,
	}
}

func devicesNotFoundNLG(deviceSlots DeviceSlots) libnlg.NLG {
	deviceTypes := deviceSlots.DeviceTypes()
	demoDevices := deviceSlots.DemoDevices()

	switch {
	case len(deviceTypes) == 0 && len(demoDevices) == 1:
		return nlg.CannotFindDevice(DemoDeviceIDToName[strings.TrimPrefix(demoDevices[0], "demo--")])
	case len(deviceTypes) == 1 && len(demoDevices) == 0:
		return nlg.CannotFindRequestedDeviceType(DeviceTypeToName[deviceTypes[0]])
	default:
		return nlg.CannotFindDevices
	}
}
