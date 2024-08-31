package megamind

import (
	"context"

	"a.yandex-team.ru/alice/iot/bulbasaur/model"
	"a.yandex-team.ru/alice/iot/bulbasaur/protos"
	"a.yandex-team.ru/alice/megamind/protos/scenarios"
)

type RunProcessor interface {
	Name() string
	IsRunnable(request *scenarios.TScenarioRunRequest) bool
	Run(ctx context.Context, request *scenarios.TScenarioRunRequest, u model.ExtendedUserInfo) (*scenarios.TScenarioRunResponse, error)
}

type ApplyProcessor interface {
	IsApplicable(request *scenarios.TScenarioApplyRequest, arguments *protos.TApplyArguments) bool
	Apply(ctx context.Context, request *scenarios.TScenarioApplyRequest, arguments *protos.TApplyArguments, user model.User) (*scenarios.TScenarioApplyResponse, error)
}

type RunApplyProcessor interface {
	RunProcessor
	ApplyProcessor
}
