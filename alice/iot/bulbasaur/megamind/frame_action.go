package megamind

import (
	"fmt"
	"strconv"

	"google.golang.org/protobuf/types/known/structpb"

	"a.yandex-team.ru/alice/iot/bulbasaur/megamind/frames"
	"a.yandex-team.ru/alice/iot/bulbasaur/model"
	"a.yandex-team.ru/alice/library/go/inflector"
	"a.yandex-team.ru/alice/library/go/libmegamind"
)

const (
	SpecifyTimeFrame                        = "alice.iot.scenarios_timespecify"
	SpecifyTimeCallbackName                 = "time_specify_scenario"
	SpecifyHouseholdFrame                   = "alice.iot.household_specify"
	SpecifyHouseholdCallbackName            = "household_specify"
	ScenarioLaunchIDPayloadFieldName        = "scenario_launch_id"
	ScenarioLaunchStepIndexPayloadFieldName = "scenario_launch_step_index"
	DeviceActionResultFieldName             = "device_action_result"
	HouseholdIDPayloadFieldName             = "household_id"
)

func GetTimeSpecifyEmptyCallback() libmegamind.CallbackFrameAction {
	return libmegamind.CallbackFrameAction{
		FrameSlug:       SpecifyTimeFrame,
		FrameName:       SpecifyTimeFrame,
		CallbackName:    SpecifyTimeCallbackName,
		Phrases:         []string{},
		CallbackPayload: &structpb.Struct{Fields: map[string]*structpb.Value{}},
	}
}

func GetCancelScenarioLaunchCallback(scenarioLaunchID string) libmegamind.CallbackFrameAction {
	return libmegamind.CallbackFrameAction{
		FrameSlug:    frames.CancelOnetimeScenarioFrame,
		FrameName:    frames.CancelOnetimeScenarioFrame,
		CallbackName: frames.CancelScenarioCallbackName,
		CallbackPayload: &structpb.Struct{
			Fields: map[string]*structpb.Value{
				ScenarioLaunchIDPayloadFieldName: {
					Kind: &structpb.Value_StringValue{
						StringValue: scenarioLaunchID,
					},
				},
			},
		},
	}
}

func GetHouseholdSpecifyCallbacks(households []model.Household, inf inflector.IInflector) []libmegamind.CallbackFrameAction {
	result := make([]libmegamind.CallbackFrameAction, 0, len(households))
	for _, household := range households {
		nameInflection := inflector.TryInflect(inf, household.Name, inflector.GrammaticalCases)
		result = append(result, libmegamind.CallbackFrameAction{
			FrameSlug:    fmt.Sprintf("%s_%s", SpecifyHouseholdFrame, household.ID),
			FrameName:    fmt.Sprintf("%s_%s", SpecifyHouseholdFrame, household.ID),
			CallbackName: libmegamind.CallbackName(fmt.Sprintf("%s_%s", SpecifyHouseholdCallbackName, household.ID)),
			Phrases: []string{
				nameInflection.Im,
				fmt.Sprintf("В %s", nameInflection.Pr),
				fmt.Sprintf("На %s", nameInflection.Pr),
				nameInflection.Vin,
				fmt.Sprintf("В %s", nameInflection.Im),
				fmt.Sprintf("На %s", nameInflection.Im),
			},
			CallbackPayload: &structpb.Struct{
				Fields: map[string]*structpb.Value{
					HouseholdIDPayloadFieldName: {
						Kind: &structpb.Value_StringValue{
							StringValue: household.ID,
						},
					},
				},
			},
		})
	}
	return result
}

func GetScenarioLaunchIDFromPayload(callbackPayload *structpb.Struct) string {
	return callbackPayload.Fields[ScenarioLaunchIDPayloadFieldName].GetStringValue()
}

func GetHouseholdIDFromPayload(callbackPayload *structpb.Struct) string {
	return callbackPayload.Fields[HouseholdIDPayloadFieldName].GetStringValue()
}

func GetScenarioLaunchStepIndexFromPayload(callbackPayload *structpb.Struct) uint32 {
	stringified := callbackPayload.Fields[ScenarioLaunchStepIndexPayloadFieldName].GetStringValue()
	stepIndex, _ := strconv.ParseUint(stringified, 10, 64)
	return uint32(stepIndex)
}

func GetDeviceActionResultFromPayload(callbackPayload *structpb.Struct) model.ScenarioLaunchDeviceActionStatus {
	return model.ScenarioLaunchDeviceActionStatus(callbackPayload.Fields[DeviceActionResultFieldName].GetStringValue())
}
