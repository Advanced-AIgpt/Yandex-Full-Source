package widget

import (
	"sort"

	"a.yandex-team.ru/alice/iot/bulbasaur/model"
)

type UserScenariosView struct {
	Status    string             `json:"status"`
	RequestID string             `json:"request_id"`
	Scenarios []ScenarioListView `json:"scenarios"`
}

type ScenarioListView struct {
	ID      string `json:"id"`
	Name    string `json:"name"`
	IconURL string `json:"icon_url"`
}

func (usv *UserScenariosView) FromScenarios(scenarios model.Scenarios, devices model.Devices) {
	usv.Scenarios = make([]ScenarioListView, 0, len(scenarios))
	for _, scenario := range scenarios {
		if !scenario.IsExecutable(devices) || !scenario.IsActive {
			continue
		}
		if len(usv.Scenarios) > 50 {
			// FIXME: temporary restriction
			// https://a.yandex-team.ru/review/1522472/details#comment--3514754
			break
		}
		var slv ScenarioListView
		slv.FromScenario(scenario)
		usv.Scenarios = append(usv.Scenarios, slv)
	}
	sort.Sort(ScenarioListViewSorting(usv.Scenarios))
}

func (slv *ScenarioListView) FromScenario(scenario model.Scenario) {
	slv.ID = scenario.ID
	slv.Name = string(scenario.Name)
	slv.IconURL = scenario.Icon.URL()
}
