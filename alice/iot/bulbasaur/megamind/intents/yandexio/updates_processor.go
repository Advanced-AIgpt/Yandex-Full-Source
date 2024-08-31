package yandexio

import (
	"context"
	"strconv"

	"google.golang.org/protobuf/types/known/anypb"

	"a.yandex-team.ru/alice/iot/bulbasaur/controller/callback"
	dtocallback "a.yandex-team.ru/alice/iot/bulbasaur/dto/callback"
	"a.yandex-team.ru/alice/iot/bulbasaur/megamind/common"
	"a.yandex-team.ru/alice/iot/bulbasaur/megamind/endpoints"
	"a.yandex-team.ru/alice/iot/bulbasaur/megamind/frames"
	"a.yandex-team.ru/alice/iot/bulbasaur/megamind/sdk"
	"a.yandex-team.ru/alice/iot/bulbasaur/model"
	"a.yandex-team.ru/alice/library/go/libmegamind"
	"a.yandex-team.ru/alice/library/go/timestamp"
	"a.yandex-team.ru/alice/megamind/protos/scenarios"
	endpointpb "a.yandex-team.ru/alice/protos/endpoint"
	batterypb "a.yandex-team.ru/alice/protos/endpoint/capabilities/battery"
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/xerrors"
)

var EndpointUpdatesProcessorName = "endpoint_updates_processor"

type EndpointUpdatesProcessor struct {
	logger             log.Logger
	callbackController callback.IController
}

func NewEndpointUpdatesProcessor(l log.Logger, callbackController callback.IController) *EndpointUpdatesProcessor {
	return &EndpointUpdatesProcessor{logger: l, callbackController: callbackController}
}

func (p *EndpointUpdatesProcessor) Name() string {
	return EndpointUpdatesProcessorName
}

func (p EndpointUpdatesProcessor) SupportedInputs() sdk.SupportedInputs {
	return sdk.SupportedInputs{
		SupportedFrames: []libmegamind.SemanticFrameName{
			frames.EndpointStateUpdatesTypedSemanticFrame,
		},
	}
}

func (p *EndpointUpdatesProcessor) Run(ctx context.Context, frame libmegamind.SemanticFrame, runRequest *scenarios.TScenarioRunRequest, user model.User) (*scenarios.TScenarioRunResponse, error) {
	return p.CoolerRun(sdk.NewRunContext(ctx, p.logger, runRequest, user), sdk.InputFrames(frame))
}

func (p *EndpointUpdatesProcessor) CoolerRun(_ sdk.RunContext, input sdk.Input) (*scenarios.TScenarioRunResponse, error) {
	frame := input.GetFirstFrame()
	if frame.Name() != string(frames.EndpointStateUpdatesTypedSemanticFrame) {
		return nil, xerrors.Errorf("unexpected input frame %s", frame.Name())
	}
	var endpointUpdatesFrame frames.EndpointUpdatesFrame
	if err := endpointUpdatesFrame.FromTypedSemanticFrame(frame.Frame.GetTypedSemanticFrame()); err != nil {
		return nil, xerrors.Errorf("failed to parse endpoint actions frame: %w", err)
	}
	applyArguments := EndpointUpdatesApplyArguments{EndpointUpdates: endpointUpdatesFrame.EndpointUpdates}
	serialized, err := anypb.New(applyArguments.ProtoApplyArguments())
	if err != nil {
		return nil, xerrors.Errorf("failed to serialize apply arguments: %w", err)
	}
	return libmegamind.NewRunResponse("iot", "iot").WithApplyArguments(serialized).Build(), nil
}

func (p EndpointUpdatesProcessor) IsApplicable(args *anypb.Any) bool {
	applyArguments, err := common.ExtractApplyArguments(args)
	if err != nil {
		return false
	}
	return applyArguments.GetEndpointUpdatesApplyArguments() != nil
}

func (p EndpointUpdatesProcessor) Apply(ctx context.Context, applyRequest *scenarios.TScenarioApplyRequest, user model.User) (*scenarios.TScenarioApplyResponse, error) {
	applyContext, err := common.NewApplyProcessorContextFromRequest(ctx, applyRequest, user)
	if err != nil {
		return nil, xerrors.Errorf("failed to parse apply context from request: %w", err)
	}
	if applyContext.ApplyArguments.GetEndpointUpdatesApplyArguments() == nil {
		return nil, xerrors.Errorf("unexpected input: endpoint updates apply arguments is nil")
	}
	callbackPayload := dtocallback.UpdateStatePayload{UserID: strconv.FormatUint(user.ID, 10)}
	for _, endpointUpdate := range applyContext.ApplyArguments.GetEndpointUpdatesApplyArguments().GetEndpointUpdates() {
		// IOT-1610: This processor will be removed after 10th of may
		// but right now we have to fix a big bug
		// all states except for battery and illuminance sent here are incorrect, so we skip them
		filteredUpdate := endpointUpdate
		filteredUpdate.Capabilities = filterBatteryAndIlluminance(endpointUpdate.GetCapabilities())
		if len(filteredUpdate.GetCapabilities()) == 0 {
			continue
		}
		deviceStateView := endpoints.Endpoint{TEndpoint: endpointUpdate}.ToDeviceStateView()
		callbackPayload.DeviceStates = append(callbackPayload.DeviceStates, deviceStateView)
	}
	if len(callbackPayload.DeviceStates) != 0 {
		err = p.callbackController.CallbackUpdateState(ctx, model.YANDEXIO, callbackPayload, timestamp.CurrentTimestampCtx(ctx))
		if err != nil {
			return nil, xerrors.Errorf("failed to handle yandex io callback: %w", err)
		}
	}
	return libmegamind.NewApplyResponse("iot", "iot").Build(), nil
}

func filterBatteryAndIlluminance(capabilities []*anypb.Any) []*anypb.Any {
	result := make([]*anypb.Any, 0, len(capabilities))
	for _, c := range capabilities {
		var (
			levelCapabilityMessage   = new(endpointpb.TLevelCapability)
			batteryCapabilityMessage = new(batterypb.TBatteryCapability)
		)
		switch {
		case c.MessageIs(levelCapabilityMessage):
			if err := c.UnmarshalTo(levelCapabilityMessage); err != nil {
				continue
			}
			if levelCapabilityMessage.GetParameters().GetInstance() == endpointpb.TLevelCapability_IlluminanceInstance && levelCapabilityMessage.GetState() != nil {
				result = append(result, c)
			}
		case c.MessageIs(batteryCapabilityMessage):
			if err := c.UnmarshalTo(batteryCapabilityMessage); err != nil {
				continue
			}
			if batteryCapabilityMessage.GetState() != nil {
				result = append(result, c)
			}
		}
	}
	return result
}
