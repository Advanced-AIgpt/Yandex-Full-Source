package discovery

import (
	"context"

	"a.yandex-team.ru/alice/iot/bulbasaur/megamind/frames"
	"a.yandex-team.ru/alice/iot/bulbasaur/megamind/sdk"
	"a.yandex-team.ru/alice/iot/bulbasaur/model"
	"a.yandex-team.ru/alice/library/go/libmegamind"
	"a.yandex-team.ru/alice/megamind/protos/scenarios"
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/xerrors"
)

var ForgetDevicesProcessorName = "forget_devices_processor"

type ForgetDevicesProcessor struct {
	logger log.Logger
}

func NewForgetDevicesProcessor(l log.Logger) *ForgetDevicesProcessor {
	return &ForgetDevicesProcessor{logger: l}
}

func (p *ForgetDevicesProcessor) Name() string {
	return ForgetDevicesProcessorName
}

func (p *ForgetDevicesProcessor) SupportedInputs() sdk.SupportedInputs {
	return sdk.SupportedInputs{
		SupportedFrames: []libmegamind.SemanticFrameName{
			frames.ForgetEndpointsTypedSemanticFrame,
		},
	}
}

func (p *ForgetDevicesProcessor) Run(ctx context.Context, inputFrame libmegamind.SemanticFrame, r *scenarios.TScenarioRunRequest, u model.User) (*scenarios.TScenarioRunResponse, error) {
	return p.CoolerRun(sdk.NewRunContext(ctx, p.logger, r, u), sdk.InputFrames(inputFrame))
}

func (p *ForgetDevicesProcessor) CoolerRun(ctx sdk.RunContext, input sdk.Input) (*scenarios.TScenarioRunResponse, error) {
	var frame frames.ForgetEndpointsFrame
	if err := sdk.UnmarshalTSF(input, &frame); err != nil {
		return nil, xerrors.Errorf("failed to unmarshal tsf: %w", err)
	}
	ctx.Logger().Info("got run request with frame", log.Any("frame", frame))
	directive := ForgetDevicesDirective{ExternalDeviceIDs: frame.EndpointIDs}
	scenarioDirective := directive.ToDirective(ctx.ClientInfo().DeviceID)
	return libmegamind.NewRunResponse(IoTScenarioName, DiscoveryIntent).
		WithNoOutputSpeechLayout(scenarioDirective).Build(), nil
}
