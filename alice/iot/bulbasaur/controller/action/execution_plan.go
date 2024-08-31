package action

import (
	"fmt"

	"a.yandex-team.ru/alice/iot/bulbasaur/dto/adapter"
	"a.yandex-team.ru/alice/iot/bulbasaur/model"
	"a.yandex-team.ru/alice/iot/bulbasaur/model/directives"
	"a.yandex-team.ru/alice/library/go/timestamp"
	"a.yandex-team.ru/alice/megamind/protos/common"
	scenariospb "a.yandex-team.ru/alice/megamind/protos/scenarios"
)

type executionPlan struct {
	userExecutionPlans map[uint64]userExecutionPlan
}

func emptyExecutionPlan() executionPlan {
	return executionPlan{
		make(map[uint64]userExecutionPlan),
	}
}

func emptyUserExecutionPlan(providerActionDevices model.ProviderDevicesMap) userExecutionPlan {
	return userExecutionPlan{
		providerExecutionPlans: make(map[string]providerExecutionPlan, len(providerActionDevices)),
	}
}

func (p *executionPlan) Directives() []*scenariospb.TDirective {
	result := make([]*scenariospb.TDirective, 0)
	for _, userExecutionPlan := range p.userExecutionPlans {
		for _, providerExecutionPlan := range userExecutionPlan.providerExecutionPlans {
			for _, deviceDirectives := range providerExecutionPlan.currentOriginDeviceDirectives {
				result = append(result, deviceDirectives...)
			}
		}
	}
	return result
}

func (p *userExecutionPlan) SetOriginActionsError(endpointID string, errorCode model.ErrorCode, message string) {
	for _, providerPlan := range p.providerExecutionPlans {
		for originEndpointID, batch := range providerPlan.otherOriginActions {
			if originEndpointID != endpointID {
				continue
			}
			for _, device := range batch {
				capabilityResultsMap := providerPlan.precomputedActionResults[device.GetDeviceId()]
				capabilityResultsMap.SetActionError(adapter.ErrorCode(errorCode), message)
				providerPlan.precomputedActionResults[device.GetDeviceId()] = capabilityResultsMap
			}
		}
	}
}

type deviceDirectivesMap map[string][]*scenariospb.TDirective

func (m deviceDirectivesMap) addDirectives(deviceID string, directives ...*scenariospb.TDirective) {
	m[deviceID] = append(m[deviceID], directives...)
}

type providerExecutionPlan struct {
	skillID       string
	actionDevices model.DevicesMapByID

	// protocolActionRequest is request to provider that complies with iot protocol
	protocolActionRequest *adapter.ActionRequest

	// currentOriginDeviceDirectives can hold actions to current speaker and children devices of that speaker
	currentOriginDeviceDirectives deviceDirectivesMap // deviceID -> directives
	// otherOriginActions hold actions that should be sent to other speaker devices
	otherOriginActions originActionsMap // endpointID -> TIoTDeviceActionsBatch

	// precomputedActionResults hold assumed results of currentOriginDeviceDirectives and computed results of otherOriginActions
	precomputedActionResults map[string]adapter.CapabilityActionResultsMap // deviceID -> capabilityKey -> carv
}

type originActionsMap map[string][]*common.TIoTDeviceActions

func (m originActionsMap) addOriginActions(endpointID string, actions *common.TIoTDeviceActions) {
	m[endpointID] = append(m[endpointID], actions)
}

func emptyProviderExecutionPlan(skillID string, actionDevices model.Devices) providerExecutionPlan {
	return providerExecutionPlan{
		skillID:                       skillID,
		actionDevices:                 actionDevices.ToMap(),
		protocolActionRequest:         nil,
		currentOriginDeviceDirectives: deviceDirectivesMap{},
		otherOriginActions:            map[string][]*common.TIoTDeviceActions{},
		precomputedActionResults:      map[string]adapter.CapabilityActionResultsMap{},
	}
}

func (p providerExecutionPlan) precomputedDeviceActionResults() deviceActionsResultMap {
	deviceResults := make(deviceActionsResultMap, len(p.precomputedActionResults))
	for deviceID, capabilityActionResults := range p.precomputedActionResults {
		actionDevice := p.actionDevices[deviceID]
		deviceActionsResult := deviceActionsResult{
			ActionDevice:            actionDevice,
			CapabilityActionResults: p.precomputedActionResults[deviceID],
			Status:                  capabilityActionResults.Status(),
			UpdatedCapabilities:     capabilityActionResults.UpdatedCapabilities(actionDevice).AsMap(),
		}
		deviceResults[deviceID] = deviceActionsResult
	}
	return deviceResults
}

type deviceActions struct {
	ID                string
	ExternalID        string
	SkillID           string
	CapabilityActions map[string]capabilityAction
}

func newDeviceActions(device model.Device) deviceActions {
	externalID, _ := device.GetExternalID()
	result := deviceActions{
		ID:                device.ID,
		ExternalID:        externalID,
		SkillID:           device.SkillID,
		CapabilityActions: make(map[string]capabilityAction, len(device.Capabilities)),
	}
	for _, childAction := range device.Capabilities {
		result.CapabilityActions[childAction.Key()] = capabilityAction{TargetState: childAction.State()}
	}
	return result
}

func (a *deviceActions) precomputeDirectives(ts timestamp.PastTimestamp) ([]*scenariospb.TDirective, map[string]adapter.CapabilityActionResultView) {
	result := make([]*scenariospb.TDirective, 0, len(a.CapabilityActions))
	actionResultsMap := make(adapter.CapabilityActionResultsMap, 0)
	for capabilityKey, action := range a.CapabilityActions {
		directive, err := directives.ConvertProtoActionToSpeechkitDirective(a.ExternalID, action.TargetState.ToIotCapabilityAction())
		if err == nil {
			stateActionResult := adapter.StateActionResult{Status: adapter.DONE}
			actionResultsMap[capabilityKey] = adapter.CapabilityActionResultView{
				Type: action.TargetState.Type(),
				State: adapter.CapabilityStateActionResultView{
					Instance:     action.TargetState.GetInstance(),
					ActionResult: stateActionResult,
				},
				Timestamp: ts,
			}
			result = append(result, directive.ToScenarioDirective())
		} else {
			stateActionResult := adapter.StateActionResult{
				Status:       adapter.ERROR,
				ErrorCode:    adapter.InternalError,
				ErrorMessage: err.Error(),
			}
			actionResultsMap[capabilityKey] = adapter.CapabilityActionResultView{
				Type: action.TargetState.Type(),
				State: adapter.CapabilityStateActionResultView{
					Instance:     action.TargetState.GetInstance(),
					ActionResult: stateActionResult,
				},
			}
		}
	}
	return result, actionResultsMap
}

func (a *deviceActions) precomputeOriginActions(ts timestamp.PastTimestamp) (*common.TIoTDeviceActions, map[string]adapter.CapabilityActionResultView) {
	deviceActions := &common.TIoTDeviceActions{
		DeviceId:         a.ID,
		ExternalDeviceId: a.ExternalID,
		SkillId:          a.SkillID,
	}
	actionResultsMap := map[string]adapter.CapabilityActionResultView{}
	for capabilityKey, action := range a.CapabilityActions {
		switch a.SkillID {
		case model.QUASAR:
			switch action.TargetState.Type() {
			case model.QuasarServerActionCapabilityType:
				deviceActions.Actions = append(deviceActions.Actions, action.TargetState.ToIotCapabilityAction())
				actionResultsMap[capabilityKey] = adapter.CapabilityActionResultView{
					Type: action.TargetState.Type(),
					State: adapter.CapabilityStateActionResultView{
						Instance:     action.TargetState.GetInstance(),
						ActionResult: adapter.StateActionResult{Status: adapter.DONE},
					},
					Timestamp: ts,
				}
			case model.QuasarCapabilityType:
				deviceActions.Actions = append(deviceActions.Actions, action.TargetState.ToIotCapabilityAction())
				stateActionResult := adapter.StateActionResult{Status: adapter.DONE}
				if action.TargetState.(model.QuasarCapabilityState).NeedCompletionCallback() {
					stateActionResult = adapter.StateActionResult{Status: adapter.INPROGRESS}
				}
				actionResultsMap[capabilityKey] = adapter.CapabilityActionResultView{
					Type: action.TargetState.Type(),
					State: adapter.CapabilityStateActionResultView{
						Instance:     action.TargetState.GetInstance(),
						ActionResult: stateActionResult,
					},
					Timestamp: ts,
				}
			case model.OnOffCapabilityType, model.ColorSettingCapabilityType:
				// https://st.yandex-team.ru/IOT-1352
				var colorSceneID model.ColorSceneID
				if onOffCapabilityState, ok := action.TargetState.(model.OnOffCapabilityState); ok {
					if onOffCapabilityState.Value {
						colorSceneID = model.ColorSceneIDNight
					} else {
						colorSceneID = model.ColorSceneIDInactive
					}
				} else if colorCapabilityState, ok := action.TargetState.(model.ColorSettingCapabilityState); ok {
					if scene, ok := colorCapabilityState.Value.(model.ColorSceneID); ok {
						colorSceneID = scene
					} else {
						actionResultsMap[capabilityKey] = adapter.CapabilityActionResultView{
							Type: action.TargetState.Type(),
							State: adapter.CapabilityStateActionResultView{
								Instance: action.TargetState.GetInstance(),
								ActionResult: adapter.StateActionResult{
									Status:       adapter.ERROR,
									ErrorCode:    adapter.ErrorCode(model.InvalidAction),
									ErrorMessage: fmt.Sprintf("unknown capability type: %q", action.TargetState.Type()),
								},
							},
							Timestamp: ts,
						}
						continue
					}
				}
				state := model.ColorSettingCapabilityState{Instance: model.SceneCapabilityInstance, Value: colorSceneID}
				deviceActions.Actions = append(deviceActions.Actions, state.ToIotCapabilityAction())
				actionResultsMap[capabilityKey] = adapter.CapabilityActionResultView{
					Type: action.TargetState.Type(),
					State: adapter.CapabilityStateActionResultView{
						Instance:     action.TargetState.GetInstance(),
						ActionResult: adapter.StateActionResult{Status: adapter.DONE},
					},
					Timestamp: ts,
				}
			}
		default:
			deviceActions.Actions = append(deviceActions.Actions, action.TargetState.ToIotCapabilityAction())
			actionResultsMap[capabilityKey] = adapter.CapabilityActionResultView{
				Type: action.TargetState.Type(),
				State: adapter.CapabilityStateActionResultView{
					Instance:     action.TargetState.GetInstance(),
					ActionResult: adapter.StateActionResult{Status: adapter.DONE},
				},
				Timestamp: ts,
			}
		}
	}
	return deviceActions, actionResultsMap
}

type capabilityAction struct {
	// note(galecore): someday this will turn into directives
	TargetState model.ICapabilityState
}

type userExecutionPlan struct {
	providerExecutionPlans map[string]providerExecutionPlan
}

func (p userExecutionPlan) OriginActions() map[string][]*common.TIoTDeviceActions {
	result := make(map[string][]*common.TIoTDeviceActions)
	for _, providerPlan := range p.providerExecutionPlans {
		for endpointID, batch := range providerPlan.otherOriginActions {
			result[endpointID] = append(result[endpointID], batch...)
		}
	}

	return result
}
