package model

import (
	"encoding/json"
	"time"

	"golang.org/x/exp/slices"

	bulbasaur "a.yandex-team.ru/alice/iot/bulbasaur/errors"
	"a.yandex-team.ru/alice/iot/bulbasaur/protos"
	"a.yandex-team.ru/alice/library/go/inflector"
	"a.yandex-team.ru/alice/library/go/timestamp"
	"a.yandex-team.ru/alice/megamind/protos/common"
	"a.yandex-team.ru/library/go/core/xerrors"
	"a.yandex-team.ru/library/go/ptr"
	"a.yandex-team.ru/library/go/valid"
)

type EventProperty struct {
	reportable     bool
	retrievable    bool
	state          *EventPropertyState
	parameters     EventPropertyParameters
	stateChangedAt timestamp.PastTimestamp
	lastUpdated    timestamp.PastTimestamp
	lastActivated  timestamp.PastTimestamp
}

func (p *EventProperty) Type() PropertyType {
	return EventPropertyType
}

func (p *EventProperty) Instance() string {
	return propertyInstance(p)
}

func (p *EventProperty) Reportable() bool {
	return p.reportable
}

func (p EventProperty) Retrievable() bool {
	return p.retrievable
}

func (p EventProperty) Parameters() IPropertyParameters {
	return p.parameters
}

func (p EventProperty) Key() string {
	return PropertyKey(p.Type(), p.Instance())
}

func (p *EventProperty) QuerySuggestions(inflection inflector.Inflection, options SuggestionsOptions) []string {
	suggestions := make([]string, 0)
	for i := range suggestions {
		suggestions[i] = options.AddHouseholdToSuggest(suggestions[i])
	}
	return suggestions
}

func (p *EventProperty) StateChangedAt() timestamp.PastTimestamp {
	return p.stateChangedAt
}

func (p EventProperty) LastUpdated() timestamp.PastTimestamp {
	return p.lastUpdated
}

func (p *EventProperty) WithStateChangedAt(stateChangedAt timestamp.PastTimestamp) IPropertyWithBuilder {
	p.stateChangedAt = stateChangedAt
	return p
}

func (p *EventProperty) WithLastUpdated(updated timestamp.PastTimestamp) IPropertyWithBuilder {
	p.lastUpdated = updated
	return p
}

func (p EventProperty) State() IPropertyState {
	if p.state == nil {
		return nil
	}
	return *p.state
}

func (p *EventProperty) SetReportable(reportable bool) {
	p.reportable = reportable
}

func (p *EventProperty) SetRetrievable(retrievable bool) {
	p.retrievable = retrievable
}

func (p *EventProperty) WithReportable(reportable bool) IPropertyWithBuilder {
	p.SetReportable(reportable)
	return p
}

func (p *EventProperty) WithRetrievable(retrievable bool) IPropertyWithBuilder {
	p.SetRetrievable(retrievable)
	return p
}

func (p *EventProperty) SetParameters(parameters IPropertyParameters) {
	p.parameters = parameters.(EventPropertyParameters)
}

func (p *EventProperty) WithParameters(parameters IPropertyParameters) IPropertyWithBuilder {
	p.SetParameters(parameters)
	return p
}

func (p *EventProperty) SetStateChangedAt(stateChangedAt timestamp.PastTimestamp) {
	p.stateChangedAt = stateChangedAt
}

func (p *EventProperty) SetLastUpdated(lastUpdated timestamp.PastTimestamp) {
	p.lastUpdated = lastUpdated
}

func (p *EventProperty) SetState(state IPropertyState) {
	structure, ok := state.(EventPropertyState)
	if ok {
		p.state = &structure
	}
	pointerState, ok := state.(*EventPropertyState)
	if ok {
		p.state = pointerState
	}
}

func (p *EventProperty) WithState(state IPropertyState) IPropertyWithBuilder {
	p.SetState(state)
	return p
}

func (p *EventProperty) Clone() IProperty {
	return cloneProperty(p)
}

func (p *EventProperty) Equal(other IProperty) bool {
	return equalProperty(p, other)
}

func (p *EventProperty) MarshalJSON() ([]byte, error) {
	return marshalProperty(p)
}

func (p *EventProperty) UnmarshalJSON(b []byte) error {
	var eventPropertyRaw struct {
		Reportable     bool                    `json:"reportable"`
		Retrievable    bool                    `json:"retrievable"`
		Parameters     EventPropertyParameters `json:"parameters"`
		State          *EventPropertyState     `json:"state"`
		StateChangedAt timestamp.PastTimestamp `json:"state_changed_at"`
		LastUpdated    timestamp.PastTimestamp `json:"last_updated"`
	}
	if err := json.Unmarshal(b, &eventPropertyRaw); err != nil {
		return err
	}

	p.SetReportable(eventPropertyRaw.Reportable)
	p.SetRetrievable(eventPropertyRaw.Retrievable)
	p.SetStateChangedAt(eventPropertyRaw.StateChangedAt)
	p.SetLastUpdated(eventPropertyRaw.LastUpdated)
	p.SetParameters(eventPropertyRaw.Parameters)
	if eventPropertyRaw.State != nil {
		p.SetState(eventPropertyRaw.State)
	}
	return nil
}

func (p EventProperty) ToProto() *protos.Property {
	pp := &protos.Property{
		Type:           *p.Type().toProto(),
		Reportable:     p.Reportable(),
		Retrievable:    p.Retrievable(),
		StateChangedAt: float64(p.StateChangedAt()),
		LastUpdated:    float64(p.LastUpdated()),
	}

	parameters := p.Parameters().(EventPropertyParameters)

	events := make([]*protos.Event, 0, len(parameters.Events))
	for _, event := range parameters.Events {
		protoEvent := &protos.Event{
			Value: string(event.Value),
		}
		if event.Name != nil {
			protoEvent.Name = *event.Name
		}
		events = append(events, protoEvent)
	}

	pp.Parameters = &protos.Property_EPParameters{
		EPParameters: &protos.EventPropertyParameters{
			Instance: p.Instance(),
			Events:   events,
		},
	}

	if p.State() != nil {
		s := p.State().(EventPropertyState)
		pp.State = &protos.Property_EPState{
			EPState: s.toProto(),
		}
	}
	return pp
}

func (p *EventProperty) FromProto(pp *protos.Property) {
	events := make([]Event, 0, len(pp.GetEPParameters().GetEvents()))
	for _, event := range pp.GetEPParameters().GetEvents() {
		events = append(events, Event{
			Value: EventValue(event.Value),
			Name:  ptr.String(event.Name),
		})
	}

	params := EventPropertyParameters{
		Instance: PropertyInstance(pp.GetEPParameters().Instance),
		Events:   events,
	}

	property := EventProperty{}
	property.SetReportable(pp.Reportable)
	property.SetRetrievable(pp.Retrievable)
	property.SetParameters(params)
	property.SetStateChangedAt(timestamp.PastTimestamp(pp.StateChangedAt))
	property.SetLastUpdated(timestamp.PastTimestamp(pp.LastUpdated))

	if pp.GetEPState() != nil {
		var state EventPropertyState
		state.fromProto(pp.GetEPState())
		property.SetState(state)
	}

	*p = property
}

func (p EventProperty) ToUserInfoProto() *common.TIoTUserInfo_TProperty {
	protoProperty := &common.TIoTUserInfo_TProperty{
		Type:           common.TIoTUserInfo_TProperty_EventPropertyType,
		Retrievable:    p.Retrievable(),
		Reportable:     p.Reportable(),
		StateChangedAt: float64(p.StateChangedAt()),
		LastUpdated:    float64(p.LastUpdated()),
		AnalyticsName:  analyticsPropertyName(analyticsPropertyKey{Type: EventPropertyType, Instance: p.Instance()}),
		AnalyticsType:  analyticsPropertyType(p.Type()),
		Parameters: &common.TIoTUserInfo_TProperty_EventPropertyParameters{
			EventPropertyParameters: p.parameters.ToUserInfoProto(),
		},
		State: nil,
	}

	if p.state != nil {
		protoProperty.State = &common.TIoTUserInfo_TProperty_EventPropertyState{
			EventPropertyState: p.state.ToUserInfoProto(),
		}
	}

	return protoProperty
}

func (p *EventProperty) FromUserInfoProto(pp *common.TIoTUserInfo_TProperty) {
	paramsProto := pp.GetEventPropertyParameters()
	events := make([]Event, 0, len(paramsProto.GetEvents()))
	for _, event := range paramsProto.GetEvents() {
		events = append(events, Event{
			Value: EventValue(event.GetValue()),
			Name:  ptr.String(event.GetName()),
		})
	}

	params := EventPropertyParameters{
		Instance: PropertyInstance(paramsProto.GetInstance()),
		Events:   events,
	}

	property := EventProperty{}
	property.SetReportable(pp.GetReportable())
	property.SetRetrievable(pp.GetRetrievable())
	property.SetParameters(params)
	property.SetStateChangedAt(timestamp.PastTimestamp(pp.GetStateChangedAt()))
	property.SetLastUpdated(timestamp.PastTimestamp(pp.GetLastUpdated()))

	if state := pp.GetEventPropertyState(); state != nil {
		var s EventPropertyState
		s.fromUserInfoProto(state)
		property.SetState(s)
	}

	*p = property
}

func (p EventProperty) Status(now timestamp.PastTimestamp) *PropertyStatus {
	if p.State() == nil {
		return nil
	}
	state := p.State().(EventPropertyState)
	activationValueEvent := false
	if eventValues, exist := EventPropertyInstanceToActivationEventValues[PropertyInstance(p.Instance())]; exist {
		if eventValues.Contains(state.Value) {
			activationValueEvent = true
		}
	}

	switch {
	case activationValueEvent && p.Retrievable():
		return PS(DangerStatus)
	case !activationValueEvent && p.Retrievable() && p.StateChangedAt().Add(30*time.Minute) > now:
		return PS(WarningStatus)
	case !activationValueEvent && p.Retrievable() && p.StateChangedAt().Add(30*time.Minute) <= now:
		return PS(NormalStatus)
	case p.StateChangedAt().Add(10*time.Minute) > now:
		return PS(DangerStatus)
	case p.StateChangedAt().Add(30*time.Minute) > now:
		return PS(WarningStatus)
	default:
		return PS(NormalStatus)
	}
}

var _ valid.Validator = new(EventPropertyParameters)

type EventPropertyParameters struct {
	Instance PropertyInstance `json:"instance"`
	Events   Events           `json:"events"`
}

func (pp *EventPropertyParameters) Clone() EventPropertyParameters {
	var clone EventPropertyParameters
	clone.Instance = pp.Instance
	clone.Events = make([]Event, len(pp.Events))
	for i, event := range pp.Events {
		clone.Events[i] = event.Clone()
	}
	return clone
}

func (pp *EventPropertyParameters) FillByWellKnownEvents() {
	events := make([]Event, 0, len(pp.Events))
	for i := range pp.Events {
		event := &pp.Events[i]
		eventKey := EventKey{Instance: pp.Instance, Value: event.Value}
		if v, ok := KnownEvents[eventKey]; ok {
			events = append(events, v)
		}
	}
	pp.Events = events
}

func (pp EventPropertyParameters) GetInstanceName() string {
	return KnownPropertyInstanceNames[pp.Instance]
}

func (pp EventPropertyParameters) Validate(*valid.ValidationCtx) (bool, error) {
	var verrs valid.Errors
	if !slices.Contains(KnownEventPropertyInstances, pp.Instance.String()) {
		verrs = append(verrs, xerrors.Errorf("unknown event property instance: %q", pp.Instance))
	}

	eventValues := EventPropertyInstanceToEventValues(pp.Instance)
	if len(eventValues) == 0 {
		verrs = append(verrs, xerrors.Errorf("unknown event property instance: %q", pp.Instance))
	}

	for _, event := range pp.Events {
		if !slices.Contains(eventValues, string(event.Value)) {
			verrs = append(verrs, xerrors.Errorf("unknown event property value: %q", event.Value))
		}
	}

	if len(verrs) != 0 {
		return false, verrs
	}
	return true, nil
}

func (pp EventPropertyParameters) GetInstance() string {
	return string(pp.Instance)
}

func (pp EventPropertyParameters) ToUserInfoProto() *common.TIoTUserInfo_TProperty_TEventPropertyParameters {
	protoEvents := make([]*common.TIoTUserInfo_TProperty_TEventPropertyParameters_TEvent, 0, len(pp.Events))
	for _, event := range pp.Events {
		protoEvent := &common.TIoTUserInfo_TProperty_TEventPropertyParameters_TEvent{
			Value: string(event.Value),
		}
		if event.Name != nil {
			protoEvent.Name = *event.Name
		}
		protoEvents = append(protoEvents, protoEvent)
	}

	return &common.TIoTUserInfo_TProperty_TEventPropertyParameters{
		Instance: pp.Instance.String(),
		Events:   protoEvents,
	}
}

type Event struct {
	Value EventValue `json:"value"`
	Name  *string    `json:"name,omitempty"`
}

func (e *Event) Clone() Event {
	var clone Event
	if e.Name != nil {
		clone.Name = ptr.String(*e.Name)
	}
	clone.Value = e.Value
	return clone
}

// for getting human-friendly names
type EventKey struct {
	Instance PropertyInstance `json:"instance"`
	Value    EventValue       `json:"value"`
}

type Events []Event

func (e Events) HasValue(value EventValue) bool {
	for _, event := range e {
		if value == event.Value {
			return true
		}
	}
	return false
}

func (e Events) EventByValue(value EventValue) (Event, error) {
	for _, event := range e {
		if value == event.Value {
			return event, nil
		}
	}
	return Event{}, xerrors.Errorf("event not found")
}

var _ valid.Validator = new(EventPropertyState)

type EventValue string

type EventValues []EventValue

func (ev EventValues) Contains(value EventValue) bool {
	for _, eventValue := range ev {
		if eventValue == value {
			return true
		}
	}
	return false
}

type EventPropertyState struct {
	Instance PropertyInstance `json:"instance"`
	Value    EventValue       `json:"value"`
}

func (ps EventPropertyState) GetType() PropertyType {
	return EventPropertyType
}

func (ps EventPropertyState) GetInstance() string {
	return string(ps.Instance)
}

func (ps EventPropertyState) Validate(vctx *valid.ValidationCtx) (bool, error) {
	var verrs valid.Errors
	if !slices.Contains(KnownEventPropertyInstances, ps.Instance.String()) {
		verrs = append(verrs, xerrors.Errorf("unknown event property instance: %q", ps.Instance))
	}

	eventValues := EventPropertyInstanceToEventValues(ps.Instance)
	if len(eventValues) == 0 {
		verrs = append(verrs, xerrors.Errorf("unknown event property instance: %q", ps.Instance))
	}

	if !slices.Contains(eventValues, string(ps.Value)) {
		verrs = append(verrs, xerrors.Errorf("unknown event property value: %q", ps.Value))
	}

	if len(verrs) != 0 {
		return false, verrs
	}
	return true, nil
}

func (ps EventPropertyState) ValidateState(property IProperty) error {
	var errors bulbasaur.Errors

	if property.Type() != EventPropertyType {
		errors = append(errors, xerrors.Errorf("invalid state type for current property: expected %q, got %q", property.Type(), EventPropertyType))
	}

	if ps.GetInstance() != property.Instance() {
		errors = append(errors, xerrors.Errorf("invalid state instance for current property: expected %q, got %q", property.Instance(), ps.GetInstance()))
	}

	parameters := property.Parameters().(EventPropertyParameters)
	if !parameters.Events.HasValue(ps.Value) {
		errors = append(errors, xerrors.Errorf("unknown state instance for current property: %q", ps.GetInstance()))
	}

	if len(errors) > 0 {
		return errors
	}
	return nil
}

func (ps EventPropertyState) Equals(state IPropertyState) bool {
	if state == nil {
		return false
	}
	if state.GetType() != EventPropertyType || state.GetInstance() != ps.GetInstance() {
		return false
	}
	return state.(EventPropertyState).Value == ps.Value
}

func (ps *EventPropertyState) toProto() *protos.EventPropertyState {
	p := &protos.EventPropertyState{
		Instance: string(ps.Instance),
		Value:    string(ps.Value),
	}
	return p
}

func (ps *EventPropertyState) fromProto(p *protos.EventPropertyState) {
	*ps = EventPropertyState{
		Instance: PropertyInstance(p.Instance),
		Value:    EventValue(p.Value),
	}
}

func (ps EventPropertyState) ToUserInfoProto() *common.TIoTUserInfo_TProperty_TEventPropertyState {
	return &common.TIoTUserInfo_TProperty_TEventPropertyState{
		Instance: ps.Instance.String(),
		Value:    string(ps.Value),
	}
}

func (ps *EventPropertyState) fromUserInfoProto(p *common.TIoTUserInfo_TProperty_TEventPropertyState) {
	*ps = EventPropertyState{
		Instance: PropertyInstance(p.GetInstance()),
		Value:    EventValue(p.GetValue()),
	}
}

func EventPropertyInstanceToEventValues(eventInstance PropertyInstance) []string {
	res := make([]string, 0)
	for eventKey := range KnownEvents {
		if eventKey.Instance == eventInstance {
			res = append(res, string(eventKey.Value))
		}
	}
	return res
}
