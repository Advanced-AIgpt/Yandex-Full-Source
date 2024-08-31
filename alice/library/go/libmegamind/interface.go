package libmegamind

import (
	"context"

	"a.yandex-team.ru/alice/iot/bulbasaur/model"
	"a.yandex-team.ru/alice/megamind/protos/scenarios"
)

type IProcessor interface {
	Name() string
	IsRunnable(ctx context.Context, request *scenarios.TScenarioRunRequest) bool
	IsApplicable(ctx context.Context, request *scenarios.TScenarioApplyRequest) bool
	IsContinuable(ctx context.Context, request *scenarios.TScenarioApplyRequest) bool // MM uses TScenarioApplyRequest for continue handler
	Run(ctx context.Context, runRequest *scenarios.TScenarioRunRequest, user model.User) (*scenarios.TScenarioRunResponse, error)
	Apply(ctx context.Context, applyRequest *scenarios.TScenarioApplyRequest, user model.User) (*scenarios.TScenarioApplyResponse, error)
	Continue(ctx context.Context, continueRequest *scenarios.TScenarioApplyRequest, user model.User) (*scenarios.TScenarioContinueResponse, error)
}
