package localscenarios

import (
	"encoding/json"

	"google.golang.org/protobuf/encoding/protojson"
	"google.golang.org/protobuf/types/known/wrapperspb"

	scenariospb "a.yandex-team.ru/alice/megamind/protos/scenarios"
	iotpb "a.yandex-team.ru/alice/protos/data/iot"
	iotscenariospb "a.yandex-team.ru/alice/protos/endpoint/capabilities/iot_scenarios"
)

type SyncScenariosSpeechkitDirective struct {
	endpointID string

	Scenarios []*iotpb.TLocalScenario `json:"scenarios"`
}

func NewSyncScenariosSpeechkitDirective(endpointID string, scenarios []*iotpb.TLocalScenario) *SyncScenariosSpeechkitDirective {
	return &SyncScenariosSpeechkitDirective{
		endpointID: endpointID,
		Scenarios:  scenarios,
	}
}

func (d *SyncScenariosSpeechkitDirective) EndpointID() string {
	return d.endpointID
}

func (d *SyncScenariosSpeechkitDirective) SpeechkitName() string {
	return "sync_iot_scenarios_directive"
}

func (d *SyncScenariosSpeechkitDirective) MarshalJSONPayload() ([]byte, error) {
	return protojson.Marshal(&iotscenariospb.TIotScenariosCapability_TSyncIotScenariosDirective{
		Name:      d.SpeechkitName(),
		Scenarios: d.Scenarios,
	})
}

func (d *SyncScenariosSpeechkitDirective) ToScenarioDirective() *scenariospb.TDirective {
	return &scenariospb.TDirective{
		EndpointId: wrapperspb.String(d.endpointID),
		Directive: &scenariospb.TDirective_SyncIotScenariosDirective{
			SyncIotScenariosDirective: &iotscenariospb.TIotScenariosCapability_TSyncIotScenariosDirective{
				Name:      d.SpeechkitName(),
				Scenarios: d.Scenarios,
			},
		},
	}
}

type AddScenariosSpeechkitDirective struct {
	endpointID string

	Scenarios []*iotpb.TLocalScenario `json:"scenarios"`
}

func NewAddScenariosSpeechkitDirective(endpointID string, scenarios []*iotpb.TLocalScenario) *AddScenariosSpeechkitDirective {
	return &AddScenariosSpeechkitDirective{
		endpointID: endpointID,
		Scenarios:  scenarios,
	}
}

func (d *AddScenariosSpeechkitDirective) EndpointID() string {
	return d.endpointID
}

func (d *AddScenariosSpeechkitDirective) SpeechkitName() string {
	return "add_iot_scenarios_directive"
}

func (d *AddScenariosSpeechkitDirective) MarshalJSONPayload() ([]byte, error) {
	return protojson.Marshal(&iotscenariospb.TIotScenariosCapability_TAddIotScenariosDirective{
		Name:      d.SpeechkitName(),
		Scenarios: d.Scenarios,
	})
}

func (d *AddScenariosSpeechkitDirective) ToScenarioDirective() *scenariospb.TDirective {
	return &scenariospb.TDirective{
		EndpointId: wrapperspb.String(d.endpointID),
		Directive: &scenariospb.TDirective_AddIotScenariosDirective{
			AddIotScenariosDirective: &iotscenariospb.TIotScenariosCapability_TAddIotScenariosDirective{
				Name:      d.SpeechkitName(),
				Scenarios: d.Scenarios,
			},
		},
	}
}

type RemoveScenariosSpeechkitDirective struct {
	endpointID string

	IDs []string `json:"ids"`
}

func NewRemoveScenariosSpeechkitDirective(endpointID string, scenarioIDs []string) *RemoveScenariosSpeechkitDirective {
	return &RemoveScenariosSpeechkitDirective{
		endpointID: endpointID,
		IDs:        scenarioIDs,
	}
}

func (d *RemoveScenariosSpeechkitDirective) EndpointID() string {
	return d.endpointID
}

func (d *RemoveScenariosSpeechkitDirective) SpeechkitName() string {
	return "remove_iot_scenarios_directive"
}

func (d *RemoveScenariosSpeechkitDirective) MarshalJSONPayload() ([]byte, error) {
	return json.Marshal(d)
}

func (d *RemoveScenariosSpeechkitDirective) ToScenarioDirective() *scenariospb.TDirective {
	return &scenariospb.TDirective{
		EndpointId: wrapperspb.String(d.endpointID),
		Directive: &scenariospb.TDirective_RemoveIotScenariosDirective{
			RemoveIotScenariosDirective: &iotscenariospb.TIotScenariosCapability_TRemoveIotScenariosDirective{
				Name: d.SpeechkitName(),
				IDs:  d.IDs,
			},
		},
	}
}
