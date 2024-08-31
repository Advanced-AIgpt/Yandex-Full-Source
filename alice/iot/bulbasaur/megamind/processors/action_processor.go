package processors

import (
	"context"
	"fmt"
	"time"

	"google.golang.org/protobuf/types/known/anypb"

	"a.yandex-team.ru/alice/iot/bulbasaur/controller/action"
	"a.yandex-team.ru/alice/iot/bulbasaur/controller/scenario"
	"a.yandex-team.ru/alice/iot/bulbasaur/dto/adapter"
	"a.yandex-team.ru/alice/iot/bulbasaur/logging/doublelog"
	"a.yandex-team.ru/alice/iot/bulbasaur/megamind"
	"a.yandex-team.ru/alice/iot/bulbasaur/megamind/arguments"
	"a.yandex-team.ru/alice/iot/bulbasaur/megamind/common"
	"a.yandex-team.ru/alice/iot/bulbasaur/megamind/frames"
	"a.yandex-team.ru/alice/iot/bulbasaur/megamind/sdk"
	"a.yandex-team.ru/alice/iot/bulbasaur/model"
	"a.yandex-team.ru/alice/iot/bulbasaur/nlg"
	"a.yandex-team.ru/alice/iot/bulbasaur/protos"
	"a.yandex-team.ru/alice/library/go/inflector"
	"a.yandex-team.ru/alice/library/go/libmegamind"
	libnlg "a.yandex-team.ru/alice/library/go/nlg"
	"a.yandex-team.ru/alice/library/go/timestamp"
	commonpb "a.yandex-team.ru/alice/megamind/protos/common"
	"a.yandex-team.ru/alice/megamind/protos/scenarios"
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/log/ctxlog"
	"a.yandex-team.ru/library/go/core/xerrors"
)

const ActionProcessorName megamind.FrameProcessorName = "action_processor"

var _ megamind.FrameRunApplyProcessor = &ActionProcessor{}
var _ megamind.SpecifySupportingProcessor = &ActionProcessor{}

type ActionProcessor struct {
	Logger log.Logger

	inflector          inflector.IInflector
	scenarioController *scenario.Controller
	actionController   action.IController
}

func (a *ActionProcessor) Name() string {
	return string(ActionProcessorName)
}

func (a *ActionProcessor) SupportedInputs() sdk.SupportedInputs {
	return sdk.SupportedInputs{
		SupportedFrames: []libmegamind.SemanticFrameName{
			frames.ActionCapabilityVideoStreamFrameName,
			frames.DeviceActionTypedSemanticFrame,
		},
	}
}

func (a *ActionProcessor) Run(ctx context.Context, frame libmegamind.SemanticFrame,
	runRequest *scenarios.TScenarioRunRequest, user model.User) (*scenarios.TScenarioRunResponse, error) {
	processorContext, err := common.NewRunProcessorContextFromRequest(ctx, runRequest, frame)
	if err != nil {
		doublelog.Error(ctx, a.Logger, "failed to get user info from context", log.Error(err))
		isSilent := frames.IsTypedSemanticFrame(libmegamind.SemanticFrameName(frame.Name()))
		return nlgOrSilentRunResponse(ctx, a.Logger, nlg.CommonError, isSilent)
	}

	doublelog.Info(ctx, a.Logger, "user info extracted from datasource", log.Any("user_info", processorContext.UserInfo))

	clientInfo := common.NewClientInfo(runRequest.GetBaseRequest().GetClientInfo(), runRequest.DataSources)
	doublelog.Info(ctx, a.Logger, "client info extracted", log.Any("client_info", clientInfo))

	return a.RunWithSpecifiedSlots(processorContext)
}

func (a *ActionProcessor) CoolerRun(runContext sdk.RunContext, input sdk.Input) (*scenarios.TScenarioRunResponse, error) {
	return a.RunWithSpecifiedSlotsV2(runContext, input)
}

func (a *ActionProcessor) RunWithSpecifiedSlots(processorContext common.RunProcessorContext, specifiedSlots ...libmegamind.Slot) (*scenarios.TScenarioRunResponse, error) {
	doublelog.Info(
		processorContext.Context, a.Logger,
		fmt.Sprintf("%s's run stage is running", ActionProcessorName),
		log.Any("specified_slots", specifiedSlots),
	)

	silentResponseRequired := frames.IsTypedSemanticFrame(libmegamind.SemanticFrameName(processorContext.SemanticFrame.Name()))
	actionFrame, err := frames.NewActionFrameWithDeduction(processorContext, a.Logger, specifiedSlots...)
	if err != nil {
		doublelog.Error(processorContext.Context, a.Logger, "failed to construct action frame", log.Error(err))
		return nlgOrSilentRunResponse(processorContext.Context, a.Logger, nlg.CommonError, silentResponseRequired)
	}
	doublelog.Info(processorContext.Context, a.Logger, "selected frame parsed to ActionFrame", log.Any("action_frame", actionFrame))

	if validationOk, runResponse, runResponseError := a.Validate(processorContext, actionFrame); !validationOk {
		return runResponse, runResponseError
	}

	extractedActionIntent, filtrationResult, err := actionFrame.ExtractActionIntent(processorContext)
	if err != nil {
		doublelog.Error(processorContext.Context, a.Logger, "error while extracting action intent", log.Error(err))
		return nlgOrSilentRunResponse(processorContext.Context, a.Logger, nlg.CommonError, silentResponseRequired)
	}
	doublelog.Info(processorContext.Context, a.Logger, "action intent extracted", log.Any("extracted_action_intent", extractedActionIntent))

	if filtrationResult.Reason != common.AllGoodFiltrationReason {
		doublelog.Info(processorContext.Context, a.Logger, "all devices filtered out", log.Any("filtration_reason", filtrationResult.Reason))
		return nlgOrSilentRunResponse(processorContext.Context, a.Logger, filtrationResult.Reason.NLG(), silentResponseRequired)
	}

	var applyArguments = arguments.ActionIntentApplyArguments{
		ExtractedActionIntent:  extractedActionIntent,
		SilentResponseRequired: silentResponseRequired,
	}

	applyArgumentsProto := &protos.TApplyArguments{
		Value: &protos.TApplyArguments_ActionIntentApplyArguments{
			ActionIntentApplyArguments: applyArguments.ToProto(processorContext.Context),
		},
	}

	return megamind.BuildRunResponse(processorContext.Context, a.Logger, applyArgumentsProto)
}

func (a *ActionProcessor) RunWithSpecifiedSlotsV2(runContext sdk.RunContext, input sdk.Input, specifiedSlots ...sdk.GranetSlot) (*scenarios.TScenarioRunResponse, error) {
	return sdk.CommonErrorRunResponseBuilder(runContext, xerrors.New("action processor is not cool enough to reach this code")), nil
}

func (a *ActionProcessor) Validate(processorContext common.RunProcessorContext, actionFrame *frames.ActionFrame) (bool, *scenarios.TScenarioRunResponse, error) {
	// Frame validation
	if len(actionFrame.DeviceIDs) == 0 {
		response, responseError := megamind.NlgRunResponse(processorContext.Context, a.Logger, nlg.CannotFindDevices)
		return false, response, responseError
	}

	// Datetime validation
	if !actionFrame.Time.IsZero() || !actionFrame.Date.IsZero() {
		if !actionFrame.SupportsTimeSpecification() {
			response, responseError := megamind.NlgRunResponse(processorContext.Context, a.Logger, nlg.CommonError)
			return false, response, responseError
		}

		if begemotDateTimeCheckInfo := a.checkBegemotDateTime(processorContext, *actionFrame); !begemotDateTimeCheckInfo.ok {
			return false, begemotDateTimeCheckInfo.runResponse, begemotDateTimeCheckInfo.runResponseError
		}

		timestamper, err := timestamp.TimestamperFromContext(processorContext.Context)
		if err != nil {
			ctxlog.Warn(processorContext.Context, a.Logger, "no timestamper in the context")
			response, responseError := megamind.NlgRunResponse(processorContext.Context, a.Logger, nlg.CommonError)
			return false, response, responseError
		}

		clientTime := processorContext.ClientInfo.LocalTime(timestamper.CreatedTimestamp().AsTime())
		parsedTime := common.ParseBegemotDateAndTime(clientTime, actionFrame.Date, actionFrame.Time)
		ctxlog.Info(processorContext.Context, a.Logger, "begemot datetime parsed", log.Time("parsed_time", parsedTime))
		if !parsedTime.Equal(clientTime) {
			actionFrame.SetParsedTime(parsedTime)
		}

		if parsedTimeCheckInfo := a.checkParsedTime(processorContext, *actionFrame, clientTime); !parsedTimeCheckInfo.ok {
			return false, parsedTimeCheckInfo.runResponse, parsedTimeCheckInfo.runResponseError
		}
	}
	doublelog.Info(processorContext.Context, a.Logger, "datetime validation passed successfully")

	// Household validation
	if householdCheckInfo := a.checkHouseholds(processorContext, *actionFrame); !householdCheckInfo.ok {
		return false, householdCheckInfo.runResponse, householdCheckInfo.runResponseError
	}

	// Video stream capability validation
	if actionFrame.IntentParameters.CapabilityType == model.VideoStreamCapabilityType && actionFrame.IntentParameters.CapabilityInstance == string(model.GetStreamCapabilityInstance) {
		if clientCheckInfo := a.checkClientCanPlayStream(processorContext); !clientCheckInfo.ok {
			return false, clientCheckInfo.runResponse, clientCheckInfo.runResponseError
		}
	}

	return true, nil, nil
}

type checkStatus struct {
	ok               bool
	runResponse      *scenarios.TScenarioRunResponse
	runResponseError error
}

// checkBegemotDateTime performs first-phase validation. Granet-related bugs are checked
func (a *ActionProcessor) checkBegemotDateTime(processorContext common.RunProcessorContext, actionFrame frames.ActionFrame) checkStatus {
	var (
		response      *scenarios.TScenarioRunResponse
		responseError error
	)

	ctxlog.Info(processorContext.Context, a.Logger, "validating begemot date and time",
		log.Any("begemot_date", actionFrame.Date), log.Any("begemot_time", actionFrame.Time))
	if err := actionFrame.ValidateBegemotDateTime(); err != nil {
		var validationError common.ValidationError
		switch {
		case xerrors.As(err, &validationError):
			doublelog.Info(processorContext.Context, a.Logger, "begemot datetime validation failed: time specification required",
				log.Any("validation_result", validationError.Description))
			response, responseError = nlgWithSpecifyCallback(processorContext, specifyRequestParams{
				logger:          a.Logger,
				callbackActions: []libmegamind.CallbackFrameAction{megamind.GetTimeSpecifyEmptyCallback()},
				responseNLG:     validationError.NLG(),
			})
		default:
			doublelog.Infof(processorContext.Context, a.Logger, "unknown datetime validation error: %s", err)
			response, responseError = megamind.NlgRunResponse(processorContext.Context, a.Logger, nlg.CommonError)
		}

		return checkStatus{
			ok:               false,
			runResponse:      response,
			runResponseError: responseError,
		}
	}

	return checkStatus{
		ok:               true,
		runResponse:      nil,
		runResponseError: nil,
	}
}

// checkParsedTime performs second-phase validation. The time itself is checked.
func (a *ActionProcessor) checkParsedTime(processorContext common.RunProcessorContext, actionFrame frames.ActionFrame, clientTime time.Time) checkStatus {
	var (
		response      *scenarios.TScenarioRunResponse
		responseError error
	)

	if err := actionFrame.ValidateParsedDateTime(clientTime); err != nil {
		var validationError common.ValidationError

		switch {
		case xerrors.As(err, &validationError):
			doublelog.Info(processorContext.Context, a.Logger, "parsed datetime validation failed",
				log.String("validation_result", validationError.Description))
			responseNLG := validationError.NLG()
			response, responseError = megamind.NlgRunResponse(processorContext.Context, a.Logger, responseNLG)
		default:
			doublelog.Infof(processorContext.Context, a.Logger, "unknown datetime validation error: %s", err)
			response, responseError = megamind.NlgRunResponse(processorContext.Context, a.Logger, nlg.CommonError)
		}

		return checkStatus{
			ok:               false,
			runResponse:      response,
			runResponseError: responseError,
		}
	}

	return checkStatus{
		ok:               true,
		runResponse:      nil,
		runResponseError: nil,
	}
}

func (a *ActionProcessor) checkHouseholds(processorContext common.RunProcessorContext, actionFrame frames.ActionFrame) checkStatus {
	var (
		response      *scenarios.TScenarioRunResponse
		responseError error
	)

	if err := actionFrame.ValidateHouseholds(processorContext.UserInfo); err != nil {
		var householdValidationError common.ValidationError
		switch {
		case xerrors.As(err, &householdValidationError):
			doublelog.Infof(processorContext.Context, a.Logger, "asking to specify household, household validation result: %s",
				householdValidationError.Description)
			responseNLG := householdValidationError.NLG()

			response, responseError = nlgWithSpecifyCallback(processorContext, specifyRequestParams{
				logger:          a.Logger,
				callbackActions: megamind.GetHouseholdSpecifyCallbacks(processorContext.UserInfo.Households, a.inflector),
				responseNLG:     responseNLG,
			})
		default:
			doublelog.Infof(processorContext.Context, a.Logger, "unknown household validation error: %s", err)
			response, responseError = megamind.NlgRunResponse(processorContext.Context, a.Logger, nlg.CommonError)
		}

		return checkStatus{
			ok:               false,
			runResponse:      response,
			runResponseError: responseError,
		}
	}

	return checkStatus{
		ok:               true,
		runResponse:      nil,
		runResponseError: nil,
	}
}

func (a *ActionProcessor) checkClientCanPlayStream(processorContext common.RunProcessorContext) checkStatus {
	if processorContext.ClientInfo.IsTandem() {
		return checkStatus{ok: true}
	}

	if processorContext.ClientInfo.IsIotApp() {
		// TODO(akastornov): support iot app
		response, err := megamind.Irrelevant(processorContext.Context, a.Logger, megamind.ProcessorNotFound)
		return checkStatus{ok: false, runResponse: response, runResponseError: err}
	}

	if len(processorContext.ClientInfo.GetSupportedVideoStreamProtocols()) == 0 {
		response, err := megamind.NlgRunResponse(processorContext.Context, a.Logger, nlg.CantPlayVideoOnDevice)
		return checkStatus{ok: false, runResponse: response, runResponseError: err}
	}

	if !processorContext.Interfaces.IsTvPlugged {
		response, err := megamind.NlgRunResponse(processorContext.Context, a.Logger, nlg.TvIsNotPluggedIn)
		return checkStatus{ok: false, runResponse: response, runResponseError: err}
	}

	return checkStatus{ok: true}
}

func (a *ActionProcessor) Apply(ctx context.Context, applyRequest *scenarios.TScenarioApplyRequest, user model.User) (*scenarios.TScenarioApplyResponse, error) {
	doublelog.Info(
		ctx, a.Logger,
		fmt.Sprintf("%s's apply stage is running", ActionProcessorName),
		log.Any("user", user),
	)

	applyArgumentsProto, err := extractActionIntentApplyArguments(applyRequest.GetArguments())
	if err != nil {
		return megamind.ErrorApplyResponse(ctx, a.Logger, xerrors.Errorf("failed to extract apply arguments: %w", err))
	}
	applyArguments := arguments.ActionIntentApplyArguments{}
	applyArguments.FromProto(applyArgumentsProto)

	actionIntent := applyArguments.ExtractedActionIntent
	doublelog.Info(ctx, a.Logger, "action intent extracted", log.Any("action_intent", actionIntent))

	intentContext := common.NewActionIntentContext(ctx, actionIntent, user)
	clientInfo := common.NewClientInfo(applyRequest.GetBaseRequest().GetClientInfo(), nil)

	if !actionIntent.RequestedTime.IsZero() {
		ctxlog.Debug(ctx, a.Logger, "applying delayed action", log.Any("action_intent", actionIntent))
		return a.applyDelayedAction(intentContext, clientInfo)
	}

	origin := common.NewOrigin(ctx, clientInfo.ClientInfo, user)
	doublelog.Info(ctx, a.Logger, "sending actions",
		log.Any("origin", origin),
		log.Any("action_intent", actionIntent),
	)

	timeout, err := timestamp.ComputeTimeout(intentContext.Context, megamind.MMApplyTimeout)
	if err != nil {
		timeout = megamind.DefaultApplyTimeout
	}
	// ToDo: @aaulayev migrate to v2 action IOT-1709
	sideEffects := a.actionController.SendActionsToDevices(intentContext.Context, origin, intentContext.ActionIntent.Devices)
	if len(sideEffects.Directives) > 0 {
		outputResponse := libmegamind.OutputResponse{NLG: nlg.OptimisticOK}
		if applyArguments.SilentResponseRequired {
			outputResponse.NLG = libnlg.SilentResponse
		}
		outputResponse.AddDirectives(sideEffects.Directives)
		return megamind.NlgApplyResponse(ctx, a.Logger, outputResponse, nil)
	}

	select {
	case devicesResult := <-sideEffects.Result():
		if cameraResult, ok := findCameraResult(actionIntent.Devices, devicesResult.ProviderResults); ok {
			return a.applyResponseWithVideoPlayDirective(intentContext.Context, a.Logger, cameraResult)
		}
		responseNLG := a.makeOutputNLG(intentContext.Context, devicesResult)
		// TODO(aaulayev): analytics info
		return nlgOrSilentApplyResponse(intentContext.Context, a.Logger, responseNLG, applyArguments.SilentResponseRequired)
	case <-time.After(timeout):
		doublelog.Info(ctx, a.Logger, "exiting apply by timeout", log.Duration("timeout", timeout))
		return nlgOrSilentApplyResponse(intentContext.Context, a.Logger, nlg.OptimisticOK, applyArguments.SilentResponseRequired)
	}
}

func (a *ActionProcessor) IsApplicable(applyArguments *anypb.Any) bool {
	if _, err := extractActionIntentApplyArguments(applyArguments); err != nil {
		return false
	}
	return true
}

func (a *ActionProcessor) applyDelayedAction(intentContext common.ActionIntentContext, clientInfo common.ClientInfo) (*scenarios.TScenarioApplyResponse, error) {
	scheduledScenarioID, err := a.scenarioController.CreateScheduledScenarioLaunch(intentContext.Context,
		intentContext.User.GetID(), intentContext.ActionIntent.ToScenarioLaunch())
	if err != nil {
		return megamind.ErrorApplyResponse(intentContext.Context, a.Logger, err)
	}

	callback := megamind.GetCancelScenarioLaunchCallback(scheduledScenarioID)

	outputResponse := libmegamind.OutputResponse{
		NLG:      nlg.DelayedAction(time.Now(), intentContext.ActionIntent.RequestedTime, clientInfo.GetLocation(model.DefaultTimezone)),
		Callback: &callback,
	}
	return megamind.NlgApplyResponse(intentContext.Context, a.Logger, outputResponse, nil)
}

func (a *ActionProcessor) makeOutputNLG(ctx context.Context, devicesResult action.DevicesResult) libnlg.NLG {
	deviceResults := devicesResult.Flatten()
	intentNLG := nlg.OK // TODO(aaulayev): more advanced NLG requires action intent in apply arguments, which will be added later
	if len(deviceResults) == 0 {
		doublelog.Error(ctx, a.Logger, "no device results")
		return intentNLG
	}

	errorCodeCountMap := devicesResult.ErrorCodeCountMap()
	if len(errorCodeCountMap) > 0 {
		doublelog.Error(ctx, a.Logger, "some device results contain errors", log.Any("device_results", deviceResults))
		return deviceErrorNLG(errorCodeCountMap, deviceResults, intentNLG)
	}

	return intentNLG.UseTextOnly(devicesResult.ContainsTextOnlyNLG())
}

func (a *ActionProcessor) applyResponseWithVideoPlayDirective(ctx context.Context, logger log.Logger, video VideoPlayDirective) (*scenarios.TScenarioApplyResponse, error) {
	outputResponse := libmegamind.OutputResponse{
		NLG:   nlg.OK,
		Video: video.ToDirective(),
	}
	return megamind.NlgApplyResponse(ctx, logger, outputResponse, nil)
}

func deviceErrorNLG(errorCodeCountMap adapter.ErrorCodeCountMap, deviceResults []action.ProviderDeviceResult,
	defaultNLG libnlg.NLG) libnlg.NLG {
	if len(deviceResults) == 1 {
		for errorCode := range errorCodeCountMap { // there's only one key-value pair in errorCodeCountMap
			return megamind.ErrorCodeNLG(errorCode)
		}
	}

	totalErrs := errorCodeCountMap.Total()

	// all devices returned an error
	if totalErrs == len(deviceResults) {
		// same error code from all devices
		if len(errorCodeCountMap) == 1 {
			for errorCode := range errorCodeCountMap {
				return megamind.MultipleDevicesErrorCodeNLG(errorCode)
			}
		}

		// different error codes
		return nlg.AllActionsFailedError
	}

	// some devices returned an error
	return defaultNLG
}

func NewActionProcessor(logger log.Logger, inflector inflector.IInflector, scenarioController *scenario.Controller, actionController action.IController) *ActionProcessor {
	processor := ActionProcessor{
		Logger:             logger,
		inflector:          inflector,
		scenarioController: scenarioController,
		actionController:   actionController,
	}

	return &processor
}

type specifyRequestParams struct {
	logger          log.Logger
	callbackActions []libmegamind.CallbackFrameAction
	responseNLG     libnlg.NLG
}

func nlgWithSpecifyCallback(processorContext common.RunProcessorContext, params specifyRequestParams) (*scenarios.TScenarioRunResponse, error) {
	specifyRequestState := common.SpecifyRequestState{
		SemanticFrames: []*commonpb.TSemanticFrame{
			processorContext.SemanticFrame.Frame,
		},
	}
	stateProto, err := anypb.New(specifyRequestState.ToProto())
	if err != nil {
		return megamind.ErrorRunResponse(processorContext.Context, params.logger, err)
	}

	return megamind.NlgRunResponseBuilder(processorContext.Context, params.logger, params.responseNLG).
		WithCallbackFrameActions(params.callbackActions...).
		WithState(stateProto).
		ExpectRequest().
		Build(), nil
}

func extractActionIntentApplyArguments(requestArguments *anypb.Any) (*protos.ActionIntentApplyArguments, error) {
	var aaProto protos.TApplyArguments
	if err := requestArguments.UnmarshalTo(&aaProto); err != nil {
		return nil, xerrors.Errorf("failed to unmarshall apply arguments: %w", err)
	}

	var applyArguments *protos.ActionIntentApplyArguments
	if applyArguments = aaProto.GetActionIntentApplyArguments(); applyArguments == nil {
		return nil, xerrors.Errorf("no action intent apply arguments in the request")
	}

	return applyArguments, nil
}

func nlgOrSilentRunResponse(ctx context.Context, logger log.Logger, n libnlg.NLG, isSilent bool) (*scenarios.TScenarioRunResponse, error) {
	if isSilent {
		return megamind.NlgRunResponse(ctx, logger, libnlg.SilentResponse)
	}
	return megamind.NlgRunResponse(ctx, logger, n)
}

func nlgOrSilentApplyResponse(ctx context.Context, logger log.Logger, n libnlg.NLG, isSilent bool) (*scenarios.TScenarioApplyResponse, error) {
	outputResponse := libmegamind.OutputResponse{NLG: n}
	if isSilent {
		outputResponse.NLG = nil
	}
	return megamind.NlgApplyResponse(ctx, logger, outputResponse, nil)
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
