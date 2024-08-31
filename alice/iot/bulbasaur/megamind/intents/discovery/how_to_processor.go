package discovery

import (
	"context"

	"a.yandex-team.ru/alice/iot/bulbasaur/megamind/frames"
	"a.yandex-team.ru/alice/iot/bulbasaur/megamind/sdk"
	"a.yandex-team.ru/alice/iot/bulbasaur/model"
	"a.yandex-team.ru/alice/library/go/libmegamind"
	"a.yandex-team.ru/alice/megamind/protos/scenarios"
	"a.yandex-team.ru/library/go/core/log"
)

var HowToProcessorName = "how_to_discovery_processor"

type HowToProcessor struct {
	logger log.Logger
}

func NewHowToProcessor(l log.Logger) *HowToProcessor {
	return &HowToProcessor{logger: l}
}

func (p *HowToProcessor) Name() string {
	return HowToProcessorName
}

func (p *HowToProcessor) SupportedInputs() sdk.SupportedInputs {
	return sdk.SupportedInputs{
		SupportedFrames: []libmegamind.SemanticFrameName{
			frames.HowToDiscoveryFrameName,
		},
	}
}

func (p *HowToProcessor) Run(ctx context.Context, inputFrame libmegamind.SemanticFrame, r *scenarios.TScenarioRunRequest, u model.User) (*scenarios.TScenarioRunResponse, error) {
	return p.CoolerRun(sdk.NewRunContext(ctx, p.logger, r, u), sdk.InputFrames(inputFrame))
}

func (p *HowToProcessor) CoolerRun(ctx sdk.RunContext, _ sdk.Input) (*scenarios.TScenarioRunResponse, error) {
	return sdk.RunResponse(ctx).WithNLG(howToDiscoveryNLG).Build()
}
