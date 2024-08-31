package apps

import "a.yandex-team.ru/alice/megamind/protos/scenarios"

type App interface {
	isApp()
}

type RunApp interface {
	App
	OnRunRequest(*scenarios.TScenarioRunRequest) (*scenarios.TScenarioRunResponse, error)
}

type ApplyApp interface {
	App
	OnApplyRequest(*scenarios.TScenarioApplyRequest) (*scenarios.TScenarioApplyResponse, error)
}

type CommitApp interface {
	App
	OnCommitRequest(*scenarios.TScenarioApplyRequest) (*scenarios.TScenarioCommitResponse, error)
}
