package mobile

import (
	"fmt"
	"sort"

	"a.yandex-team.ru/alice/iot/bulbasaur/model"
)

type ModeCapabilityParameters struct {
	Instance     model.ModeCapabilityInstance `json:"instance"`
	InstanceName string                       `json:"name,omitempty"`
	Modes        []Mode                       `json:"modes"`
}
type Mode struct {
	Value model.ModeValue `json:"value"`
	Name  string          `json:"name"`
}

func (mcp ModeCapabilityParameters) GetInstanceName() string {
	return mcp.InstanceName
}

// TODO: cover by tests
// Actual func ignores user language using russian only
func (mcp *ModeCapabilityParameters) FromModeCapabilityParameters(parameters model.ModeCapabilityParameters) {
	mcp.Instance = parameters.Instance
	mcp.InstanceName = model.KnownModeInstancesNames[parameters.Instance]
	modes := make([]Mode, 0, len(parameters.Modes))
	for _, mode := range parameters.Modes {
		modes = append(modes, Mode{Value: mode.Value, Name: *model.KnownModes[mode.Value].Name})
	}
	sort.Sort(ModesSorting(modes))
	mcp.Modes = modes
}

type ToggleCapabilityParameters struct {
	Instance     model.ToggleCapabilityInstance `json:"instance"`
	InstanceName string                         `json:"name,omitempty"`
}

func (tcp ToggleCapabilityParameters) GetInstanceName() string {
	return tcp.InstanceName
}

// TODO: cover by tests
// Actual func ignores user language using russian only
func (tcp *ToggleCapabilityParameters) FromToggleCapabilityParameters(parameters model.ToggleCapabilityParameters) {
	tcp.Instance = parameters.Instance
	tcp.InstanceName = parameters.GetInstanceName()
}

type RangeCapabilityParameters struct {
	Instance     model.RangeCapabilityInstance `json:"instance"`
	InstanceName string                        `json:"name,omitempty"`
	Unit         model.Unit                    `json:"unit"`
	RandomAccess bool                          `json:"random_access"`
	Looped       bool                          `json:"looped"`
	Range        *model.Range                  `json:"range,omitempty"`
}

func (rcp RangeCapabilityParameters) GetInstanceName() string {
	return rcp.InstanceName
}

type Range struct {
	Min       float64 `json:"min"`
	Max       float64 `json:"max"`
	Precision float64 `json:"precision"`
}

// TODO: cover by tests
// Actual func ignores user language using russian only
func (rcp *RangeCapabilityParameters) FromRangeCapabilityParameters(parameters model.RangeCapabilityParameters) {
	rcp.Instance = parameters.Instance
	rcp.Range = parameters.Range
	rcp.Unit = parameters.Unit
	rcp.RandomAccess = parameters.RandomAccess
	rcp.Looped = parameters.Looped
	rcp.InstanceName = parameters.GetInstanceName()
}

type ColorSettingCapabilityParameters struct {
	Instance     string           `json:"instance"`
	InstanceName string           `json:"name,omitempty"`
	Palette      []ColorStateView `json:"palette"`
	Scenes       []ColorSceneView `json:"scenes"`
}

func (cscp ColorSettingCapabilityParameters) GetInstanceName() string {
	return cscp.InstanceName
}

func (cscp *ColorSettingCapabilityParameters) FromColorSettingCapabilityParameters(parameters model.ColorSettingCapabilityParameters) {
	cscp.Palette = ColorStateViewList(parameters.GetAvailableColors())
	cscp.Instance = ColorCapabilityInstance
	cscp.InstanceName = "цвет"

	availableScenes := parameters.GetAvailableScenes()
	sort.Sort(model.ColorSceneSorting(availableScenes))
	cscp.Scenes = make([]ColorSceneView, 0, len(availableScenes))
	for _, scene := range availableScenes {
		var sceneView ColorSceneView
		sceneView.FromColorScene(scene)
		cscp.Scenes = append(cscp.Scenes, sceneView)
	}
}

type ColorSettingCapabilityStateView struct {
	Instance string                 `json:"instance"`
	Value    IColorSettingStateView `json:"value"`
}

func (stateView *ColorSettingCapabilityStateView) FromCapability(capability model.ICapability) {
	if capability.Type() != model.ColorSettingCapabilityType {
		return
	}
	// nil-state processor
	state := capability.State()
	if state == nil {
		state = capability.DefaultState()
	}
	colorParameters := capability.Parameters().(model.ColorSettingCapabilityParameters)
	switch state.GetInstance() {
	case string(model.SceneCapabilityInstance):
		if colorSettingState, ok := state.(model.ColorSettingCapabilityState); ok {
			if colorSceneID, ok := colorSettingState.Value.(model.ColorSceneID); ok {
				var sceneViewValue ColorSceneView
				sceneViewValue.FromColorSceneID(colorSceneID)
				stateView.Instance = string(model.SceneCapabilityInstance)
				stateView.Value = sceneViewValue
				return
			}
		}
	default:
		color, ok := colorParameters.GetDefaultColor()
		if !ok {
			return
		}
		if colorSettingState, ok := state.(model.ColorSettingCapabilityState); ok {
			if stateColor, ok := colorSettingState.ToColor(); ok {
				color = stateColor
			}
		}
		// ColorStateView in mobile has a different Palette than Color from model,
		// so in case of different default color that needs to be done
		mobileColor := ColorStateView{}
		mobileColor.FromColor(color)
		stateView.Instance = ColorCapabilityInstance
		stateView.Value = mobileColor
	}

}

type CustomButtonCapabilityParameters struct {
	Instance     model.CustomButtonCapabilityInstance `json:"instance"`
	InstanceName string                               `json:"name"`
}

func (cbcp CustomButtonCapabilityParameters) GetInstanceName() string {
	return cbcp.InstanceName
}

// TODO: cover by tests
func (cbcp *CustomButtonCapabilityParameters) FromCustomButtonCapabilityParameters(parameters model.CustomButtonCapabilityParameters) {
	cbcp.Instance = parameters.Instance
	cbcp.InstanceName = parameters.GetInstanceName()
}

type OnOffCapabilityParameters struct {
	Split bool `json:"split"`
}

func (oocp *OnOffCapabilityParameters) FromOnOffCapabilityParameters(parameters model.OnOffCapabilityParameters) {
	oocp.Split = parameters.Split
}

func (oocp OnOffCapabilityParameters) GetInstanceName() string {
	return "включение/выключение"
}

type QuasarServerActionCapabilityParameters struct {
	Instance model.QuasarServerActionCapabilityInstance `json:"instance"`
}

func (sacp QuasarServerActionCapabilityParameters) GetInstanceName() string {
	return "команда для выполнения"
}

func (sacp *QuasarServerActionCapabilityParameters) FromQuasarServerActionCapabilityParameters(parameters model.QuasarServerActionCapabilityParameters) {
	sacp.Instance = parameters.Instance
}

type QuasarCapabilityParameters struct {
	Instance model.QuasarCapabilityInstance `json:"instance"`
}

func (sacp QuasarCapabilityParameters) GetInstanceName() string {
	return "команда на колонку"
}

func (sacp *QuasarCapabilityParameters) FromQuasarCapabilityParameters(parameters model.QuasarCapabilityParameters) {
	sacp.Instance = parameters.Instance
}

type VideoStreamCapabilityParameters struct {
	Protocols []string `json:"protocols"`
}

func (p VideoStreamCapabilityParameters) GetInstanceName() string {
	return "видеопоток"
}

func (p *VideoStreamCapabilityParameters) FromVideoStreamCapabilityParameters(parameters model.VideoStreamCapabilityParameters) {
	p.Protocols = make([]string, 0, len(parameters.Protocols))
	for _, protocol := range parameters.Protocols {
		p.Protocols = append(p.Protocols, string(protocol))
	}
}

type QuasarCapabilityStateView struct {
	Instance model.QuasarCapabilityInstance `json:"instance"`
	Value    model.QuasarCapabilityValue    `json:"value"`
}

func (v *QuasarCapabilityStateView) FromCapability(capability model.ICapability) {
	state := capability.State()
	if state == nil {
		state = capability.DefaultState()
	}
	quasarState := state.(model.QuasarCapabilityState)
	v.Instance = model.QuasarCapabilityInstance(capability.Instance())
	switch v.Instance {
	case model.NewsCapabilityInstance:
		var value NewsQuasarCapabilityValue
		value.FromNewsQuasarCapabilityValue(quasarState.Value.(model.NewsQuasarCapabilityValue))
		v.Value = value
	case model.SoundPlayCapabilityInstance:
		var value SoundPlayQuasarCapabilityValue
		value.FromSoundPlayQuasarCapabilityValue(quasarState.Value.(model.SoundPlayQuasarCapabilityValue))
		v.Value = value
	default:
		v.Value = quasarState.Value
	}
}

type NewsQuasarCapabilityValue struct {
	model.NewsQuasarCapabilityValue
	TopicName string `json:"topic_name"`
}

func (v *NewsQuasarCapabilityValue) FromNewsQuasarCapabilityValue(value model.NewsQuasarCapabilityValue) {
	v.NewsQuasarCapabilityValue = value
	if topicName, exist := speakerNewsTopicsNameMap[v.Topic]; exist {
		v.TopicName = topicName
	}
}

type SoundPlayQuasarCapabilityValue struct {
	model.SoundPlayQuasarCapabilityValue
	SoundName string `json:"sound_name"`
}

func (v *SoundPlayQuasarCapabilityValue) FromSoundPlayQuasarCapabilityValue(value model.SoundPlayQuasarCapabilityValue) {
	v.SoundPlayQuasarCapabilityValue = value
	if speakerSound, exist := model.KnownSpeakerSounds[value.Sound]; exist {
		v.SoundName = speakerSound.Name
	}
}

type IParameters interface {
	GetInstanceName() string
}

type CapabilityStateView struct {
	Retrievable bool                 `json:"retrievable"`
	Type        model.CapabilityType `json:"type"`
	Split       bool                 `json:"split,omitempty"`
	State       interface{}          `json:"state"`
	Parameters  IParameters          `json:"parameters"`
}

// TODO: cover by tests
func (c *CapabilityStateView) FromCapability(capability model.ICapability) {
	c.Retrievable = capability.Retrievable()
	c.Type = capability.Type()
	// nil-state processor
	if capability.State() == nil && capability.Retrievable() {
		capability.SetState(capability.DefaultState())
	}
	switch capability.Type() {
	case model.ColorSettingCapabilityType:
		colorParameters := capability.Parameters().(model.ColorSettingCapabilityParameters)
		var parameters ColorSettingCapabilityParameters
		parameters.FromColorSettingCapabilityParameters(colorParameters)
		c.Parameters = parameters
		var stateView ColorSettingCapabilityStateView
		stateView.FromCapability(capability)
		c.State = stateView
	case model.RangeCapabilityType:
		var parameters RangeCapabilityParameters
		parameters.FromRangeCapabilityParameters(capability.Parameters().(model.RangeCapabilityParameters))
		c.Parameters = parameters
		c.State = capability.State()
	case model.ModeCapabilityType:
		var parameters ModeCapabilityParameters
		parameters.FromModeCapabilityParameters(capability.Parameters().(model.ModeCapabilityParameters))
		c.Parameters = parameters
		c.State = capability.State()
	case model.ToggleCapabilityType:
		var parameters ToggleCapabilityParameters
		parameters.FromToggleCapabilityParameters(capability.Parameters().(model.ToggleCapabilityParameters))
		c.Parameters = parameters
		c.State = capability.State()
	case model.CustomButtonCapabilityType:
		var parameters CustomButtonCapabilityParameters
		parameters.FromCustomButtonCapabilityParameters(capability.Parameters().(model.CustomButtonCapabilityParameters))
		c.Parameters = parameters
		c.State = capability.State()
	case model.OnOffCapabilityType:
		var parameters OnOffCapabilityParameters
		parameters.FromOnOffCapabilityParameters(capability.Parameters().(model.OnOffCapabilityParameters))
		c.Parameters = parameters
		c.State = capability.State()
	case model.QuasarServerActionCapabilityType:
		var parameters QuasarServerActionCapabilityParameters
		parameters.FromQuasarServerActionCapabilityParameters(capability.Parameters().(model.QuasarServerActionCapabilityParameters))
		c.Parameters = parameters
		c.State = capability.State()
	case model.QuasarCapabilityType:
		var parameters QuasarCapabilityParameters
		parameters.FromQuasarCapabilityParameters(capability.Parameters().(model.QuasarCapabilityParameters))
		c.Parameters = parameters
		var stateView QuasarCapabilityStateView
		stateView.FromCapability(capability)
		c.State = stateView
	case model.VideoStreamCapabilityType:
		var parameters VideoStreamCapabilityParameters
		parameters.FromVideoStreamCapabilityParameters(capability.Parameters().(model.VideoStreamCapabilityParameters))
		c.Parameters = parameters
		c.State = capability.State()
	default:
		panic(fmt.Sprintf("unknown capability type: %q", capability.Type()))
	}
}

func (c *CapabilityStateView) FromRequestedSpeakerCapability(sc model.ScenarioCapability) bool {
	switch sc.Type {
	case model.QuasarCapabilityType:
		capability := model.MakeCapabilityByType(sc.Type)
		params := model.MakeQuasarCapabilityParametersByInstance(model.QuasarCapabilityInstance(sc.State.GetInstance()))
		capability.SetParameters(params)
		capability.SetState(sc.State)
		c.FromCapability(capability)
		return true
	case model.QuasarServerActionCapabilityType:
		capability := model.MakeCapabilityByType(sc.Type)
		params := model.MakeQuasarServerActionParametersByInstance(model.QuasarServerActionCapabilityInstance(sc.State.GetInstance()))
		capability.SetParameters(params)
		capability.SetState(sc.State)
		c.FromCapability(capability)
		return true
	default:
		return false
	}
}
