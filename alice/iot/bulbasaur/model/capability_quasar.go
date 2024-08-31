package model

import (
	"context"
	"encoding/json"
	"fmt"

	"golang.org/x/exp/slices"

	"a.yandex-team.ru/alice/iot/bulbasaur/controller/experiments"
	"a.yandex-team.ru/alice/iot/bulbasaur/protos"
	"a.yandex-team.ru/alice/library/go/inflector"
	"a.yandex-team.ru/alice/library/go/timestamp"
	"a.yandex-team.ru/alice/megamind/protos/common"
	"a.yandex-team.ru/library/go/core/xerrors"
	"a.yandex-team.ru/library/go/valid"
)

type QuasarCapability struct {
	reportable  bool
	retrievable bool
	state       *QuasarCapabilityState
	parameters  QuasarCapabilityParameters
	lastUpdated timestamp.PastTimestamp
}

func (c QuasarCapability) Type() CapabilityType {
	return QuasarCapabilityType
}

func (c *QuasarCapability) Instance() string {
	return capabilityInstance(c)
}

func (c *QuasarCapability) Reportable() bool {
	return c.reportable
}

func (c *QuasarCapability) Retrievable() bool {
	return c.retrievable
}

func (c QuasarCapability) LastUpdated() timestamp.PastTimestamp {
	return c.lastUpdated
}

func (c QuasarCapability) State() ICapabilityState {
	if c.state == nil {
		return nil
	}
	return c.state.Clone()
}

func (c QuasarCapability) DefaultState() ICapabilityState {
	var value QuasarCapabilityValue
	switch QuasarCapabilityInstance(c.Instance()) {
	case WeatherCapabilityInstance:
		value = WeatherQuasarCapabilityValue{}
	case VolumeCapabilityInstance:
		value = VolumeQuasarCapabilityValue{Value: 3}
	case MusicPlayCapabilityInstance:
		value = MusicPlayQuasarCapabilityValue{PlayInBackground: true}
	case NewsCapabilityInstance:
		value = NewsQuasarCapabilityValue{Topic: IndexSpeakerNewsTopic, PlayInBackground: true}
	case SoundPlayCapabilityInstance:
		value = SoundPlayQuasarCapabilityValue{Sound: "chainsaw-1"}
	case StopEverythingCapabilityInstance:
		value = StopEverythingQuasarCapabilityValue{}
	case TTSCapabilityInstance:
		value = TTSQuasarCapabilityValue{Text: ""}
	case AliceShowCapabilityInstance:
		value = AliceShowQuasarCapabilityValue{}
	default:
		panic(fmt.Sprintf("unknown QuasarCapabilityInstance: %q", c.Instance()))
	}
	return QuasarCapabilityState{
		Instance: QuasarCapabilityInstance(c.Instance()),
		Value:    value,
	}
}

func (c QuasarCapability) Parameters() ICapabilityParameters {
	return c.parameters
}

func (c *QuasarCapability) Key() string {
	return CapabilityKey(c.Type(), c.Instance())
}

func (c *QuasarCapability) IsInternal() bool {
	return true
}

func (c *QuasarCapability) BasicSuggestions(dt DeviceType, inflection inflector.Inflection, options SuggestionsOptions) []string {
	suggestions := make([]string, 0)
	for i := range suggestions {
		suggestions[i] = options.AddHouseholdToSuggest(suggestions[i])
	}
	return suggestions
}

func (c *QuasarCapability) QuerySuggestions(inflection inflector.Inflection, options SuggestionsOptions) []string {
	// no state -> can't be queried
	suggestions := make([]string, 0)
	for i := range suggestions {
		suggestions[i] = options.AddHouseholdToSuggest(suggestions[i])
	}
	return suggestions
}

func (c *QuasarCapability) SetReportable(reportable bool) {
	c.reportable = reportable
}

func (c *QuasarCapability) SetRetrievable(retrievable bool) {
	c.retrievable = retrievable
}

func (c *QuasarCapability) SetLastUpdated(lastUpdated timestamp.PastTimestamp) {
	c.lastUpdated = lastUpdated
}

func (c *QuasarCapability) WithLastUpdated(lastUpdated timestamp.PastTimestamp) ICapabilityWithBuilder {
	c.SetLastUpdated(lastUpdated)
	return c
}

func (c *QuasarCapability) SetState(state ICapabilityState) {
	structure, ok := state.(QuasarCapabilityState)
	if ok {
		c.state = &structure
	}
	pointerState, ok := state.(*QuasarCapabilityState)
	if ok {
		c.state = pointerState
	}
}

func (c *QuasarCapability) SetParameters(parameters ICapabilityParameters) {
	c.parameters = parameters.(QuasarCapabilityParameters)
}

func (c *QuasarCapability) WithReportable(reportable bool) ICapabilityWithBuilder {
	c.SetReportable(reportable)
	return c
}

func (c *QuasarCapability) WithRetrievable(retrievable bool) ICapabilityWithBuilder {
	c.SetRetrievable(retrievable)
	return c
}

func (c *QuasarCapability) WithState(state ICapabilityState) ICapabilityWithBuilder {
	c.SetState(state)
	return c
}

func (c *QuasarCapability) WithParameters(parameters ICapabilityParameters) ICapabilityWithBuilder {
	c.SetParameters(parameters)
	return c
}

func (c *QuasarCapability) Merge(capability ICapability) (ICapability, bool) {
	return mergeCapability(c, capability)
}

func (c *QuasarCapability) Clone() ICapabilityWithBuilder {
	return cloneCapability(c)
}

func (c *QuasarCapability) Equal(capability ICapability) bool {
	return equalCapability(c, capability)
}

func (c *QuasarCapability) MarshalJSON() ([]byte, error) {
	return marshalCapability(c)
}

func (c *QuasarCapability) UnmarshalJSON(b []byte) error {
	cRaw := rawCapability{}
	if err := json.Unmarshal(b, &cRaw); err != nil {
		return err
	}
	c.SetReportable(cRaw.Reportable)
	c.SetRetrievable(cRaw.Retrievable)
	c.SetLastUpdated(cRaw.LastUpdated)

	var p QuasarCapabilityParameters
	if err := json.Unmarshal(cRaw.Parameters, &p); err != nil {
		return err
	}
	c.SetParameters(p)

	if cRaw.State != nil && string(cRaw.State) != "null" {
		s := QuasarCapabilityState{}
		if err := json.Unmarshal(cRaw.State, &s); err != nil {
			return err
		}
		c.SetState(s)
	}
	return nil
}

func (c QuasarCapability) ToProto() *protos.Capability {
	pc := &protos.Capability{
		Type:        *c.Type().toProto(),
		Reportable:  c.Reportable(),
		Retrievable: c.Retrievable(),
		LastUpdated: float64(c.LastUpdated()),
	}
	pc.Parameters = &protos.Capability_QCParameters{
		QCParameters: &protos.QuasarCapabilityParameters{Instance: string(c.parameters.Instance)},
	}
	if c.State() != nil {
		s := c.State().(QuasarCapabilityState)
		pc.State = &protos.Capability_QCState{
			QCState: s.toProto(),
		}
	}
	return pc
}

func (c *QuasarCapability) FromProto(p *protos.Capability) {
	sac := QuasarCapability{}
	sac.SetReportable(p.Reportable)
	sac.SetRetrievable(p.Retrievable)
	sac.SetLastUpdated(timestamp.PastTimestamp(p.LastUpdated))
	sac.SetParameters(
		QuasarCapabilityParameters{
			Instance: QuasarCapabilityInstance(p.GetQCParameters().Instance),
		},
	)
	if p.State != nil {
		var s QuasarCapabilityState
		s.fromProto(p.GetQCState())
		sac.SetState(s)
	}
	*c = sac
}

func (c QuasarCapability) ToUserInfoProto() *common.TIoTUserInfo_TCapability {
	pc := &common.TIoTUserInfo_TCapability{
		Type:          common.TIoTUserInfo_TCapability_QuasarCapabilityType,
		Reportable:    c.Reportable(),
		Retrievable:   c.Retrievable(),
		LastUpdated:   float64(c.LastUpdated()),
		AnalyticsType: analyticsCapabilityType(c.Type()),
		AnalyticsName: analyticsCapabilityName(analyticsCapabilityKey{Type: c.Type(), Instance: c.Instance()}),
	}
	pc.Parameters = &common.TIoTUserInfo_TCapability_QuasarCapabilityParameters{
		QuasarCapabilityParameters: c.parameters.ToUserInfoProto(),
	}
	if c.state != nil {
		pc.State = &common.TIoTUserInfo_TCapability_QuasarCapabilityState{
			QuasarCapabilityState: c.state.ToUserInfoProto(),
		}
	}
	return pc
}

func (c *QuasarCapability) FromUserInfoProto(p *common.TIoTUserInfo_TCapability) {
	sac := QuasarCapability{}
	sac.SetReportable(p.Reportable)
	sac.SetRetrievable(p.Retrievable)
	sac.SetLastUpdated(timestamp.PastTimestamp(p.LastUpdated))
	sac.SetParameters(
		QuasarCapabilityParameters{
			Instance: QuasarCapabilityInstance(p.GetQuasarCapabilityParameters().GetInstance()),
		},
	)
	if state := p.GetQuasarCapabilityState(); state != nil {
		var s QuasarCapabilityState
		s.fromUserInfoProto(state)
		sac.SetState(s)
	}
	*c = sac
}

type QuasarCapabilityInstance string

func (instance QuasarCapabilityInstance) String() string {
	return string(instance)
}

var _ valid.Validator = new(QuasarCapabilityParameters)

type QuasarCapabilityParameters struct {
	Instance QuasarCapabilityInstance `json:"instance" yson:"instance"`
}

func (p QuasarCapabilityParameters) GetInstance() string {
	return string(p.Instance)
}

func (p QuasarCapabilityParameters) Validate(_ *valid.ValidationCtx) (bool, error) {
	if !slices.Contains(KnownQuasarCapabilityInstances, string(p.Instance)) {
		return false, fmt.Errorf("unsupported by current device quasar capability parameters instance: '%s'", p.Instance)
	}
	return false, nil
}

func (p QuasarCapabilityParameters) Merge(params ICapabilityParameters) ICapabilityParameters {
	otherParameters, isQuasarParameters := params.(QuasarCapabilityParameters)
	if !isQuasarParameters || otherParameters.Instance != p.Instance {
		return nil
	}
	return QuasarCapabilityParameters{Instance: p.Instance}
}

func (p QuasarCapabilityParameters) ToUserInfoProto() *common.TIoTUserInfo_TCapability_TQuasarCapabilityParameters {
	return &common.TIoTUserInfo_TCapability_TQuasarCapabilityParameters{
		Instance: p.Instance.String(),
	}
}

type QuasarCapabilityState struct {
	Instance QuasarCapabilityInstance `json:"instance" yson:"instance"`
	Value    QuasarCapabilityValue    `json:"value" yson:"value"`
}

func (s *QuasarCapabilityState) UnmarshalJSON(b []byte) (err error) {
	sRaw := struct {
		Instance QuasarCapabilityInstance
		Value    json.RawMessage
	}{}
	if err := json.Unmarshal(b, &sRaw); err != nil {
		return err
	}

	s.Instance = sRaw.Instance

	switch sRaw.Instance {
	case WeatherCapabilityInstance:
		var v WeatherQuasarCapabilityValue
		err = json.Unmarshal(sRaw.Value, &v)
		s.Value = v
	case VolumeCapabilityInstance:
		var v VolumeQuasarCapabilityValue
		err = json.Unmarshal(sRaw.Value, &v)
		s.Value = v
	case MusicPlayCapabilityInstance:
		var v MusicPlayQuasarCapabilityValue
		err = json.Unmarshal(sRaw.Value, &v)
		s.Value = v
	case NewsCapabilityInstance:
		var v NewsQuasarCapabilityValue
		err = json.Unmarshal(sRaw.Value, &v)
		s.Value = v
	case SoundPlayCapabilityInstance:
		var v SoundPlayQuasarCapabilityValue
		err = json.Unmarshal(sRaw.Value, &v)
		s.Value = v
	case StopEverythingCapabilityInstance:
		var v StopEverythingQuasarCapabilityValue
		err = json.Unmarshal(sRaw.Value, &v)
		s.Value = v
	case TTSCapabilityInstance:
		var v TTSQuasarCapabilityValue
		err = json.Unmarshal(sRaw.Value, &v)
		s.Value = v
	case AliceShowCapabilityInstance:
		var v AliceShowQuasarCapabilityValue
		err = json.Unmarshal(sRaw.Value, &v)
		s.Value = v
	default:
		err = fmt.Errorf("unknown quasar state instance: `%s`", sRaw.Instance)
	}

	if err != nil {
		return xerrors.Errorf("cannot parse capability state: %w", err)
	}
	return nil
}

func (s QuasarCapabilityState) GetInstance() string {
	return string(s.Instance)
}

func (s QuasarCapabilityState) Validate(vctx *valid.ValidationCtx) (bool, error) {
	var err valid.Errors

	if !slices.Contains(KnownQuasarCapabilityInstances, string(s.Instance)) {
		err = append(err, xerrors.Errorf("unsupported by current device quasar capability state instance: '%s'", s.Instance))
	}

	if s.Value == nil {
		err = append(err, xerrors.New("invalid quasar capability value: value is nil"))
	} else {
		if _, e := s.Value.Validate(vctx); e != nil {
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

func (s QuasarCapabilityState) Type() CapabilityType {
	return QuasarCapabilityType
}

func (s QuasarCapabilityState) ValidateState(cap ICapability) error {
	if !slices.Contains(KnownQuasarCapabilityInstances, string(s.Instance)) {
		return fmt.Errorf("unsupported by current device quasar capability state instance: %q", s.Instance)
	}
	if s.Value != nil {
		if _, err := s.Value.Validate(valid.NewValidationCtx()); err != nil {
			return err
		}
	}
	return nil
}

func (s QuasarCapabilityState) ToIotCapabilityAction() *common.TIoTCapabilityAction {
	return &common.TIoTCapabilityAction{
		Type: QuasarCapabilityType.ToUserInfoProto(),
		State: &common.TIoTCapabilityAction_QuasarCapabilityState{
			QuasarCapabilityState: s.ToUserInfoProto(),
		},
	}
}

func (s QuasarCapabilityState) Merge(state ICapabilityState) ICapabilityState {
	otherState, isQuasarCapabilityState := state.(QuasarCapabilityState)
	if !isQuasarCapabilityState || s.Instance != otherState.Instance || s.Value != otherState.Value {
		return nil
	}
	return QuasarCapabilityState{Instance: s.Instance, Value: s.Value}
}

func (s QuasarCapabilityState) ToUserInfoProto() *common.TIoTUserInfo_TCapability_TQuasarCapabilityState {
	state := &common.TIoTUserInfo_TCapability_TQuasarCapabilityState{
		Instance: s.GetInstance(),
	}
	switch s.Instance {
	case WeatherCapabilityInstance:
		weatherValue := s.Value.(WeatherQuasarCapabilityValue)
		state.Value = weatherValue.toUserInfoProto()
	case VolumeCapabilityInstance:
		volumeValue := s.Value.(VolumeQuasarCapabilityValue)
		state.Value = volumeValue.toUserInfoProto()
	case MusicPlayCapabilityInstance:
		musicValue := s.Value.(MusicPlayQuasarCapabilityValue)
		state.Value = musicValue.toUserInfoProto()
	case NewsCapabilityInstance:
		newsValue := s.Value.(NewsQuasarCapabilityValue)
		state.Value = newsValue.toUserInfoProto()
	case SoundPlayCapabilityInstance:
		soundPlay := s.Value.(SoundPlayQuasarCapabilityValue)
		state.Value = soundPlay.toUserInfoProto()
	case StopEverythingCapabilityInstance:
		stopEverything := s.Value.(StopEverythingQuasarCapabilityValue)
		state.Value = stopEverything.toUserInfoProto()
	case TTSCapabilityInstance:
		tts := s.Value.(TTSQuasarCapabilityValue)
		state.Value = tts.toUserInfoProto()
	case AliceShowCapabilityInstance:
		aliceShow := s.Value.(AliceShowQuasarCapabilityValue)
		state.Value = aliceShow.toUserInfoProto()
	}
	return state
}

func (s *QuasarCapabilityState) fromUserInfoProto(p *common.TIoTUserInfo_TCapability_TQuasarCapabilityState) {
	var value QuasarCapabilityValue
	switch QuasarCapabilityInstance(p.GetInstance()) {
	case WeatherCapabilityInstance:
		weatherValue := WeatherQuasarCapabilityValue{}
		weatherValue.fromUserInfoProto(p.GetWeatherValue())
		value = weatherValue
	case VolumeCapabilityInstance:
		volumeValue := VolumeQuasarCapabilityValue{}
		volumeValue.fromUserInfoProto(p.GetVolumeValue())
		value = volumeValue
	case MusicPlayCapabilityInstance:
		musicPlayValue := MusicPlayQuasarCapabilityValue{}
		musicPlayValue.fromUserInfoProto(p.GetMusicPlayValue())
		value = musicPlayValue
	case NewsCapabilityInstance:
		newsValue := NewsQuasarCapabilityValue{}
		newsValue.fromUserInfoProto(p.GetNewsValue())
		value = newsValue
	case SoundPlayCapabilityInstance:
		soundPlayValue := SoundPlayQuasarCapabilityValue{}
		soundPlayValue.fromUserInfoProto(p.GetSoundPlayValue())
		value = soundPlayValue
	case StopEverythingCapabilityInstance:
		stopEverythingValue := StopEverythingQuasarCapabilityValue{}
		stopEverythingValue.fromUserInfoProto(p.GetStopEverythingValue())
		value = stopEverythingValue
	case TTSCapabilityInstance:
		ttsValue := TTSQuasarCapabilityValue{}
		ttsValue.fromUserInfoProto(p.GetTtsValue())
		value = ttsValue
	case AliceShowCapabilityInstance:
		aliceShowValue := AliceShowQuasarCapabilityValue{}
		aliceShowValue.fromUserInfoProto(p.GetAliceShowValue())
		value = aliceShowValue
	}
	*s = QuasarCapabilityState{
		Instance: QuasarCapabilityInstance(p.GetInstance()),
		Value:    value,
	}
}

func (s *QuasarCapabilityState) toProto() *protos.QuasarCapabilityState {
	state := &protos.QuasarCapabilityState{
		Instance: s.GetInstance(),
	}
	switch s.Instance {
	case WeatherCapabilityInstance:
		weatherValue := s.Value.(WeatherQuasarCapabilityValue)
		state.Value = weatherValue.toProto()
	case VolumeCapabilityInstance:
		volumeValue := s.Value.(VolumeQuasarCapabilityValue)
		state.Value = volumeValue.toProto()
	case MusicPlayCapabilityInstance:
		musicValue := s.Value.(MusicPlayQuasarCapabilityValue)
		state.Value = musicValue.toProto()
	case NewsCapabilityInstance:
		newsValue := s.Value.(NewsQuasarCapabilityValue)
		state.Value = newsValue.toProto()
	case SoundPlayCapabilityInstance:
		soundPlay := s.Value.(SoundPlayQuasarCapabilityValue)
		state.Value = soundPlay.toProto()
	case StopEverythingCapabilityInstance:
		stopEverything := s.Value.(StopEverythingQuasarCapabilityValue)
		state.Value = stopEverything.toProto()
	case TTSCapabilityInstance:
		tts := s.Value.(TTSQuasarCapabilityValue)
		state.Value = tts.toProto()
	case AliceShowCapabilityInstance:
		aliceShow := s.Value.(AliceShowQuasarCapabilityValue)
		state.Value = aliceShow.toProto()
	}
	return state
}

func (s *QuasarCapabilityState) fromProto(p *protos.QuasarCapabilityState) {
	var value QuasarCapabilityValue
	switch QuasarCapabilityInstance(p.Instance) {
	case WeatherCapabilityInstance:
		var weatherWhere *HouseholdLocation
		var weatherHousehold *WeatherHouseholdInfo
		if p.GetWeather().GetWhere() != nil {
			weatherWhere = &HouseholdLocation{
				Longitude:    p.GetWeather().GetWhere().GetLongitude(),
				Latitude:     p.GetWeather().GetWhere().GetLatitude(),
				Address:      p.GetWeather().GetWhere().GetAddress(),
				ShortAddress: p.GetWeather().GetWhere().GetShortAddress(),
			}
		}
		if p.GetWeather().GetHousehold() != nil {
			weatherHousehold = &WeatherHouseholdInfo{
				ID:   p.GetWeather().GetHousehold().GetId(),
				Name: p.GetWeather().GetHousehold().GetName(),
			}
		}
		value = WeatherQuasarCapabilityValue{
			Where:     weatherWhere,
			Household: weatherHousehold,
		}
	case VolumeCapabilityInstance:
		value = VolumeQuasarCapabilityValue{
			Value:    int(p.GetVolume().GetValue()),
			Relative: p.GetVolume().GetRelative(),
		}
	case MusicPlayCapabilityInstance:
		var musicObject *MusicPlayObject
		if p.GetMusicPlay().GetObject() != nil {
			musicObject = &MusicPlayObject{
				ID:   p.GetMusicPlay().GetObject().GetId(),
				Type: MusicPlayObjectType(p.GetMusicPlay().GetObject().GetType()),
				Name: p.GetMusicPlay().GetObject().GetName(),
			}
		}
		value = MusicPlayQuasarCapabilityValue{
			Object:           musicObject,
			SearchText:       p.GetMusicPlay().GetSearchText(),
			PlayInBackground: p.GetMusicPlay().GetPlayInBackground(),
		}
	case NewsCapabilityInstance:
		value = NewsQuasarCapabilityValue{
			Topic:            SpeakerNewsTopic(p.GetNews().GetTopic()),
			Provider:         p.GetNews().GetProvider(),
			PlayInBackground: p.GetNews().GetPlayInBackground(),
		}
	case SoundPlayCapabilityInstance:
		value = SoundPlayQuasarCapabilityValue{
			Sound: SpeakerSoundID(p.GetSoundPlay().GetSound()),
		}
	case StopEverythingCapabilityInstance:
		value = StopEverythingQuasarCapabilityValue{}
	case TTSCapabilityInstance:
		value = TTSQuasarCapabilityValue{
			Text: p.GetTTS().GetText(),
		}
	case AliceShowCapabilityInstance:
		value = AliceShowQuasarCapabilityValue{}
	}
	*s = QuasarCapabilityState{
		Instance: QuasarCapabilityInstance(p.Instance),
		Value:    value,
	}
}

func (s QuasarCapabilityState) Clone() ICapabilityState {
	return QuasarCapabilityState{
		Instance: s.Instance,
		Value:    s.Value,
	}
}

func (s QuasarCapabilityState) NeedCompletionCallback() bool {
	switch value := s.Value.(type) {
	case MusicPlayQuasarCapabilityValue:
		return !value.PlayInBackground
	case NewsQuasarCapabilityValue:
		return !value.PlayInBackground
	case AliceShowQuasarCapabilityValue:
		return false
	default:
		return true
	}
}

func MakeQuasarCapabilityValueByInstance(instance QuasarCapabilityInstance, value interface{}) QuasarCapabilityValue {
	switch instance {
	case WeatherCapabilityInstance:
		return value.(WeatherQuasarCapabilityValue)
	case VolumeCapabilityInstance:
		return value.(VolumeQuasarCapabilityValue)
	case MusicPlayCapabilityInstance:
		return value.(MusicPlayQuasarCapabilityValue)
	case NewsCapabilityInstance:
		return value.(NewsQuasarCapabilityValue)
	case SoundPlayCapabilityInstance:
		return value.(SoundPlayQuasarCapabilityValue)
	case StopEverythingCapabilityInstance:
		return value.(StopEverythingQuasarCapabilityValue)
	case TTSCapabilityInstance:
		return value.(TTSQuasarCapabilityValue)
	case AliceShowCapabilityInstance:
		return value.(AliceShowQuasarCapabilityValue)
	default:
		panic(fmt.Sprintf("unknown quasar capability instance: %q", instance))
	}
}

func MakeQuasarCapabilityParametersByInstance(instance QuasarCapabilityInstance) QuasarCapabilityParameters {
	return QuasarCapabilityParameters{Instance: instance}
}

type SpeakerNewsTopic string

const (
	PoliticsSpeakerNewsTopic  SpeakerNewsTopic = "politics"
	SocietySpeakerNewsTopic   SpeakerNewsTopic = "society"
	BusinessSpeakerNewsTopic  SpeakerNewsTopic = "business"
	WorldSpeakerNewsTopic     SpeakerNewsTopic = "world"
	SportSpeakerNewsTopic     SpeakerNewsTopic = "sport"
	IncidentSpeakerNewsTopic  SpeakerNewsTopic = "incident"
	IndexSpeakerNewsTopic     SpeakerNewsTopic = "index"
	CultureSpeakerNewsTopic   SpeakerNewsTopic = "culture"
	ComputersSpeakerNewsTopic SpeakerNewsTopic = "computers"
	ScienceSpeakerNewsTopic   SpeakerNewsTopic = "science"
	AutoSpeakerNewsTopic      SpeakerNewsTopic = "auto"
)

type SpeakerSoundID string

type SpeakerSoundCategoryID string

const (
	ThingsSpeakerSoundCategoryID  SpeakerSoundCategoryID = "things"
	AnimalsSpeakerSoundCategoryID SpeakerSoundCategoryID = "animals"
	GameSpeakerSoundCategoryID    SpeakerSoundCategoryID = "game"
	HumanSpeakerSoundCategoryID   SpeakerSoundCategoryID = "human"
	MusicSpeakerSoundCategoryID   SpeakerSoundCategoryID = "music"
	NatureSpeakerSoundCategoryID  SpeakerSoundCategoryID = "nature"
)

type SpeakerSound struct {
	ID         SpeakerSoundID
	CategoryID SpeakerSoundCategoryID
	Name       string
}

func (ss SpeakerSound) Opus() string {
	return fmt.Sprintf(`<speaker audio="alice-%s-%s.opus">`, ss.soundNamespace(), ss.ID)
}

func (ss SpeakerSound) AudioURL() string {
	return fmt.Sprintf(`https://dialogs.s3.yandex.net/sounds/alice-%s-%s.mp3`, ss.soundNamespace(), ss.ID)
}

func (ss SpeakerSound) soundNamespace() string {
	switch {
	case ss.ID == "ship-horn-1", ss.ID == "ship-horn-2":
		return "sounds-transport"
	case ss.CategoryID == MusicSpeakerSoundCategoryID:
		return "music"
	default:
		return fmt.Sprintf("sounds-%s", ss.CategoryID)
	}
}

type SpeakerSoundCategory struct {
	ID   SpeakerSoundCategoryID
	Name string
}

func GenerateQuasarCapabilities(ctx context.Context, deviceType DeviceType) Capabilities {
	result := make(Capabilities, 0, len(KnownQuasarCapabilityInstances)+len(KnownQuasarServerActionInstances))
	for _, knownQuasarServerActionInstance := range KnownQuasarServerActionInstances {
		quasarServerActionCapability := MakeCapabilityByType(QuasarServerActionCapabilityType)
		quasarServerActionCapability.SetRetrievable(false)
		quasarServerActionCapability.SetReportable(false)
		quasarServerActionCapability.SetParameters(
			MakeQuasarServerActionParametersByInstance(QuasarServerActionCapabilityInstance(knownQuasarServerActionInstance)),
		)
		quasarServerActionCapability.SetState(quasarServerActionCapability.DefaultState())
		result = append(result, quasarServerActionCapability)
	}

	if experiments.DropNewCapabilitiesForOldSpeakers.IsEnabled(ctx) && !ParovozSpeakers[deviceType] {
		return result
	}
	for _, knownQuasarInstance := range KnownQuasarCapabilityInstances {
		quasarCapability := MakeCapabilityByType(QuasarCapabilityType)
		quasarCapability.SetRetrievable(false)
		quasarCapability.SetReportable(false)
		quasarCapability.SetParameters(
			MakeQuasarCapabilityParametersByInstance(QuasarCapabilityInstance(knownQuasarInstance)),
		)
		quasarCapability.SetState(quasarCapability.DefaultState())
		result = append(result, quasarCapability)
	}
	return result
}

var ParovozSpeakers = map[DeviceType]bool{
	YandexStationDeviceType:             true,
	YandexStation2DeviceType:            true,
	YandexStationMiniDeviceType:         true,
	YandexStationMini2DeviceType:        true,
	YandexStationMini2NoClockDeviceType: true,
	YandexStationMicroDeviceType:        true,
	YandexStationCentaurDeviceType:      true,
	YandexStationChironDeviceType:       true,
	YandexStationPholDeviceType:         true,
	YandexStationMidiDeviceType:         true,
	JBLLinkMusicDeviceType:              true,
	JBLLinkPortableDeviceType:           true,
}
