package yandexio

import (
	"context"

	"a.yandex-team.ru/alice/iot/bulbasaur/megamind/frames"
	"a.yandex-team.ru/alice/iot/bulbasaur/megamind/sdk"
	"a.yandex-team.ru/alice/iot/bulbasaur/model"
	"a.yandex-team.ru/alice/iot/bulbasaur/model/directives"
	"a.yandex-team.ru/alice/library/go/libmegamind"
	"a.yandex-team.ru/alice/megamind/protos/scenarios"
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/xerrors"
)

var EndpointActionsProcessorName = "endpoint_actions_processor"

type EndpointActionsProcessor struct {
	logger log.Logger
}

func NewEndpointActionsProcessor(l log.Logger) *EndpointActionsProcessor {
	return &EndpointActionsProcessor{logger: l}
}

func (p *EndpointActionsProcessor) Name() string {
	return EndpointActionsProcessorName
}

func (p *EndpointActionsProcessor) SupportedInputs() sdk.SupportedInputs {
	return sdk.SupportedInputs{
		SupportedFrames: []libmegamind.SemanticFrameName{
			frames.EndpointActionsTypedSemanticFrame,
		},
	}
}

func (p *EndpointActionsProcessor) Run(ctx context.Context, frame libmegamind.SemanticFrame, runRequest *scenarios.TScenarioRunRequest, user model.User) (*scenarios.TScenarioRunResponse, error) {
	return p.CoolerRun(sdk.NewRunContext(ctx, p.logger, runRequest, user), sdk.InputFrames(frame))
}

func (p *EndpointActionsProcessor) CoolerRun(runContext sdk.RunContext, input sdk.Input) (*scenarios.TScenarioRunResponse, error) {
	frame := input.GetFirstFrame()
	if frame.Name() != string(frames.EndpointActionsTypedSemanticFrame) {
		return nil, xerrors.Errorf("unexpected input frame %s", frame.Name())
	}

	var endpointActionsFrame frames.EndpointActionsFrame
	if err := endpointActionsFrame.FromTypedSemanticFrame(frame.Frame.GetTypedSemanticFrame()); err != nil {
		return nil, xerrors.Errorf("failed to parse endpoint actions frame: %w", err)
	}

	result := p.convertFrameToDirectives(runContext, endpointActionsFrame)
	return libmegamind.NewRunResponse("iot", "iot").WithNoOutputSpeechLayout(result...).Build(), nil
}

func (p *EndpointActionsProcessor) convertFrameToDirectives(ctx sdk.RunContext, f frames.EndpointActionsFrame) []*scenarios.TDirective {
	result := make([]*scenarios.TDirective, 0, len(f.Actions))
	for _, deviceAction := range f.Actions {
		endpointID := deviceAction.GetExternalDeviceId()
		for _, capabilityAction := range deviceAction.GetActions() {
			directive, err := directives.ConvertProtoActionToSpeechkitDirective(endpointID, capabilityAction)
			if err != nil {
				ctx.Logger().Errorf("failed to convert action to directive: %s", err)
				continue
			}
			result = append(result, directive.ToScenarioDirective())
		}
	}
	return result
}
