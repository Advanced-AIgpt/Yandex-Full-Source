package model

import (
	"fmt"
	"math"
	"strconv"
	"strings"

	"github.com/mitchellh/mapstructure"

	"a.yandex-team.ru/alice/iot/adapters/xiaomi_adapter/xiaomi/iotapi"
	"a.yandex-team.ru/alice/iot/adapters/xiaomi_adapter/xiaomi/miotspec"
	"a.yandex-team.ru/alice/iot/bulbasaur/dto/adapter"
	"a.yandex-team.ru/alice/iot/bulbasaur/model"
	"a.yandex-team.ru/alice/library/go/tools"
	"a.yandex-team.ru/library/go/core/xerrors"
)

type Room struct {
	ID   string
	Name string
}

type Region iotapi.Region

func (region Region) String() string {
	return string(region)
}

type XiaomiCustomData struct {
	Type    string `json:"type"`
	Region  Region `json:"region"`
	IsSplit bool   `json:"isSplit"`
	CloudID int    `json:"cloudId"`
}

type Device struct {
	DID            string
	Type           string
	Name           string
	Category       string
	LastUpdateTime uint64
	Region         Region
	CloudID        int
	Room           Room
	Services       []Service
	PropertyStates []PropertyState
	EventsOccurred []EventOccurred
	IsSplit        bool
}

func (d *Device) IsTypeOf(typeSubstr string) bool {
	return strings.Contains(d.Type, typeSubstr)
}

func (d Device) Split() []Device {
	splitResult := make([]Device, 0, 5)
	switch d.Category {
	case switchCategory, socketCategory:
		for _, service := range d.Services {
			if service.IsTypeOf(switchService) {
				switchDevice := d
				switchDevice.DID = fmt.Sprintf("%s.%d", d.DID, service.Iid)
				if len(splitResult) == 0 {
					switchDevice.Name = d.Name
				} else {
					switchDevice.Name = fmt.Sprintf("%s %d", d.Name, len(splitResult)+1)
				}
				deviceInfoService, _ := d.GetDeviceInfoService() // todo: can't log here!
				switchDevice.Services = []Service{deviceInfoService, service}
				switchDevice.IsSplit = true

				splitResult = append(splitResult, switchDevice)
			}
		}
	default:
		splitResult = append(splitResult, d)
	}
	return splitResult
}

func (d *Device) GetDeviceID() string {
	if d.IsSplit {
		if index := strings.LastIndexByte(d.DID, '.'); index >= 0 {
			return d.DID[:index]
		}
	}
	return d.DID
}

func (d *Device) PopulateRegion(region iotapi.Region) {
	d.Region = Region(region)
}

func (d *Device) PopulateDeviceData(rawDevice iotapi.Device) {
	d.DID = rawDevice.DID
	d.Type = rawDevice.Type
	d.Name = rawDevice.Name
	d.Category = rawDevice.Category
	d.LastUpdateTime = rawDevice.LastUpdateTimestamp
	d.CloudID = rawDevice.CloudID
}

func (d *Device) GetCustomData() XiaomiCustomData {
	return XiaomiCustomData{
		Type:    d.Type,
		Region:  d.Region,
		IsSplit: d.IsSplit,
		CloudID: d.CloudID,
	}
}

func (d *Device) GetDeviceInfo() *model.DeviceInfo {
	deviceInfo := &model.DeviceInfo{}
	if dManufacturer := d.GetPropertyStateValue(manufacturerProperty); dManufacturer != nil {
		if manufacturer := dManufacturer.(string); len(manufacturer) > 0 {
			deviceInfo.Manufacturer = &manufacturer
		}
	}
	if dModel := d.GetPropertyStateValue(modelProperty); dModel != nil {
		if mdl := dModel.(string); len(mdl) > 0 {
			deviceInfo.Model = &mdl
		}
	}
	if dSwVersion := d.GetPropertyStateValue(firmwareVersionProperty); dSwVersion != nil {
		if swVersion := dSwVersion.(string); len(swVersion) > 0 {
			deviceInfo.SwVersion = &swVersion
		}
	}
	return deviceInfo
}

func (d *Device) PopulateCustomData(cd interface{}) {
	var xcd XiaomiCustomData
	_ = mapstructure.Decode(cd, &xcd)

	d.Type = xcd.Type
	d.Region = xcd.Region
	d.IsSplit = xcd.IsSplit
	d.CloudID = xcd.CloudID
}

func (d *Device) PopulateRoomData(rawDevice iotapi.Device, rawHomes []iotapi.Home) {
	for _, h := range rawHomes {
		for _, r := range h.Rooms {
			if rawDevice.RID == r.ID {
				d.Room = Room{ID: r.ID, Name: r.Name}
			}
		}
	}
}

func (d *Device) PopulateServices(services []miotspec.Service) {
	d.Services = make([]Service, 0, len(services))
	for _, service := range services {
		if d.IsSplit && !strings.HasSuffix(d.DID, strconv.Itoa(service.Iid)) {
			continue
		}

		properties := make([]Property, 0, len(service.Properties))
		for _, p := range service.Properties {
			properties = append(properties, Property(p))
		}

		actions := make([]Action, 0, len(service.Actions))
		for _, a := range service.Actions {
			actions = append(actions, Action(a))
		}

		events := make([]Event, 0, len(service.Events))
		for _, e := range service.Events {
			events = append(events, Event(e))
		}

		d.Services = append(d.Services, Service{
			Iid:        service.Iid,
			Type:       service.Type,
			Properties: properties,
			Actions:    actions,
			Events:     events,
		})
	}
}

func (d *Device) PopulatePropertyStates(properties []iotapi.Property) {
	d.PropertyStates = make([]PropertyState, 0, len(properties))
	for _, p := range properties {
		d.PropertyStates = append(d.PropertyStates, PropertyState{Property: p})
	}
}

func (d *Device) PopulateEventsOccurred(events []iotapi.Event) {
	d.EventsOccurred = make([]EventOccurred, 0, len(events))
	for _, e := range events {
		d.EventsOccurred = append(d.EventsOccurred, EventOccurred{Event: e})
	}
}

func (d *Device) GetActionID(actionSub string) (string, bool) {
	for _, s := range d.Services {
		for _, a := range s.Actions {
			if a.IsTypeOf(actionSub) {
				deviceID := d.DID
				if d.IsSplit {
					deviceID = strings.Split(deviceID, ".")[0]
				}
				return fmt.Sprintf("%s.%d.%d", deviceID, s.Iid, a.Iid), true
			}
		}
	}
	return "", false
}

func (d *Device) GetPropertyState(propertySub string) (PropertyState, bool) {
	propID, ok := d.GetPropertyID(propertySub)
	if !ok {
		return PropertyState{}, false
	}

	for _, ps := range d.PropertyStates {
		if ps.Property.Pid == propID {
			return ps, true
		}
	}

	return PropertyState{}, false
}

func (d *Device) GetEventOccurred(eventSub string) (EventOccurred, bool) {
	eventID, ok := d.GetEventID(eventSub)
	if !ok {
		return EventOccurred{}, false
	}

	for _, e := range d.EventsOccurred {
		if e.Event.Eid == eventID {
			return e, true
		}
	}

	return EventOccurred{}, false
}

func (d *Device) GetService(serviceSub string) (Service, bool) {
	for _, s := range d.Services {
		if s.IsTypeOf(serviceSub) {
			return s, true
		}
	}
	return Service{}, false
}

func (d *Device) GetServiceProperty(serviceSub, propertySub string) (Property, bool) {
	if s, ok := d.GetService(serviceSub); ok {
		if p, ok := s.GetProperty(propertySub); ok {
			return p, true
		}
	}
	return Property{}, false
}

func (d *Device) GetServiceAction(serviceSub, actionSub string) (Action, bool) {
	if s, ok := d.GetService(serviceSub); ok {
		if a, ok := s.GetAction(actionSub); ok {
			return a, true
		}
	}
	return Action{}, false
}

func (d *Device) GetPropertyID(propertySub string) (string, bool) {
	for _, s := range d.Services {
		for _, p := range s.Properties {
			if p.IsTypeOf(propertySub) {
				deviceID := d.DID
				if d.IsSplit {
					deviceID = strings.Split(deviceID, ".")[0]
				}
				return fmt.Sprintf("%s.%d.%d", deviceID, s.Iid, p.Iid), true
			}
		}
	}
	return "", false
}

func (d *Device) GetProperty(propertySub string) (Property, bool) {
	for _, s := range d.Services {
		for _, p := range s.Properties {
			if p.IsTypeOf(propertySub) {
				return p, true
			}
		}
	}
	return Property{}, false
}

func (d *Device) GetEventID(eventSub string) (string, bool) {
	for _, s := range d.Services {
		for _, e := range s.Events {
			if e.IsTypeOf(eventSub) {
				deviceID := d.DID
				if d.IsSplit {
					deviceID = strings.Split(deviceID, ".")[0]
				}
				return fmt.Sprintf("%s.%d.%d", deviceID, s.Iid, e.Iid), true
			}
		}
	}
	return "", false
}

func (d *Device) GetPropertyStateValue(propertySub string) interface{} {
	ps, ok := d.GetPropertyState(propertySub)
	if !ok {
		return nil
	}
	return ps.Property.Value
}

func (d *Device) GetStatePropertyIDs() []string {
	propertyIDs := make([]string, 0)
	for _, stateProperty := range stateProperties {
		if id, ok := d.GetPropertyID(stateProperty); ok {
			propertyIDs = append(propertyIDs, id)
		}
	}
	return propertyIDs
}

func (d *Device) GetDeviceInfoPropertyIDs() []string {
	propIDs := make([]string, 0)
	for _, p := range deviceInfoProperties {
		if prop, ok := d.GetPropertyID(p); ok {
			propIDs = append(propIDs, prop)
		}
	}
	return propIDs
}

func (d *Device) GetDeviceInfoService() (Service, bool) {
	for _, s := range d.Services {
		if s.IsTypeOf(deviceInfoService) {
			return s, true
		}
	}
	return Service{}, false
}

type Devices []Device

func (d Devices) GroupByCloudID() map[int]Devices {
	result := make(map[int]Devices)
	for _, device := range d {
		result[device.CloudID] = append(result[device.CloudID], device)
	}
	return result
}

func (d Devices) GroupByRegion() map[Region]Devices {
	result := make(map[Region]Devices)
	for _, device := range d {
		result[device.Region] = append(result[device.Region], device)
	}
	return result
}

func (d Devices) AsMap() map[string]Device {
	result := make(map[string]Device)
	for _, device := range d {
		result[device.GetDeviceID()] = device
	}
	return result
}

func (d Devices) GetPropertyStates(deviceRequests map[string]adapter.DeviceActionRequestView) []PropertyState {
	result := make([]PropertyState, 0)
	for _, device := range d {
		result = append(result, device.GetPropertyStates(deviceRequests[device.DID])...)
	}
	return result
}

func (d Devices) GetActionStates(deviceRequests map[string]adapter.DeviceActionRequestView) []ActionState {
	result := make([]ActionState, 0)
	for _, device := range d {
		result = append(result, device.GetActionStates(deviceRequests[device.DID])...)
	}
	return result
}

type Action miotspec.Action

func (a *Action) IsTypeOf(typeSubstr string) bool {
	return strings.Contains(a.Type, typeSubstr)
}

type Property miotspec.Property

func (p *Property) IsTypeOf(typeSubstr string) bool {
	return strings.Contains(p.Type, typeSubstr)
}

func (p *Property) HasAccess(access AccessType) bool {
	return tools.Contains(string(access), p.Access)
}

func (p *Property) GetValueRange() (ValueRange, error) {
	if len(p.ValueRange) != 3 {
		return ValueRange{}, xerrors.New("unable to create value range")
	}
	return ValueRange{
		Begin:     p.ValueRange[0],
		End:       p.ValueRange[1],
		Precision: p.ValueRange[2],
	}, nil
}

type ValueRange struct {
	Begin, End, Precision float64
}

func (vr ValueRange) GetThirds() (float64, float64) {
	third := math.Floor((vr.End-vr.Begin)/3/vr.Precision) * vr.Precision
	return vr.Begin + third, vr.Begin + 2*third
}

type Event miotspec.Event

func (e *Event) IsTypeOf(typeSubstr string) bool {
	return strings.Contains(e.Type, typeSubstr)
}

type Service struct {
	Iid        int
	Type       string
	Properties []Property
	Events     []Event
	Actions    []Action
}

func (s *Service) IsTypeOf(typeSubstr string) bool {
	return strings.Contains(s.Type, typeSubstr)
}

func (s *Service) GetProperty(propertySub string) (Property, bool) {
	for _, p := range s.Properties {
		if p.IsTypeOf(propertySub) {
			return p, true
		}
	}
	return Property{}, false
}

func (s *Service) GetAction(actionSub string) (Action, bool) {
	for _, a := range s.Actions {
		if a.IsTypeOf(actionSub) {
			return a, true
		}
	}
	return Action{}, false
}

func getKelvinFromPercent(percent float64) float64 {
	return percent*27 + 3000 // 0%=3000, 100%=5700
}

func getPercentFromKelvin(kelvin float64) float64 {
	return (kelvin - 3000) / 27
}

func getMMHGFromPascal(pascal float64) float64 {
	return pascal / 133
}

func constraintBetween(value float64, min float64, max float64) float64 {
	if value > max {
		return max
	}
	if value < min {
		return min
	}
	return value
}

func boolPropertyValue(value interface{}) bool {
	boolValue, isBool := value.(bool)
	if isBool {
		return boolValue
	}
	stringValue, isString := value.(string)
	if isString {
		if strings.EqualFold(stringValue, "on") {
			return true
		}
		if strings.EqualFold(stringValue, "off") {
			return false
		}
	}
	panic(fmt.Sprintf("invalid bool property value - %+v", value))
}

func getXiaomiVacuumCleanerModeList(property Property) map[model.ModeValue]float64 {
	// hack for vacuum cleaners which modes values are 101,102,103,104
	if valuesListValuesOver100(property) {
		return xiaomiVacuumModes100Map
	}
	return xiaomiVacuumModesMap
}

func getYandexVacuumCleanerModeList(property Property) map[float64]model.ModeValue {
	// hack for vacuum cleaners which modes values are 101,102,103,104
	if valuesListValuesOver100(property) {
		return yandexVacuumModes100Map
	}
	return yandexVacuumModesMap
}

func valuesListValuesOver100(property Property) bool {
	return len(property.ValueList) > 0 && property.ValueList[0].Value > 100
}
