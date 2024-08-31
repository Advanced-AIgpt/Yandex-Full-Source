package sdk

import (
	"time"

	analyticspb "a.yandex-team.ru/alice/megamind/protos/analytics/scenarios/iot"
	"a.yandex-team.ru/alice/megamind/protos/scenarios"
)

const (
	IotReactionsObjectID = "iot_reactions"
	ActionReactionType   = "action"
	ScenarioReactionType = "scenario"
	QueryReactionType    = "query"
)

const (
	IoTProductScenarioName = "iot_do" // should be snake cased
	IoTScenarioIntent      = "iot"    // main iot intent
)

type AnalyticsInfoBuilder interface {
	WithAction(action ActionAnalyticsInfo) AnalyticsInfoBuilder
	WithQuery(target string, deviceIDs []string) AnalyticsInfoBuilder
	WithScenario(scenarioID string) AnalyticsInfoBuilder

	Build() (*scenarios.TAnalyticsInfo, error)
}

func AnalyticsInfo() AnalyticsInfoBuilder {
	return &analyticsInfo{
		reactions: &analyticspb.TReactions{},
	}
}

type analyticsInfo struct {
	reactions *analyticspb.TReactions
}

func (a *analyticsInfo) WithAction(action ActionAnalyticsInfo) AnalyticsInfoBuilder {
	reaction := &analyticspb.TReaction{
		Type: ActionReactionType,
		Parameters: &analyticspb.TReaction_ActionParameters{
			ActionParameters: &analyticspb.TReaction_TActionParameters{
				CapabilityType:     action.CapabilityType,
				CapabilityInstance: action.CapabilityInstance,
				CapabilityUnit:     action.CapabilityUnit,
				CapabilityValue:    action.CapabilityValue,
				RelativityType:     action.RelativityType,
				TimeInfo:           action.timeInfo(),
				Devices:            action.DeviceIDs,
			},
		},
	}
	a.reactions.Reactions = append(a.reactions.Reactions, reaction)

	return a
}

func (a *analyticsInfo) WithScenario(scenarioID string) AnalyticsInfoBuilder {
	reaction := &analyticspb.TReaction{
		Type: ScenarioReactionType,
		Parameters: &analyticspb.TReaction_ScenarioParameters{
			ScenarioParameters: &analyticspb.TReaction_TScenarioParameters{
				Scenarios: []string{scenarioID},
			},
		},
	}
	a.reactions.Reactions = append(a.reactions.Reactions, reaction)

	return a
}

func (a *analyticsInfo) WithQuery(target string, deviceIDs []string) AnalyticsInfoBuilder {
	reaction := &analyticspb.TReaction{
		Type: QueryReactionType,
		Parameters: &analyticspb.TReaction_QueryParameters{
			QueryParameters: &analyticspb.TReaction_TQueryParameters{
				Target:  target,
				Devices: deviceIDs,
			},
		},
	}
	a.reactions.Reactions = append(a.reactions.Reactions, reaction)

	return a
}

func (a *analyticsInfo) Build() (*scenarios.TAnalyticsInfo, error) {
	result := &scenarios.TAnalyticsInfo{
		ProductScenarioName: IoTProductScenarioName,
		Intent:              IoTScenarioIntent,
	}

	if a.reactions != nil {
		result.Objects = []*scenarios.TAnalyticsInfo_TObject{
			{
				Id: IotReactionsObjectID,
				Payload: &scenarios.TAnalyticsInfo_TObject_IotReactions{
					IotReactions: a.reactions,
				},
			},
		}
	}

	return result, nil
}

type ActionAnalyticsInfo struct {
	CapabilityType     string
	CapabilityInstance string
	CapabilityValue    string
	CapabilityUnit     string
	RelativityType     string
	RequestedTime      time.Time
	IntervalStartTime  time.Time
	IntervalEndTime    time.Time
	DeviceIDs          []string
}

func (ai *ActionAnalyticsInfo) timeInfo() *analyticspb.TTimeInfo {
	if !ai.IntervalEndTime.IsZero() {
		return &analyticspb.TTimeInfo{
			Value: &analyticspb.TTimeInfo_TimeInterval{
				TimeInterval: &analyticspb.TTimeInterval{
					StartTime: ai.IntervalStartTime.Format(time.RFC3339),
					EndTime:   ai.IntervalEndTime.Format(time.RFC3339),
				},
			},
		}
	}

	if !ai.RequestedTime.IsZero() {
		return &analyticspb.TTimeInfo{
			Value: &analyticspb.TTimeInfo_TimePoint{
				TimePoint: &analyticspb.TTimePoint{
					Time: ai.RequestedTime.Format(time.RFC3339),
				},
			},
		}
	}

	return nil
}
