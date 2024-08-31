package model

import (
	"encoding/json"
	"fmt"

	bulbasaur "a.yandex-team.ru/alice/iot/bulbasaur/errors"
	"a.yandex-team.ru/alice/iot/bulbasaur/protos"
	"a.yandex-team.ru/alice/library/go/inflector"
	"a.yandex-team.ru/alice/library/go/timestamp"
	"a.yandex-team.ru/alice/library/go/tools"
	"a.yandex-team.ru/alice/megamind/protos/common"
	"a.yandex-team.ru/library/go/valid"
)

type VideoStreamCapability struct {
	reportable  bool
	retrievable bool
	state       *VideoStreamCapabilityState
	parameters  VideoStreamCapabilityParameters
	lastUpdated timestamp.PastTimestamp
}

func (c VideoStreamCapability) Type() CapabilityType {
	return VideoStreamCapabilityType
}

func (c *VideoStreamCapability) Instance() string {
	return capabilityInstance(c)
}

func (c *VideoStreamCapability) Reportable() bool {
	return c.reportable
}

func (c *VideoStreamCapability) Retrievable() bool {
	return c.retrievable
}

func (c VideoStreamCapability) LastUpdated() timestamp.PastTimestamp {
	return c.lastUpdated
}

func (c VideoStreamCapability) State() ICapabilityState {
	if c.state == nil {
		return nil
	}
	return c.state.Clone()
}

func (c VideoStreamCapability) DefaultState() ICapabilityState {
	return nil
}

func (c VideoStreamCapability) Parameters() ICapabilityParameters {
	return c.parameters
}

func (c *VideoStreamCapability) Key() string {
	return CapabilityKey(c.Type(), c.Instance())
}

func (c *VideoStreamCapability) IsInternal() bool {
	return false
}

func (c *VideoStreamCapability) BasicSuggestions(dt DeviceType, inflection inflector.Inflection, options SuggestionsOptions) []string {
	suggestions := make([]string, 0)

	switch VideoStreamCapabilityInstance(c.Instance()) {
	case GetStreamCapabilityInstance:
		suggestions = append(suggestions,
			fmt.Sprintf("Покажи видео с %s?", inflection.Rod),
			fmt.Sprintf("Выведи %s", inflection.Vin),
		)
	}

	for i := range suggestions {
		suggestions[i] = options.AddHouseholdToSuggest(suggestions[i])
	}
	return suggestions
}

func (c *VideoStreamCapability) QuerySuggestions(inflection inflector.Inflection, options SuggestionsOptions) []string {
	return make([]string, 0)
}

func (c *VideoStreamCapability) SetReportable(reportable bool) {
	c.reportable = reportable
}

func (c *VideoStreamCapability) SetRetrievable(retrievable bool) {
	c.retrievable = retrievable
}

func (c *VideoStreamCapability) SetLastUpdated(lastUpdated timestamp.PastTimestamp) {
	c.lastUpdated = lastUpdated
}

func (c *VideoStreamCapability) WithLastUpdated(lastUpdated timestamp.PastTimestamp) ICapabilityWithBuilder {
	c.SetLastUpdated(lastUpdated)
	return c
}

func (c *VideoStreamCapability) SetState(state ICapabilityState) {
	structure, ok := state.(VideoStreamCapabilityState)
	if ok {
		c.state = &structure
	}
	pointerState, ok := state.(*VideoStreamCapabilityState)
	if ok {
		c.state = pointerState
	}
}

func (c *VideoStreamCapability) SetParameters(parameters ICapabilityParameters) {
	c.parameters = parameters.(VideoStreamCapabilityParameters)
}

func (c *VideoStreamCapability) WithReportable(reportable bool) ICapabilityWithBuilder {
	c.SetReportable(reportable)
	return c
}

func (c *VideoStreamCapability) WithRetrievable(retrievable bool) ICapabilityWithBuilder {
	c.SetRetrievable(retrievable)
	return c
}

func (c *VideoStreamCapability) WithState(state ICapabilityState) ICapabilityWithBuilder {
	c.SetState(state)
	return c
}

func (c *VideoStreamCapability) WithParameters(parameters ICapabilityParameters) ICapabilityWithBuilder {
	c.SetParameters(parameters)
	return c
}

func (c *VideoStreamCapability) Merge(capability ICapability) (ICapability, bool) {
	return nil, false
}

func (c *VideoStreamCapability) Clone() ICapabilityWithBuilder {
	return cloneCapability(c)
}

func (c *VideoStreamCapability) Equal(capability ICapability) bool {
	return equalCapability(c, capability)
}

func (c *VideoStreamCapability) MarshalJSON() ([]byte, error) {
	return marshalCapability(c)
}

func (c *VideoStreamCapability) UnmarshalJSON(b []byte) error {
	cRaw := rawCapability{}
	if err := json.Unmarshal(b, &cRaw); err != nil {
		return err
	}
	c.SetReportable(cRaw.Reportable)
	c.SetRetrievable(cRaw.Retrievable)
	c.SetLastUpdated(cRaw.LastUpdated)

	p := VideoStreamCapabilityParameters{}
	if err := json.Unmarshal(cRaw.Parameters, &p); err != nil {
		return err
	}
	c.SetParameters(p)

	if cRaw.State != nil && string(cRaw.State) != "null" {
		s := VideoStreamCapabilityState{}
		if err := json.Unmarshal(cRaw.State, &s); err != nil {
			return err
		}
		c.SetState(s)
	}
	return nil
}

func (c VideoStreamCapability) ToProto() *protos.Capability {
	pc := &protos.Capability{
		Type:        *c.Type().toProto(),
		Reportable:  c.Reportable(),
		Retrievable: c.Retrievable(),
		LastUpdated: float64(c.LastUpdated()),
	}

	pc.Parameters = &protos.Capability_VSCParameters{
		VSCParameters: c.Parameters().(VideoStreamCapabilityParameters).toProto(),
	}

	if c.State() != nil {
		s := c.State().(VideoStreamCapabilityState)
		pc.State = &protos.Capability_VSCState{
			VSCState: s.toProto(),
		}
	}

	return pc
}

func (c *VideoStreamCapability) FromProto(p *protos.Capability) {
	nc := VideoStreamCapability{}
	nc.SetReportable(p.Reportable)
	nc.SetRetrievable(p.Retrievable)
	nc.SetLastUpdated(timestamp.PastTimestamp(p.LastUpdated))

	protocols := make([]VideoStreamProtocol, 0, len(p.GetVSCParameters().Protocols))
	for _, protocol := range p.GetVSCParameters().Protocols {
		protocols = append(protocols, VideoStreamProtocol(protocol))
	}

	nc.SetParameters(VideoStreamCapabilityParameters{
		Protocols: protocols,
	})

	if p.State != nil {
		var s VideoStreamCapabilityState
		s.fromProto(p.GetVSCState())
		nc.SetState(s)
	}

	*c = nc
}

func (c VideoStreamCapability) ToUserInfoProto() *common.TIoTUserInfo_TCapability {
	pc := &common.TIoTUserInfo_TCapability{
		Type:          common.TIoTUserInfo_TCapability_VideoStreamCapabilityType,
		Reportable:    c.Reportable(),
		Retrievable:   c.Retrievable(),
		LastUpdated:   float64(c.LastUpdated()),
		AnalyticsType: analyticsCapabilityType(c.Type()),
		AnalyticsName: analyticsCapabilityName(analyticsCapabilityKey{Type: c.Type(), Instance: c.Instance()}),
	}

	pc.Parameters = &common.TIoTUserInfo_TCapability_VideoStreamCapabilityParameters{
		VideoStreamCapabilityParameters: c.Parameters().(VideoStreamCapabilityParameters).ToUserInfoProto(),
	}

	if c.state != nil {
		pc.State = &common.TIoTUserInfo_TCapability_VideoStreamCapabilityState{
			VideoStreamCapabilityState: c.state.ToUserInfoProto(),
		}
	}

	return pc
}

func (c *VideoStreamCapability) FromUserInfoProto(p *common.TIoTUserInfo_TCapability) {
	nc := VideoStreamCapability{}
	nc.SetReportable(p.GetReportable())
	nc.SetRetrievable(p.GetRetrievable())
	nc.SetLastUpdated(timestamp.PastTimestamp(p.GetLastUpdated()))

	var params VideoStreamCapabilityParameters
	params.FromUserInfoProto(p.GetVideoStreamCapabilityParameters())
	nc.SetParameters(params)

	if state := p.GetVideoStreamCapabilityState(); state != nil {
		var s VideoStreamCapabilityState
		s.FromUserInfoProto(state)
		nc.SetState(s)
	}

	*c = nc
}

type VideoStreamCapabilityInstance string

func (i VideoStreamCapabilityInstance) String() string {
	return string(i)
}

type VideoStreamCapabilityState struct {
	Instance VideoStreamCapabilityInstance `json:"instance" yson:"instance"`
	Value    VideoStreamCapabilityValue    `json:"value" yson:"value"`
}

func (s VideoStreamCapabilityState) GetInstance() string {
	return string(s.Instance)
}

func (s VideoStreamCapabilityState) Type() CapabilityType {
	return VideoStreamCapabilityType
}

func (s VideoStreamCapabilityState) ValidateState(c ICapability) error {
	var err bulbasaur.Errors

	if p, ok := c.Parameters().(VideoStreamCapabilityParameters); ok {
		if string(s.Instance) != p.GetInstance() {
			err = append(err, fmt.Errorf("unexpected instance: expected %s, got %s", p.GetInstance(), s.Instance))
		}

		protocols := make([]string, 0, len(p.Protocols))
		for _, protocol := range p.Protocols {
			protocols = append(protocols, string(protocol))
		}

		if len(s.Value.Protocol) > 0 && !tools.Contains(string(s.Value.Protocol), protocols) {
			err = append(err, fmt.Errorf("unsupported protocol for current device: %s", s.Value.Protocol))
		}

		for _, protocol := range s.Value.Protocols {
			if !tools.Contains(string(protocol), KnownVideoStreamProtocols) {
				err = append(err, fmt.Errorf("unexpected protocol: expected one of %v, got %s", KnownVideoStreamProtocols, protocol))
			}
		}
	}

	if len(err) > 0 {
		return err
	}

	return nil
}

func (s VideoStreamCapabilityState) ToIotCapabilityAction() *common.TIoTCapabilityAction {
	return &common.TIoTCapabilityAction{
		Type: VideoStreamCapabilityType.ToUserInfoProto(),
		State: &common.TIoTCapabilityAction_VideoStreamCapabilityState{
			VideoStreamCapabilityState: s.ToUserInfoProto(),
		},
	}
}

func (s VideoStreamCapabilityState) Merge(state ICapabilityState) ICapabilityState {
	return nil
}

func (s VideoStreamCapabilityState) Clone() ICapabilityState {
	return VideoStreamCapabilityState{
		Instance: s.Instance,
		Value:    s.Value.Clone(),
	}
}

func (s *VideoStreamCapabilityState) toProto() *protos.VideoStreamCapabilityState {
	protocols := make([]string, 0, len(s.Value.Protocols))
	for _, protocol := range s.Value.Protocols {
		protocols = append(protocols, string(protocol))
	}
	return &protos.VideoStreamCapabilityState{
		Instance: string(s.Instance),
		Value: &protos.VideoStreamCapabilityState_VideoStreamCapabilityValue{
			Protocol:       string(s.Value.Protocol),
			Protocols:      protocols,
			StreamURL:      s.Value.StreamURL,
			ExpirationTime: uint64(s.Value.ExpirationTime.Unix()),
		},
	}
}

func (s *VideoStreamCapabilityState) fromProto(p *protos.VideoStreamCapabilityState) {
	protocols := make([]VideoStreamProtocol, 0, len(p.GetValue().GetProtocols()))
	for _, protocol := range p.GetValue().GetProtocols() {
		protocols = append(protocols, VideoStreamProtocol(protocol))
	}
	*s = VideoStreamCapabilityState{
		Instance: VideoStreamCapabilityInstance(p.GetInstance()),
		Value: VideoStreamCapabilityValue{
			Protocols:      protocols,
			Protocol:       VideoStreamProtocol(p.GetValue().GetProtocol()),
			StreamURL:      p.GetValue().GetStreamURL(),
			ExpirationTime: timestamp.PastTimestamp(p.GetValue().GetExpirationTime()),
		},
	}
}

func (s VideoStreamCapabilityState) ToUserInfoProto() *common.TIoTUserInfo_TCapability_TVideoStreamCapabilityState {
	protocols := make([]string, 0, len(s.Value.Protocols))
	for _, protocol := range s.Value.Protocols {
		protocols = append(protocols, string(protocol))
	}
	return &common.TIoTUserInfo_TCapability_TVideoStreamCapabilityState{
		Instance: string(s.Instance),
		Value: &common.TIoTUserInfo_TCapability_TVideoStreamCapabilityState_TVideoStreamCapabilityValue{
			Protocol:       string(s.Value.Protocol),
			Protocols:      protocols,
			StreamURL:      s.Value.StreamURL,
			ExpirationTime: uint64(s.Value.ExpirationTime.Unix()),
		},
	}
}

func (s *VideoStreamCapabilityState) FromUserInfoProto(p *common.TIoTUserInfo_TCapability_TVideoStreamCapabilityState) {
	protocols := make([]VideoStreamProtocol, 0, len(p.GetValue().GetProtocols()))
	for _, protocol := range p.GetValue().GetProtocols() {
		protocols = append(protocols, VideoStreamProtocol(protocol))
	}
	*s = VideoStreamCapabilityState{
		Instance: VideoStreamCapabilityInstance(p.GetInstance()),
		Value: VideoStreamCapabilityValue{
			Protocols:      protocols,
			Protocol:       VideoStreamProtocol(p.GetValue().GetProtocol()),
			StreamURL:      p.GetValue().GetStreamURL(),
			ExpirationTime: timestamp.PastTimestamp(p.GetValue().GetExpirationTime()),
		},
	}
}

func (s *VideoStreamCapabilityState) IsEmpty() bool {
	empty := VideoStreamCapabilityState{}

	return s.Instance == empty.Instance &&
		s.Value.Protocol == empty.Value.Protocol &&
		s.Value.StreamURL == empty.Value.StreamURL &&
		s.Value.ExpirationTime == empty.Value.ExpirationTime &&
		len(s.Value.Protocols) == 0
}

type VideoStreamProtocol string

const (
	HLSStreamingProtocol            VideoStreamProtocol = "hls"
	ProgressiveMP4StreamingProtocol VideoStreamProtocol = "progressive_mp4"
)

func IntersectProtocols(left, right []VideoStreamProtocol) []VideoStreamProtocol {
	if len(left) > len(right) {
		left, right = right, left
	}

	leftSet := make(map[VideoStreamProtocol]bool)
	for _, protocol := range left {
		leftSet[protocol] = true
	}

	result := make([]VideoStreamProtocol, 0, len(leftSet))
	for _, protocol := range right {
		if leftSet[protocol] {
			result = append(result, protocol)
		}
	}

	return result
}

// VideoStreamCapabilityValue doesn't really describe capability state. It's used as
// * action request
// * action response
// TODO: use different objects to store action request/response and actual state
type VideoStreamCapabilityValue struct {
	// Action request values
	Protocols []VideoStreamProtocol `json:"protocols,omitempty" yson:"protocols,omitempty"`

	// Action result values
	Protocol       VideoStreamProtocol     `json:"protocol,omitempty" yson:"protocol,omitempty"`
	StreamURL      string                  `json:"stream_url,omitempty" yson:"stream_url,omitempty"`
	ExpirationTime timestamp.PastTimestamp `json:"expiration_time,omitempty" yson:"expiration_time,omitempty"`
}

func (v *VideoStreamCapabilityValue) Clone() VideoStreamCapabilityValue {
	clone := VideoStreamCapabilityValue{
		Protocol:       v.Protocol,
		StreamURL:      v.StreamURL,
		ExpirationTime: v.ExpirationTime,
	}

	if v.Protocols != nil {
		clone.Protocols = make([]VideoStreamProtocol, len(v.Protocols))
		copy(clone.Protocols, v.Protocols)
	}

	return clone
}

type VideoStreamCapabilityParameters struct {
	Protocols []VideoStreamProtocol `json:"protocols" yson:"protocols"`
}

func (p VideoStreamCapabilityParameters) Validate(ctx *valid.ValidationCtx) (bool, error) {
	for _, protocol := range p.Protocols {
		if !tools.Contains(string(protocol), KnownVideoStreamProtocols) {
			return false, fmt.Errorf("unknown protocol in video stream parameters: %s not in %+v", protocol, KnownVideoStreamProtocols)
		}
	}

	return false, nil
}

func (p VideoStreamCapabilityParameters) GetInstance() string {
	return string(GetStreamCapabilityInstance)
}

func (p VideoStreamCapabilityParameters) Merge(parameters ICapabilityParameters) ICapabilityParameters {
	return nil
}

func (p VideoStreamCapabilityParameters) ToUserInfoProto() *common.TIoTUserInfo_TCapability_TVideoStreamCapabilityParameters {
	protocols := make([]string, 0, len(p.Protocols))
	for _, protocol := range p.Protocols {
		protocols = append(protocols, string(protocol))
	}

	return &common.TIoTUserInfo_TCapability_TVideoStreamCapabilityParameters{
		Protocols: protocols,
	}
}

func (p *VideoStreamCapabilityParameters) FromUserInfoProto(parameters *common.TIoTUserInfo_TCapability_TVideoStreamCapabilityParameters) {
	protocols := make([]VideoStreamProtocol, 0, len(parameters.GetProtocols()))
	for _, protocol := range parameters.GetProtocols() {
		protocols = append(protocols, VideoStreamProtocol(protocol))
	}

	*p = VideoStreamCapabilityParameters{
		Protocols: protocols,
	}
}

func (p VideoStreamCapabilityParameters) toProto() *protos.VideoStreamCapabilityParameters {
	protocols := make([]string, 0, len(p.Protocols))
	for _, protocol := range p.Protocols {
		protocols = append(protocols, string(protocol))
	}

	return &protos.VideoStreamCapabilityParameters{
		Protocols: protocols,
	}
}

func (p *VideoStreamCapabilityParameters) fromProto(parameters *protos.VideoStreamCapabilityParameters) {
	protocols := make([]VideoStreamProtocol, 0, len(parameters.GetProtocols()))
	for _, protocol := range parameters.GetProtocols() {
		protocols = append(protocols, VideoStreamProtocol(protocol))
	}

	*p = VideoStreamCapabilityParameters{
		Protocols: protocols,
	}
}
