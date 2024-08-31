package action

import (
	"context"

	"a.yandex-team.ru/alice/iot/bulbasaur/dto/adapter"
	"a.yandex-team.ru/alice/iot/bulbasaur/model"
	"a.yandex-team.ru/alice/library/go/timestamp"
)

func computeExecutionPlan(ctx context.Context, origin model.Origin, actionDevices model.Devices) executionPlan {
	devicesByOrigin := actionDevices.ToDevicesByOwnerIDMap(origin)
	executionPlan := emptyExecutionPlan()
	for ownerID, devices := range devicesByOrigin {
		sharedOrigin := origin.ToSharedOrigin(ownerID)
		executionPlan.userExecutionPlans[ownerID] = computeOriginExecutionPlan(ctx, sharedOrigin, devices)
	}
	return executionPlan
}

func computeOriginExecutionPlan(ctx context.Context, origin model.Origin, actionDevices model.Devices) userExecutionPlan {
	if origin.ScenarioLaunchInfo != nil {
		return computeScenarioUserExecutionPlan(ctx, origin, actionDevices)
	}
	providerActionDevices := actionDevices.GroupByProvider()
	originExecutionPlan := emptyUserExecutionPlan(providerActionDevices)
	for skillID, actionDevices := range providerActionDevices {
		providerExecutionPlan := emptyProviderExecutionPlan(skillID, actionDevices)
		switch skillID {
		case model.YANDEXIO:
			nonDirectiveActionDevices := make(model.Devices, 0, len(actionDevices))

			parentToChildEndpointsMap := model.YandexIODevices(actionDevices).ParentToChildEndpointsMap()
			for parentEndpointID, children := range parentToChildEndpointsMap {
				if origin.IsSpeakerDeviceOrigin(parentEndpointID) {
					for _, childActions := range children {
						deviceActions := newDeviceActions(childActions)
						directives, actionResultsMap := deviceActions.precomputeDirectives(timestamp.CurrentTimestampCtx(ctx))
						providerExecutionPlan.currentOriginDeviceDirectives.addDirectives(deviceActions.ID, directives...)
						providerExecutionPlan.precomputedActionResults[deviceActions.ID] = actionResultsMap
					}
				} else {
					// todo: this should become
					//   providerExecutionPlan.otherOriginActions[parentEndpointID] = make(map[string]deviceActions, len(children))
					//   for _, childActions := range children {
					//   	actions := newDeviceActions(childActions)
					//   	providerExecutionPlan.otherOriginActions[parentEndpointID][actions.ID] = actions
					//   }
					// but processor for this new mighty tsf is not ready yet, so we use current provider hacks with old tsf
					nonDirectiveActionDevices = append(nonDirectiveActionDevices, children...)
				}
			}

			// todo: this part is here only while new mighty tsf is not ready
			if len(nonDirectiveActionDevices) > 0 {
				actionRequest := adapter.NewActionRequest(nonDirectiveActionDevices)
				providerExecutionPlan.protocolActionRequest = &actionRequest
			}
		case model.QUASAR:
			// todo: this will transform into new mighty tsf as well
			//	 for _, actionDevice := range actionDevices {
			//	 	actions := newDeviceActions(actionDevice)
			//	 	providerExecutionPlan.otherOriginActions[actions.ExternalID] = map[string]deviceActions{
			//	 		actionDevice.ID: actions,
			//	 	}
			//	 }
			fallthrough
		default:
			actionRequest := adapter.NewActionRequest(actionDevices)
			providerExecutionPlan.protocolActionRequest = &actionRequest
		}
		originExecutionPlan.providerExecutionPlans[skillID] = providerExecutionPlan
	}
	return originExecutionPlan
}

func computeScenarioUserExecutionPlan(ctx context.Context, origin model.Origin, actionDevices model.Devices) userExecutionPlan {
	providerActionDevices := actionDevices.GroupByProvider()

	executionPlan := emptyUserExecutionPlan(providerActionDevices)
	for skillID, actionDevices := range providerActionDevices {
		providerExecutionPlan := emptyProviderExecutionPlan(skillID, actionDevices)
		switch skillID {
		case model.YANDEXIO:
			parentToChildEndpointsMap := model.YandexIODevices(actionDevices).ParentToChildEndpointsMap()
			for parentEndpointID, children := range parentToChildEndpointsMap {
				for _, actionDevice := range children {
					deviceActions := newDeviceActions(actionDevice)
					originActions, precomputedResults := deviceActions.precomputeOriginActions(timestamp.CurrentTimestampCtx(ctx))
					providerExecutionPlan.otherOriginActions.addOriginActions(parentEndpointID, originActions)
					providerExecutionPlan.precomputedActionResults[deviceActions.ID] = precomputedResults
				}
			}
		case model.QUASAR:
			var oldSpeakers model.Devices
			for _, actionDevice := range actionDevices {
				if model.ParovozSpeakers[actionDevice.Type] {
					deviceActions := newDeviceActions(actionDevice)
					originActions, precomputedResults := deviceActions.precomputeOriginActions(timestamp.CurrentTimestampCtx(ctx))
					providerExecutionPlan.otherOriginActions.addOriginActions(deviceActions.ExternalID, originActions)
					providerExecutionPlan.precomputedActionResults[actionDevice.ID] = precomputedResults
				} else {
					oldSpeakers = append(oldSpeakers, actionDevice)
				}
			}
			if len(oldSpeakers) > 0 {
				actionRequest := adapter.NewActionRequest(oldSpeakers)
				providerExecutionPlan.protocolActionRequest = &actionRequest
			}
		default:
			actionRequest := adapter.NewActionRequest(actionDevices)
			providerExecutionPlan.protocolActionRequest = &actionRequest
		}
		executionPlan.providerExecutionPlans[skillID] = providerExecutionPlan
	}
	return executionPlan
}
