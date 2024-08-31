package action

import (
	"context"
	"fmt"
	"time"

	"google.golang.org/protobuf/types/known/anypb"

	"a.yandex-team.ru/alice/iot/bulbasaur/controller/action"
	"a.yandex-team.ru/alice/iot/bulbasaur/controller/experiments"
	"a.yandex-team.ru/alice/iot/bulbasaur/controller/scenario"
	"a.yandex-team.ru/alice/iot/bulbasaur/megamind"
	"a.yandex-team.ru/alice/iot/bulbasaur/megamind/common"
	"a.yandex-team.ru/alice/iot/bulbasaur/megamind/frames"
	scenariointent "a.yandex-team.ru/alice/iot/bulbasaur/megamind/intents/scenario"
	"a.yandex-team.ru/alice/iot/bulbasaur/megamind/intents/specification"
	"a.yandex-team.ru/alice/iot/bulbasaur/megamind/sdk"
	"a.yandex-team.ru/alice/iot/bulbasaur/model"
	"a.yandex-team.ru/alice/iot/bulbasaur/nlg"
	"a.yandex-team.ru/alice/library/go/inflector"
	"a.yandex-team.ru/alice/library/go/libmegamind"
	libnlg "a.yandex-team.ru/alice/library/go/nlg"
	"a.yandex-team.ru/alice/library/go/timestamp"
	"a.yandex-team.ru/alice/megamind/protos/scenarios"
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/log/ctxlog"
	"a.yandex-team.ru/library/go/core/xerrors"
)

// ToDo: @aaulayev support V2 version for action processor: IOT-1709
// TODO(aaulayev): change when v1 processor is removed
const processorName = "action_processor_v2"

var _ megamind.FrameRunApplyProcessor = &Processor{}
var _ megamind.SpecifySupportingProcessor = &Processor{}

type Processor struct {
	Logger log.Logger

	inflector          inflector.IInflector
	scenarioController *scenario.Controller
	actionController   action.IController
}

func NewProcessor(logger log.Logger, inflector inflector.IInflector, scenarioController *scenario.Controller, actionController action.IController) *Processor {
	processor := Processor{
		Logger:             logger,
		inflector:          inflector,
		scenarioController: scenarioController,
		actionController:   actionController,
	}

	return &processor
}

func (p *Processor) RunWithSpecifiedSlots(_ common.RunProcessorContext, _ ...libmegamind.Slot) (*scenarios.TScenarioRunResponse, error) {
	//TODO implement me
	panic("implement me")
}

func (p *Processor) Run(_ context.Context, _ libmegamind.SemanticFrame, _ *scenarios.TScenarioRunRequest, _ model.User) (*scenarios.TScenarioRunResponse, error) {
	//TODO implement me
	panic("implement me")
}

func (p *Processor) Name() string {
	return processorName
}

func (p *Processor) SupportedInputs() sdk.SupportedInputs {
	return sdk.SupportedInputs{
		SupportedFrames: []libmegamind.SemanticFrameName{
			frames.ActionCapabilityColorSettingFrameName,
			frames.ActionCapabilityModeFrameName,
			frames.ActionCapabilityOnOffFrameName,
			frames.ActionCapabilityRangeFrameName,
			frames.ActionCapabilityToggleFrameName,
			// frames.ActionCapabilityVideoStreamFrameName,
			frames.ActionCapabilityCustomButtonFrameName,
			// frames.DeviceActionTypedSemanticFrame,
		},
	}
}

func (p *Processor) CoolerRun(runContext sdk.RunContext, input sdk.Input) (*scenarios.TScenarioRunResponse, error) {
	processorIsEnabled := runContext.Request().GetBaseRequest().GetExperiments().GetFields()[megamind.EnableGranetProcessorsExp] != nil ||
		experiments.EnableGranetProcessors.IsEnabled(runContext.Context())
	if !processorIsEnabled {
		return sdk.IrrelevantResponse(
			runContext,
			fmt.Sprintf("action processor is disabled (missing %q experiment)", megamind.EnableGranetProcessorsExp),
		)
	}
	return p.RunWithSpecifiedSlotsV2(runContext, input)
}

func (p *Processor) RunWithSpecifiedSlotsV2(runContext sdk.RunContext, input sdk.Input, specifiedSlots ...sdk.GranetSlot) (*scenarios.TScenarioRunResponse, error) {
	runContext.Logger().Info("action processor's run",
		log.Any("run_context", runContext),
		log.Any("specified_slots", specifiedSlots),
	)

	silentResponseRequired := input.Type() == sdk.TypedSemanticFrameInputType

	// If something is wrong with the userInfo, we must detect this in the first place
	userInfo, err := runContext.UserInfo()
	if err != nil {
		runContext.Logger().Errorf("failed to get user info from context: %w", err)
		return nlgOrSilentRunResponse(runContext, nlg.CommonError, silentResponseRequired)
	}
	runContext.Logger().Info("user info extracted", log.Any("user_info", userInfo))
	runContext.Logger().Info("client info", log.Any("client_info", runContext.ClientInfo()))

	if noSmartHomeDevices(userInfo) {
		return nlgOrSilentRunResponse(runContext, nlg.NeedDevices, silentResponseRequired)
	}

	frame := frames.ActionFrameV2{}
	if err := frame.FromInput(input); err != nil {
		runContext.Logger().Errorf("failed unmarshal input: %w", err)
		return nlgOrSilentRunResponse(runContext, nlg.CommonError, silentResponseRequired)
	}
	runContext.Logger().Info("unmarshalled action frame", log.Any("action_frame", frame))

	if err := frame.AppendSlots(specifiedSlots...); err != nil {
		runContext.Logger().Errorf("failed to append specified slots: %w", err)
		return nlgOrSilentRunResponse(runContext, nlg.CommonError, silentResponseRequired)
	}
	if len(specifiedSlots) > 0 {
		runContext.Logger().Info("specifiedSlots appended successfully", log.Any("action_frame", frame))
	}

	frame.SetParsedTimeIfNotNow(runContext)
	frame.SetIntervalEndTime(runContext)

	if err := validateFrame(runContext, &frame); err != nil {
		if silentResponseRequired {
			// We don't use nlgWithSpecifyCallback when silent response is required,
			// because such requests are not voice requests from user, but TSFs from speakers.
			return nlgOrSilentRunResponse(runContext, nlg.CommonError, silentResponseRequired)
		}
		return p.validationErrorResponse(runContext, err)
	}

	extractionResult, err := frame.ExtractActionIntent(runContext)
	if err != nil {
		runContext.Logger().Error("error while extracting action intent", log.Error(err))
		return nlgOrSilentRunResponse(runContext, nlg.CommonError, silentResponseRequired)
	}
	if extractionResult.Status != frames.OkExtractionStatus {
		return p.extractionErrorResponse(runContext, extractionResult)
	}
	ctxlog.Debug(runContext.Context(), runContext.Logger().InternalLogger(), "action intent extracted", log.Any("action_intent", extractionResult))

	var applyArguments = ApplyArguments{
		NLG:                    p.nlgFromExtractionResult(runContext, extractionResult, frame),
		SilentResponseRequired: silentResponseRequired,
		IntentParametersSlot:   extractionResult.IntentParametersSlot,
		Devices:                extractionResult.Devices,
		CreatedTime:            extractionResult.CreatedTime,
		ValueSlot:              newAAValueSlot(extractionResult.ValueSlot),
		RequestedTime:          extractionResult.RequestedTime,
		IntervalEndTime:        extractionResult.IntervalEndTime,
	}
	runContext.Logger().Info(
		"action apply arguments constructed",
		log.Any("nlg", applyArguments.NLG),
		log.Bool("silent_response_required", applyArguments.SilentResponseRequired),
		log.Strings("devices", applyArguments.Devices.GetIDs()),
		log.Any("intent_parameters_slot", applyArguments.IntentParametersSlot),
		log.Time("created_time", applyArguments.CreatedTime),
		log.Any("value_slot", applyArguments.ValueSlot),
		log.Time("requested_time", applyArguments.RequestedTime),
		log.Time("interval_end_time", applyArguments.IntervalEndTime),
	)

	return sdk.RunApplyResponse(&applyArguments)
}

func validateFrame(runContext sdk.RunContext, actionFrame *frames.ActionFrameV2) error {
	if err := actionFrame.ValidateAllDevicesRequired(); err != nil {
		return err
	}

	if err := actionFrame.ValidateVideoStreamCapability(runContext); err != nil {
		return err
	}

	if err := actionFrame.ValidateUnknownNumMode(); err != nil {
		return err
	}

	if actionFrame.ContainsDateTime() {
		if err := actionFrame.ValidateBegemotDateTime(); err != nil {
			return err
		}

		timestamper, err := timestamp.TimestamperFromContext(runContext.Context())
		if err != nil {
			return xerrors.Errorf("failed to get timestamper from context: %w", err)
		}
		if err := actionFrame.ValidateParsedTime(runContext.ClientInfo().LocalTime(timestamper.CreatedTimestamp().AsTime())); err != nil {
			return err
		}
	}

	if err := actionFrame.ValidateHouseholds(); err != nil {
		return err
	}

	return nil
}

func (p *Processor) extractionErrorResponse(runContext sdk.RunContext, extractionResult frames.ActionFrameExtractionResult) (*scenarios.TScenarioRunResponse, error) {
	runContext.Logger().Info(
		"extraction status is not ok",
		log.Any("extraction_status", extractionResult.Status),
	)

	userInfo, _ := runContext.UserInfo()

	if extractionResult.Status == frames.MultipleHouseholdsExtractionStatus {
		runContext.Logger().Info("asking to specify household")
		return nlgWithSpecifyCallback(runContext, specifyRequestParams{
			callbackActions: specification.HouseholdSpecificationCallbacks(userInfo.Households, p.inflector),
			responseNLG:     nlg.NoHouseholdSpecifiedAction,
		})
	}

	// https://st.yandex-team.ru/DIALOG-5842
	if extractionResult.Status == frames.ShortCommandWithMultiplePossibleRooms {
		runContext.Logger().Info(
			"there are multiple devices in different rooms matched to the short command, returning irrelevant",
			log.Any("device_ids", extractionResult.Devices.GetIDs()),
		)
		return sdk.IrrelevantResponse(runContext, "short command to devices in different rooms")
	}

	if extractionResult.Status == frames.RequestToTandem {
		return sdk.IrrelevantResponse(runContext, "request to tandem, returning irrelevant")
	}

	return sdk.RunResponse(runContext).WithNLG(extractionResult.FailureNLG).Build()
}

func (p *Processor) Apply(ctx context.Context, applyRequest *scenarios.TScenarioApplyRequest, user model.User) (*scenarios.TScenarioApplyResponse, error) {
	applyContext := sdk.NewApplyContext(ctx, p.Logger, applyRequest, user)
	applyArguments := ApplyArguments{}
	if err := sdk.UnmarshalApplyArguments(applyRequest.GetArguments(), &applyArguments); err != nil {
		return nil, xerrors.Errorf("failed to unmarshal apply arguments: %w", err)
	}

	return p.apply(applyContext, applyArguments)
}

func (p *Processor) apply(applyContext sdk.ApplyContext, applyArguments ApplyArguments) (*scenarios.TScenarioApplyResponse, error) {
	applyContext.Logger().Info(
		"action processor's apply",
		log.Any("nlg", applyArguments.NLG),
		log.Bool("silent_response_required", applyArguments.SilentResponseRequired),
		log.Strings("devices", applyArguments.Devices.GetIDs()),
		log.Any("intent_parameters_slot", applyArguments.IntentParametersSlot),
		log.Time("created_time", applyArguments.CreatedTime),
		log.Any("value_slot", applyArguments.ValueSlot),
		log.Time("requested_time", applyArguments.RequestedTime),
		log.Time("interval_end_time", applyArguments.IntervalEndTime),
		log.Bool("silent_response_required", applyArguments.SilentResponseRequired),
	)

	if _, ok := applyContext.User(); !ok {
		applyContext.Logger().Error("failed to get user from context")
		return nlgOrSilentApplyResponse(applyContext, nlg.CommonError, applyArguments.SilentResponseRequired, nil, nil)
	}

	if !applyArguments.RequestedTime.IsZero() {
		applyContext.Logger().Info("applying delayed action")
		return p.applyDelayedAction(applyContext, applyArguments)
	}

	return p.applyInstantAction(applyContext, applyArguments)
}

func (p *Processor) IsApplicable(applyArguments *anypb.Any) bool {
	return sdk.IsApplyArguments(applyArguments, &ApplyArguments{})
}

func (p *Processor) validationErrorResponse(runContext sdk.RunContext, err error) (*scenarios.TScenarioRunResponse, error) {
	switch {
	case xerrors.Is(err, common.TurnOnEverythingIsForbiddenValidationError):
		validationError, _ := err.(common.ValidationError)
		runContext.Logger().Infof("frame validation failed: %s", validationError.Description)
		return sdk.RunResponse(runContext).WithNLG(validationError.NLG()).Build()
	case xerrors.Is(err, common.TimeIsNotSpecifiedValidationError), xerrors.Is(err, common.WeirdTimeRelativityValidationError):
		validationError, _ := err.(common.ValidationError)
		runContext.Logger().Infof("begemot datetime validation failed: time specification required: %s", validationError.Description)
		return nlgWithSpecifyCallback(runContext, specifyRequestParams{
			callbackActions: []libmegamind.CallbackFrameAction{specification.TimeSpecifyEmptyCallback()},
			responseNLG:     validationError.NLG(),
		})
	case xerrors.Is(err, common.PastActionValidationError), xerrors.Is(err, common.FarFutureValidationError):
		validationError, _ := err.(common.ValidationError)
		runContext.Logger().Infof("parsed datetime validation failed: %s", validationError.Description)
		return sdk.RunResponse(runContext).WithNLG(validationError.NLG()).Build()
	case xerrors.Is(err, common.MultipleHouseholdsInRequestValidationError):
		validationError, _ := err.(common.ValidationError)
		runContext.Logger().Infof("household validation failed: asking to specify household: %s", validationError.Description)
		userInfo, _ := runContext.UserInfo()
		return nlgWithSpecifyCallback(runContext, specifyRequestParams{
			callbackActions: specification.HouseholdSpecificationCallbacks(userInfo.Households, p.inflector),
			responseNLG:     validationError.NLG(),
		})
	case xerrors.Is(err, common.CannotPlayVideoOnDeviceValidationError), xerrors.Is(err, common.TVIsNotPluggedValidationError):
		validationError, _ := err.(common.ValidationError)
		runContext.Logger().Infof("video stream validation failed: %s", validationError.Description)
		return sdk.RunResponse(runContext).WithNLG(validationError.NLG()).Build()
	case xerrors.Is(err, common.CannotPlayVideoStreamInIotAppValidationError):
		validationError, _ := err.(common.ValidationError)
		runContext.Logger().Infof("video stream validation failed: %s", validationError.Description)
		return sdk.IrrelevantResponse(runContext, string(megamind.ProcessorNotFound))
	case xerrors.Is(err, common.UnknownModeValidationError):
		validationError, _ := err.(common.ValidationError)
		runContext.Logger().Infof("frame validation failed: %s", validationError.Description)
		return sdk.RunResponse(runContext).WithNLG(validationError.NLG()).Build()
	default:
		return sdk.CommonErrorRunResponseBuilder(runContext, xerrors.Errorf("unknown validation error: %v", err)), nil
	}
}

func (p *Processor) nlgFromExtractionResult(runContext sdk.RunContext, extractionResult frames.ActionFrameExtractionResult, frame frames.ActionFrameV2) libnlg.NLG {
	clientLocation := runContext.ClientInfo().GetLocation(model.DefaultTimezone)
	if !extractionResult.RequestedTime.IsZero() {
		return nlg.DelayedAction(extractionResult.CreatedTime, extractionResult.RequestedTime, clientLocation)
	}

	if !extractionResult.IntervalEndTime.IsZero() {
		return customIntervalActionNLG(extractionResult.Devices, extractionResult.CreatedTime, extractionResult.IntervalEndTime, clientLocation)
	}

	return nlgByIntentParameters(runContext, extractionResult.Devices, extractionResult.IntentParametersSlot, frame, p.inflector)
}

func (p *Processor) applyInstantAction(applyContext sdk.ApplyContext, applyArguments ApplyArguments) (*scenarios.TScenarioApplyResponse, error) {
	origin, ok := applyContext.Origin()
	if !ok {
		applyContext.Logger().Error("failed to get origin from context")
		return nlgOrSilentApplyResponse(applyContext, nlg.CommonError, applyArguments.SilentResponseRequired, nil, nil)
	}

	sideEffects := p.actionController.SendActionsToDevices(applyContext.Context(), origin, applyArguments.Devices)
	analyticsInfo := analyticsInfoFromApplyArguments(applyArguments)

	// If some directives must be sent back, responding instantly
	if len(sideEffects.Directives) > 0 {
		if !applyArguments.IntervalEndTime.IsZero() {
			return p.postProcessIntervalAction(applyContext, applyArguments, sideEffects.Directives...)
		}
		applyContext.Logger().Info(
			"responding with directives",
			log.Any("directives", sideEffects.Directives),
			log.Any("nlg", applyArguments.NLG),
		)
		return nlgOrSilentApplyResponse(
			applyContext,
			applyArguments.NLG,
			applyArguments.SilentResponseRequired,
			analyticsInfo,
			sideEffects.Directives,
		)
	}

	// If there's no directives, waiting for providers' answers for some time.
	timeout, err := timestamp.ComputeTimeout(applyContext.Context(), megamind.MMApplyTimeout)
	if err != nil {
		timeout = megamind.DefaultApplyTimeout
	}

	select {
	case devicesResult := <-sideEffects.Result():
		applyContext.Logger().Info("received devices result from providers", log.Any("devices_result", devicesResult))
		if !applyArguments.IntervalEndTime.IsZero() {
			return p.postProcessIntervalAction(applyContext, applyArguments)
		}
		if cameraDirective, ok := findCameraResult(applyArguments.Devices, devicesResult.ProviderResults); ok {
			applyContext.Logger().Info("responding with camera directive", log.Any("camera_directive", cameraDirective.ToTDirective()))
			return nlgOrSilentApplyResponse(
				applyContext,
				applyArguments.NLG,
				applyArguments.SilentResponseRequired,
				analyticsInfo,
				[]*scenarios.TDirective{cameraDirective.ToTDirective()},
			)
		}
		responseNLG := nlgFromDevicesResult(applyArguments.NLG, devicesResult)
		return nlgOrSilentApplyResponse(
			applyContext,
			responseNLG,
			applyArguments.SilentResponseRequired,
			analyticsInfo,
			nil,
		)
	case <-time.After(timeout):
		applyContext.Logger().Info("exiting apply by timeout", log.Duration("timeout", timeout))
		if !applyArguments.IntervalEndTime.IsZero() {
			return p.postProcessIntervalAction(applyContext, applyArguments)
		}
		return nlgOrSilentApplyResponse(
			applyContext,
			nlg.OptimisticOK,
			applyArguments.SilentResponseRequired,
			analyticsInfo,
			nil,
		)
	}
}

func (p *Processor) applyDelayedAction(applyContext sdk.ApplyContext, applyArguments ApplyArguments, directives ...*scenarios.TDirective) (*scenarios.TScenarioApplyResponse, error) {
	user, _ := applyContext.User()
	scheduledScenarioID, err := p.scenarioController.CreateScheduledScenarioLaunch(applyContext.Context(), user.GetID(),
		applyArguments.ToTimerScenarioLaunch())
	if err != nil {
		applyContext.Logger().Errorf("failed to create scheduled scenario launch: %v", err)
		return nlgOrSilentApplyResponse(applyContext, nlg.CommonError, applyArguments.SilentResponseRequired, nil, nil)
	}

	callback := scenariointent.GetCancelScenarioLaunchCallback(scheduledScenarioID)
	responseNLG := applyArguments.NLG
	if applyArguments.SilentResponseRequired {
		responseNLG = nil
	}

	// TODO(aaulayev): time info (https://st.yandex-team.ru/IOT-1547)
	return sdk.ApplyResponse(applyContext).
		WithNLG(responseNLG).
		WithCallbackFrameActions(callback).
		WithDirectives(directives...).
		WithAnalyticsInfo(analyticsInfoFromApplyArguments(applyArguments)).
		Build()
}

func (p *Processor) postProcessIntervalAction(applyContext sdk.ApplyContext, applyArguments ApplyArguments, directives ...*scenarios.TDirective) (*scenarios.TScenarioApplyResponse, error) {
	// Invert states
	applyArguments, err := applyArguments.InvertStates()
	if err != nil {
		return nlgOrSilentApplyResponse(applyContext, nlg.CannotDo, applyArguments.SilentResponseRequired, nil, nil)
	}
	applyArguments.RequestedTime = applyArguments.IntervalEndTime

	// Schedule reversed action
	applyContext.Logger().Info("applying inverted delayed action", log.Any("apply_arguments", applyArguments))
	return p.applyDelayedAction(applyContext, applyArguments, directives...)
}

type specifyRequestParams struct {
	callbackActions []libmegamind.CallbackFrameAction
	responseNLG     libnlg.NLG
}

func nlgWithSpecifyCallback(runContext sdk.RunContext, params specifyRequestParams) (*scenarios.TScenarioRunResponse, error) {
	specifyRequestState := common.SpecifyRequestState{
		SemanticFrames: runContext.Request().Input.GetSemanticFrames(),
	}
	stateProto, err := anypb.New(specifyRequestState.ToProto())
	if err != nil {
		return nil, xerrors.Errorf("failed to make specify request state: %w", err)
	}

	return sdk.RunResponse(runContext).
		WithCallbackFrameActions(params.callbackActions...).
		WithNLG(params.responseNLG).
		WithState(stateProto).
		ExpectRequest().
		Build()
}

func nlgOrSilentRunResponse(runContext sdk.RunContext, n libnlg.NLG, isSilent bool) (*scenarios.TScenarioRunResponse, error) {
	if isSilent {
		return sdk.RunResponse(runContext).Build()
	}
	return sdk.RunResponse(runContext).WithNLG(n).Build()
}

func nlgOrSilentApplyResponse(applyContext sdk.ApplyContext, n libnlg.NLG, isSilent bool, analyticsInfo sdk.AnalyticsInfoBuilder,
	directives []*scenarios.TDirective) (*scenarios.TScenarioApplyResponse, error) {
	if isSilent {
		return sdk.ApplyResponse(applyContext).WithDirectives(directives...).Build()
	}
	return sdk.ApplyResponse(applyContext).WithDirectives(directives...).WithNLG(n).WithAnalyticsInfo(analyticsInfo).Build()
}

func findCameraResult(devices model.Devices, devicesResults []action.ProviderDevicesResult) (VideoPlayDirective, bool) {
	devicesMap := devices.ToMap()

	for _, providerResults := range devicesResults {
		for _, deviceResult := range providerResults.DeviceResults {
			cameraCapability, ok := deviceResult.UpdatedCapabilities.GetCapabilityByTypeAndInstance(
				model.VideoStreamCapabilityType,
				model.GetStreamCapabilityInstance.String(),
			)
			if !ok {
				continue
			}

			camera, ok := devicesMap[deviceResult.ID]
			if !ok {
				continue
			}

			state := cameraCapability.State().(model.VideoStreamCapabilityState)

			videoPlayDirective := VideoPlayDirective{
				URL:         state.Value.StreamURL,
				Name:        camera.Name,
				Description: camera.Room.GetName(),
				Protocol:    state.Value.Protocol,
			}

			return videoPlayDirective, true
		}
	}

	return VideoPlayDirective{}, false
}

func analyticsInfoFromApplyArguments(applyArguments ApplyArguments) sdk.AnalyticsInfoBuilder {
	return sdk.AnalyticsInfo().WithAction(sdk.ActionAnalyticsInfo{
		CapabilityType:     applyArguments.IntentParametersSlot.CapabilityType,
		CapabilityInstance: applyArguments.IntentParametersSlot.CapabilityInstance,
		CapabilityValue:    stringFromValueSlot(applyArguments.ValueSlot),
		RelativityType:     applyArguments.IntentParametersSlot.RelativityType,
		DeviceIDs:          applyArguments.Devices.GetIDs(),
		IntervalStartTime:  applyArguments.CreatedTime,
		IntervalEndTime:    applyArguments.IntervalEndTime,
		RequestedTime:      applyArguments.RequestedTime,
	})
}

func stringFromValueSlot(valueSlot ApplyArgumentsValueSlot) string {
	switch {
	case valueSlot.OnOff != nil:
		return fmt.Sprintf("%t", valueSlot.OnOff.Value)
	case valueSlot.Toggle != nil:
		return fmt.Sprintf("%t", valueSlot.Toggle.Value)
	case valueSlot.Mode != nil:
		return string(valueSlot.Mode.ModeValue)
	case valueSlot.Range != nil:
		switch libmegamind.SlotType(valueSlot.Range.Type()) {
		case frames.NumSlotType:
			return fmt.Sprintf("%d", valueSlot.Range.NumValue)
		case frames.StringSlotType:
			return valueSlot.Range.StringValue
		}
	case valueSlot.ColorSetting != nil:
		switch libmegamind.SlotType(valueSlot.ColorSetting.Type()) {
		case frames.ColorSlotType:
			return string(valueSlot.ColorSetting.Color)
		case frames.ColorSceneSlotType:
			return string(valueSlot.ColorSetting.ColorScene)
		}
	case valueSlot.CustomButtonInstance != nil:
		if len(valueSlot.CustomButtonInstance.Instances) == 0 {
			return ""
		}
		return string(valueSlot.CustomButtonInstance.Instances[0])
	}
	return ""
}

func noSmartHomeDevices(userInfo model.UserInfo) bool {
	return (len(userInfo.Devices) == 0 || userInfo.Devices.ContainsOnlySpeakerDevices()) &&
		len(userInfo.Devices.FilterByDeviceTypes(model.DeviceTypes{model.YandexStationMidiDeviceType})) == 0
}
