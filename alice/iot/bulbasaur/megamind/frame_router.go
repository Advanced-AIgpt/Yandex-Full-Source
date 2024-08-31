package megamind

import (
	"context"
	"encoding/json"
	"fmt"

	"golang.org/x/exp/slices"

	"a.yandex-team.ru/alice/iot/bulbasaur/logging/doublelog"
	"a.yandex-team.ru/alice/iot/bulbasaur/megamind/common"
	"a.yandex-team.ru/alice/iot/bulbasaur/megamind/sdk"
	"a.yandex-team.ru/alice/iot/bulbasaur/model"
	"a.yandex-team.ru/alice/iot/bulbasaur/nlg"
	"a.yandex-team.ru/alice/library/go/libmegamind"
	commonpb "a.yandex-team.ru/alice/megamind/protos/common"
	"a.yandex-team.ru/alice/megamind/protos/scenarios"
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/log/ctxlog"
	"a.yandex-team.ru/library/go/core/xerrors"
)

type FrameRouter struct {
	FrameRunProcessors          []FrameRunProcessor
	RunProcessorByFrame         map[libmegamind.SemanticFrameName]FrameRunProcessor
	CallbackRunProcessors       map[libmegamind.CallbackName]FrameRunProcessor
	FrameApplyProcessors        []FrameApplyProcessor
	FrameContinueProcessors     []FrameContinueProcessor
	SpecifySupportingProcessors []SpecifySupportingProcessor

	Logger log.Logger
}

var _ libmegamind.IProcessor = &FrameRouter{}

func NewFrameRouter(logger log.Logger) *FrameRouter {
	return &FrameRouter{
		FrameRunProcessors:          make([]FrameRunProcessor, 0),
		RunProcessorByFrame:         make(map[libmegamind.SemanticFrameName]FrameRunProcessor),
		CallbackRunProcessors:       make(map[libmegamind.CallbackName]FrameRunProcessor),
		FrameApplyProcessors:        make([]FrameApplyProcessor, 0),
		FrameContinueProcessors:     make([]FrameContinueProcessor, 0),
		SpecifySupportingProcessors: make([]SpecifySupportingProcessor, 0),
		Logger:                      logger,
	}
}

func (f *FrameRouter) Name() string {
	return IoTScenarioIntent
}

func (f *FrameRouter) IsRunnable(_ context.Context, request *scenarios.TScenarioRunRequest) bool {
	if requestCallback := request.GetInput().GetCallback(); requestCallback != nil {
		if _, ok := f.CallbackRunProcessors[libmegamind.CallbackName(requestCallback.GetName())]; ok {
			return true
		}
	}

	requestFrames := request.GetInput().GetSemanticFrames()
	if requestFrames == nil {
		return false
	}

	for _, frame := range requestFrames {
		if _, ok := f.RunProcessorByFrame[libmegamind.SemanticFrameName(frame.GetName())]; ok {
			return true
		}
	}

	return false
}

func (f *FrameRouter) Run(ctx context.Context, runRequest *scenarios.TScenarioRunRequest, user model.User) (*scenarios.TScenarioRunResponse, error) {
	// https://st.yandex-team.ru/IOT-1601
	runContext := sdk.NewRunContext(ctx, f.Logger, runRequest, user)
	if runContext.ClientInfo().IsSelfDrivingCar() {
		runContext.Logger().Info("requests from self driving cars are not supported, exiting frame router")
		return sdk.RunResponse(runContext).WithNLG(nlg.SelfDrivingCarsNotSupported).Build()
	}

	if requestCallback := runRequest.GetInput().GetCallback(); requestCallback != nil {
		return f.runCallbackProcessor(runContext, requestCallback)
	}

	requestFrames := runRequest.GetInput().GetSemanticFrames()
	if requestFrames == nil {
		return nil, xerrors.New("failed to get semantic frames from request")
	}

	return f.runFrameProcessor(runContext, requestFrames)
}

// TODO(aaulayev): move specify processors to megamind/processors and refactor RunWithSpecifiedSlots
func (f *FrameRouter) RunWithSpecifiedSlots(processorContext common.RunProcessorContext, specifiedSlots ...libmegamind.Slot) (*scenarios.TScenarioRunResponse, error) {
	for _, processor := range f.SpecifySupportingProcessors {
		for _, supportedFrame := range processor.SupportedInputs().SupportedFrames {
			if string(supportedFrame) == processorContext.SemanticFrame.Name() {
				return processor.RunWithSpecifiedSlots(processorContext, specifiedSlots...)
			}
		}
	}

	return nil, xerrors.New("suitable processor for specified slot not found")
}

// RunWithSpecifiedSlotsV2 is a new format of slots specification
func (f *FrameRouter) RunWithSpecifiedSlotsV2(runContext sdk.RunContext, semanticFrames []libmegamind.SemanticFrame, specifiedSlots ...sdk.GranetSlot) (*scenarios.TScenarioRunResponse, error) {
	for _, processor := range f.SpecifySupportingProcessors {
		input := sdk.InputFrames(semanticFrames...)
		filteredInput := filterBySupported(input, processor.SupportedInputs())
		if len(filteredInput.GetFrames()) == 0 {
			continue
		}

		runContext.Logger().Info(
			"suitable frame run processor found",
			log.String("frame_run_processor", processor.Name()),
			log.Strings("semantic_frames", filteredInput.GetFrames().Names()),
		)
		runContext = runContext.WithFields(log.String("processor", processor.Name()))

		return processor.RunWithSpecifiedSlotsV2(runContext, input, specifiedSlots...)
	}

	return nil, xerrors.New("suitable processor for specified slot not found")
}

func (f *FrameRouter) IsApplicable(_ context.Context, request *scenarios.TScenarioApplyRequest) bool {
	applyArguments := request.GetArguments()
	if applyArguments == nil {
		return false
	}

	for _, processor := range f.FrameApplyProcessors {
		if processor.IsApplicable(applyArguments) {
			return true
		}
	}

	return false
}

func (f *FrameRouter) Apply(ctx context.Context, applyRequest *scenarios.TScenarioApplyRequest, user model.User) (*scenarios.TScenarioApplyResponse, error) {
	applyArguments := applyRequest.GetArguments()
	if applyArguments == nil {
		return nil, xerrors.New("failed to get apply arguments from request")
	}

	for _, processor := range f.FrameApplyProcessors {
		if processor.IsApplicable(applyArguments) {
			doublelog.Info(
				ctx, f.Logger,
				"suitable frame apply processor found",
				log.String("frame_apply_processor", processor.Name()),
			)
			ctx = ctxlog.WithFields(ctx, log.String("processor", processor.Name()))
			return processor.Apply(ctx, applyRequest, user)
		}
	}

	return nil, xerrors.New("suitable frame apply processor not found")
}

func (f *FrameRouter) IsContinuable(_ context.Context, request *scenarios.TScenarioApplyRequest) bool {
	continueArguments := request.GetArguments()
	if continueArguments == nil {
		return false
	}

	for _, processor := range f.FrameContinueProcessors {
		if processor.IsContinuable(continueArguments) {
			return true
		}
	}

	return false
}

func (f *FrameRouter) Continue(ctx context.Context, continueRequest *scenarios.TScenarioApplyRequest, user model.User) (*scenarios.TScenarioContinueResponse, error) {
	continueArguments := continueRequest.GetArguments()
	if continueArguments == nil {
		return nil, xerrors.New("failed to get continue arguments from request")
	}

	for _, processor := range f.FrameContinueProcessors {
		if processor.IsContinuable(continueArguments) {
			doublelog.Info(
				ctx, f.Logger,
				"suitable frame continue processor found",
				log.String("frame_continue_processor", processor.Name()),
			)
			ctx = ctxlog.WithFields(ctx, log.String("processor", processor.Name()))
			return processor.Continue(ctx, continueRequest, user)
		}
	}

	return nil, xerrors.New("suitable frame continue processor not found")
}

func (f *FrameRouter) WithRunProcessor(processor FrameRunProcessor) *FrameRouter {
	for _, callbackName := range processor.SupportedInputs().SupportedCallbacks {
		if supportingProcessor, ok := f.CallbackRunProcessors[callbackName]; ok {
			panic(fmt.Sprintf(
				"callback must not be supported by multiple processors: %s and %s",
				processor.Name(),
				supportingProcessor.Name(),
			))
		}
		f.CallbackRunProcessors[callbackName] = processor
	}

	for _, frameName := range processor.SupportedInputs().SupportedFrames {
		if supportingProcessor, ok := f.RunProcessorByFrame[frameName]; ok {
			panic(fmt.Sprintf(
				"frame must not be supported by multiple processors: %s and %s",
				processor.Name(),
				supportingProcessor.Name(),
			))
		}
		f.RunProcessorByFrame[frameName] = processor
	}

	if specifySupportingProcessor, ok := processor.(SpecifySupportingProcessor); ok {
		f.SpecifySupportingProcessors = append(f.SpecifySupportingProcessors, specifySupportingProcessor)
	}

	f.FrameRunProcessors = append(f.FrameRunProcessors, processor)

	return f
}

func (f *FrameRouter) WithRunApplyProcessor(processor FrameRunApplyProcessor) *FrameRouter {
	f.FrameApplyProcessors = append(f.FrameApplyProcessors, processor)
	return f.WithRunProcessor(processor)
}

func (f *FrameRouter) WithRunContinueProcessor(processor FrameRunContinueProcessor) *FrameRouter {
	f.FrameContinueProcessors = append(f.FrameContinueProcessors, processor)
	return f.WithRunProcessor(processor)
}

func (f *FrameRouter) runCallbackProcessor(runContext sdk.RunContext, requestCallback *scenarios.TCallbackDirective) (*scenarios.TScenarioRunResponse, error) {
	callbackName := libmegamind.CallbackName(requestCallback.GetName())
	processor, ok := f.CallbackRunProcessors[callbackName]
	if !ok {
		return nil, xerrors.New("suitable callback processor not found")
	}

	payloadBytes, err := requestCallback.GetPayload().MarshalJSON()
	if err != nil {
		return nil, xerrors.New("failed to create callback from request")
	}

	callback := libmegamind.Callback{
		Name:    callbackName,
		Payload: json.RawMessage(payloadBytes),
	}
	runContext.Logger().Info(
		"suitable callback run processor found",
		log.String("frame_run_processor", processor.Name()),
		log.String("callback", string(callbackName)),
	)

	runContext = runContext.WithFields(log.String("processor", processor.Name()))
	return processor.CoolerRun(runContext, sdk.InputCallback(callback))
}

func (f *FrameRouter) runFrameProcessor(runContext sdk.RunContext, requestFrames []*commonpb.TSemanticFrame) (*scenarios.TScenarioRunResponse, error) {
	runContext.Logger().Info("searching for appropriate frame run processor", log.Any("semantic_frames", requestFrames))
	input := sdk.InputFrames(libmegamind.NewSemanticFrames(requestFrames)...)

	for _, processor := range f.FrameRunProcessors {
		filteredInput := filterBySupported(input, processor.SupportedInputs())
		if len(filteredInput.GetFrames()) == 0 {
			continue
		}

		runContext.Logger().Info(
			"suitable frame run processor found",
			log.String("frame_run_processor", processor.Name()),
			log.Strings("semantic_frames", filteredInput.GetFrames().Names()),
		)
		ctx := ctxlog.WithFields(runContext.Context(), log.String("processor", processor.Name()))
		runContext = runContext.WithFields(log.String("processor", processor.Name()))

		// TODO(aaulayev): remove the kostyl when action processor is rewritten to use sdk.GranetSlot
		if processor.Name() == "action_processor" {
			user, _ := runContext.User()
			return processor.Run(ctx, libmegamind.SemanticFrame{Frame: filteredInput.GetFirstFrame().Frame}, runContext.Request(), user)
		}
		return processor.CoolerRun(runContext, filteredInput)
	}

	return nil, xerrors.New("suitable frame processor not found")
}

func filterBySupported(input sdk.Input, supportedInputs sdk.SupportedInputs) sdk.Input {
	if callback := input.GetCallback(); callback != nil && slices.Contains(supportedInputs.SupportedCallbacks, callback.Name) {
		return sdk.InputCallback(*callback)
	}

	inputFrames := input.GetFrames().ToMap()
	supportedFrames := make(libmegamind.SemanticFrames, 0, len(inputFrames))
	for _, supportedFrameName := range supportedInputs.SupportedFrames {
		if frame, ok := inputFrames[supportedFrameName]; ok {
			supportedFrames = append(supportedFrames, frame)
		}
	}

	return sdk.InputFrames(supportedFrames...)
}
