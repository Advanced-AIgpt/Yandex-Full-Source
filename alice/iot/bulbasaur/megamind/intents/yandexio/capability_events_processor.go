package yandexio

import (
	"context"
	"strconv"

	"google.golang.org/protobuf/types/known/anypb"

	"a.yandex-team.ru/alice/iot/bulbasaur/controller/callback"
	dtocallback "a.yandex-team.ru/alice/iot/bulbasaur/dto/callback"
	"a.yandex-team.ru/alice/iot/bulbasaur/megamind/endpoints"
	"a.yandex-team.ru/alice/iot/bulbasaur/megamind/frames"
	"a.yandex-team.ru/alice/iot/bulbasaur/megamind/sdk"
	"a.yandex-team.ru/alice/iot/bulbasaur/model"
	"a.yandex-team.ru/alice/library/go/libmegamind"
	"a.yandex-team.ru/alice/library/go/timestamp"
	"a.yandex-team.ru/alice/megamind/protos/scenarios"
	eventspb "a.yandex-team.ru/alice/protos/endpoint/events"
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/xerrors"
)

var EndpointCapabilityEventsProcessorName = "endpoint_capability_events_processor"

type EndpointCapabilityEventsProcessor struct {
	logger             log.Logger
	callbackController callback.IController
}

func NewEndpointCapabilityEventsProcessor(l log.Logger, callbackController callback.IController) *EndpointCapabilityEventsProcessor {
	return &EndpointCapabilityEventsProcessor{logger: l, callbackController: callbackController}
}

func (p *EndpointCapabilityEventsProcessor) Name() string {
	return EndpointCapabilityEventsProcessorName
}

func (p *EndpointCapabilityEventsProcessor) SupportedInputs() sdk.SupportedInputs {
	return sdk.SupportedInputs{
		SupportedFrames: []libmegamind.SemanticFrameName{
			frames.EndpointCapabilityEventsTypedSemanticFrame,
		},
	}
}

func (p *EndpointCapabilityEventsProcessor) Run(ctx context.Context, inputFrame libmegamind.SemanticFrame, r *scenarios.TScenarioRunRequest, u model.User) (*scenarios.TScenarioRunResponse, error) {
	return p.CoolerRun(sdk.NewRunContext(ctx, p.logger, r, u), sdk.InputFrames(inputFrame))
}

func (p *EndpointCapabilityEventsProcessor) IsApplicable(args *anypb.Any) bool {
	// note: if we agree that all frame processors use UniversalArguments, IsApplicable is not needed.
	return sdk.IsApplyArguments(args, new(EndpointCapabilityEventsApplyArguments))
}

func (p *EndpointCapabilityEventsProcessor) Apply(ctx context.Context, applyRequest *scenarios.TScenarioApplyRequest, user model.User) (*scenarios.TScenarioApplyResponse, error) {
	applyContext := sdk.NewApplyContext(ctx, p.logger, applyRequest, user)
	applyArguments := p.supportedApplyArguments()
	if err := sdk.UnmarshalApplyArguments(applyContext.Arguments(), applyArguments); err != nil {
		return nil, err
	}
	applyContext.Logger().Info("got apply with arguments", log.Any("args", applyArguments))
	return p.apply(applyContext, applyArguments)
}

func (p *EndpointCapabilityEventsProcessor) CoolerRun(ctx sdk.RunContext, input sdk.Input) (*scenarios.TScenarioRunResponse, error) {
	var frame EndpointCapabilityEventsTypedSemanticFrame
	if err := sdk.UnmarshalTSF(input, &frame); err != nil {
		return nil, xerrors.Errorf("failed to unmarshal tsf: %w", err)
	}

	ctx.Logger().Info("got run request with frame", log.Any("frame", frame))

	args := &EndpointCapabilityEventsApplyArguments{
		Events: frame.Events,
	}

	protoArgs, err := sdk.MarshalApplyArguments(args)
	if err != nil {
		return nil, err
	}

	return libmegamind.NewRunResponse("iot", "iot").WithApplyArguments(protoArgs).Build(), nil
}

func (p *EndpointCapabilityEventsProcessor) supportedApplyArguments() sdk.UniversalApplyArguments {
	return new(EndpointCapabilityEventsApplyArguments)
}

func (p *EndpointCapabilityEventsProcessor) apply(ctx sdk.ApplyContext, applyArguments sdk.UniversalApplyArguments) (*scenarios.TScenarioApplyResponse, error) {
	args := applyArguments.(*EndpointCapabilityEventsApplyArguments)
	ctx.Logger().Info("got apply with arguments", log.Any("arguments", args))
	user, _ := ctx.User()
	deviceStateView := endpoints.ConvertEndpointEventsToDeviceStateView(ctx.Context(), p.logger, &eventspb.TEndpointEvents{
		EndpointId:       args.Events.GetEndpointId(),
		CapabilityEvents: args.Events.GetEvents(),
	})
	updateStatePayload := dtocallback.UpdateStatePayload{
		UserID:       strconv.FormatUint(user.ID, 10),
		DeviceStates: []dtocallback.DeviceStateView{deviceStateView},
	}
	ctx.Logger().Info("callback update state payload", log.Any("payload", updateStatePayload))
	err := p.callbackController.CallbackUpdateState(ctx.Context(), model.YANDEXIO, updateStatePayload, timestamp.CurrentTimestampCtx(ctx.Context()))
	if err != nil {
		ctx.Logger().Errorf("failed to run callback update state: %v", err)
	}
	return libmegamind.NewApplyResponse("iot", "iot").Build(), nil
}
