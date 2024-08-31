package centaur

import (
	"context"

	"a.yandex-team.ru/alice/iot/bulbasaur/megamind/frames"
	"a.yandex-team.ru/alice/iot/bulbasaur/megamind/sdk"
	"a.yandex-team.ru/alice/iot/bulbasaur/model"
	"a.yandex-team.ru/alice/library/go/libmegamind"
	"a.yandex-team.ru/alice/megamind/protos/scenarios"
	"a.yandex-team.ru/alice/protos/data/scenario"
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/xerrors"
)

var CollectMainScreenProcessorName = "collect_main_screen_processor"

type CollectMainScreenProcessor struct {
	logger log.Logger
}

func NewCollectMainScreenProcessor(logger log.Logger) *CollectMainScreenProcessor {
	return &CollectMainScreenProcessor{logger: logger}
}

func (p *CollectMainScreenProcessor) Name() string {
	return CollectMainScreenProcessorName
}

func (p *CollectMainScreenProcessor) SupportedInputs() sdk.SupportedInputs {
	return sdk.SupportedInputs{
		SupportedFrames: []libmegamind.SemanticFrameName{
			frames.CentaurCollectMainScreenFrameName,
		},
	}
}

func (p *CollectMainScreenProcessor) Run(ctx context.Context, inputFrame libmegamind.SemanticFrame, r *scenarios.TScenarioRunRequest, u model.User) (*scenarios.TScenarioRunResponse, error) {
	return nil, xerrors.New("old run is not supported")
}

func (p *CollectMainScreenProcessor) CoolerRun(ctx sdk.RunContext, input sdk.Input) (*scenarios.TScenarioRunResponse, error) {
	userInfo, err := ctx.UserInfo()
	if err != nil {
		return nil, err
	}

	scenarioData := &scenario.TScenarioData{
		Data: &scenario.TScenarioData_IoTUserData{
			IoTUserData: userInfo.ToUserInfoProto(ctx.Context()),
		},
	}

	return sdk.RunResponse(ctx).WithScenarioData(scenarioData).Build()
}
