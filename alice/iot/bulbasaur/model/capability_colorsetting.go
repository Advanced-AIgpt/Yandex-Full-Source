package model

import (
	"encoding/json"
	"fmt"
	"math"
	"strings"

	"golang.org/x/exp/slices"

	bulbasaur "a.yandex-team.ru/alice/iot/bulbasaur/errors"
	"a.yandex-team.ru/alice/iot/bulbasaur/protos"
	"a.yandex-team.ru/alice/library/go/inflector"
	"a.yandex-team.ru/alice/library/go/timestamp"
	"a.yandex-team.ru/alice/library/go/tools"
	"a.yandex-team.ru/alice/megamind/protos/common"
	"a.yandex-team.ru/library/go/core/xerrors"
	"a.yandex-team.ru/library/go/valid"
)

var _ valid.Validator = new(TemperatureKParameters)
var _ valid.Validator = new(ColorSettingCapabilityParameters)
var _ valid.Validator = new(ColorModelType)

type ColorSettingCapability struct {
	reportable  bool
	retrievable bool
	state       *ColorSettingCapabilityState
	parameters  ColorSettingCapabilityParameters
	lastUpdated timestamp.PastTimestamp
}

func (c ColorSettingCapability) Type() CapabilityType {
	return ColorSettingCapabilityType
}

func (c *ColorSettingCapability) Instance() string {
	return capabilityInstance(c)
}

func (c *ColorSettingCapability) Reportable() bool {
	return c.reportable
}

func (c ColorSettingCapability) Retrievable() bool {
	return c.retrievable
}

func (c ColorSettingCapability) LastUpdated() timestamp.PastTimestamp {
	return c.lastUpdated
}

func (c ColorSettingCapability) State() ICapabilityState {
	if c.state == nil {
		return nil
	}
	return c.state.Clone()
}

func (c ColorSettingCapability) DefaultState() ICapabilityState {
	var value ColorSettingValue
	defaultColor, okDefaultColor := c.parameters.GetDefaultColor()
	switch instance := c.parameters.GetColorSettingCapabilityInstance(); instance {
	case TemperatureKCapabilityInstance:
		if okDefaultColor {
			value = defaultColor.Temperature
		}
	case RgbColorCapabilityInstance:
		if okDefaultColor {
			value = defaultColor.ValueRGB
		}
	case HsvColorCapabilityInstance:
		if okDefaultColor {
			value = defaultColor.ValueHSV
		}
	case SceneCapabilityInstance:
		if c.parameters.ColorSceneParameters != nil && len(c.parameters.ColorSceneParameters.Scenes) > 0 {
			value = c.parameters.ColorSceneParameters.Scenes[0].ID
		}
	default:
		panic(fmt.Sprintf("unknown GetColorSettingCapabilityInstance: `%s`", instance))
	}
	return ColorSettingCapabilityState{
		Instance: c.parameters.GetColorSettingCapabilityInstance(),
		Value:    value,
	}
}

func (c ColorSettingCapability) Parameters() ICapabilityParameters {
	return c.parameters
}

func (c *ColorSettingCapability) Key() string {
	return CapabilityKey(c.Type(), c.Instance())
}

func (c *ColorSettingCapability) IsInternal() bool {
	return false
}

func (c *ColorSettingCapability) SetReportable(reportable bool) {
	c.reportable = reportable
}

func (c *ColorSettingCapability) BasicSuggestions(dt DeviceType, inflection inflector.Inflection, options SuggestionsOptions) []string {
	suggestions := make([]string, 0)
	parameters := c.parameters
	if parameters.TemperatureK != nil && dt == LightDeviceType {
		suggestions = append(suggestions,
			fmt.Sprintf("Сделай свет %s потеплее", inflection.Rod),
		)
	}
	defaultColor, ok := parameters.GetDefaultColor()
	if ok {
		defaultColorName := strings.ToLower(defaultColor.Name)
		suggestions = append(suggestions,
			fmt.Sprintf("Включи %s свет на %s", defaultColorName, inflection.Pr),
		)
	}
	if parameters := c.Parameters().(ColorSettingCapabilityParameters); parameters.ColorSceneParameters != nil {
		if scenes := parameters.GetAvailableScenes(); len(scenes) > 0 {
			// due to discovery validation it always safe to call
			// we need to get suggestions without color scene with name Alice because suggestions would look ugly otherwise
			defaultSceneID := scenes[0].ID
			if defaultSceneID == ColorSceneIDAlice && len(scenes) > 1 {
				defaultSceneID = scenes[1].ID
			}
			defaultSceneName := KnownColorScenes[defaultSceneID].Name
			suggestions = append(suggestions,
				"поставь режим "+defaultSceneName+" на "+inflection.Pr,
			)
		}
	}
	for i := range suggestions {
		suggestions[i] = options.AddHouseholdToSuggest(suggestions[i])
	}
	return suggestions
}

func (c *ColorSettingCapability) QuerySuggestions(inflection inflector.Inflection, options SuggestionsOptions) []string {
	suggestions := []string{
		fmt.Sprintf("Какой цвет горит на %s?", inflection.Pr),
	}
	for i := range suggestions {
		suggestions[i] = options.AddHouseholdToSuggest(suggestions[i])
	}
	return suggestions
}

func (c *ColorSettingCapability) SetRetrievable(retrievable bool) {
	c.retrievable = retrievable
}

func (c *ColorSettingCapability) SetLastUpdated(lastUpdated timestamp.PastTimestamp) {
	c.lastUpdated = lastUpdated
}

func (c *ColorSettingCapability) WithLastUpdated(lastUpdated timestamp.PastTimestamp) ICapabilityWithBuilder {
	c.SetLastUpdated(lastUpdated)
	return c
}

func (c *ColorSettingCapability) SetState(state ICapabilityState) {
	structure, ok := state.(ColorSettingCapabilityState)
	if ok {
		c.state = &structure
	}
	pointerState, ok := state.(*ColorSettingCapabilityState)
	if ok {
		c.state = pointerState
	}
}

func (c *ColorSettingCapability) SetParameters(parameters ICapabilityParameters) {
	c.parameters = parameters.(ColorSettingCapabilityParameters)
}

func (c *ColorSettingCapability) WithReportable(reportable bool) ICapabilityWithBuilder {
	c.SetReportable(reportable)
	return c
}

func (c *ColorSettingCapability) WithRetrievable(retrievable bool) ICapabilityWithBuilder {
	c.SetRetrievable(retrievable)
	return c
}

func (c *ColorSettingCapability) WithState(state ICapabilityState) ICapabilityWithBuilder {
	c.SetState(state)
	return c
}

func (c *ColorSettingCapability) WithParameters(parameters ICapabilityParameters) ICapabilityWithBuilder {
	c.SetParameters(parameters)
	return c
}

func (c *ColorSettingCapability) Merge(capability ICapability) (ICapability, bool) {
	return mergeCapability(c, capability)
}

func (c *ColorSettingCapability) Clone() ICapabilityWithBuilder {
	return cloneCapability(c)
}

func (c *ColorSettingCapability) Equal(capability ICapability) bool {
	return equalCapability(c, capability)
}

func (c *ColorSettingCapability) MarshalJSON() ([]byte, error) {
	return marshalCapability(c)
}

func (c *ColorSettingCapability) UnmarshalJSON(b []byte) error {
	cRaw := rawCapability{}
	if err := json.Unmarshal(b, &cRaw); err != nil {
		return err
	}
	c.SetReportable(cRaw.Reportable)
	c.SetRetrievable(cRaw.Retrievable)
	c.SetLastUpdated(cRaw.LastUpdated)

	p := ColorSettingCapabilityParameters{}
	if err := json.Unmarshal(cRaw.Parameters, &p); err != nil {
		return err
	}
	c.SetParameters(p)

	if cRaw.State != nil && string(cRaw.State) != "null" {
		s := ColorSettingCapabilityState{}
		if err := json.Unmarshal(cRaw.State, &s); err != nil {
			return err
		}
		c.SetState(s)
	}
	return nil
}

func (c ColorSettingCapability) ToProto() *protos.Capability {
	pc := &protos.Capability{
		Type:        *c.Type().toProto(),
		Reportable:  c.Reportable(),
		Retrievable: c.Retrievable(),
		LastUpdated: float64(c.LastUpdated()),
	}

	pc.Parameters = &protos.Capability_CSCParameters{
		CSCParameters: &protos.ColorSettingCapabilityParameters{},
	}
	myParams := c.Parameters().(ColorSettingCapabilityParameters)
	if myParams.ColorModel != nil {
		pc.GetCSCParameters().ColorModel = &protos.ColorModel{
			ColorModelType: *myParams.ColorModel.toProto(),
		}
	}
	if myParams.TemperatureK != nil {
		pc.GetCSCParameters().TemperatureK = &protos.TemperatureKCapabilityParameters{
			Min: int32(myParams.TemperatureK.Min),
			Max: int32(myParams.TemperatureK.Max),
		}
	}
	if myParams.ColorSceneParameters != nil {
		pc.GetCSCParameters().ColorScene = myParams.ColorSceneParameters.toProto()
	}

	if c.State() != nil {
		s := c.State().(ColorSettingCapabilityState)
		pc.State = &protos.Capability_CSCState{
			CSCState: s.toProto(),
		}
	}

	return pc
}

func (c *ColorSettingCapability) FromProto(p *protos.Capability) {
	nc := ColorSettingCapability{}
	nc.SetReportable(p.Reportable)
	nc.SetRetrievable(p.Retrievable)
	nc.SetLastUpdated(timestamp.PastTimestamp(p.LastUpdated))

	params := ColorSettingCapabilityParameters{}
	if p.GetCSCParameters().ColorModel != nil {
		var cModel ColorModelType
		cModel.fromProto(&p.GetCSCParameters().ColorModel.ColorModelType)
		params.ColorModel = CM(cModel)
	}
	if p.GetCSCParameters().TemperatureK != nil {
		tk := TemperatureKParameters{
			Min: TemperatureK(p.GetCSCParameters().TemperatureK.Min),
			Max: TemperatureK(p.GetCSCParameters().TemperatureK.Max),
		}
		params.TemperatureK = &tk
	}
	if p.GetCSCParameters().ColorScene != nil {
		csp := ColorSceneParameters{}
		csp.fromProto(p.GetCSCParameters().ColorScene)
		params.ColorSceneParameters = &csp
	}
	nc.SetParameters(params)

	if p.State != nil {
		var s ColorSettingCapabilityState
		s.fromProto(p.GetCSCState())
		nc.SetState(s)
	}

	*c = nc
}

func (c ColorSettingCapability) ToUserInfoProto() *common.TIoTUserInfo_TCapability {
	pc := &common.TIoTUserInfo_TCapability{
		Type:          common.TIoTUserInfo_TCapability_ColorSettingCapabilityType,
		Retrievable:   c.Retrievable(),
		Reportable:    c.Reportable(),
		LastUpdated:   float64(c.LastUpdated()),
		AnalyticsType: analyticsCapabilityType(c.Type()),
		AnalyticsName: analyticsCapabilityName(analyticsCapabilityKey{Type: c.Type(), Instance: c.Instance()}),
	}
	pc.Parameters = &common.TIoTUserInfo_TCapability_ColorSettingCapabilityParameters{
		ColorSettingCapabilityParameters: c.parameters.ToUserInfoProto(),
	}
	if c.state != nil {
		pc.State = &common.TIoTUserInfo_TCapability_ColorSettingCapabilityState{
			ColorSettingCapabilityState: c.state.ToUserInfoProto(),
		}
	}
	return pc
}

func (c *ColorSettingCapability) FromUserInfoProto(p *common.TIoTUserInfo_TCapability) {
	nc := ColorSettingCapability{}
	nc.SetReportable(p.GetReportable())
	nc.SetRetrievable(p.GetRetrievable())
	nc.SetLastUpdated(timestamp.PastTimestamp(p.GetLastUpdated()))

	if paramsProto := p.GetColorSettingCapabilityParameters(); paramsProto != nil {
		params := ColorSettingCapabilityParameters{}
		if colorModelProto := paramsProto.GetColorModel(); colorModelProto != nil {
			var cModel ColorModelType
			colorModelTypeProto := colorModelProto.GetType()
			cModel.fromUserInfoProto(&colorModelTypeProto)
			params.ColorModel = CM(cModel)
		}
		if temperatureKProto := paramsProto.GetTemperatureK(); temperatureKProto != nil {
			tk := TemperatureKParameters{
				Min: TemperatureK(temperatureKProto.GetMin()),
				Max: TemperatureK(temperatureKProto.GetMax()),
			}
			params.TemperatureK = &tk
		}
		if colorSceneParametersProto := paramsProto.GetColorSceneParameters(); colorSceneParametersProto != nil {
			csp := ColorSceneParameters{}
			csp.fromUserInfoProto(colorSceneParametersProto)
			params.ColorSceneParameters = &csp
		}
		nc.SetParameters(params)
	}

	if stateProto := p.GetColorSettingCapabilityState(); stateProto != nil {
		var s ColorSettingCapabilityState
		s.fromUserInfoProto(stateProto)
		nc.SetState(s)
	}

	*c = nc
}

type ColorSettingCapabilityParameters struct {
	ColorModel           *ColorModelType         `json:"color_model,omitempty" yson:"color_model,omitempty"`
	TemperatureK         *TemperatureKParameters `json:"temperature_k,omitempty" yson:"temperature_k,omitempty"`
	ColorSceneParameters *ColorSceneParameters   `json:"color_scene,omitempty" yson:"color_scene,omitempty"`
}

func (c ColorSettingCapabilityParameters) GetInstance() string {
	return string(c.GetColorSettingCapabilityInstance())
}

func (c ColorSettingCapabilityParameters) GetColorSettingCapabilityInstance() ColorSettingCapabilityInstance {
	switch {
	case c.ColorModel != nil:
		switch cm := *c.ColorModel; cm {
		case RgbModelType:
			return RgbColorCapabilityInstance
		case HsvModelType:
			return HsvColorCapabilityInstance
		default:
			panic(fmt.Sprintf("unknown ColorModel: `%s`", cm))
		}
	case c.TemperatureK != nil:
		return TemperatureKCapabilityInstance
	default:
		return SceneCapabilityInstance
	}
}

func (c ColorSettingCapabilityParameters) Merge(params ICapabilityParameters) ICapabilityParameters {
	otherCSParameters, isCSParameters := params.(ColorSettingCapabilityParameters)
	if !isCSParameters {
		return nil
	}

	vctx := valid.NewValidationCtx()

	mergedCSParameters := ColorSettingCapabilityParameters{}
	if tempK, otherTempK := c.TemperatureK, otherCSParameters.TemperatureK; tempK != nil && otherTempK != nil {
		mergedCSParameters.TemperatureK = &TemperatureKParameters{
			Min: TemperatureK(math.Max(float64(tempK.Min), float64(otherTempK.Min))),
			Max: TemperatureK(math.Min(float64(tempK.Max), float64(otherTempK.Max))),
		}
		if _, err := mergedCSParameters.TemperatureK.Validate(vctx); err != nil {
			mergedCSParameters.TemperatureK = nil
		}
	}
	if colorModel, otherColorModel := c.ColorModel, otherCSParameters.ColorModel; colorModel != nil && otherColorModel != nil {
		mergedCSParameters.ColorModel = CM(HsvModelType) // explicitly set parameters to HSV if they exist
	}
	if scene, otherScene := c.ColorSceneParameters, otherCSParameters.ColorSceneParameters; scene != nil && otherScene != nil {
		sceneMap := scene.Scenes.AsMap()
		mergedScenes := make(ColorScenes, 0)
		for _, scene := range otherScene.Scenes {
			if _, exist := sceneMap[scene.ID]; exist {
				mergedScenes = append(mergedScenes, scene)
			}
		}
		mergedCSParameters.ColorSceneParameters = &ColorSceneParameters{Scenes: mergedScenes}
	}
	if _, err := mergedCSParameters.Validate(vctx); err != nil {
		return nil
	}
	return mergedCSParameters
}

func (c ColorSettingCapabilityParameters) Validate(vctx *valid.ValidationCtx) (bool, error) {
	var err valid.Errors
	if c.ColorModel == nil && c.TemperatureK == nil && c.ColorSceneParameters == nil {
		err = append(err, fmt.Errorf("you have to specify either color_model or temperature_k or color_scene"))
		return false, err
	}

	if c.ColorModel != nil {
		if _, e := c.ColorModel.Validate(vctx); e != nil {
			err = append(err, e)
		}
	}

	if c.TemperatureK != nil {
		if _, e := c.TemperatureK.Validate(vctx); e != nil {
			if ves, ok := e.(valid.Errors); ok {
				err = append(err, ves...)
			} else {
				err = append(err, e)
			}
		}
	}

	if c.ColorSceneParameters != nil {
		if _, e := c.ColorSceneParameters.Validate(vctx); e != nil {
			if ves, ok := e.(valid.Errors); ok {
				err = append(err, ves...)
			} else {
				err = append(err, e)
			}
		}
	}

	if len(err) > 0 {
		return false, err
	}
	return false, nil
}

//returns sorted colors available by current parameters
func (c ColorSettingCapabilityParameters) GetAvailableColors() []Color {
	var colors []Color
	//whities
	if c.TemperatureK != nil {
		whities := make([]Color, 0)
		for _, color := range ColorPalette.FilterType(WhiteColor).ToSortedSlice() {
			if c.TemperatureK.Min <= color.Temperature && color.Temperature <= c.TemperatureK.Max {
				whities = append(whities, color)
			}
		}

		//fallback
		if len(whities) == 0 {
			colors = append(colors, ColorPalette.GetDefaultWhiteColor())
		} else {
			colors = append(colors, whities...)
		}
	}

	//seats for colored
	if c.ColorModel != nil {
		colors = append(colors, ColorPalette.FilterType(Multicolor).ToSortedSlice()...)
	}

	return colors
}

func (c ColorSettingCapabilityParameters) GetAvailableScenes() ColorScenes {
	scenes := make(ColorScenes, 0)
	if c.ColorSceneParameters != nil {
		scenes = append(scenes, c.ColorSceneParameters.Scenes...)
	}
	return scenes
}

func (c ColorSettingCapabilityParameters) GetDefaultColor() (Color, bool) {
	// We can do that cause of ColorSettingCapabilityParameters.Validate() guarantees a non empty color slice
	colors := c.GetAvailableColors()
	if len(colors) == 0 {
		return Color{}, false
	}
	return colors[0], true
}

func (c ColorSettingCapabilityParameters) ToUserInfoProto() *common.TIoTUserInfo_TCapability_TColorSettingCapabilityParameters {
	p := &common.TIoTUserInfo_TCapability_TColorSettingCapabilityParameters{}
	if c.ColorModel != nil {
		p.ColorModel = &common.TIoTUserInfo_TCapability_TColorSettingCapabilityParameters_TColorModel{
			Type:          c.ColorModel.toUserInfoProto(),
			AnalyticsName: analyticsCapabilityName(analyticsCapabilityKey{Type: ColorSettingCapabilityType, Instance: string(HsvColorCapabilityInstance)}),
		}
	}
	if c.TemperatureK != nil {
		p.TemperatureK = &common.TIoTUserInfo_TCapability_TColorSettingCapabilityParameters_TTemperatureKCapabilityParameters{
			Min:           int32(c.TemperatureK.Min),
			Max:           int32(c.TemperatureK.Max),
			AnalyticsName: analyticsCapabilityName(analyticsCapabilityKey{Type: ColorSettingCapabilityType, Instance: string(TemperatureKCapabilityInstance)}),
		}
	}
	if c.ColorSceneParameters != nil {
		p.ColorSceneParameters = &common.TIoTUserInfo_TCapability_TColorSettingCapabilityParameters_TColorSceneParameters{
			Scenes:        make([]*common.TIoTUserInfo_TCapability_TColorSettingCapabilityParameters_TColorScene, 0, len(c.ColorSceneParameters.Scenes)), // every code should have a cup of java in it
			AnalyticsName: analyticsCapabilityName(analyticsCapabilityKey{Type: ColorSettingCapabilityType, Instance: string(SceneCapabilityInstance)}),
		}
		for _, scene := range c.ColorSceneParameters.Scenes {
			if protoScene := scene.toUserInfoProto(); protoScene != nil {
				p.ColorSceneParameters.Scenes = append(p.ColorSceneParameters.Scenes, protoScene)
			}
		}
	}
	return p
}

type ColorSettingCapabilityInstance string

func (i ColorSettingCapabilityInstance) String() string {
	return string(i)
}

type TemperatureKParameters struct {
	Min TemperatureK `json:"min" yson:"min"`
	Max TemperatureK `json:"max" yson:"max"`
}

func (t TemperatureKParameters) Validate(_ *valid.ValidationCtx) (bool, error) {
	var err valid.Errors

	if t.Min < 100 {
		err = append(err, fmt.Errorf("min temperature_k cannot be less than 100"))
	}
	if t.Max > 10000 {
		err = append(err, fmt.Errorf("max temperature_k cannot be more than 10000"))
	}
	if t.Max < t.Min {
		err = append(err, fmt.Errorf("max temperature_k cannot be less than min"))
	}

	if len(err) > 0 {
		return false, err
	}
	return false, nil
}

type ColorModelType string

// small hack to get address of constant
func CM(cm ColorModelType) *ColorModelType { return &cm }

func (cmt ColorModelType) Validate(_ *valid.ValidationCtx) (bool, error) {
	if len(cmt) == 0 {
		return false, fmt.Errorf("color_model is empty")
	}

	if !tools.Contains(string(cmt), KnownColorModels) {
		return false, fmt.Errorf("unknown color_model: %s", cmt)
	}

	return false, nil
}

func (cmt ColorModelType) toProto() *protos.ColorModelType {
	v, ok := colorModelTypeToProtoMap[ColorModelType(cmt)]
	if !ok {
		panic(fmt.Sprintf("unknown color model: `%s`", cmt))
	}
	return &v
}

func (cmt *ColorModelType) fromProto(pcmt *protos.ColorModelType) {
	v, ok := protoToColorModelTypeMap[*pcmt]
	if !ok {
		panic(fmt.Sprintf("unknown color model: `%s`", string(*pcmt)))
	}
	*cmt = v
}

func (cmt ColorModelType) toUserInfoProto() common.TIoTUserInfo_TCapability_TColorSettingCapabilityParameters_TColorModel_EColorModelType {
	v, ok := mmColorModelTypeToProtoMap[cmt]
	if !ok {
		return common.TIoTUserInfo_TCapability_TColorSettingCapabilityParameters_TColorModel_UnknownColorModel
	}
	return v
}

func (cmt *ColorModelType) fromUserInfoProto(p *common.TIoTUserInfo_TCapability_TColorSettingCapabilityParameters_TColorModel_EColorModelType) {
	colorModelType, ok := mmProtoToColorModelTypeMap[*p]
	if !ok {
		panic(fmt.Sprintf("unknown color model: %q", string(*p)))
	}
	*cmt = colorModelType
}

type ColorSceneParameters struct {
	Scenes ColorScenes `json:"scenes"`
}

func (p ColorSceneParameters) Validate(_ *valid.ValidationCtx) (bool, error) {
	var err valid.Errors

	for _, scene := range p.Scenes {
		if _, exist := KnownColorScenes[scene.ID]; !exist {
			err = append(err, fmt.Errorf("unknown color scene id %s", scene.ID))
		}
	}

	if len(p.Scenes) == 0 {
		return false, xerrors.New("color_setting validation failed: expected at least one scene in scenes list")
	}

	if len(err) > 0 {
		return false, err
	}
	return false, nil
}

func (p *ColorSceneParameters) toProto() *protos.ColorSceneParameters {
	result := &protos.ColorSceneParameters{}
	for _, scene := range p.Scenes {
		if protoScene := scene.toProto(); protoScene != nil {
			result.Scenes = append(result.Scenes, protoScene)
		}
	}
	return result
}

func (p *ColorSceneParameters) fromProto(pcsp *protos.ColorSceneParameters) {
	p.Scenes = make(ColorScenes, 0, len(pcsp.Scenes))
	for _, scene := range pcsp.Scenes {
		var s ColorScene
		s.fromProto(scene)
		p.Scenes = append(p.Scenes, s)
	}
}

func (p *ColorSceneParameters) toUserInfoProto() *common.TIoTUserInfo_TCapability_TColorSettingCapabilityParameters_TColorSceneParameters {
	result := &common.TIoTUserInfo_TCapability_TColorSettingCapabilityParameters_TColorSceneParameters{}
	for _, scene := range p.Scenes {
		if protoScene := scene.toUserInfoProto(); protoScene != nil {
			result.Scenes = append(result.Scenes, protoScene)
		}
	}
	return result
}

func (p *ColorSceneParameters) fromUserInfoProto(pcsp *common.TIoTUserInfo_TCapability_TColorSettingCapabilityParameters_TColorSceneParameters) {
	p.Scenes = make(ColorScenes, 0, len(pcsp.GetScenes()))
	for _, scene := range pcsp.GetScenes() {
		s := ColorScene{}
		s.fromUserInfoProto(scene)
		p.Scenes = append(p.Scenes, s)
	}
}

type ColorSettingCapabilityState struct {
	Instance ColorSettingCapabilityInstance `json:"instance" yson:"instance"`
	Value    ColorSettingValue              `json:"value" yson:"value"`
}

func (cscs ColorSettingCapabilityState) GetInstance() string {
	return string(cscs.Instance)
}

func (cscs *ColorSettingCapabilityState) UnmarshalJSON(b []byte) (err error) {
	sRaw := struct {
		Instance ColorSettingCapabilityInstance
		Value    json.RawMessage
	}{}
	if err := json.Unmarshal(b, &sRaw); err != nil {
		return err
	}

	cscs.Instance = sRaw.Instance

	switch sRaw.Instance {
	case TemperatureKCapabilityInstance:
		var v TemperatureK
		err = json.Unmarshal(sRaw.Value, &v)
		cscs.Value = v
	case HsvColorCapabilityInstance:
		var v HSV
		err = json.Unmarshal(sRaw.Value, &v)
		cscs.Value = v
	case RgbColorCapabilityInstance:
		var v RGB
		err = json.Unmarshal(sRaw.Value, &v)
		cscs.Value = v
	case SceneCapabilityInstance:
		var v ColorSceneID
		err = json.Unmarshal(sRaw.Value, &v)
		cscs.Value = v
	default:
		err = fmt.Errorf("unknown color_setting state instance: `%s`", sRaw.Instance)
	}

	if err != nil {
		return xerrors.Errorf("cannot parse capability state: %w", err)
	}
	return nil
}

func (cscs ColorSettingCapabilityState) Merge(state ICapabilityState) ICapabilityState {
	otherCSState, isColorSettingState := state.(ColorSettingCapabilityState)
	if !isColorSettingState {
		return nil
	}
	// scene merging
	if cscs.Instance == SceneCapabilityInstance || otherCSState.Instance == SceneCapabilityInstance {
		if cscs.Instance != otherCSState.Instance {
			return nil
		}
		scene, ok := cscs.Value.(ColorSceneID)
		otherScene, otherOk := cscs.Value.(ColorSceneID)
		if ok && otherOk && scene == otherScene {
			return cscs
		} else {
			return nil
		}
	}
	// temperature and color model merging
	color, isKnownColor := cscs.ToColor()
	otherColor, isOtherKnownColor := otherCSState.ToColor()
	if !isKnownColor || !isOtherKnownColor {
		return nil
	}
	if color == otherColor {
		switch {
		case cscs.Instance == TemperatureKCapabilityInstance && otherCSState.Instance == TemperatureKCapabilityInstance:
			return color.ToColorSettingCapabilityState(TemperatureKCapabilityInstance)
		case (cscs.Instance == HsvColorCapabilityInstance || cscs.Instance == RgbColorCapabilityInstance) &&
			(otherCSState.Instance == HsvColorCapabilityInstance || otherCSState.Instance == RgbColorCapabilityInstance):
			return color.ToColorSettingCapabilityState(HsvColorCapabilityInstance)
		}
	}
	return nil
}

func (cscs ColorSettingCapabilityState) Type() CapabilityType {
	return ColorSettingCapabilityType
}

func (cscs ColorSettingCapabilityState) ValidateState(capability ICapability) error {
	var err bulbasaur.Errors

	if p, ok := capability.Parameters().(ColorSettingCapabilityParameters); ok {
		switch cscs.Instance {
		case TemperatureKCapabilityInstance:
			if p.TemperatureK == nil {
				err = append(err, fmt.Errorf("unsupported by current device state color_setting instance: '%s'", cscs.Instance))
			} else {
				v, _ := cscs.Value.(TemperatureK)
				if v < TemperatureK(p.TemperatureK.Min) || TemperatureK(p.TemperatureK.Max) < v {
					err = append(err, fmt.Errorf("temperature_k color_setting instance state value is out of supported range: '%d'", cscs.Value))
				}
			}
		case HsvColorCapabilityInstance:
			if p.ColorModel != nil && *p.ColorModel == HsvModelType {
				hsv, _ := cscs.Value.(HSV)
				if hsv.H < 0 || 360 < hsv.H {
					err = append(err, fmt.Errorf("hsv hue color_setting state value is out of supported range: '%d'", hsv.H))
				}
				if hsv.S < 0 || 100 < hsv.S {
					err = append(err, fmt.Errorf("hsv saturation color_setting state value is out of supported range: '%d'", hsv.S))
				}
				if hsv.V < 0 || 100 < hsv.V {
					err = append(err, fmt.Errorf("hsv value color_setting state value is out of supported range: '%d'", hsv.V))
				}
			} else {
				err = append(err, fmt.Errorf("unsupported by current device color_setting state instance: '%s'", cscs.Instance))
			}
		case RgbColorCapabilityInstance:
			if p.ColorModel != nil && *p.ColorModel == RgbModelType {
				rgb, _ := cscs.Value.(RGB)
				if rgb < 0 || 16777215 < rgb {
					err = append(err, fmt.Errorf("rgb color_setting instance state value is out of supported range: '%d'", cscs.Value))
				}
			} else {
				err = append(err, fmt.Errorf("unsupported by current device color_setting state instance: '%s'", cscs.Instance))
			}
		case SceneCapabilityInstance:
			if p.ColorSceneParameters != nil {
				sceneID, _ := cscs.Value.(ColorSceneID)
				sceneIDs := make([]string, 0, len(p.ColorSceneParameters.Scenes))
				for _, scene := range p.ColorSceneParameters.Scenes {
					sceneIDs = append(sceneIDs, string(scene.ID))
				}
				if !slices.Contains(sceneIDs, string(sceneID)) {
					err = append(err, fmt.Errorf("scene value color_setting state value is not supported by current device: '%s'", sceneID))
				}
			} else {
				err = append(err, fmt.Errorf("unsupported by current device color_setting state instance: '%s'", cscs.Instance))
			}
		default:
			err = append(err, fmt.Errorf("unknown color_setting state instance: '%s'", cscs.Instance))
		}
	}

	if len(err) > 0 {
		return err
	}
	return nil
}

func (cscs ColorSettingCapabilityState) ToIotCapabilityAction() *common.TIoTCapabilityAction {
	return &common.TIoTCapabilityAction{
		Type: ColorSettingCapabilityType.ToUserInfoProto(),
		State: &common.TIoTCapabilityAction_ColorSettingCapabilityState{
			ColorSettingCapabilityState: cscs.ToUserInfoProto(),
		},
	}
}

func (cscs *ColorSettingCapabilityState) toProto() *protos.ColorSettingCapabilityState {
	p := &protos.ColorSettingCapabilityState{
		Instance: cscs.GetInstance(),
	}
	switch cscs.Instance {
	case TemperatureKCapabilityInstance:
		p.Value = &protos.ColorSettingCapabilityState_TemperatureK{
			TemperatureK: int32(cscs.Value.(TemperatureK)),
		}
	case RgbColorCapabilityInstance:
		p.Value = &protos.ColorSettingCapabilityState_RGB{
			RGB: int32(cscs.Value.(RGB)),
		}
	case HsvColorCapabilityInstance:
		p.Value = &protos.ColorSettingCapabilityState_HSV{
			HSV: &protos.HSV{
				H: int32(cscs.Value.(HSV).H),
				S: int32(cscs.Value.(HSV).S),
				V: int32(cscs.Value.(HSV).V),
			},
		}
	case SceneCapabilityInstance:
		p.Value = &protos.ColorSettingCapabilityState_ColorSceneID{ColorSceneID: string(cscs.Value.(ColorSceneID))}
	}
	return p
}

func (cscs *ColorSettingCapabilityState) fromProto(p *protos.ColorSettingCapabilityState) {
	state := ColorSettingCapabilityState{
		Instance: ColorSettingCapabilityInstance(p.Instance),
	}
	switch p.Value.(type) {
	case *protos.ColorSettingCapabilityState_TemperatureK:
		state.Value = TemperatureK(p.GetTemperatureK())
	case *protos.ColorSettingCapabilityState_RGB:
		state.Value = RGB(p.GetRGB())
	case *protos.ColorSettingCapabilityState_HSV:
		state.Value = HSV{
			H: int(p.GetHSV().H),
			S: int(p.GetHSV().S),
			V: int(p.GetHSV().V),
		}
	case *protos.ColorSettingCapabilityState_ColorSceneID:
		state.Value = ColorSceneID(p.GetColorSceneID())
	}
	*cscs = state
}

func (cscs ColorSettingCapabilityState) ToUserInfoProto() *common.TIoTUserInfo_TCapability_TColorSettingCapabilityState {
	p := &common.TIoTUserInfo_TCapability_TColorSettingCapabilityState{
		Instance: cscs.GetInstance(),
	}
	switch cscs.Instance {
	case TemperatureKCapabilityInstance:
		p.Value = &common.TIoTUserInfo_TCapability_TColorSettingCapabilityState_TemperatureK{
			TemperatureK: int32(cscs.Value.(TemperatureK)),
		}
	case RgbColorCapabilityInstance:
		p.Value = &common.TIoTUserInfo_TCapability_TColorSettingCapabilityState_RGB{
			RGB: int32(cscs.Value.(RGB)),
		}
	case HsvColorCapabilityInstance:
		p.Value = &common.TIoTUserInfo_TCapability_TColorSettingCapabilityState_HSV{
			HSV: &common.TIoTUserInfo_TCapability_TColorSettingCapabilityState_THSV{
				H: int32(cscs.Value.(HSV).H),
				S: int32(cscs.Value.(HSV).S),
				V: int32(cscs.Value.(HSV).V),
			},
		}
	case SceneCapabilityInstance:
		p.Value = &common.TIoTUserInfo_TCapability_TColorSettingCapabilityState_ColorSceneID{
			ColorSceneID: string(cscs.Value.(ColorSceneID)),
		}
	}
	return p
}

func (cscs *ColorSettingCapabilityState) fromUserInfoProto(p *common.TIoTUserInfo_TCapability_TColorSettingCapabilityState) {
	state := ColorSettingCapabilityState{
		Instance: ColorSettingCapabilityInstance(p.GetInstance()),
	}
	switch p.Value.(type) {
	case *common.TIoTUserInfo_TCapability_TColorSettingCapabilityState_TemperatureK:
		state.Value = TemperatureK(p.GetTemperatureK())
	case *common.TIoTUserInfo_TCapability_TColorSettingCapabilityState_RGB:
		state.Value = RGB(p.GetRGB())
	case *common.TIoTUserInfo_TCapability_TColorSettingCapabilityState_HSV:
		state.Value = HSV{
			H: int(p.GetHSV().GetH()),
			S: int(p.GetHSV().GetS()),
			V: int(p.GetHSV().GetV()),
		}
	case *common.TIoTUserInfo_TCapability_TColorSettingCapabilityState_ColorSceneID:
		state.Value = ColorSceneID(p.GetColorSceneID())
	}
	*cscs = state
}

func (cscs ColorSettingCapabilityState) Clone() ICapabilityState {
	var value ColorSettingValue
	if cscs.Value != nil {
		value = cscs.Value.Clone()
	}
	return ColorSettingCapabilityState{
		Instance: cscs.Instance,
		Value:    value,
	}
}

type ColorSettingValue interface {
	isColorSettingValue()
	Clone() ColorSettingValue
}

type TemperatureK uint32

func (tkv TemperatureK) isColorSettingValue() {}

func (tkv TemperatureK) Clone() ColorSettingValue { return tkv }
