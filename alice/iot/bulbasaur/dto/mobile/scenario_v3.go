package mobile

import (
	"context"
	"encoding/json"
	"math"
	"sort"
	"strings"
	"time"

	"golang.org/x/exp/slices"

	"a.yandex-team.ru/alice/iot/bulbasaur/begemot"
	"a.yandex-team.ru/alice/iot/bulbasaur/model"
	"a.yandex-team.ru/alice/library/go/timestamp"
	"a.yandex-team.ru/alice/library/go/tools"
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/log/ctxlog"
	"a.yandex-team.ru/library/go/core/xerrors"
	"a.yandex-team.ru/library/go/ptr"
	"a.yandex-team.ru/library/go/valid"
)

type ScenarioCreateRequestV3 struct {
	Name          model.ScenarioName         `json:"name"`
	Icon          model.ScenarioIcon         `json:"icon"`
	Triggers      model.ScenarioTriggers     `json:"triggers"`
	Steps         ScenarioCreateRequestSteps `json:"steps"`
	IsActive      *bool                      `json:"is_active"`
	EffectiveTime *EffectiveTime             `json:"effective_time,omitempty"`
	PushOnInvoke  bool                       `json:"push_on_invoke,omitempty"`
}

func (scr ScenarioCreateRequestV3) Validate(vctx *valid.ValidationCtx) (bool, error) {
	var err valid.Errors

	//name
	if _, e := scr.Name.Validate(vctx); e != nil {
		err = append(err, e)
	}

	//icon
	if _, e := scr.Icon.Validate(vctx); e != nil {
		err = append(err, e)
	}

	//triggers
	if len(scr.Triggers) == 0 {
		err = append(err, &model.EmptyTriggersFieldError{})
	}

	if _, e := scr.Triggers.Validate(vctx); e != nil {
		err = append(err, e)
	}

	//steps
	if scr.Steps != nil {
		for _, s := range scr.Steps {
			if _, e := s.Validate(vctx); e != nil {
				if ves, ok := e.(valid.Errors); ok {
					err = append(err, ves...)
				} else {
					err = append(err, e)
				}
			}
		}
	}

	//effective time
	if scr.EffectiveTime != nil {
		if e := scr.EffectiveTime.Validate(); e != nil {
			err = append(err, e)
		}
	}

	//is_active pass

	if len(err) == 0 {
		return false, nil
	}
	return false, err
}

func (scr ScenarioCreateRequestV3) ValidateRequest(ctx context.Context, logger log.Logger, begemotClient begemot.IClient, createdScenarioID string, extendedUserInfo model.ExtendedUserInfo) error {
	if err := scr.validateTriggers(ctx, extendedUserInfo.UserInfo); err != nil {
		return xerrors.Errorf("failed to validate triggers: %w", err)
	}
	if err := scr.validateRangeCapabilities(extendedUserInfo.UserInfo.Devices); err != nil {
		return xerrors.Errorf("failed to validate range capabilities: %w", err)
	}
	if err := scr.validatePushTextActions(ctx, logger, begemotClient, createdScenarioID, extendedUserInfo.UserInfo); err != nil {
		return xerrors.Errorf("failed to validate push text actions: %w", err)
	}
	if err := scr.validateSteps(extendedUserInfo.UserInfo.Devices); err != nil {
		return xerrors.Errorf("failed to validate steps: %w", err)
	}
	if err := scr.validateSharedDevices(extendedUserInfo.UserInfo.Devices); err != nil {
		return xerrors.Errorf("failed to validate shared devices: %w", err)
	}
	return nil
}

func (scr ScenarioCreateRequestV3) validatePushTextActions(ctx context.Context, logger log.Logger, begemotClient begemot.IClient, createdScenarioID string, userInfo model.UserInfo) error {
	// if scenario has no server actions that push text - no need to validate
	pushTexts := make([]string, 0)
	for _, step := range scr.Steps {
		pushTexts = append(pushTexts, step.Parameters.GetTextQuasarServerActionCapabilityValues()...)
	}
	if len(pushTexts) == 0 {
		return nil
	}
	// create scenario from userInfo data with fake id for validation
	createdScenario := scr.ToScenario(userInfo)
	if createdScenario.ID = createdScenarioID; createdScenario.ID == "" {
		// this scenario is new - use fake begemot validation id to add it to userInfo
		createdScenario.ID = model.ScenarioBegemotValidationID
		userInfo.Scenarios = append(userInfo.Scenarios.Clone(), createdScenario)
	} else {
		// this scenario already exists - replace it in userInfo list
		userInfo.Scenarios = userInfo.Scenarios.ReplaceFirstByID(createdScenario)
	}

	// all scenario ids here
	allScenarioIDs := userInfo.Scenarios.GetIDs()

	// validate every push text in scenario
	ctxlog.Info(ctx, logger, "UserInfo in pushText validation", log.Any("user_info", userInfo))
	for _, pushText := range pushTexts {
		if err := begemot.ValidatePushText(ctx, logger, begemotClient, pushText, allScenarioIDs, userInfo); err != nil {
			return err
		}
	}
	return nil
}

func (scr ScenarioCreateRequestV3) validateTriggers(ctx context.Context, userInfo model.UserInfo) error {
	if len(scr.Triggers) == 0 {
		return &model.EmptyTriggersFieldError{}
	}

	if len(scr.Triggers.FilterByType(model.VoiceScenarioTriggerType)) > model.ScenarioVoiceTriggersNumLimit {
		return &model.TooManyVoiceTriggersError{}
	}

	if len(scr.Triggers.FilterByType(model.TimetableScenarioTriggerType)) > model.ScenarioTimetableTriggersNumLimit {
		return &model.TooManyTimetableTriggersError{}
	}

	for _, t := range scr.Triggers {
		switch t.Type() {
		case model.TimerScenarioTriggerType:
			return &model.TimerTriggerScenarioCreationError{}

		case model.TimetableScenarioTriggerType:
			value, ok := t.(model.TimetableScenarioTrigger)
			if !ok {
				return xerrors.Errorf("failed to convert trigger with type %s to TimetableScenarioTrigger", model.TimetableScenarioTriggerType)
			}
			if err := value.IsValid(userInfo.Households); err != nil {
				return err
			}
		}
	}
	return nil
}

func (scr ScenarioCreateRequestV3) validateRangeCapabilities(devices model.Devices) error {
	for _, step := range scr.Steps {
		if err := step.Parameters.ValidateRangeCapabilities(devices); err != nil {
			return err
		}
	}
	return nil
}

func (scr ScenarioCreateRequestV3) validateSteps(devices model.Devices) error {
	var overallDelayMs int
	var hasAtLeastOneActionsStep bool
	var consecutiveDelay bool
	var lastStepDelay bool
	for i, step := range scr.Steps {
		if step.Parameters == nil {
			return xerrors.Errorf("scenario step %d parameters are nil", i)
		}
		switch step.Type {
		case model.ScenarioStepActionsType:
			if scenarioStep := step.ToScenarioStep(devices); !scenarioStep.IsEmpty() {
				hasAtLeastOneActionsStep = true
			}
		case model.ScenarioStepDelayType:
			params, ok := step.Parameters.(ScenarioCreateRequestStepDelayParameters)
			if !ok {
				return xerrors.Errorf("scenario step %d parameters have invalid format", i)
			}
			overallDelayMs += params.DelayMs
			if i > 0 && scr.Steps[i-1].Type == model.ScenarioStepDelayType {
				consecutiveDelay = true
			}
			if i == (len(scr.Steps) - 1) {
				lastStepDelay = true
			}
		default:
			return xerrors.Errorf("unknown scenario step type: %s", step.Type)
		}
	}
	switch {
	case !hasAtLeastOneActionsStep && !scr.PushOnInvoke:
		return &model.ScenarioStepsAtLeastOneActionError{}
	case consecutiveDelay:
		return &model.ScenarioStepsConsecutiveDelaysError{}
	case lastStepDelay:
		return &model.ScenarioStepsDelayLastStepError{}
	case overallDelayMs > 24*60*60*1000:
		return &model.ScenarioStepsDelayLimitReachedError{}
	default:
		return nil
	}
}

func (scr ScenarioCreateRequestV3) validateSharedDevices(devices model.Devices) error {
	devicesMap := devices.ToMap()
	// check steps
	for i, step := range scr.Steps {
		if step.Parameters == nil {
			return xerrors.Errorf("scenario step %d parameters are nil", i)
		}
		if step.Type != model.ScenarioStepActionsType {
			continue
		}
		params, ok := step.Parameters.(ScenarioCreateRequestStepActionsParameters)
		if !ok {
			return xerrors.Errorf("scenario step %d parameters have invalid format", i)
		}
		for _, device := range params.LaunchDevices {
			modelDevice, ok := devicesMap[device.ID]
			if !ok {
				return xerrors.Errorf("scenario step %d contains unknown device %s: %w", i, device.ID, &model.DeviceNotFoundError{})
			}
			if modelDevice.IsShared() {
				return xerrors.Errorf("shared device %s cannot be used in scenarios: %w", device.ID, &model.SharedDeviceUsedInScenarioError{})
			}
		}
	}
	// check triggers
	for i, trigger := range scr.Triggers {
		if trigger.Type() != model.PropertyScenarioTriggerType {
			continue
		}
		modelTrigger, ok := trigger.(model.DevicePropertyScenarioTrigger)
		if !ok {
			return xerrors.Errorf("scenario trigger %d has invalid format", i)
		}
		modelDevice, ok := devicesMap[modelTrigger.DeviceID]
		if !ok {
			return xerrors.Errorf("scenario trigger %d contains unknown device %s: %w", i, modelTrigger.DeviceID, &model.DeviceNotFoundError{})
		}
		if modelDevice.IsShared() {
			return xerrors.Errorf("shared device %s cannot be used in scenarios: %w", modelTrigger.DeviceID, &model.SharedDeviceUsedInScenarioError{})
		}
	}
	return nil
}

func (scr ScenarioCreateRequestV3) ToScenario(userInfo model.UserInfo) model.Scenario {
	var s model.Scenario
	s.Name = scr.Name
	s.Icon = scr.Icon
	s.Triggers = scr.Triggers
	s.Triggers.EnrichData(userInfo.Households)

	s.Steps = make(model.ScenarioSteps, 0, len(scr.Steps))
	for _, step := range scr.Steps {
		if scenarioStep := step.ToScenarioStep(userInfo.Devices); !scenarioStep.IsEmpty() {
			s.Steps = append(s.Steps, step.ToScenarioStep(userInfo.Devices))
		}
	}

	if scr.IsActive != nil {
		s.IsActive = *scr.IsActive
	} else {
		s.IsActive = true
	}

	if scr.EffectiveTime != nil {
		effectiveTime, _ := scr.EffectiveTime.ToModel()
		s.EffectiveTime = &effectiveTime
	}
	s.PushOnInvoke = scr.PushOnInvoke

	return s
}

type ScenarioCreateRequestStep struct {
	Type       model.ScenarioStepType               `json:"type"`
	Parameters IScenarioCreateRequestStepParameters `json:"parameters"`
}

func (s ScenarioCreateRequestStep) Validate(vctx *valid.ValidationCtx) (bool, error) {
	var err valid.Errors

	if !slices.Contains(model.KnownScenarioStepTypes, string(s.Type)) {
		err = append(err, xerrors.Errorf("unknown scenario step type: %q", s.Type))
	}

	if s.Parameters == nil {
		err = append(err, xerrors.New("parameters is null"))
	} else {
		if _, e := s.Parameters.Validate(vctx); e != nil {
			if ves, ok := e.(valid.Errors); ok {
				err = append(err, ves...)
			} else {
				err = append(err, e)
			}
		}
	}

	if len(err) == 0 {
		return false, nil
	}
	return false, err
}

func (s ScenarioCreateRequestStep) ToScenarioStep(devices model.Devices) model.IScenarioStep {
	step := model.MakeScenarioStepByType(s.Type)
	step.SetParameters(s.Parameters.ToScenarioStepParameters(devices))
	return step
}

func (s *ScenarioCreateRequestStep) UnmarshalJSON(b []byte) error {
	sRaw := struct {
		Type       model.ScenarioStepType `json:"type"`
		Parameters json.RawMessage        `json:"parameters"`
	}{}
	if err := json.Unmarshal(b, &sRaw); err != nil {
		return err
	}

	switch sRaw.Type {
	case model.ScenarioStepActionsType:
		p := ScenarioCreateRequestStepActionsParameters{}
		if err := json.Unmarshal(sRaw.Parameters, &p); err != nil {
			return err
		}
		s.Parameters = p
	case model.ScenarioStepDelayType:
		p := ScenarioCreateRequestStepDelayParameters{}
		if err := json.Unmarshal(sRaw.Parameters, &p); err != nil {
			return err
		}
		s.Parameters = p
	default:
		return xerrors.Errorf("unknown scenario step type: %q", sRaw.Type)
	}
	s.Type = sRaw.Type
	return nil
}

type IScenarioCreateRequestStepParameters interface {
	Validate(vctx *valid.ValidationCtx) (bool, error)
	ValidateRangeCapabilities(devices model.Devices) error
	GetTextQuasarServerActionCapabilityValues() []string
	ToScenarioStepParameters(devices model.Devices) model.IScenarioStepParameters
}

type ScenarioCreateRequestStepActionsParameters struct {
	LaunchDevices                ScenarioCreateDevices      `json:"launch_devices"`
	RequestedSpeakerCapabilities ScenarioCreateCapabilities `json:"requested_speaker_capabilities"`
}

func (p ScenarioCreateRequestStepActionsParameters) Validate(vctx *valid.ValidationCtx) (bool, error) {
	var err valid.Errors

	if p.LaunchDevices != nil {
		if _, e := p.LaunchDevices.Validate(vctx); e != nil {
			if ves, ok := e.(valid.Errors); ok {
				err = append(err, ves...)
			} else {
				err = append(err, e)
			}
		}
	}

	if p.RequestedSpeakerCapabilities != nil {
		if _, e := p.RequestedSpeakerCapabilities.Validate(vctx); e != nil {
			if ves, ok := e.(valid.Errors); ok {
				err = append(err, ves...)
			} else {
				err = append(err, e)
			}
		}
	}

	if len(err) == 0 {
		return false, nil
	}
	return false, err
}

func (p ScenarioCreateRequestStepActionsParameters) ValidateRangeCapabilities(devices model.Devices) error {
	devicesMap := devices.ToMap()
	for _, scenarioDevice := range p.LaunchDevices {
		device, exist := devicesMap[scenarioDevice.ID]
		if !exist {
			return &model.DeviceNotFoundError{}
		}
		deviceCapabilities := device.Capabilities.AsMap()
		scenarioRangeCapabilities := scenarioDevice.Capabilities.CapabilitiesOfType(model.RangeCapabilityType)
		for _, sc := range scenarioRangeCapabilities {
			cKey := model.CapabilityKey(sc.Type, sc.State.Instance)
			capability, exist := deviceCapabilities[cKey]
			if !exist {
				return &model.UnknownCapabilityError{}
			}
			parameters := capability.Parameters().(model.RangeCapabilityParameters)

			var value float64
			switch castedValue := sc.State.Value.(type) {
			case int64:
				value = float64(castedValue)
			case float64:
				value = castedValue
			default:
				return &model.ScenarioActionValueError{}
			}
			if parameters.Range != nil && sc.State.Relative == nil && !parameters.Range.Contains(value) {
				return &model.ScenarioActionValueError{}
			}
			// sometimes values bigger than int64 can break backups
			// due to situations when big round float64 is stored and
			// fails to convert to int64 [-2**63; 2**64-1] in backup leading to backup crash
			if value < math.MinInt64 || value > math.MaxInt64 {
				return &model.ScenarioActionValueError{}
			}
		}
	}
	return nil
}

func (p ScenarioCreateRequestStepActionsParameters) GetTextQuasarServerActionCapabilityValues() []string {
	cType, cInstance := model.QuasarServerActionCapabilityType, model.TextActionCapabilityInstance.String()
	result := make([]string, 0)
	for _, d := range p.LaunchDevices {
		capabilities := d.Capabilities.CapabilitiesOfTypeAndInstance(cType, cInstance)
		for _, c := range capabilities {
			result = append(result, c.State.Value.(string))
		}
	}
	for _, c := range p.RequestedSpeakerCapabilities.CapabilitiesOfTypeAndInstance(cType, cInstance) {
		result = append(result, c.State.Value.(string))
	}
	return result
}

func (p ScenarioCreateRequestStepActionsParameters) ToScenarioStepParameters(devices model.Devices) model.IScenarioStepParameters {
	modelParameters := model.ScenarioStepActionsParameters{}
	modelParameters.Devices = make(model.ScenarioLaunchDevices, 0, len(p.LaunchDevices))
	modelParameters.RequestedSpeakerCapabilities = make(model.ScenarioCapabilities, 0, len(p.RequestedSpeakerCapabilities))
	userDevicesMap := devices.ToMap()

	for _, d := range p.LaunchDevices {
		if ud, exist := userDevicesMap[d.ID]; exist {
			if scenarioLaunchDevice := d.ToScenarioLaunchDevice(ud); len(scenarioLaunchDevice.Capabilities) > 0 {
				modelParameters.Devices = append(modelParameters.Devices, scenarioLaunchDevice)
			}
		}
	}

	for _, c := range p.RequestedSpeakerCapabilities {
		if !model.KnownQuasarCapabilityTypes.Contains(c.Type) {
			continue
		}
		capability := c.ToQuasarCapability()
		var scCapability model.ScenarioCapability
		scCapability.Type = capability.Type()
		scCapability.State = capability.State()
		modelParameters.RequestedSpeakerCapabilities = append(modelParameters.RequestedSpeakerCapabilities, scCapability)
	}
	return modelParameters
}

type ScenarioCreateRequestStepDelayParameters struct {
	DelayMs int `json:"delay_ms"`
}

func (p ScenarioCreateRequestStepDelayParameters) Validate(vctx *valid.ValidationCtx) (bool, error) {
	var err valid.Errors

	if p.DelayMs <= 0 {
		err = append(err, xerrors.New("delay_ms should be strictly positive"))
	}

	if len(err) == 0 {
		return false, nil
	}
	return false, err
}

func (p ScenarioCreateRequestStepDelayParameters) ValidateRangeCapabilities(devices model.Devices) error {
	return nil
}

func (p ScenarioCreateRequestStepDelayParameters) GetTextQuasarServerActionCapabilityValues() []string {
	return nil
}

func (p ScenarioCreateRequestStepDelayParameters) ToScenarioStepParameters(devices model.Devices) model.IScenarioStepParameters {
	return model.ScenarioStepDelayParameters{DelayMs: p.DelayMs}
}

type ScenarioCreateRequestSteps []ScenarioCreateRequestStep

func (scrs ScenarioCreateRequestSteps) Validate(vctx *valid.ValidationCtx) (bool, error) {
	var verrs valid.Errors
	for _, step := range scrs {
		if _, e := step.Validate(vctx); e != nil {
			if ves, ok := e.(valid.Errors); ok {
				verrs = append(verrs, ves...)
			} else {
				verrs = append(verrs, e)
			}
		}
	}

	if len(verrs) == 0 {
		return false, nil
	}
	return false, verrs
}

type ScenarioEditResultV3 struct {
	Status    string             `json:"status"`
	RequestID string             `json:"request_id"`
	Scenario  ScenarioEditViewV3 `json:"scenario"`
}

type ScenarioEditViewV3 struct {
	ID       string                    `json:"id"`
	Name     model.ScenarioName        `json:"name"`
	Triggers []ScenarioTriggerEditView `json:"triggers"`
	Steps    []ScenarioEditViewStep    `json:"steps"`
	Icon     model.ScenarioIcon        `json:"icon"`
	IconURL  string                    `json:"icon_url"`

	IsActive      bool           `json:"is_active"`
	EffectiveTime *EffectiveTime `json:"effective_time,omitempty"`
	Favorite      bool           `json:"favorite"`
	PushOnInvoke  *bool          `json:"push_on_invoke,omitempty"`
}

func (sev *ScenarioEditResultV3) FromScenario(ctx context.Context, scenario model.Scenario, devices []model.Device, stereopairs model.Stereopairs) error {
	sev.Scenario.ID = scenario.ID
	sev.Scenario.Name = scenario.Name

	sev.Scenario.Triggers = makeScenarioTriggerEditViews(scenario.Triggers, devices)

	sev.Scenario.Icon = scenario.Icon
	sev.Scenario.IconURL = scenario.Icon.URL()

	scenarioSteps := scenario.ScenarioSteps(devices).FilterByActualDevices(devices, true)
	sev.Scenario.Steps = make([]ScenarioEditViewStep, 0, len(scenarioSteps))
	for _, step := range scenarioSteps {
		var stepView ScenarioEditViewStep
		if err := stepView.FromScenarioStep(ctx, step, stereopairs, ""); err != nil {
			return err
		}
		sev.Scenario.Steps = append(sev.Scenario.Steps, stepView)
	}
	sev.Scenario.IsActive = scenario.IsActive

	if scenario.EffectiveTime != nil {
		var effectiveTime EffectiveTime
		effectiveTime.FromModel(*scenario.EffectiveTime)
		sev.Scenario.EffectiveTime = &effectiveTime
	}
	sev.Scenario.Favorite = scenario.Favorite
	if scenario.PushOnInvoke {
		sev.Scenario.PushOnInvoke = ptr.Bool(scenario.PushOnInvoke)
	}
	return nil
}

type ScenarioStepStatus string

type ScenarioEditViewStep struct {
	Type        model.ScenarioStepType           `json:"type"`
	Parameters  IScenarioEditViewStepParameters  `json:"parameters"`
	ProgressBar *ScenarioEditViewStepProgressBar `json:"progress_bar,omitempty"`
	Status      ScenarioStepStatus               `json:"status,omitempty"`
}

func (s *ScenarioEditViewStep) FromScenarioStep(ctx context.Context, step model.IScenarioStep, stereopairs model.Stereopairs, status ScenarioStepStatus) error {
	s.Type = step.Type()
	s.Status = status
	switch s.Type {
	case model.ScenarioStepActionsType:
		params := ScenarioEditViewStepActionsParameters{}
		modelParams := step.Parameters().(model.ScenarioStepActionsParameters)
		params.FromScenarioStepActionsParameters(ctx, modelParams, stereopairs)
		s.Parameters = params
	case model.ScenarioStepDelayType:
		params := ScenarioEditViewStepDelayParameters{}
		modelParams := step.Parameters().(model.ScenarioStepDelayParameters)
		params.FromScenarioStepDelayParameters(modelParams)
		s.Parameters = params
	default:
		return xerrors.Errorf("unknown scenario step type: %q", s.Type)
	}
	return nil
}

func (s *ScenarioEditViewStep) FillProgressBar(now timestamp.PastTimestamp, scheduled timestamp.PastTimestamp) {
	if s.Type != model.ScenarioStepDelayType {
		return
	}
	delayMs := s.Parameters.(ScenarioEditViewStepDelayParameters).DelayMs
	scheduledUTC := scheduled.AsTime().UTC()
	nowUTC := now.AsTime().UTC()
	delayStartUTC := scheduledUTC.Add(-1 * time.Duration(delayMs) * time.Millisecond)
	initialTimer := int(math.Round(scheduledUTC.Sub(delayStartUTC).Seconds()))
	if initialTimer < 0 {
		initialTimer = 0
	}
	currentTimer := int(math.Round(scheduledUTC.Sub(nowUTC).Seconds()))
	if currentTimer < 0 {
		currentTimer = 0
	}
	s.ProgressBar = &ScenarioEditViewStepProgressBar{
		InitialTimerValue: initialTimer,
		CurrentTimerValue: currentTimer,
	}
}

type ScenarioEditViewStepProgressBar struct {
	InitialTimerValue int `json:"initial_timer_value"`
	CurrentTimerValue int `json:"current_timer_value"`
}

type IScenarioEditViewStepParameters interface {
	isScenarioEditViewStepParameters()
}

type ScenarioEditViewStepActionsParameters struct {
	LaunchDevices                []ScenarioEditViewLaunchDevice `json:"launch_devices"`
	RequestedSpeakerCapabilities []CapabilityStateView          `json:"requested_speaker_capabilities"`
}

func (p ScenarioEditViewStepActionsParameters) isScenarioEditViewStepParameters() {}

func (p *ScenarioEditViewStepActionsParameters) FromScenarioStepActionsParameters(ctx context.Context, parameters model.ScenarioStepActionsParameters, stereopairs model.Stereopairs) {
	if stereopairs == nil {
		stereopairs = parameters.Stereopairs.ToStereopairs()
	}
	p.LaunchDevices = make([]ScenarioEditViewLaunchDevice, 0, len(parameters.Devices))
	for _, d := range parameters.Devices {
		var launchDevice ScenarioEditViewLaunchDevice
		switch stereopairs.GetDeviceRole(d.ID) {
		case model.LeaderRole:
			stereopair, _ := stereopairs.GetByDeviceID(d.ID)
			launchDevice.FromScenarioLaunchStereopair(ctx, d, stereopair)
		case model.FollowerRole:
			// skip device
			continue
		default:
			launchDevice.FromScenarioLaunchDevice(d)
		}
		p.LaunchDevices = append(p.LaunchDevices, launchDevice)
	}

	p.RequestedSpeakerCapabilities = make([]CapabilityStateView, 0, len(parameters.RequestedSpeakerCapabilities))
	for _, sc := range parameters.RequestedSpeakerCapabilities {
		var capabilityStateView CapabilityStateView
		if capabilityStateView.FromRequestedSpeakerCapability(sc) {
			p.RequestedSpeakerCapabilities = append(p.RequestedSpeakerCapabilities, capabilityStateView)
		}
	}
}

type ScenarioLaunchDeviceActionResultView struct {
	Status     string `json:"status"`
	ActionTime string `json:"action_time"`
}

func (v *ScenarioLaunchDeviceActionResultView) FromScenarioLaunchDeviceActionResult(actionResult model.ScenarioLaunchDeviceActionResult) {
	// sorry for that
	// status is already stored in lowercase in db
	// however, everywhere else in mobile handlers we have uppercase
	// for style consistency we pray
	v.Status = strings.ToUpper(string(actionResult.Status))

	v.ActionTime = formatTimestamp(actionResult.ActionTime)
}

type ScenarioEditViewLaunchDevice struct {
	ID           string                                `json:"id"`
	Name         string                                `json:"name"`
	Type         model.DeviceType                      `json:"type"`
	QuasarInfo   *QuasarInfo                           `json:"quasar_info,omitempty"`
	RenderInfo   *RenderInfoView                       `json:"render_info,omitempty"`
	Error        string                                `json:"error,omitempty"`
	ErrorCode    model.ErrorCode                       `json:"error_code,omitempty"`
	Capabilities []CapabilityStateView                 `json:"capabilities"`
	ItemType     ItemInfoViewType                      `json:"item_type,omitempty"`
	Stereopair   *StereopairView                       `json:"stereopair,omitempty"`
	ActionResult *ScenarioLaunchDeviceActionResultView `json:"action_result,omitempty"`
}

func (d *ScenarioEditViewLaunchDevice) FromScenarioLaunchDevice(launchDevice model.ScenarioLaunchDevice) {
	d.ID = launchDevice.ID
	d.Name = launchDevice.Name
	d.Type = launchDevice.Type
	d.ItemType = DeviceItemInfoViewType
	d.RenderInfo = NewRenderInfoView(launchDevice.SkillID, launchDevice.Type, launchDevice.CustomData)

	capabilities := make([]CapabilityStateView, 0, len(launchDevice.Capabilities))
	for _, capability := range launchDevice.Capabilities {
		var capabilityStateView CapabilityStateView
		capabilityStateView.FromCapability(capability)
		capabilities = append(capabilities, capabilityStateView)
	}

	if launchDevice.ErrorCode != "" {
		d.Error = getErrorMessageByCode(model.ErrorCode(launchDevice.ErrorCode))
		d.ErrorCode = model.ErrorCode(launchDevice.ErrorCode)
	}

	if launchDevice.IsQuasarDevice() {
		var quasarInfo QuasarInfo
		quasarInfo.FromCustomData(launchDevice.CustomData, launchDevice.Type)
		d.QuasarInfo = &quasarInfo
	}

	if launchDevice.ActionResult != nil {
		var actionResult ScenarioLaunchDeviceActionResultView
		actionResult.FromScenarioLaunchDeviceActionResult(*launchDevice.ActionResult)
		d.ActionResult = &actionResult
	}

	sort.Sort(CapabilityStateViewSorting(capabilities))
	d.Capabilities = capabilities
}

func (d *ScenarioEditViewLaunchDevice) FromScenarioLaunchStereopair(ctx context.Context, leaderLaunchDevice model.ScenarioLaunchDevice, stereopair model.Stereopair) {
	d.FromScenarioLaunchDevice(leaderLaunchDevice)
	d.Name = stereopair.Name
	d.ItemType = StereopairItemInfoViewType
	d.Stereopair = &StereopairView{}
	d.Stereopair.From(ctx, stereopair)
}

type ScenarioEditViewStepDelayParameters struct {
	DelayMs int `json:"delay_ms"`
}

func (p ScenarioEditViewStepDelayParameters) isScenarioEditViewStepParameters() {}

func (p *ScenarioEditViewStepDelayParameters) FromScenarioStepDelayParameters(parameters model.ScenarioStepDelayParameters) {
	p.DelayMs = parameters.DelayMs
}

type ScenarioLaunchEditResultV3 struct {
	Status    string                   `json:"status"`
	RequestID string                   `json:"request_id"`
	Launch    ScenarioLaunchEditViewV3 `json:"launch"`
}

type ScenarioLaunchEditViewV3 struct {
	ID            string                       `json:"id"`
	Name          model.ScenarioName           `json:"name"`
	TriggerType   model.ScenarioTriggerType    `json:"launch_trigger_type"`
	Trigger       *ScenarioLaunchValueEditView `json:"launch_trigger,omitempty"`
	Steps         []ScenarioEditViewStep       `json:"steps"`
	CreatedTime   string                       `json:"created_time"`
	ScheduledTime string                       `json:"scheduled_time,omitempty"`
	FinishedTime  string                       `json:"finished_time,omitempty"`
	Status        model.ScenarioLaunchStatus   `json:"status"`
	PushOnInvoke  bool                         `json:"push_on_invoke"`
}

func (r *ScenarioLaunchEditResultV3) FromLaunch(ctx context.Context, now timestamp.PastTimestamp, launch model.ScenarioLaunch) error {
	r.Launch.ID = launch.ID
	r.Launch.Name = launch.ScenarioName

	if launch.Status == model.ScenarioLaunchFailed && launch.ErrorCode == "" {
		launch.ErrorCode = string(model.UnknownError)
	}

	launchSteps := launch.ScenarioSteps()
	r.Launch.Steps = make([]ScenarioEditViewStep, 0, len(launchSteps))
	for i, step := range launchSteps {
		var stepView ScenarioEditViewStep
		var stepStatus ScenarioStepStatus
		if i < launch.CurrentStepIndex {
			stepStatus = DoneScenarioStepStatus
		}
		if err := stepView.FromScenarioStep(ctx, step, nil, stepStatus); err != nil {
			return err
		}
		r.Launch.Steps = append(r.Launch.Steps, stepView)
	}

	if launch.CurrentStepIndex > 0 && launch.Status == model.ScenarioLaunchScheduled && len(launchSteps) >= launch.CurrentStepIndex {
		r.Launch.Steps[launch.CurrentStepIndex-1].FillProgressBar(now, launch.Scheduled)
	}

	r.Launch.CreatedTime = formatTimestamp(launch.Created)
	r.Launch.ScheduledTime = formatTimestamp(launch.Scheduled)
	r.Launch.Status = launch.Status

	if launch.Status.IsFinal() {
		r.Launch.FinishedTime = formatTimestamp(launch.Scheduled)
	}

	r.Launch.TriggerType = launch.LaunchTriggerType
	if launch.LaunchTriggerValue != nil {
		var sv ScenarioLaunchValueEditView
		if err := sv.FromLaunch(launch); err != nil {
			return err
		}
		r.Launch.Trigger = &sv
	}
	r.Launch.PushOnInvoke = launch.PushOnInvoke
	return nil
}

type ScenarioCreateResponseV3 struct {
	Status     string `json:"status"`
	RequestID  string `json:"request_id"`
	ScenarioID string `json:"scenario_id"`
}

type ScenarioEditRequestV3 struct {
	ScenarioCreateRequestV3
	ScenarioID string `json:"id"`
}

type ScenarioTriggerValidationRequestV3 struct {
	Scenario ScenarioEditRequestV3 `json:"scenario"`
	Trigger  model.ScenarioTrigger `json:"trigger"`
}

func (r *ScenarioTriggerValidationRequestV3) UnmarshalJSON(b []byte) error {
	cRaw := struct {
		Scenario ScenarioEditRequestV3 `json:"scenario"`
		Trigger  json.RawMessage       `json:"trigger"`
	}{}
	if err := json.Unmarshal(b, &cRaw); err != nil {
		return xerrors.Errorf("failed to unmarshal scenario trigger validation request: %w", err)
	}

	trigger, err := model.JSONUnmarshalTrigger(cRaw.Trigger)
	if err != nil {
		return xerrors.Errorf("failed to unmarshal scenario trigger in scenario trigger validation request: %w", err)
	}

	r.Scenario = cRaw.Scenario
	r.Trigger = trigger

	return nil
}

func (r ScenarioTriggerValidationRequestV3) Validate(vctx *valid.ValidationCtx) (bool, error) {
	var err valid.Errors

	triggers := make(model.ScenarioTriggers, 0, len(r.Scenario.Triggers)+1)
	triggers = append(triggers, r.Scenario.Triggers...)
	triggers = append(triggers, r.Trigger)

	if len(triggers.FilterByType(model.VoiceScenarioTriggerType)) > model.ScenarioVoiceTriggersNumLimit {
		err = append(err, &model.TooManyVoiceTriggersError{})
	}

	if len(triggers.FilterByType(model.TimetableScenarioTriggerType)) > 1 {
		err = append(err, &model.TooManyTimetableTriggersError{})
	}

	//triggers
	if _, e := triggers.Validate(vctx); e != nil {
		err = append(err, e)
	}

	for _, t := range triggers {
		if t.Type() == model.TimerScenarioTriggerType {
			err = append(err, &model.TimerTriggerScenarioCreationError{})
		}
	}

	// steps
	if len(r.Scenario.Steps) != 0 {
		if _, e := r.Scenario.Steps.Validate(vctx); e != nil {
			if ves, ok := e.(valid.Errors); ok {
				err = append(err, ves...)
			} else {
				err = append(err, e)
			}
		}
	}

	if len(err) == 0 {
		return false, nil
	}
	return false, err
}

func (r *ScenarioTriggerValidationRequestV3) ValidateRequest(ctx context.Context, logger log.Logger, begemotClient begemot.IClient, createdScenarioID string, userInfo model.UserInfo) error {
	if err := r.validateByExistingScenarios(userInfo.Scenarios); err != nil {
		return xerrors.Errorf("failed to validate by existing scenarios: %w", err)
	}
	if err := r.validateVoiceTrigger(ctx, logger, begemotClient, createdScenarioID, userInfo); err != nil {
		return xerrors.Errorf("failed to validate voice trigger: %w", err)
	}
	return nil
}

func (r *ScenarioTriggerValidationRequestV3) validateByExistingScenarios(userScenarios model.Scenarios) error {
	// validate trigger with current scenario existing triggers
	if voiceTrigger, ok := r.Trigger.(model.VoiceScenarioTrigger); ok {
		for _, phrase := range r.Scenario.Triggers.GetVoiceTriggerPhrases() {
			if tools.IsWordSetsEqual(phrase, voiceTrigger.Phrase) {
				return &model.VoiceTriggerPhraseAlreadyTakenError{}
			}
		}
	}

	// validate trigger with already existing scenario triggers
	scenarioPhrases := r.Scenario.Triggers.GetVoiceTriggerPhrases()
	scenarioPhrases = append(scenarioPhrases, model.ScenarioTriggers{r.Trigger}.GetVoiceTriggerPhrases()...)

	validateScenario := userScenarios
	if r.Scenario.ScenarioID != "" {
		// if we have scenario id in request -> it's update of already existing scenario; we skip it in validation
		validateScenario = make([]model.Scenario, 0, len(userScenarios))
		for _, s := range userScenarios {
			if s.ID == r.Scenario.ScenarioID {
				continue
			}
			validateScenario = append(validateScenario, s)
		}
	}

	for _, sp := range scenarioPhrases {
		for _, phrase := range validateScenario.GetVoiceTriggerPhrases() {
			if tools.IsWordSetsEqual(phrase, sp) {
				return &model.VoiceTriggerPhraseAlreadyTakenError{}
			}
		}
	}

	return nil
}

func (r *ScenarioTriggerValidationRequestV3) validateVoiceTrigger(ctx context.Context, logger log.Logger, begemotClient begemot.IClient, createdScenarioID string, userInfo model.UserInfo) error {
	voiceTrigger, isVoiceTrigger := r.Trigger.(model.VoiceScenarioTrigger)
	if !isVoiceTrigger {
		return nil
	}

	// create scenario from userInfo data with new trigger, add it to userInfo
	createdScenario := r.Scenario.ToScenario(userInfo)
	voiceTrigger.Phrase = tools.Standardize(voiceTrigger.Phrase)
	createdScenario.Triggers = append(createdScenario.Triggers, voiceTrigger)
	if createdScenario.ID = createdScenarioID; createdScenario.ID == "" {
		// this scenario is new - use fake begemot validation id to add it to userInfo
		createdScenario.ID = model.ScenarioBegemotValidationID
		userInfo.Scenarios = append(userInfo.Scenarios.Clone(), createdScenario)
	} else {
		// this scenario already exists - replace it in userInfo list
		userInfo.Scenarios = userInfo.Scenarios.ReplaceFirstByID(createdScenario)
	}

	// all scenario ids here
	allScenarioIDs := userInfo.Scenarios.GetIDs()

	// validate every push text in scenario
	ctxlog.Info(ctx, logger, "UserInfo in pushText validation", log.Any("user_info", userInfo))
	pushTexts := createdScenario.ScenarioSteps(userInfo.Devices).GetTextQuasarServerActionCapabilityValues()
	for _, pushText := range pushTexts {
		if err := begemot.ValidatePushText(ctx, logger, begemotClient, pushText, allScenarioIDs, userInfo); err != nil {
			return err
		}
	}
	return nil
}

type ScenarioQuasarCapabilityValidationRequestV3 struct {
	Scenario   ScenarioEditRequestV3 `json:"scenario"`
	Capability CapabilityActionView  `json:"capability"`
}

func (r ScenarioQuasarCapabilityValidationRequestV3) Validate(vctx *valid.ValidationCtx) (bool, error) {
	var verrs valid.Errors

	//type
	if _, e := r.Capability.Type.Validate(vctx); e != nil {
		if ves, ok := e.(valid.Errors); ok {
			verrs = append(verrs, ves...)
		} else {
			verrs = append(verrs, e)
		}
		return false, verrs // do not process further, as we dont know how to validate unknown types
	}

	if model.KnownQuasarCapabilityTypes.Contains(r.Capability.Type) {
		r.Scenario.Steps = append(r.Scenario.Steps, r.Capability.ToScenarioCreateRequestStep())

		// scenario steps
		if len(r.Scenario.Steps) != 0 {
			if _, e := r.Scenario.Steps.Validate(vctx); e != nil {
				if ves, ok := e.(valid.Errors); ok {
					verrs = append(verrs, ves...)
				} else {
					verrs = append(verrs, e)
				}
			}
		}
	} else {
		// there is no need to validate not quasar type (yet)
		return false, nil
	}

	if len(verrs) == 0 {
		return false, nil
	}
	return false, verrs
}

func (r ScenarioQuasarCapabilityValidationRequestV3) ValidatePushTextCapability(ctx context.Context, logger log.Logger, begemotClient begemot.IClient, createdScenarioID string, userInfo model.UserInfo) error {
	// if scenario has no server actions that push text - ignore
	cType, cInstance := model.QuasarServerActionCapabilityType, model.TextActionCapabilityInstance.String()
	isPushTextCapability := r.Capability.IsOfTypeAndInstance(cType, cInstance)
	if !isPushTextCapability {
		return nil
	}

	// create scenario from userInfo data with new steps with speaker, add it to userInfo
	createdScenario := r.Scenario.ToScenario(userInfo)
	createdScenario.Steps = append(createdScenario.Steps, r.Capability.ToScenarioCreateRequestStep().ToScenarioStep(userInfo.Devices))
	if createdScenario.ID = createdScenarioID; createdScenario.ID == "" {
		// this scenario is new - use fake begemot validation id to add it to userInfo
		createdScenario.ID = model.ScenarioBegemotValidationID
		userInfo.Scenarios = append(userInfo.Scenarios.Clone(), createdScenario)
	} else {
		// this scenario already exists - replace it in userInfo list
		userInfo.Scenarios = userInfo.Scenarios.ReplaceFirstByID(createdScenario)
	}

	// all scenario ids here
	allScenarioIDs := userInfo.Scenarios.GetIDs()

	// validate new push text in scenario
	ctxlog.Info(ctx, logger, "UserInfo in pushText validation", log.Any("user_info", userInfo))
	pushText := r.Capability.State.Value.(string)
	if err := begemot.ValidatePushText(ctx, logger, begemotClient, pushText, allScenarioIDs, userInfo); err != nil {
		return err
	}
	return nil
}
