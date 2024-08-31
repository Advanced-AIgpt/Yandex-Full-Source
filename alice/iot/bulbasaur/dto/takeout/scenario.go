package takeout

import "a.yandex-team.ru/alice/iot/bulbasaur/model"

type ScenarioView struct {
	ID                           string                     `json:"id"`
	Name                         model.ScenarioName         `json:"name"`
	AffectedDevices              []string                   `json:"affected_device_ids"`
	RequestedSpeakerCapabilities []model.ScenarioCapability `json:"requested_actions"`
}

func (sv *ScenarioView) FromModel(scenario model.Scenario) {
	sv.ID = scenario.ID
	sv.Name = scenario.Name

	sv.AffectedDevices = make([]string, len(scenario.Devices))
	for i, device := range scenario.Devices {
		sv.AffectedDevices[i] = device.ID
	}

	sv.RequestedSpeakerCapabilities = make([]model.ScenarioCapability, 0, len(scenario.RequestedSpeakerCapabilities))
	for _, qsac := range scenario.RequestedSpeakerCapabilities {
		sv.RequestedSpeakerCapabilities = append(sv.RequestedSpeakerCapabilities, qsac)
	}
}
