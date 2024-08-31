package query

import (
	"context"
	"fmt"
	"runtime/debug"
	"sort"
	"time"

	"google.golang.org/protobuf/types/known/anypb"

	"a.yandex-team.ru/alice/iot/bulbasaur/controller/experiments"
	"a.yandex-team.ru/alice/iot/bulbasaur/controller/query"
	"a.yandex-team.ru/alice/iot/bulbasaur/megamind"
	"a.yandex-team.ru/alice/iot/bulbasaur/megamind/common"
	"a.yandex-team.ru/alice/iot/bulbasaur/megamind/frames"
	"a.yandex-team.ru/alice/iot/bulbasaur/megamind/intents/specification"
	"a.yandex-team.ru/alice/iot/bulbasaur/megamind/sdk"
	"a.yandex-team.ru/alice/iot/bulbasaur/model"
	"a.yandex-team.ru/alice/iot/bulbasaur/nlg"
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

var processorName = "query_processor"

var _ megamind.FrameRunApplyProcessor = &Processor{}
var _ megamind.SpecifySupportingProcessor = &Processor{}

type Processor struct {
	inflector       inflector.IInflector
	logger          log.Logger
	queryController query.IController
}

func NewProcessor(logger log.Logger, inflector inflector.IInflector, queryController query.IController) *Processor {
	return &Processor{
		inflector,
		logger,
		queryController,
	}
}

func (p *Processor) Name() string {
	return processorName
}

func (p *Processor) SupportedInputs() sdk.SupportedInputs {
	return sdk.SupportedInputs{
		SupportedFrames: []libmegamind.SemanticFrameName{
			frames.QueryCapabilityOnOffFrameName,
			frames.QueryCapabilityColorSettingFrameName,
			frames.QueryCapabilityModeFrameName,
			frames.QueryCapabilityRangeFrameName,
			frames.QueryCapabilityToggleFrameName,
			frames.QueryPropertyFloatFrameName,
			frames.QueryStateFrameName,
		},
	}
}

func (p *Processor) Run(ctx context.Context, frame libmegamind.SemanticFrame, runRequest *scenarios.TScenarioRunRequest, user model.User) (*scenarios.TScenarioRunResponse, error) {
	return p.CoolerRun(sdk.NewRunContext(ctx, p.logger, runRequest, user), sdk.InputFrames(frame))
}

func (p *Processor) CoolerRun(runContext sdk.RunContext, input sdk.Input) (*scenarios.TScenarioRunResponse, error) {
	processorIsEnabled := runContext.Request().GetBaseRequest().GetExperiments().GetFields()[megamind.EnableGranetProcessorsExp] != nil ||
		experiments.EnableGranetProcessors.IsEnabled(runContext.Context())
	if !processorIsEnabled {
		return sdk.IrrelevantResponse(
			runContext,
			fmt.Sprintf("query processor is disabled (missing %q experiment)", megamind.EnableGranetProcessorsExp),
		)
	}
	return p.RunWithSpecifiedSlotsV2(runContext, input)
}

func (p *Processor) RunWithSpecifiedSlots(_ common.RunProcessorContext, _ ...libmegamind.Slot) (*scenarios.TScenarioRunResponse, error) {
	return nil, xerrors.New("RunWithSpecifiedSlotsV2 is preferred")
}

// RunWithSpecifiedSlotsV2 contains main logic of the run processor.
// It can be called either from Processor.Run or a specification processor like specification.HouseholdSpecificationProcessor.
// In the first case, specifiedSlots slice is empty. In the second one, it contains slots specified by user.
func (p *Processor) RunWithSpecifiedSlotsV2(runContext sdk.RunContext, input sdk.Input, specifiedSlots ...sdk.GranetSlot) (*scenarios.TScenarioRunResponse, error) {
	runContext.Logger().Info("query processor's run",
		log.Any("run_context", runContext),
		log.Any("specified_slots", specifiedSlots),
	)

	// If something is wrong with the userInfo, we must detect this in the first place
	_, err := runContext.UserInfo()
	if err != nil {
		return nil, xerrors.Errorf("user info not found: %w", err)
	}

	frame := frames.QueryFrame{}
	if err := sdk.UnmarshalSlots(input.GetFirstFrame(), &frame); err != nil {
		return nil, xerrors.Errorf("failed to unmarshal slots: %w", err)
	}
	runContext.Logger().Info("unmarshalled query frame", log.Any("query_frame", frame))

	if err := frame.AppendSlots(specifiedSlots...); err != nil {
		return nil, xerrors.Errorf("failed to append specified slots to query frame: %w", err)
	}
	if len(specifiedSlots) > 0 {
		runContext.Logger().Info("specifiedSlots appended successfully", log.Any("query_frame", frame))
	}

	if err := p.ValidateFrame(frame); err != nil {
		return nil, xerrors.Errorf("invalid frame: %w", err)
	}
	runContext.Logger().Info("frame is valid")

	// There will be multiple intent parameters slots in requests like "Какая температура и влажность на датчике?"
	// They are not supported at the moment, so we just take the first one.
	frame.IntentParameters = frames.QueryIntentParametersSlots{frame.IntentParameters[0]}

	extractionResult, err := frame.ExtractQueryIntent(runContext)
	if err != nil {
		return nil, xerrors.Errorf("failed to extract query intent: %w", err)
	}
	if extractionResult.Status != frames.OkExtractionStatus {
		runContext.Logger().Info(
			"extraction status is not ok",
			log.Any("extraction_status", extractionResult.Status),
		)

		if extractionResult.Status == frames.MultipleHouseholdsExtractionStatus {
			runContext.Logger().Info("asking to specify household")
			return p.specifyHouseholdResponse(runContext, input.GetFirstFrame(), extractionResult.FailureNLG)
		}

		return sdk.RunResponse(runContext).WithNLG(extractionResult.FailureNLG).Build()
	}

	applyArguments := &ApplyArguments{
		Devices:          extractionResult.Devices,
		IntentParameters: extractionResult.IntentParameters,
	}
	runContext.Logger().Info(
		"query apply arguments constructed",
		log.Strings("devices", applyArguments.Devices.GetIDs()),
		log.Any("intent_parameters", applyArguments.IntentParameters),
	)

	return sdk.RunApplyResponse(applyArguments)
}

func (p *Processor) specifyHouseholdResponse(runContext sdk.RunContext, semanticFrame *libmegamind.SemanticFrame, specifyNLG libnlg.NLG) (*scenarios.TScenarioRunResponse, error) {
	userInfo, _ := runContext.UserInfo()
	specifyRequestState := common.SpecifyRequestState{
		SemanticFrames: []*commonpb.TSemanticFrame{
			semanticFrame.Frame,
		},
	}
	stateProto, err := anypb.New(specifyRequestState.ToProto())
	if err != nil {
		return nil, xerrors.Errorf("failed to make household specification request state: %w", err)
	}
	return sdk.RunResponse(runContext).
		WithNLG(specifyNLG).
		WithState(stateProto).
		WithCallbackFrameActions(specification.HouseholdSpecificationCallbacks(userInfo.Households, p.inflector)...).
		Build()
}

// ValidateFrame checks if there is at least one intent parameters slot
func (p *Processor) ValidateFrame(frame frames.QueryFrame) error {
	// If there is no intent parameters, most likely something wrong with granet form.
	if len(frame.IntentParameters) < 1 {
		return xerrors.New("no intent parameters in the frame")
	}

	return nil
}

func (p *Processor) IsApplicable(applyArguments *anypb.Any) bool {
	return sdk.IsApplyArguments(applyArguments, &ApplyArguments{})
}

func (p *Processor) Apply(ctx context.Context, applyRequest *scenarios.TScenarioApplyRequest, user model.User) (*scenarios.TScenarioApplyResponse, error) {
	if user.IsEmpty() {
		return nil, xerrors.New("user is not authorized")
	}

	applyContext := sdk.NewApplyContext(ctx, p.logger, applyRequest, user)
	applyArguments := ApplyArguments{}
	if err := sdk.UnmarshalApplyArguments(applyRequest.GetArguments(), &applyArguments); err != nil {
		return nil, xerrors.Errorf("failed to unmarshal apply arguments: %w", err)
	}
	applyContext.Logger().Info(
		"query processor's apply",
		log.Strings("arguments_device_ids", applyArguments.Devices.GetIDs()),
		log.Any("arguments_intent_parameters", applyArguments.IntentParameters),
	)

	return p.apply(applyContext, applyArguments)
}

func (p *Processor) apply(applyContext sdk.ApplyContext, applyArguments ApplyArguments) (*scenarios.TScenarioApplyResponse, error) {
	intentParameters := applyArguments.IntentParameters
	if len(intentParameters) == 0 {
		return nil, xerrors.New("no intent parameters in apply arguments")
	}
	requestedDevices := applyArguments.Devices
	if len(requestedDevices) == 0 {
		return nil, xerrors.New("no devices in apply arguments")
	}

	origin, _ := applyContext.Origin() // user has been checked earlier, so no need to check if ok here

	queryResults := make(chan queryResult, 1)
	go func() {
		defer func() {
			if r := recover(); r != nil {
				err := xerrors.Errorf("caught panic in apply: %v", r)
				ctxlog.Warn(applyContext.Context(), p.logger, fmt.Sprintf("%v", err), log.Any("stacktrace", string(debug.Stack())))
				queryResults <- queryResult{err: err}
			}
		}()

		applyContext.Logger().Info("updating device states")
		devices, deviceStatuses, err := p.queryController.UpdateDevicesState(applyContext.Context(), requestedDevices, origin)
		queryResults <- queryResult{
			Devices:        devices,
			DeviceStatuses: deviceStatuses,
			err:            err,
		}
	}()

	timeout, err := timestamp.ComputeTimeout(applyContext.Context(), megamind.MMApplyTimeout)
	if err != nil {
		timeout = megamind.DefaultApplyTimeout
	}

	select {
	case result := <-queryResults:
		if result.err != nil {
			applyContext.Logger().Errorf("error in querying devices: %v", result.err)
		}
		// If intentParameters contain more than 1 element, their target must be the same, so it's ok to use the first element
		analyticsInfo := sdk.AnalyticsInfo().WithQuery(intentParameters[0].Target, result.Devices.GetIDs())
		responseNLG, animation := p.processQueryResult(applyContext, intentParameters, result)
		applyContext.Logger().Info("query result processed", log.Any("nlg", responseNLG))
		return sdk.ApplyResponse(applyContext).
			WithAnalyticsInfo(analyticsInfo).
			WithNLG(responseNLG).
			WithAnimation(animation).
			Build()
	case <-time.After(timeout):
		applyContext.Logger().Infof("timeout was reached after %dms, answering with default timeout NLG", timeout.Milliseconds())
		analyticsInfo := sdk.AnalyticsInfo().WithQuery(intentParameters[0].Target, applyArguments.Devices.GetIDs())
		return sdk.ApplyResponse(applyContext).
			WithAnalyticsInfo(analyticsInfo).
			WithNLG(nlg.QueryTimeout).
			Build()
	}
}

func (p *Processor) processQueryResult(applyContext sdk.ApplyContext, intentParameters common.QueryIntentParametersSlice, queryResult queryResult) (libnlg.NLG, *libmegamind.LEDAnimation) {
	deviceResults := queryResult.toProcessedDeviceResults(intentParameters)
	applyContext.Logger().Info("VoiceQueryDeviceResults", log.Any("voice_query_device_results", deviceResults.toLogMap()))

	if deviceResults.totalFailed() >= len(queryResult.Devices) {
		return deviceResults.errorNLG(queryResult.Devices), nil
	}

	sort.Sort(model.DevicesSorting(deviceResults.online))
	sort.Sort(DeviceCapabilityQueryResultsSorting(deviceResults.capabilityResults))
	sort.Sort(DevicePropertyQueryResultsSorting(deviceResults.propertyResults))
	for i := range deviceResults.online {
		sort.Sort(model.CapabilitiesSorting(deviceResults.online[i].Capabilities))
		sort.Sort(model.PropertiesSorting(deviceResults.online[i].Properties))
	}

	// multi-target queries are not supported at the moment, so it's safe to use intentParameters[0]'s target
	switch target := intentParameters[0].Target; target {
	case common.StateTarget:
		return queryStateNLG(p.inflector, deviceResults.online), nil
	case common.CapabilityTarget:
		return queryCapabilitiesNLG(p.inflector, deviceResults.capabilityResults, intentParameters, deviceResults.online), nil
	case common.PropertyTarget:
		return queryPropertiesNLG(applyContext, p.inflector, deviceResults.propertyResults, intentParameters)
	default:
		applyContext.Logger().Infof("unknown query target in apply: %q", target)
		return nlg.QueryCannotDo, nil
	}
}

type queryResult struct {
	Devices        model.Devices
	DeviceStatuses model.DeviceStatusMap
	err            error
}

func (r *queryResult) toProcessedDeviceResults(intentParameters common.QueryIntentParametersSlice) processedDeviceResults {
	deviceResults := processedDeviceResults{
		online:            make(model.Devices, 0, len(r.Devices)),
		offline:           make(model.Devices, 0, len(r.Devices)),
		notFound:          make(model.Devices, 0, len(r.Devices)),
		targetNotFound:    make(model.Devices, 0, len(r.Devices)),
		otherErr:          make(model.Devices, 0, len(r.Devices)),
		capabilityResults: make(DeviceCapabilityQueryResults, 0, len(r.Devices)),
		propertyResults:   make(DevicePropertyQueryResults, 0, len(r.Devices)),
	}

	for _, device := range r.Devices {
		deviceResults.populateFromDevice(device, r.DeviceStatuses, intentParameters)
	}

	return deviceResults
}

type processedDeviceResults struct {
	online         model.Devices
	offline        model.Devices
	notFound       model.Devices
	targetNotFound model.Devices
	otherErr       model.Devices

	capabilityResults DeviceCapabilityQueryResults
	propertyResults   DevicePropertyQueryResults
}

// populateFromDevice adds results for device to processedDeviceResults.
// If device is online and has at least one capability or property requested in intentParameters, then it's appended to
// processedDeviceResults.online and processedDeviceResults.capabilityResults or processedDeviceResults.propertyResults.
// If it's offline or has no requested capabilities or properties, it's appended to one of the error fields:
// processedDeviceResults.offline, processedDeviceResults.notFound, processedDeviceResults.targetNotFound or processedDeviceResults.otherErr.
//
// If the target is state, no capabilities or properties are stored to r, but the device can still be added to error fields.
func (r *processedDeviceResults) populateFromDevice(device model.Device, deviceStates model.DeviceStatusMap, intentParameters common.QueryIntentParametersSlice) {
	deviceCapabilities := device.Capabilities.AsMap()
	deviceProperties := device.Properties.AsMap()

	switch deviceState := deviceStates[device.ID]; deviceState {
	case model.OnlineDeviceStatus:
		r.online = append(r.online, device)

		targetFound := false
		unknownTarget := false
		// append capability or property result from the device for each element in intentParameters
		for _, parameters := range intentParameters {
			switch target := parameters.Target; target {
			case common.CapabilityTarget:
				key := model.CapabilityKey(model.CapabilityType(parameters.CapabilityType), parameters.CapabilityInstance)
				capability, ok := deviceCapabilities[key]
				if !ok {
					continue
				}
				targetFound = true
				r.capabilityResults = append(r.capabilityResults, NewDeviceCapabilityQueryResult(
					device.ID,
					device.Name,
					device.Type,
					device.Room,
					capability,
				))
			case common.PropertyTarget:
				key := model.PropertyKey(model.PropertyType(parameters.PropertyType), parameters.PropertyInstance)
				property, ok := deviceProperties[key]
				if !ok {
					continue
				}
				targetFound = true
				r.propertyResults = append(r.propertyResults, NewDevicePropertyQueryResult(
					device.ID,
					device.Name,
					device.Type,
					device.Room,
					property,
				))
			case common.StateTarget:
				targetFound = true
			default:
				unknownTarget = true
			}
		}

		if !targetFound && unknownTarget {
			// If none of the requested capabilities or properties were found in the device,
			// but there was an unknown target in the intentParameters, append device to otherErr.
			r.otherErr = append(r.otherErr, device)
		} else if !targetFound {
			// If there were no unknown targets, but still no requested capabilities or properties in the device,
			// append device to targetNotFound
			r.targetNotFound = append(r.targetNotFound, device)
		}

	case model.OfflineDeviceStatus:
		r.offline = append(r.offline, device)
	case model.NotFoundDeviceStatus:
		r.notFound = append(r.notFound, device)
	default:
		r.otherErr = append(r.otherErr, device)
	}
}

func (r *processedDeviceResults) toLogMap() map[string]interface{} {
	return map[string]interface{}{
		"online_count":           len(r.online),
		"offline_count":          len(r.offline),
		"not_found_count":        len(r.notFound),
		"target_not_found_count": len(r.targetNotFound),
		"other_err_count":        len(r.otherErr),
		"online":                 r.online.GetIDs(),
		"offline":                r.offline.GetIDs(),
		"not_found":              r.notFound.GetIDs(),
		"target_not_found":       r.targetNotFound.GetIDs(),
		"other_err":              r.otherErr.GetIDs(),
	}
}

func (r *processedDeviceResults) totalFailed() int {
	return len(r.offline) + len(r.notFound) + len(r.targetNotFound) + len(r.otherErr)
}

func (r *processedDeviceResults) errorNLG(devices model.Devices) libnlg.NLG {
	switch {
	case len(r.offline) >= len(devices) && len(r.offline) == 1:
		return nlg.DeviceUnreachable
	case len(r.offline) >= len(devices) && len(r.offline) > 1:
		return nlg.MultipleDeviceUnreachable
	case len(r.notFound) >= len(devices) && len(r.notFound) == 1:
		return nlg.DeviceNotFound
	case len(r.notFound) >= len(devices) && len(r.notFound) > 1:
		return nlg.MultipleDeviceNotFound
	case len(r.targetNotFound) >= len(devices):
		return nlg.QueryTargetNotFound
	default:
		return nlg.QueryMixedError
	}
}
