package discovery

import (
	"context"

	"a.yandex-team.ru/alice/iot/bulbasaur/megamind/sdk"
	"a.yandex-team.ru/alice/iot/bulbasaur/model"
	"a.yandex-team.ru/alice/library/go/libmegamind"
	"a.yandex-team.ru/alice/megamind/protos/scenarios"
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/xerrors"
)

var CancelDiscoveryProcessorName = "cancel_discovery_processor"

type CancelDiscoveryProcessor struct {
	logger log.Logger
}

func NewCancelDiscoveryProcessor(l log.Logger) *CancelDiscoveryProcessor {
	return &CancelDiscoveryProcessor{logger: l}
}

func (p *CancelDiscoveryProcessor) Name() string {
	return CancelDiscoveryProcessorName
}

func (p *CancelDiscoveryProcessor) SupportedInputs() sdk.SupportedInputs {
	return sdk.SupportedInputs{
		SupportedCallbacks: []libmegamind.CallbackName{
			CancelDiscoveryCallbackName,
		},
	}
}

func (p *CancelDiscoveryProcessor) Run(ctx context.Context, inputFrame libmegamind.SemanticFrame, r *scenarios.TScenarioRunRequest, u model.User) (*scenarios.TScenarioRunResponse, error) {
	return nil, xerrors.New("frames are not supported")
}

func (p *CancelDiscoveryProcessor) CoolerRun(ctx sdk.RunContext, _ sdk.Input) (*scenarios.TScenarioRunResponse, error) {
	directive := CancelDiscoveryDirective{}
	return libmegamind.NewRunResponse(IoTScenarioName, DiscoveryIntent).
		WithLayout("Хорошо, отменяю поиск.", "Хорошо, отменяю поиск.",
			directive.ToDirective(ctx.ClientDeviceID())).Build(), nil
}
