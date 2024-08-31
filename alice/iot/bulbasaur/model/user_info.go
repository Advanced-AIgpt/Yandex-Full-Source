package model

import (
	"context"
	"encoding/json"
	"fmt"
	"time"

	"a.yandex-team.ru/alice/iot/bulbasaur/model/quasar"
	"a.yandex-team.ru/alice/library/go/timestamp"
	"a.yandex-team.ru/alice/megamind/protos/common"
	"a.yandex-team.ru/alice/protos/data/location"
	"a.yandex-team.ru/library/go/core/xerrors"
	"a.yandex-team.ru/library/go/ptr"

	"github.com/mitchellh/mapstructure"
)

type UserInfo struct {
	Devices            Devices
	Groups             Groups
	Rooms              Rooms
	Scenarios          Scenarios
	Stereopairs        Stereopairs
	FavoriteRelations  FavoriteRelations
	Households         Households
	CurrentHouseholdID string
}

func NewEmptyUserInfo() UserInfo {
	return UserInfo{
		Devices:           Devices{},
		Groups:            Groups{},
		Rooms:             Rooms{},
		Scenarios:         Scenarios{},
		Stereopairs:       Stereopairs{},
		FavoriteRelations: NewEmptyFavoriteRelations(),
		Households:        Households{},
	}
}

func (i UserInfo) IsEmpty() bool {
	return i.CurrentHouseholdID == "" &&
		len(i.Households) == 0 &&
		len(i.Rooms) == 0 &&
		len(i.Groups) == 0 &&
		len(i.Devices) == 0 &&
		len(i.Scenarios) == 0 &&
		len(i.Stereopairs) == 0 &&
		i.FavoriteRelations.IsEmpty()
}

func (i *UserInfo) FromUserInfoProto(ctx context.Context, protoUserInfo *common.TIoTUserInfo) error {
	i.Devices = []Device{}
	i.Groups = []Group{}
	i.Rooms = []Room{}
	i.Scenarios = []Scenario{}
	i.Households = []Household{}
	i.CurrentHouseholdID = protoUserInfo.GetCurrentHouseholdId()
	i.Stereopairs = Stereopairs{}
	i.FavoriteRelations = NewEmptyFavoriteRelations() // todo(galecore): fill favorites from proto

	// make map id:room to fill devices later
	roomsMap := make(map[string]Room)
	for _, protoRoom := range protoUserInfo.GetRooms() {
		room := Room{
			ID:      protoRoom.GetId(),
			Name:    protoRoom.GetName(),
			Devices: []string{},
		}
		if protoRoom.GetHouseholdId() != "" {
			room.HouseholdID = protoRoom.GetHouseholdId()
		}
		if protoRoom.GetSharingInfo() != nil {
			var sharingInfo SharingInfo
			sharingInfo.fromUserInfoProto(protoRoom.GetSharingInfo())
			room.SharingInfo = &sharingInfo
		}
		roomsMap[room.ID] = room
	}
	groupsMap := make(map[string]Group)
	for _, protoGroup := range protoUserInfo.GetGroups() {
		group := Group{
			ID:      protoGroup.GetId(),
			Name:    protoGroup.GetName(),
			Aliases: protoGroup.GetAliases(),
			Devices: []string{},
			Type:    mmProtoToDeviceTypeMap[protoGroup.GetType()],
		}
		if protoGroup.GetHouseholdId() != "" {
			group.HouseholdID = protoGroup.GetHouseholdId()
		}
		if protoGroup.GetSharingInfo() != nil {
			var sharingInfo SharingInfo
			sharingInfo.fromUserInfoProto(protoGroup.GetSharingInfo())
			group.SharingInfo = &sharingInfo
		}
		groupsMap[group.ID] = group
	}

	// fill devices,
	for _, protoDevice := range protoUserInfo.GetDevices() {
		device := Device{
			ID:            protoDevice.GetId(),
			Name:          protoDevice.GetName(),
			Aliases:       protoDevice.GetAliases(),
			ExternalID:    protoDevice.GetExternalId(),
			ExternalName:  protoDevice.GetExternalName(),
			SkillID:       protoDevice.GetSkillId(),
			Groups:        []Group{},
			Type:          mmProtoToDeviceTypeMap[protoDevice.GetType()],
			OriginalType:  mmProtoToDeviceTypeMap[protoDevice.GetOriginalType()],
			Capabilities:  Capabilities{},
			Properties:    Properties{},
			Updated:       timestamp.PastTimestamp(protoDevice.GetUpdated()),
			Created:       timestamp.PastTimestamp(protoDevice.GetCreated()),
			Status:        mmProtoToDeviceStatusMap[protoDevice.GetStatus()],
			StatusUpdated: timestamp.PastTimestamp(protoDevice.GetStatusUpdated()),
		}

		// custom data
		// if we can't parse custom data that we marshalled, something funky is happening
		if protoCustomData := protoDevice.GetCustomData(); protoCustomData != nil {
			if err := json.Unmarshal(protoDevice.GetCustomData(), &device.CustomData); err != nil {
				return xerrors.Errorf("unable to parse device custom data: %w", err)
			}
		}

		// https://st.yandex-team.ru/IOT-1352
		var quasarInfo quasar.CustomData
		if err := mapstructure.Decode(device.CustomData, &quasarInfo); err == nil {
			if device.Type == LightDeviceType && quasarInfo.Platform == string(YandexStationMidiQuasarPlatform) {
				device.Type, device.OriginalType = YandexStationMidiDeviceType, YandexStationMidiDeviceType
			}
		}

		if protoDevice.GetHouseholdId() != "" {
			device.HouseholdID = protoDevice.GetHouseholdId()
		}

		// room
		if roomID := protoDevice.GetRoomId(); roomID != "" {
			if room, ok := roomsMap[roomID]; ok {
				room.Devices = append(room.Devices, device.ID)
				device.Room = &room
				roomsMap[roomID] = room
			}
		}

		// group
		for _, groupID := range protoDevice.GetGroupIds() {
			if group, ok := groupsMap[groupID]; ok {
				group.Devices = append(group.Devices, device.ID)
				device.Groups = append(device.Groups, group)
				groupsMap[groupID] = group
			}
		}

		// capabilities
		for _, protoCapability := range protoDevice.GetCapabilities() {
			if protoCapability.GetType() == common.TIoTUserInfo_TCapability_QuasarServerActionCapabilityType ||
				protoCapability.GetType() == common.TIoTUserInfo_TCapability_QuasarCapabilityType {
				continue
			}
			capability, err := MakeCapabilityFromUserInfoProto(protoCapability)
			if err != nil {
				return err
			}
			device.Capabilities = append(device.Capabilities, capability)
		}
		if device.Type.IsSmartSpeaker() {
			device.Capabilities = append(device.Capabilities, GenerateQuasarCapabilities(ctx, device.Type)...)
		}

		// properties
		for _, protoProperty := range protoDevice.GetProperties() {
			property, err := MakePropertyFromUserInfoProto(protoProperty)
			if err != nil {
				return err
			}
			device.Properties = append(device.Properties, property)
		}

		// deviceInfo
		if protoDeviceInfo := protoDevice.GetDeviceInfo(); protoDeviceInfo != nil {
			device.DeviceInfo = &DeviceInfo{}
			if manufacturer := protoDeviceInfo.GetManufacturer(); manufacturer != "" {
				device.DeviceInfo.Manufacturer = ptr.String(manufacturer)
			}
			if model := protoDeviceInfo.GetModel(); model != "" {
				device.DeviceInfo.Model = ptr.String(model)
			}
			if hwVersion := protoDeviceInfo.GetHwVersion(); hwVersion != "" {
				device.DeviceInfo.HwVersion = ptr.String(hwVersion)
			}
			if svVersion := protoDeviceInfo.GetSwVersion(); svVersion != "" {
				device.DeviceInfo.SwVersion = ptr.String(svVersion)
			}
		}

		// sharing info
		if protoSharing := protoDevice.GetSharingInfo(); protoSharing != nil {
			var sharingInfo SharingInfo
			sharingInfo.fromUserInfoProto(protoSharing)
			device.SharingInfo = &sharingInfo
		}

		i.Devices = append(i.Devices, device)
	}

	// fill rooms
	for _, room := range roomsMap {
		i.Rooms = append(i.Rooms, room)
	}

	// fill groups
	for _, group := range groupsMap {
		i.Groups = append(i.Groups, group)
	}

	// fill households
	for _, protoHousehold := range protoUserInfo.GetHouseholds() {
		household := Household{
			ID:   protoHousehold.GetId(),
			Name: protoHousehold.GetName(),
		}
		if protoHousehold.GetAddress() != "" {
			location := HouseholdLocation{
				Address:   protoHousehold.GetAddress(),
				Longitude: protoHousehold.GetLongitude(),
				Latitude:  protoHousehold.GetLatitude(),
			}
			household.Location = &location
		}
		if protoHousehold.GetSharingInfo() != nil {
			var sharingInfo SharingInfo
			sharingInfo.fromUserInfoProto(protoHousehold.GetSharingInfo())
			household.SharingInfo = &sharingInfo
		}
		i.Households = append(i.Households, household)
	}

	// fill scenarios as is
	for _, protoScenario := range protoUserInfo.GetScenarios() {
		scenario := Scenario{
			ID:           protoScenario.GetId(),
			Name:         ScenarioName(protoScenario.GetName()),
			Icon:         ScenarioIcon(protoScenario.GetIcon()),
			IsActive:     protoScenario.GetIsActive(),
			PushOnInvoke: protoScenario.GetPushOnInvoke(),
			Steps:        make(ScenarioSteps, 0), // for empty slice instead of nil while json serializer
		}

		// triggers
		for _, protoTrigger := range protoScenario.GetTriggers() {
			scenarioTrigger, err := MakeTriggerFromUserInfoProto(protoTrigger)
			if err != nil {
				return err
			}
			scenario.Triggers = append(scenario.Triggers, scenarioTrigger)
		}

		// devices
		for _, protoDevice := range protoScenario.GetDevices() {
			scenarioDevice := ScenarioDevice{ID: protoDevice.GetId()}
			for _, protoScenarioCapability := range protoDevice.GetCapabilities() {
				scenarioCapability, err := MakeScenarioCapabilityFromUserInfoProto(protoScenarioCapability)
				if err != nil {
					return err
				}
				scenarioDevice.Capabilities = append(scenarioDevice.Capabilities, scenarioCapability)
			}
			scenario.Devices = append(scenario.Devices, scenarioDevice)
		}

		// requestedSpeakerCapabilities
		for _, protoRequestedCapability := range protoScenario.GetRequestedSpeakerCapabilities() {
			scenarioCapability, err := MakeScenarioCapabilityFromUserInfoProto(protoRequestedCapability)
			if err != nil {
				return err
			}
			scenario.RequestedSpeakerCapabilities = append(scenario.RequestedSpeakerCapabilities, scenarioCapability)
		}

		// steps
		for _, protoStep := range protoScenario.GetSteps() {
			scenarioStep, err := MakeScenarioStepFromUserInfoProto(protoStep)
			if err != nil {
				return err
			}
			scenario.Steps = append(scenario.Steps, scenarioStep)
		}
		i.Scenarios = append(i.Scenarios, scenario)
	}

	// fill stereopairs
	for _, stereopair := range protoUserInfo.GetStereopairs() {
		i.Stereopairs = append(i.Stereopairs, MakeStereopairFromUserInfoProto(stereopair))
	}

	return nil
}

func MakeTriggerFromUserInfoProto(trigger *common.TIoTUserInfo_TScenario_TTrigger) (ScenarioTrigger, error) {
	switch trigger.GetType() {
	case common.TIoTUserInfo_TScenario_TTrigger_VoiceScenarioTriggerType:
		return VoiceScenarioTrigger{
			Phrase: trigger.GetVoiceTriggerPhrase(),
		}, nil
	case common.TIoTUserInfo_TScenario_TTrigger_TimerScenarioTriggerType:
		return TimerScenarioTrigger{
			Time: timestamp.PastTimestamp(trigger.GetTimerTriggerTimestamp()),
		}, nil
	case common.TIoTUserInfo_TScenario_TTrigger_TimetableScenarioTriggerType:
		protoWeekdays := trigger.GetTimetable().GetWeekdays()
		weekdays := make([]time.Weekday, 0, len(protoWeekdays))
		for _, protoWeekday := range protoWeekdays {
			weekdays = append(weekdays, time.Weekday(protoWeekday))
		}
		// ToDO: implement from user proto
		return TimetableScenarioTrigger{
			Condition: SpecificTimeCondition{
				TimeOffset: timestamp.PastTimestamp(trigger.GetTimetable().GetTimeOffset()),
				Weekdays:   weekdays,
			},
		}, nil
	case common.TIoTUserInfo_TScenario_TTrigger_DevicePropertyScenarioTriggerType:
		pTrigger := trigger.GetDeviceProperty()
		return &DevicePropertyScenarioTrigger{
			DeviceID:     pTrigger.GetDeviceID(),
			PropertyType: PropertyType(pTrigger.GetPropertyType()),
			Instance:     pTrigger.GetInstance(),
			Condition:    MakeDeviceTriggerConditionFromUserInfoProto(pTrigger),
		}, nil
	default:
		return nil, xerrors.Errorf("unknown proto trigger type: %s", trigger.GetType().String())
	}
}

func MakeDeviceTriggerConditionFromUserInfoProto(trigger *common.TIoTUserInfo_TScenario_TTrigger_TDeviceProperty) PropertyTriggerCondition {
	switch trigger.ConditionType {
	case common.TIoTUserInfo_TScenario_TTrigger_TDeviceProperty_FloatPropertyConditionType:
		cond := trigger.GetFloatPropertyCondition()
		var res = FloatPropertyCondition{}
		if cond.GetLowerBound() != nil {
			res.LowerBound = ptr.Float64(cond.GetLowerBound().GetValue())
		}
		if cond.GetUpperBound() != nil {
			res.UpperBound = ptr.Float64(cond.GetUpperBound().GetValue())
		}
		return &res
	case common.TIoTUserInfo_TScenario_TTrigger_TDeviceProperty_EventPropertyConditionType:
		cond := trigger.GetEventPropertyCondition()
		var res EventPropertyCondition
		res.Values = make([]EventValue, len(cond.Values))
		for i := range cond.Values {
			res.Values[i] = EventValue(cond.Values[i])
		}
		return &res
	default:
		panic(fmt.Sprintf("Unknown trigger condition type: %q", trigger.ConditionType.String()))
	}
}

func MakeScenarioCapabilityFromUserInfoProto(capability *common.TIoTUserInfo_TScenario_TCapability) (ScenarioCapability, error) {
	cType, err := MakeCapabilityTypeFromUserInfoProto(capability.GetType())
	if err != nil {
		return ScenarioCapability{}, err
	}
	cState, err := MakeCapabilityStateFromUserInfoProto(capability)
	if err != nil {
		return ScenarioCapability{}, err
	}
	return ScenarioCapability{
		Type:  cType,
		State: cState,
	}, nil
}

func MakePropertyFromUserInfoProto(property *common.TIoTUserInfo_TProperty) (IProperty, error) {
	switch property.GetType() {
	case common.TIoTUserInfo_TProperty_FloatPropertyType:
		result := &FloatProperty{
			reportable:  property.GetReportable(),
			retrievable: property.GetRetrievable(),
			parameters: FloatPropertyParameters{
				Instance: PropertyInstance(property.GetFloatPropertyParameters().GetInstance()),
				Unit:     Unit(property.GetFloatPropertyParameters().GetUnit()),
			},
			lastUpdated: timestamp.PastTimestamp(property.GetLastUpdated()),
		}
		if property.GetFloatPropertyState() != nil {
			result.state = &FloatPropertyState{
				Instance: PropertyInstance(property.GetFloatPropertyState().GetInstance()),
				Value:    property.GetFloatPropertyState().GetValue(),
			}
		}
		return result, nil
	case common.TIoTUserInfo_TProperty_EventPropertyType:
		parameters := property.GetEventPropertyParameters()
		state := property.GetEventPropertyState()

		events := make([]Event, 0, len(parameters.GetEvents()))
		for _, event := range parameters.GetEvents() {
			events = append(events, Event{
				Value: EventValue(event.GetValue()),
				Name:  ptr.String(event.GetName()),
			})
		}

		result := &EventProperty{
			reportable:  property.GetReportable(),
			retrievable: property.GetRetrievable(),
			parameters: EventPropertyParameters{
				Instance: PropertyInstance(parameters.GetInstance()),
				Events:   events,
			},
			lastUpdated: timestamp.PastTimestamp(property.GetLastUpdated()),
		}
		if property.GetEventPropertyState() != nil {
			result.state = &EventPropertyState{
				Instance: PropertyInstance(state.GetInstance()),
				Value:    EventValue(state.GetValue()),
			}
		}
		return result, nil
	default:
		return nil, xerrors.Errorf("unknown proto property type: %s", property.GetType().String())
	}
}

func MakeCapabilityFromUserInfoProto(protoCapability *common.TIoTUserInfo_TCapability) (ICapability, error) {
	cType, err := MakeCapabilityTypeFromUserInfoProto(protoCapability.GetType())
	if err != nil {
		return nil, err
	}
	cParameters, err := MakeCapabilityParametersFromUserInfoProto(protoCapability)
	if err != nil {
		return nil, err
	}
	cState, err := MakeCapabilityStateFromUserInfoProto(protoCapability)
	if err != nil {
		return nil, err
	}
	capability := MakeCapabilityByType(cType)
	capability.SetParameters(cParameters)
	capability.SetState(cState)
	capability.SetReportable(protoCapability.GetReportable())
	capability.SetRetrievable(protoCapability.GetRetrievable())
	capability.SetLastUpdated(timestamp.PastTimestamp(protoCapability.GetLastUpdated()))
	return capability, nil
}

type protoUserInfoStateHolder interface {
	GetType() common.TIoTUserInfo_TCapability_ECapabilityType
	GetOnOffCapabilityState() *common.TIoTUserInfo_TCapability_TOnOffCapabilityState
	GetColorSettingCapabilityState() *common.TIoTUserInfo_TCapability_TColorSettingCapabilityState
	GetModeCapabilityState() *common.TIoTUserInfo_TCapability_TModeCapabilityState
	GetRangeCapabilityState() *common.TIoTUserInfo_TCapability_TRangeCapabilityState
	GetToggleCapabilityState() *common.TIoTUserInfo_TCapability_TToggleCapabilityState
	GetCustomButtonCapabilityState() *common.TIoTUserInfo_TCapability_TCustomButtonCapabilityState
	GetQuasarServerActionCapabilityState() *common.TIoTUserInfo_TCapability_TQuasarServerActionCapabilityState
	GetQuasarCapabilityState() *common.TIoTUserInfo_TCapability_TQuasarCapabilityState
	GetVideoStreamCapabilityState() *common.TIoTUserInfo_TCapability_TVideoStreamCapabilityState
}

func MakeCapabilityTypeFromUserInfoProto(protoType common.TIoTUserInfo_TCapability_ECapabilityType) (CapabilityType, error) {
	switch protoType {
	case common.TIoTUserInfo_TCapability_OnOffCapabilityType:
		return OnOffCapabilityType, nil
	case common.TIoTUserInfo_TCapability_ColorSettingCapabilityType:
		return ColorSettingCapabilityType, nil
	case common.TIoTUserInfo_TCapability_ModeCapabilityType:
		return ModeCapabilityType, nil
	case common.TIoTUserInfo_TCapability_RangeCapabilityType:
		return RangeCapabilityType, nil
	case common.TIoTUserInfo_TCapability_ToggleCapabilityType:
		return ToggleCapabilityType, nil
	case common.TIoTUserInfo_TCapability_QuasarServerActionCapabilityType:
		return QuasarServerActionCapabilityType, nil
	case common.TIoTUserInfo_TCapability_CustomButtonCapabilityType:
		return CustomButtonCapabilityType, nil
	case common.TIoTUserInfo_TCapability_QuasarCapabilityType:
		return QuasarCapabilityType, nil
	case common.TIoTUserInfo_TCapability_VideoStreamCapabilityType:
		return VideoStreamCapabilityType, nil
	default:
		return "", xerrors.Errorf("unknown proto capability type: %s", protoType.String())
	}
}

// TODO: someday switch-case statements like this one will die https://st.yandex-team.ru/IOT-1241
func MakeCapabilityParametersFromUserInfoProto(protoCapability *common.TIoTUserInfo_TCapability) (ICapabilityParameters, error) {
	switch protoCapability.GetType() {
	case common.TIoTUserInfo_TCapability_OnOffCapabilityType:
		return OnOffCapabilityParameters{
			Split: protoCapability.GetOnOffCapabilityParameters().GetSplit(),
		}, nil
	case common.TIoTUserInfo_TCapability_ColorSettingCapabilityType:
		parameters := ColorSettingCapabilityParameters{}
		protoParameters := protoCapability.GetColorSettingCapabilityParameters()
		if protoColorModel := protoParameters.GetColorModel(); protoColorModel != nil {
			protoColorModelType := protoColorModel.GetType()
			switch protoColorModelType {
			case common.TIoTUserInfo_TCapability_TColorSettingCapabilityParameters_TColorModel_HsvColorModel:
				parameters.ColorModel = CM(HsvModelType)
			case common.TIoTUserInfo_TCapability_TColorSettingCapabilityParameters_TColorModel_RgbColorModel:
				parameters.ColorModel = CM(RgbModelType)
			default:
				return nil, xerrors.Errorf("unknown color model type: %s", protoColorModelType.String())
			}
		}
		if protoTemperatureK := protoParameters.GetTemperatureK(); protoTemperatureK != nil {
			parameters.TemperatureK = &TemperatureKParameters{
				Min: TemperatureK(protoTemperatureK.GetMin()),
				Max: TemperatureK(protoTemperatureK.GetMax()),
			}
		}
		if protoSceneParameters := protoParameters.GetColorSceneParameters(); protoSceneParameters != nil {
			protoScenes := protoSceneParameters.GetScenes()
			scenes := make([]ColorScene, 0, len(protoScenes))
			for _, protoScene := range protoScenes {
				scenes = append(scenes, ColorScene{
					ID:   ColorSceneID(protoScene.GetID()),
					Name: protoScene.GetName(),
				})
			}
			parameters.ColorSceneParameters = &ColorSceneParameters{
				Scenes: scenes,
			}
		}
		return parameters, nil
	case common.TIoTUserInfo_TCapability_ModeCapabilityType:
		protoModes := protoCapability.GetModeCapabilityParameters().GetModes()
		modes := make([]Mode, 0, len(protoModes))
		for _, protoMode := range protoModes {
			modes = append(modes, Mode{
				Value: ModeValue(protoMode.GetValue()),
				//Name:  ptr.String(protoMode.GetName()),
			})
		}
		return ModeCapabilityParameters{
			Instance: ModeCapabilityInstance(protoCapability.GetModeCapabilityParameters().GetInstance()),
			Modes:    modes,
		}, nil
	case common.TIoTUserInfo_TCapability_RangeCapabilityType:
		parameters := RangeCapabilityParameters{
			Instance:     RangeCapabilityInstance(protoCapability.GetRangeCapabilityParameters().GetInstance()),
			Unit:         Unit(protoCapability.GetRangeCapabilityParameters().GetUnit()),
			RandomAccess: protoCapability.GetRangeCapabilityParameters().GetRandomAccess(),
			Looped:       protoCapability.GetRangeCapabilityParameters().GetLooped(),
		}
		if protoRange := protoCapability.GetRangeCapabilityParameters().GetRange(); protoRange != nil {
			parameters.Range = &Range{
				Min:       protoRange.GetMin(),
				Max:       protoRange.GetMax(),
				Precision: protoRange.GetPrecision(),
			}
		}
		return parameters, nil
	case common.TIoTUserInfo_TCapability_ToggleCapabilityType:
		return ToggleCapabilityParameters{
			Instance: ToggleCapabilityInstance(protoCapability.GetToggleCapabilityParameters().GetInstance()),
		}, nil
	case common.TIoTUserInfo_TCapability_QuasarServerActionCapabilityType:
		return QuasarServerActionCapabilityParameters{
			Instance: QuasarServerActionCapabilityInstance(protoCapability.GetQuasarServerActionCapabilityParameters().GetInstance()),
		}, nil
	case common.TIoTUserInfo_TCapability_CustomButtonCapabilityType:
		return CustomButtonCapabilityParameters{
			Instance:      CustomButtonCapabilityInstance(protoCapability.GetCustomButtonCapabilityParameters().GetInstance()),
			InstanceNames: protoCapability.GetCustomButtonCapabilityParameters().GetInstanceNames(),
		}, nil
	case common.TIoTUserInfo_TCapability_QuasarCapabilityType:
		return QuasarCapabilityParameters{
			Instance: QuasarCapabilityInstance(protoCapability.GetQuasarCapabilityParameters().GetInstance()),
		}, nil
	case common.TIoTUserInfo_TCapability_VideoStreamCapabilityType:
		var params VideoStreamCapabilityParameters
		params.FromUserInfoProto(protoCapability.GetVideoStreamCapabilityParameters())
		return params, nil
	default:
		return nil, xerrors.Errorf("unknown proto capability type: %s", protoCapability.GetType().String())
	}
}

func MakeCapabilityStateFromUserInfoProto(stateHolder protoUserInfoStateHolder) (ICapabilityState, error) {
	switch stateHolder.GetType() {
	case common.TIoTUserInfo_TCapability_OnOffCapabilityType:
		if stateHolder.GetOnOffCapabilityState() == nil {
			return nil, nil
		}
		state := OnOffCapabilityState{
			Instance: OnOffCapabilityInstance(stateHolder.GetOnOffCapabilityState().GetInstance()),
			Value:    stateHolder.GetOnOffCapabilityState().GetValue(),
		}
		if relative := stateHolder.GetOnOffCapabilityState().GetRelative(); relative != nil {
			state.Relative = ptr.Bool(relative.GetIsRelative())
		}
		return state, nil
	case common.TIoTUserInfo_TCapability_ColorSettingCapabilityType:
		if stateHolder.GetColorSettingCapabilityState() == nil {
			return nil, nil
		}
		state := ColorSettingCapabilityState{
			Instance: ColorSettingCapabilityInstance(stateHolder.GetColorSettingCapabilityState().GetInstance()),
		}
		switch state.Instance {
		case TemperatureKCapabilityInstance:
			state.Value = TemperatureK(stateHolder.GetColorSettingCapabilityState().GetTemperatureK())
		case HsvColorCapabilityInstance:
			protoHSV := stateHolder.GetColorSettingCapabilityState().GetHSV()
			state.Value = HSV{
				H: int(protoHSV.GetH()),
				S: int(protoHSV.GetS()),
				V: int(protoHSV.GetV()),
			}
		case RgbColorCapabilityInstance:
			state.Value = RGB(stateHolder.GetColorSettingCapabilityState().GetRGB())
		case SceneCapabilityInstance:
			state.Value = ColorSceneID(stateHolder.GetColorSettingCapabilityState().GetColorSceneID())
		}
		return state, nil
	case common.TIoTUserInfo_TCapability_ModeCapabilityType:
		if stateHolder.GetModeCapabilityState() == nil {
			return nil, nil
		}
		return ModeCapabilityState{
			Instance: ModeCapabilityInstance(stateHolder.GetModeCapabilityState().GetInstance()),
			Value:    ModeValue(stateHolder.GetModeCapabilityState().GetValue()),
		}, nil
	case common.TIoTUserInfo_TCapability_RangeCapabilityType:
		if stateHolder.GetRangeCapabilityState() == nil {
			return nil, nil
		}
		return RangeCapabilityState{
			Instance: RangeCapabilityInstance(stateHolder.GetRangeCapabilityState().GetInstance()),
			Value:    stateHolder.GetRangeCapabilityState().GetValue(),
			Relative: ptr.Bool(stateHolder.GetRangeCapabilityState().GetRelative().GetIsRelative()),
		}, nil
	case common.TIoTUserInfo_TCapability_ToggleCapabilityType:
		if stateHolder.GetToggleCapabilityState() == nil {
			return nil, nil
		}
		return ToggleCapabilityState{
			Instance: ToggleCapabilityInstance(stateHolder.GetToggleCapabilityState().GetInstance()),
			Value:    stateHolder.GetToggleCapabilityState().GetValue(),
		}, nil
	case common.TIoTUserInfo_TCapability_QuasarServerActionCapabilityType:
		if stateHolder.GetQuasarServerActionCapabilityState() == nil {
			return nil, nil
		}
		return QuasarServerActionCapabilityState{
			Instance: QuasarServerActionCapabilityInstance(stateHolder.GetQuasarServerActionCapabilityState().GetInstance()),
			Value:    stateHolder.GetQuasarServerActionCapabilityState().GetValue(),
		}, nil
	case common.TIoTUserInfo_TCapability_CustomButtonCapabilityType:
		if stateHolder.GetCustomButtonCapabilityState() == nil {
			return nil, nil
		}
		return CustomButtonCapabilityState{
			Instance: CustomButtonCapabilityInstance(stateHolder.GetCustomButtonCapabilityState().GetInstance()),
			Value:    stateHolder.GetCustomButtonCapabilityState().GetValue(),
		}, nil
	case common.TIoTUserInfo_TCapability_QuasarCapabilityType:
		if stateHolder.GetQuasarCapabilityState() == nil {
			return nil, nil
		}
		value, err := MakeQuasarCapabilityValueFromProto(stateHolder.GetQuasarCapabilityState())
		if err != nil {
			return nil, xerrors.Errorf("failed to quasar capability value: %w", err)
		}
		return QuasarCapabilityState{
			Instance: QuasarCapabilityInstance(stateHolder.GetQuasarCapabilityState().GetInstance()),
			Value:    value,
		}, nil
	case common.TIoTUserInfo_TCapability_VideoStreamCapabilityType:
		if stateHolder.GetVideoStreamCapabilityState() == nil {
			return nil, nil
		}
		var state VideoStreamCapabilityState
		state.FromUserInfoProto(stateHolder.GetVideoStreamCapabilityState())
		return state, nil
	default:
		return nil, xerrors.Errorf("unknown proto capability type: %s", stateHolder.GetType().String())
	}
}

func MakeQuasarCapabilityValueFromProto(state *common.TIoTUserInfo_TCapability_TQuasarCapabilityState) (QuasarCapabilityValue, error) {
	switch QuasarCapabilityInstance(state.GetInstance()) {
	case WeatherCapabilityInstance:
		if state.GetWeatherValue() == nil {
			return nil, nil
		}
		var weatherValue WeatherQuasarCapabilityValue
		weatherValue.fromUserInfoProto(state.GetWeatherValue())
		return weatherValue, nil
	case VolumeCapabilityInstance:
		if state.GetVolumeValue() == nil {
			return nil, nil
		}
		var volumeValue VolumeQuasarCapabilityValue
		volumeValue.fromUserInfoProto(state.GetVolumeValue())
		return volumeValue, nil
	case MusicPlayCapabilityInstance:
		if state.GetMusicPlayValue() == nil {
			return nil, nil
		}
		var musicPlayValue MusicPlayQuasarCapabilityValue
		musicPlayValue.fromUserInfoProto(state.GetMusicPlayValue())
		return musicPlayValue, nil
	case NewsCapabilityInstance:
		if state.GetNewsValue() == nil {
			return nil, nil
		}
		var newsValue NewsQuasarCapabilityValue
		newsValue.fromUserInfoProto(state.GetNewsValue())
		return newsValue, nil
	case SoundPlayCapabilityInstance:
		if state.GetSoundPlayValue() == nil {
			return nil, nil
		}
		var soundPlayValue SoundPlayQuasarCapabilityValue
		soundPlayValue.fromUserInfoProto(state.GetSoundPlayValue())
		return soundPlayValue, nil
	case StopEverythingCapabilityInstance:
		if state.GetStopEverythingValue() == nil {
			return nil, nil
		}
		var stopEverythingValue StopEverythingQuasarCapabilityValue
		stopEverythingValue.fromUserInfoProto(state.GetStopEverythingValue())
		return stopEverythingValue, nil
	case TTSCapabilityInstance:
		if state.GetTtsValue() == nil {
			return nil, nil
		}
		var ttsValue TTSQuasarCapabilityValue
		ttsValue.fromUserInfoProto(state.GetTtsValue())
		return ttsValue, nil
	case AliceShowCapabilityInstance:
		if state.GetAliceShowValue() == nil {
			return nil, nil
		}
		var aliceShowValue AliceShowQuasarCapabilityValue
		aliceShowValue.fromUserInfoProto(state.GetAliceShowValue())
		return aliceShowValue, nil
	default:
		return nil, xerrors.Errorf("unknown quasar capability instance: %q", state.GetInstance())
	}
}

func MakeScenarioStepFromUserInfoProto(p *common.TIoTUserInfo_TScenario_TStep) (IScenarioStep, error) {
	switch p.GetType() {
	case common.TIoTUserInfo_TScenario_TStep_ActionsScenarioStepType:
		var params ScenarioStepActionsParameters
		if err := params.FromUserInfoProto(p.GetScenarioStepActionsParameters()); err != nil {
			return nil, err
		}
		return &ScenarioStepActions{parameters: params}, nil
	case common.TIoTUserInfo_TScenario_TStep_DelayScenarioStepType:
		var params ScenarioStepDelayParameters
		if err := params.FromUserInfoProto(p.GetScenarioStepDelayParameters()); err != nil {
			return nil, err
		}
		return &ScenarioStepDelay{parameters: params}, nil
	default:
		return nil, xerrors.Errorf("unknown proto scenario step type: %s", p.GetType())
	}
}

func (i UserInfo) ToUserInfoProto(ctx context.Context) *common.TIoTUserInfo {
	type protoColor struct {
		ID   ColorID
		Name string
	}
	colorSet := make(map[protoColor]struct{})

	userInfo := &common.TIoTUserInfo{
		Rooms:              []*location.TUserRoom{},
		Groups:             []*location.TUserGroup{},
		Devices:            []*common.TIoTUserInfo_TDevice{},
		Scenarios:          []*common.TIoTUserInfo_TScenario{},
		Colors:             []*common.TIoTUserInfo_TColor{},
		Households:         []*common.TIoTUserInfo_THousehold{},
		CurrentHouseholdId: i.CurrentHouseholdID,
	}
	// todo(galecore): marshal favorites to user info protos

	// iterate over rooms
	for _, room := range i.Rooms {
		userInfo.Rooms = append(userInfo.Rooms, room.ToUserInfoProto())
	}
	// iterate over groups
	for _, group := range i.Groups {
		userInfo.Groups = append(userInfo.Groups, group.ToUserInfoProto())
	}
	// iterate over devices
	for _, device := range i.Devices {
		for _, c := range device.AvailableColors() {
			colorSet[protoColor{ID: c.ID, Name: c.Name}] = struct{}{}
			for _, colorAlias := range ColorIDToAdditionalAliases[c.ID] {
				colorSet[protoColor{ID: c.ID, Name: colorAlias}] = struct{}{}
			}
		}
		userInfo.Devices = append(userInfo.Devices, device.ToUserInfoProto(ctx))
	}
	// iterate over scenarios
	for _, scenario := range i.Scenarios {
		if !scenario.IsActive {
			continue
		}
		userInfo.Scenarios = append(userInfo.Scenarios, scenario.ToUserInfoProto())
	}
	// iterate over colors
	for color := range colorSet {
		userInfo.Colors = append(userInfo.Colors, &common.TIoTUserInfo_TColor{
			Id:   color.ID.String(),
			Name: color.Name,
		})
	}
	// iterate over households
	for _, household := range i.Households {
		userInfo.Households = append(userInfo.Households, household.ToUserInfoProto())
	}
	return userInfo
}

func (i UserInfo) Favorites() Favorites {
	devicesMap := i.Devices.ToMap()
	favoriteDeviceProperties := make(FavoritesDeviceProperties, 0, len(i.FavoriteRelations.FavoriteDevicePropertyKeys))
	for favoriteDevicePropertyKey := range i.FavoriteRelations.FavoriteDevicePropertyKeys {
		device, exist := devicesMap[favoriteDevicePropertyKey.DeviceID]
		if !exist {
			continue
		}
		propertyType, instance := PropertyTypeInstanceFromKey(favoriteDevicePropertyKey.PropertyKey)
		property, exist := device.GetPropertyByTypeAndInstance(propertyType, instance)
		if !exist {
			continue
		}
		favoriteDeviceProperty := FavoritesDeviceProperty{DeviceID: device.ID, Property: property}
		favoriteDeviceProperties = append(favoriteDeviceProperties, favoriteDeviceProperty)
	}
	return Favorites{
		Scenarios:  i.Scenarios.FilterByFavorite(true),
		Devices:    i.Devices.FilterByFavorite(true),
		Groups:     i.Groups.FilterByFavorite(true),
		Properties: favoriteDeviceProperties,
	}
}

func (i *UserInfo) Merge(other UserInfo) {
	i.FavoriteRelations = i.FavoriteRelations.Merge(other.FavoriteRelations)
	i.Devices = append(i.Devices, other.Devices...)
	i.Groups = append(i.Groups, other.Groups...)
	i.Rooms = append(i.Rooms, other.Rooms...)
	i.Scenarios = append(i.Scenarios, other.Scenarios...)
	i.Stereopairs = append(i.Stereopairs, other.Stereopairs...)
	i.Households = append(i.Households, other.Households...)
}
