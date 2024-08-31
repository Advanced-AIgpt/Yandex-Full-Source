package model

import (
	"context"
	"encoding/json"
	"fmt"
	"math"
	"sort"
	"strings"

	"github.com/google/go-cmp/cmp"
	"github.com/mitchellh/copystructure"
	"github.com/mitchellh/mapstructure"
	"golang.org/x/exp/slices"

	"a.yandex-team.ru/alice/iot/bulbasaur/controller/experiments"
	"a.yandex-team.ru/alice/iot/bulbasaur/model/quasar"
	yandexiocd "a.yandex-team.ru/alice/iot/bulbasaur/model/yandexio"
	"a.yandex-team.ru/alice/iot/bulbasaur/protos"
	"a.yandex-team.ru/alice/library/go/inflector"
	"a.yandex-team.ru/alice/library/go/timestamp"
	"a.yandex-team.ru/alice/library/go/tools"
	"a.yandex-team.ru/alice/megamind/protos/common"
	devicepb "a.yandex-team.ru/alice/protos/data/device"
	"a.yandex-team.ru/library/go/core/xerrors"
	"a.yandex-team.ru/library/go/ptr"
	yslices "a.yandex-team.ru/library/go/slices"
	"a.yandex-team.ru/library/go/valid"
)

var _ valid.Validator = new(DeviceInfo)
var _ valid.Validator = new(DeviceType)

const eventPropertyAntiFlapTimeout float64 = 1.0 // anti-flap timeout for updating history

type Device struct {
	ID             string `json:"id"`
	Name           string
	Aliases        []string
	Description    *string // deprecated: not stored in db and not shown to user
	ExternalID     string
	ExternalName   string
	SkillID        string
	HouseholdID    string `json:"household_id,omitempty"`
	Type           DeviceType
	OriginalType   DeviceType
	Room           *Room
	Groups         []Group
	Capabilities   Capabilities `json:"capabilities"`
	Properties     Properties   `json:"properties"`
	DeviceInfo     *DeviceInfo
	CustomData     interface{}
	Updated        timestamp.PastTimestamp
	Created        timestamp.PastTimestamp
	Favorite       bool `json:"favorite"`
	Status         DeviceStatus
	StatusUpdated  timestamp.PastTimestamp
	InternalConfig DeviceConfig
	SharingInfo    *SharingInfo
}

type DeviceInfo struct {
	Manufacturer *string `json:"manufacturer,omitempty"`
	Model        *string `json:"model,omitempty"`
	HwVersion    *string `json:"hw_version,omitempty"`
	SwVersion    *string `json:"sw_version,omitempty"`
}

func (di DeviceInfo) Validate(_ *valid.ValidationCtx) (bool, error) {
	var err valid.Errors

	if di.Manufacturer != nil && len(*di.Manufacturer) > 1024 {
		err = append(err, fmt.Errorf("manufacturer is too long, should be less than 1024"))
	}

	if di.Model != nil && len(*di.Model) > 1024 {
		err = append(err, fmt.Errorf("model is too long, should be less than 1024"))
	}

	if di.HwVersion != nil && len(*di.HwVersion) > 1024 {
		err = append(err, fmt.Errorf("hw_version is too long, should be less than 1024"))
	}

	if di.SwVersion != nil && len(*di.SwVersion) > 1024 {
		err = append(err, fmt.Errorf("sw_version is too long, should be less than 1024"))
	}

	if len(err) == 0 {
		return false, nil
	}
	return false, err
}

func (di *DeviceInfo) HasManufacturer() bool {
	return di != nil && di.Manufacturer != nil && len(*di.Manufacturer) > 0
}

func (di *DeviceInfo) GetManufacturer() string {
	if di.HasManufacturer() {
		return *di.Manufacturer
	}
	return ""
}

func (di *DeviceInfo) HasModel() bool {
	return di != nil && di.Model != nil && len(*di.Model) > 0
}

func (di *DeviceInfo) GetModel() string {
	if di.HasModel() {
		return *di.Model
	}
	return ""
}

func (di *DeviceInfo) HasHwVersion() bool {
	return di != nil && di.HwVersion != nil && len(*di.HwVersion) > 0
}

func (di *DeviceInfo) GetHwVersion() string {
	if di.HasHwVersion() {
		return *di.HwVersion
	}
	return ""
}

func (di *DeviceInfo) HasSwVersion() bool {
	return di != nil && di.SwVersion != nil && len(*di.SwVersion) > 0
}

func (di *DeviceInfo) GetSwVersion() string {
	if di.HasSwVersion() {
		return *di.SwVersion
	}
	return ""
}

func (di DeviceInfo) ToProto() *protos.DeviceInfo {
	pdi := &protos.DeviceInfo{}
	if di.HwVersion != nil {
		pdi.HwVersion = *di.HwVersion
	}
	if di.Manufacturer != nil {
		pdi.Manufacturer = *di.Manufacturer
	}
	if di.Model != nil {
		pdi.Model = *di.Model
	}
	if di.SwVersion != nil {
		pdi.SwVersion = *di.SwVersion
	}
	return pdi
}

func (di *DeviceInfo) FromProto(p *protos.DeviceInfo) {
	if len(p.HwVersion) > 0 {
		di.HwVersion = ptr.String(p.HwVersion)
	}
	if len(p.Manufacturer) > 0 {
		di.Manufacturer = ptr.String(p.Manufacturer)
	}
	if len(p.Model) > 0 {
		di.Model = ptr.String(p.Model)
	}
	if len(p.SwVersion) > 0 {
		di.SwVersion = ptr.String(p.SwVersion)
	}
}

func (di DeviceInfo) ToUserInfoProto() *common.TIoTUserInfo_TDevice_TDeviceInfo {
	return &common.TIoTUserInfo_TDevice_TDeviceInfo{
		Manufacturer: di.GetManufacturer(),
		Model:        di.GetModel(),
		HwVersion:    di.GetHwVersion(),
		SwVersion:    di.GetSwVersion(),
	}
}

func (di *DeviceInfo) FromUserInfoProto(pdt *common.TIoTUserInfo_TDevice_TDeviceInfo) {
	deviceInfo := DeviceInfo{}
	deviceInfo.Manufacturer = ptr.String(pdt.GetManufacturer())
	deviceInfo.Model = ptr.String(pdt.GetModel())
	deviceInfo.HwVersion = ptr.String(pdt.GetHwVersion())
	deviceInfo.SwVersion = ptr.String(pdt.GetSwVersion())
	*di = deviceInfo
}

type DeviceType string

type DeviceTypes []DeviceType

func (dtypes DeviceTypes) Contains(need DeviceType) bool {
	for _, item := range dtypes {
		if item == need {
			return true
		}
	}
	return false
}

type QuasarPlatform string

func (dt DeviceType) IconURL(format IconFormat) string {
	if dt == "" {
		return ""
	}
	url, exist := KnownDeviceIconsURL[dt]
	if !exist {
		url = KnownDeviceIconsURL[OtherDeviceType]
	}
	switch {
	case dt.IsSmartSpeakerOrModule() && dt != SmartSpeakerDeviceType:
		switch format {
		case RawIconFormat:
			return url
		case PNG40x40IconFormat:
			return fmt.Sprintf("%s/40x40", url)
		case PNG80x80IconFormat:
			return fmt.Sprintf("%s/80x80", url)
		case PNG120x120IconFormat:
			return fmt.Sprintf("%s/120x120", url)
		case OriginalIconFormat:
			return fmt.Sprintf("%s/orig", url)
		default:
			panic(fmt.Sprintf("unknown icon format %s in device type %s icon url", format, dt))
		}
	default:
		switch format {
		case RawIconFormat:
			return url
		case OriginalIconFormat:
			return fmt.Sprintf("%s/orig", url)
		default:
			panic(fmt.Sprintf("unknown icon format %s in device type %s icon url", format, dt))
		}
	}
}

func (dt DeviceType) GenerateDeviceName() string {
	v, ok := deviceTypeDefaultNameMap[dt]
	if !ok && !slices.Contains(KnownQuasarDeviceTypes, dt.String()) {
		return "Умное устройство"
	}
	return v
}

func (dt DeviceType) GenerateOnOffSuggestions(inflection inflector.Inflection, options SuggestionsOptions) []string {
	suggestions := make([]string, 0)

	switch deviceType := dt; deviceType {
	case OpenableDeviceType:
		suggestions = append(suggestions,
			"открой "+inflection.Vin, "закрой "+inflection.Vin)
	case CurtainDeviceType:
		if inflection.Vin != "шторы" {
			suggestions = append(suggestions,
				"открой шторы", "закрой шторы")
		}
		suggestions = append(suggestions,
			"открой "+inflection.Vin, "закрой "+inflection.Vin)
	case LightDeviceType:
		if inflection.Vin != "свет" {
			suggestions = append(suggestions,
				"включи свет", "выключи свет")
		}
		suggestions = append(suggestions,
			"включи "+inflection.Vin, "выключи "+inflection.Vin)
	case KettleDeviceType:
		suggestions = append(suggestions,
			"поставь "+inflection.Vin,
			"вскипяти "+inflection.Vin,
			"нагрей "+inflection.Vin,
			"подогрей "+inflection.Vin,
			"грей "+inflection.Vin,
			"согрей "+inflection.Vin)
	case CoffeeMakerDeviceType:
		suggestions = append(suggestions,
			"сделай кофе", "свари кофе",
		)
		if inflection.Vin != "кофеварку" {
			suggestions = append(suggestions,
				"включи кофеварку", "выключи кофеварку")
		}
		if inflection.Vin != "кофемашину" {
			suggestions = append(suggestions,
				"включи кофемашину", "выключи кофемашину")
		}
		suggestions = append(suggestions,
			"включи "+inflection.Vin, "выключи "+inflection.Vin,
		)
	case PurifierDeviceType:
		if inflection.Vin != "очиститель" {
			suggestions = append(suggestions,
				"включи очиститель", "выключи очиститель")
		}
		if inflection.Vin != "очиститель воздуха" {
			suggestions = append(suggestions,
				"включи очиститель воздуха", "выключи очиститель воздуха")
		}
		suggestions = append(suggestions,
			"включи "+inflection.Vin, "выключи "+inflection.Vin)
	case HumidifierDeviceType:
		if inflection.Vin != "увлажнитель" {
			suggestions = append(suggestions,
				"включи увлажнитель", "выключи увлажнитель")
		}
		if inflection.Vin != "увлажнитель воздуха" {
			suggestions = append(suggestions,
				"включи увлажнитель воздуха", "выключи увлажнитель воздуха")
		}
		suggestions = append(suggestions,
			"включи "+inflection.Vin, "выключи "+inflection.Vin)
	case VacuumCleanerDeviceType:
		suggestions = append(suggestions,
			"пропылесось", "верни пылесос на зарядку", "верни пылесос на базу",
		)
		if inflection.Vin != "пылесос" {
			suggestions = append(suggestions,
				"включи пылесос", "выключи пылесос")
		}
		if inflection.Vin != "робот-пылесос" {
			suggestions = append(suggestions,
				"включи робот-пылесос", "выключи робот-пылесос")
		}
		suggestions = append(suggestions,
			"включи "+inflection.Vin, "выключи "+inflection.Vin,
		)
	case WashingMachineDeviceType:
		if inflection.Vin != "стиральную машину" {
			suggestions = append(suggestions,
				"включи стиральную машину", "выключи стиральную машину")
		}
		if inflection.Vin != "стиральную машинку" {
			suggestions = append(suggestions,
				"запусти стиральную машинку", "выруби стиральную машинку")
		}
		if inflection.Vin != "стиралку" {
			suggestions = append(suggestions,
				"запусти стиралку", "выруби стиралку")
		}
		suggestions = append(suggestions,
			"включи "+inflection.Vin, "выключи "+inflection.Vin,
		)
	case DishwasherDeviceType:
		if inflection.Vin != "посудомоечную машину" {
			suggestions = append(suggestions,
				"включи посудомоечную машину", "выключи посудомоечную машину")
		}
		if inflection.Vin != "посудомойку" {
			suggestions = append(suggestions,
				"запусти посудомойку", "выруби посудомойку")
		}
		suggestions = append(suggestions,
			"включи "+inflection.Vin, "выключи "+inflection.Vin,
		)
	case MulticookerDeviceType:
		if inflection.Vin != "мультиварку" {
			suggestions = append(suggestions,
				"включи мультиварку", "выключи мультиварку")
		}
		suggestions = append(suggestions,
			"включи "+inflection.Vin, "выключи "+inflection.Vin,
		)
	case RefrigeratorDeviceType:
		if inflection.Vin != "холодильник" {
			suggestions = append(suggestions,
				"включи холодильник", "выключи холодильник")
		}
		suggestions = append(suggestions,
			"включи "+inflection.Vin, "выключи "+inflection.Vin,
		)
	case FanDeviceType:
		if inflection.Vin != "вентилятор" {
			suggestions = append(suggestions,
				"включи вентилятор", "выключи вентилятор")
		}
		suggestions = append(suggestions,
			"включи "+inflection.Vin, "выключи "+inflection.Vin,
		)
	case IronDeviceType: // iron can only be turned off
		if inflection.Vin != "утюг" {
			suggestions = append(suggestions,
				"выключи утюг")
		}
		suggestions = append(suggestions,
			"выключи "+inflection.Vin,
		)
	case PetFeederDeviceType: // you cannot unfeed the pet
		suggestions = append(suggestions,
			"покорми кота",
			"наполни миску",
			"насыпь корм коту",
			"пора кормить щенка",
		)
	default:
		suggestions = append(suggestions,
			"включи "+inflection.Vin, "выключи "+inflection.Vin)
	}
	for i := range suggestions {
		suggestions[i] = options.AddHouseholdToSuggest(suggestions[i])
	}
	return suggestions
}

func (dt DeviceType) Validate(_ *valid.ValidationCtx) (bool, error) {
	if len(dt) == 0 {
		return false, fmt.Errorf("type is empty")
	}
	if !tools.Contains(string(dt), KnownDeviceTypes) {
		return false, fmt.Errorf("unknown type: %s", dt)
	}
	return false, nil
}

func (dt DeviceType) IsSmartSpeaker() bool {
	return strings.Contains(dt.String(), SmartSpeakerDeviceType.String())
}

func (dt DeviceType) IsSmartSpeakerOrModule() bool {
	return dt.IsSmartSpeaker() || dt == YandexModuleDeviceType || dt == YandexModule2DeviceType
}

func (dt DeviceType) IsRemoteCarDeviceType() bool {
	return tools.Contains(string(dt), KnownRemoteCarDeviceTypes)
}

func (dt DeviceType) toProto() *protos.DeviceType {
	v, ok := deviceTypeToProtoMap[dt]
	if !ok {
		panic(fmt.Sprintf("unknown device_type: `%s`", dt))
	}
	return &v
}

func (dt *DeviceType) fromProto(pdt *protos.DeviceType) {
	v, ok := protoToDeviceTypeMap[*pdt]
	if !ok {
		panic(fmt.Sprintf("unknown device_type: `%s`", pdt.String()))
	}
	*dt = v
}

func (dt DeviceType) String() string {
	return string(dt)
}

func (dt DeviceType) IsMediaDeviceType() bool {
	return strings.Contains(dt.String(), MediaDeviceDeviceType.String())
}

func (dt DeviceType) GenerateBasicSuggestions(inflection inflector.Inflection, capabilities Capabilities, options SuggestionsOptions) []string {
	differentSuggestionsCount := 0

	suggestions := make([]string, 0)
	for _, capability := range capabilities {
		lenBefore := len(suggestions)
		suggestions = append(suggestions, capability.BasicSuggestions(dt, inflection, options)...)
		lenAfter := len(suggestions)
		if lenAfter > lenBefore {
			differentSuggestionsCount++
		}
	}

	if differentSuggestionsCount >= 3 {
		return suggestions
	}
	return []string{}
}

func (d *Device) RoomID() string {
	if d.Room != nil {
		return d.Room.ID
	}
	return ""
}

func (d Device) AvailableColors() []Color {
	colors := make([]Color, 0)
	capabilities := d.GetCapabilitiesByType(ColorSettingCapabilityType)
	for _, capability := range capabilities {
		colorSettingCapability := capability.(*ColorSettingCapability)
		colors = append(colors, colorSettingCapability.parameters.GetAvailableColors()...)
	}
	return colors
}

func (dt DeviceType) ToUserInfoProto() devicepb.EUserDeviceType {
	v, ok := mmDeviceTypeToProtoMap[dt]
	if !ok {
		return devicepb.EUserDeviceType_UnknownDeviceType
	}
	return v
}

func (dt *DeviceType) fromUserInfoProto(pdt devicepb.EUserDeviceType) {
	v, ok := mmProtoToDeviceTypeMap[pdt]
	if !ok {
		panic(fmt.Sprintf("unknown device_type: %q", pdt.String()))
	}
	*dt = v
}

func (dt DeviceType) GetDeviceDiscoveryMethods() []DiscoveryMethod {
	protocols := make([]DiscoveryMethod, 0)
	if ZigbeeSpeakers[dt] {
		protocols = append(protocols, ZigbeeDiscoveryMethod)
	}
	return protocols
}

type DeviceStatus string

func (s DeviceStatus) ToUserInfoProto() common.TIoTUserInfo_TDevice_EDeviceState {
	v, ok := mmDeviceStatusToProtoMap[s]
	if !ok {
		return common.TIoTUserInfo_TDevice_UnknownDeviceState
	}
	return v
}

func (s *DeviceStatus) fromUserInfoProto(p common.TIoTUserInfo_TDevice_EDeviceState) {
	v, ok := mmProtoToDeviceStatusMap[p]
	if !ok {
		panic(fmt.Sprintf("unknown device_state: %q", p.String()))
	}
	*s = v
}

func (d *Device) GetCapabilitiesByType(cType CapabilityType) []ICapability {
	var capabilities []ICapability
	for i, capability := range d.Capabilities {
		if capability.Type() == cType {
			capabilities = append(capabilities, d.Capabilities[i])
		}
	}
	return capabilities
}

func (d *Device) GetCapabilityByTypeAndInstance(cType CapabilityType, cInstance string) (ICapability, bool) {
	return d.Capabilities.GetCapabilityByTypeAndInstance(cType, cInstance)
}

func (d *Device) GetPropertyByTypeAndInstance(pType PropertyType, pInstance string) (IProperty, bool) {
	for i, property := range d.Properties {
		if property.Type() == pType && property.Instance() == pInstance {
			return d.Properties[i], true
		}
	}
	return nil, false
}

func (d *Device) GetCapabilitiesMap() map[string]ICapability {
	capMap := make(map[string]ICapability)
	for _, c := range d.Capabilities {
		capMap[c.Key()] = c
	}
	return capMap
}

func (d *Device) AssertName() error {
	if err := validateDeviceName(d.Name, d.IsQuasarDevice()); err != nil {
		return err
	}
	standardizeName := tools.StandardizeSpaces(strings.ToLower(d.Name))

	for _, alias := range d.Aliases {
		if standardizeName == strings.ToLower(alias) {
			return &DeviceAliasesNameAlreadyExistsError{}
		}
	}

	return nil
}

func (d *Device) AssertAliases() error {
	if len(d.Aliases) > DeviceNameAliasesLimit {
		return &DeviceAliasesLimitReachedError{}
	}

	for i := 0; i < len(d.Aliases); i++ {
		if err := validateDeviceName(d.Aliases[i], d.IsQuasarDevice()); err != nil {
			return err
		}
		standardizedAlias := tools.StandardizeSpaces(strings.ToLower(d.Aliases[i]))

		if standardizedAlias == tools.StandardizeSpaces(strings.ToLower(d.Name)) {
			return &DeviceAliasesNameAlreadyExistsError{}
		}

		for j := 0; j < len(d.Aliases); j++ {
			if i == j {
				continue
			}
			if standardizedAlias == tools.StandardizeSpaces(strings.ToLower(d.Aliases[j])) {
				return &DeviceAliasesNameAlreadyExistsError{}
			}
		}

	}

	return nil
}

func validateDeviceName(name string, isQuasarDevice bool) error {
	if isQuasarDevice {
		return validQuasarName(name, QuasarDeviceNameLength)
	}
	return validName(name, DeviceNameLength)
}

func (d *Device) IsQuasarDevice() bool {
	return d.SkillID == QUASAR
}

func (d *Device) IsVirtualQuasarDevice() bool {
	return d.SkillID == VIRTUAL && d.Type.IsSmartSpeakerOrModule()
}

func (d *Device) IsRemoteCar() bool {
	return d.Type.IsRemoteCarDeviceType()
}

func (d *Device) IsStereopairAvailable() bool {
	return d.IsQuasarDevice() && len(StereopairAvailablePairs[d.Type]) > 0
}

func (d *Device) IsTandemCompatibleWith(candidate Device) bool {
	return d.IsQuasarDevice() && candidate.IsQuasarDevice() && (tandemDisplayCompatibleSpeakerTypes[d.Type].Contains(candidate.Type) || tandemSpeakerCompatibleDisplayTypes[d.Type].Contains(candidate.Type))
}

func (d *Device) IsInTandem() bool {
	return d.InternalConfig.Tandem != nil
}

func (d *Device) IsTandemSpeaker() bool {
	_, exist := tandemSpeakerCompatibleDisplayTypes[d.Type]
	return exist
}

func (d *Device) IsTandemDisplay() bool {
	_, exist := tandemDisplayCompatibleSpeakerTypes[d.Type]
	return exist
}

func (d *Device) AssertSetup() error {
	if d.Room == nil {
		return &DeviceWithoutRoomError{}
	}

	if err := d.AssertName(); err != nil {
		return err
	}
	if err := d.Room.AssertName(); err != nil {
		return err
	}

	return nil
}

func (d *Device) QuasarCustomData() (*quasar.CustomData, error) {
	switch {
	case d.IsQuasarDevice():
		var quasarInfo quasar.CustomData
		if err := mapstructure.Decode(d.CustomData, &quasarInfo); err == nil {
			return &quasarInfo, nil
		} else {
			return nil, xerrors.New("failed to decode quasar custom data")
		}
	case d.IsVirtualQuasarDevice():
		quasarPlatform := UnknownQuasarPlatform
		if platform, exist := KnownQuasarPlatforms[d.Type]; exist {
			quasarPlatform = platform
		}
		return &quasar.CustomData{DeviceID: d.ExternalID, Platform: string(quasarPlatform)}, nil
	default:
		return nil, xerrors.New("not a quasar device")
	}
}

func (d *Device) GetExternalID() (string, error) {
	if d.IsQuasarDevice() {
		quasarInfo, err := d.QuasarCustomData()
		if err != nil {
			return "", err
		}
		return quasarInfo.DeviceID, nil
	}
	return d.ExternalID, nil
}

func (d *Device) PopulateInternals(device Device) {
	d.ID = device.ID
	d.Name = device.Name
	d.Description = device.Description
	d.ExternalID = device.ExternalID
	d.ExternalName = device.ExternalName
	d.SkillID = device.SkillID
	d.Type = device.Type
	d.OriginalType = device.OriginalType
	d.Room = device.Room
	d.Groups = device.Groups
	d.Properties = device.Properties
	d.DeviceInfo = device.DeviceInfo
	d.CustomData = device.CustomData
	d.Updated = device.Updated
	d.Favorite = device.Favorite
	d.InternalConfig = device.InternalConfig.Clone()
	d.SharingInfo = device.SharingInfo.Clone()
}

// PopulateAsStateContainer is used to set state and send it to provider
func (d *Device) PopulateAsStateContainer(userDevice Device, capabilityStates Capabilities) {
	d.PopulateInternals(userDevice)
	d.Capabilities = make(Capabilities, 0, len(capabilityStates))
	d.Capabilities = append(d.Capabilities, capabilityStates...)
	d.Capabilities.PopulateInternals(userDevice.Capabilities)
	// action processing only for device state container
	d.processCapabilitiesActions(userDevice.Capabilities)

	// Temporary weird fix cause Tuya dont store valid ac state at their side.
	// Unreverted from https://a.yandex-team.ru/review/829359/
	// someday someone will find this call and actually understand the meaning of it
	if userDevice.SkillID == TUYA && userDevice.Type == AcDeviceType {
		*d = TuyaACDeviceWeirdFix(*d, userDevice)
	}
}

// processCapabilitiesActions executes logic over capability state
// actualCapabilities - device capabilities with actual state
func (d *Device) processCapabilitiesActions(actualCapabilities Capabilities) {
	actualMap := actualCapabilities.AsMap()
	for i := range d.Capabilities {
		if actualCap, exist := actualMap[d.Capabilities[i].Key()]; exist {
			// https://st.yandex-team.ru/IOT-1506 - support invert state
			if actualCap.Type() == OnOffCapabilityType {
				onOffState := d.Capabilities[i].State().(OnOffCapabilityState)
				// invert capability during action
				if onOffState.Relative != nil && *onOffState.Relative {
					newState := onOffState.Clone().(OnOffCapabilityState)
					newState.Value = true // if actual state is nil - set value=true for relative
					newState.Relative = nil
					if actualState, ok := actualCap.State().(OnOffCapabilityState); ok {
						newState.Value = !actualState.Value
					}
					d.Capabilities[i].SetState(newState)
				}
			}
		}
	}

	// when "turn off" command is sent, other commands should not matter
	onOffCapability, ok := d.Capabilities.GetCapabilityByTypeAndInstance(OnOffCapabilityType, OnOnOffCapabilityInstance.String())
	if ok {
		if onOffState, ok := onOffCapability.State().(OnOffCapabilityState); ok && !onOffState.Value {
			d.Capabilities = Capabilities{onOffCapability}
		}
	}
}

type UpdatedCapabilitiesMap map[string]Capabilities

func (m UpdatedCapabilitiesMap) Latest() Capabilities {
	result := make(Capabilities, 0, len(m))
	for key := range m {
		capabilities := m[key]
		if len(capabilities) == 0 {
			continue
		}
		sort.SliceStable(capabilities, func(i, j int) bool {
			return capabilities[i].LastUpdated() >= capabilities[j].LastUpdated()
		})
		result = append(result, capabilities[0])
	}
	return result
}

type UpdatedPropertiesMap map[string]Properties

func (m UpdatedPropertiesMap) Latest() Properties {
	result := make(Properties, 0, len(m))
	for key := range m {
		properties := m[key]
		if len(properties) == 0 {
			continue
		}
		sort.SliceStable(properties, func(i, j int) bool {
			return properties[i].LastUpdated() >= properties[j].LastUpdated()
		})
		result = append(result, properties[0])
	}
	return result
}

func (m UpdatedPropertiesMap) Flatten() Properties {
	result := make(Properties, 0, len(m))
	for key := range m {
		result = append(result, m[key]...)
	}
	return result
}

type DeviceUpdatesSnapshot struct {
	UpdatedCapabilitiesMap UpdatedCapabilitiesMap
	UpdatedPropertiesMap   UpdatedPropertiesMap
}

func (s *DeviceUpdatesSnapshot) Latest() (Capabilities, Properties) {
	return s.UpdatedCapabilitiesMap.Latest(), s.UpdatedPropertiesMap.Latest()
}

func (d Device) GetUpdatedState(state Device) (Device, DeviceUpdatesSnapshot, PropertiesChangedStates) {
	updatedDevice := d.Clone()
	updatedCapabilitiesMap := updatedDevice.updateCapabilityStates(state.Capabilities)
	updatedPropertiesMap, changedProperties := updatedDevice.updatePropertyStates(state.Properties)
	return updatedDevice, DeviceUpdatesSnapshot{UpdatedCapabilitiesMap: updatedCapabilitiesMap, UpdatedPropertiesMap: updatedPropertiesMap}, changedProperties
}

func (d *Device) UpdateState(capabilities Capabilities, properties Properties) (Capabilities, Properties) {
	updatedCapabilities := d.updateCapabilityStates(capabilities)
	updatedProperties, _ := d.updatePropertyStates(properties)
	return updatedCapabilities.Latest(), updatedProperties.Latest()
}

func (d *Device) updateCapabilityStates(capabilities Capabilities) UpdatedCapabilitiesMap {
	sort.SliceStable(capabilities, func(i, j int) bool {
		return capabilities[i].LastUpdated() < capabilities[j].LastUpdated()
	})

	updatedCapabilities := make(UpdatedCapabilitiesMap, len(d.Capabilities))

	deviceCapabilitiesMap := d.Capabilities.AsMap()
	for _, capability := range capabilities {
		if capability.State() == nil {
			continue
		}
		deviceCapability, exists := deviceCapabilitiesMap[capability.Key()]
		if !exists || !deviceCapability.Retrievable() {
			continue
		}
		switch capability.Type() {
		case RangeCapabilityType: // IOT-303: specific logic: retrievable true range - do not save relative flag
			if state, ok := capability.State().(RangeCapabilityState); ok {
				if state.IsRelative() {
					continue
				} else {
					// just to be sure that we will not store relative false flag in db
					// FIXME: ALICE-5776, IOT-303: after assuring relative = false is not present in mobile_handlers.go/mobilePostUserDeviceActions
					state.Relative = nil
					capability.SetState(state)
				}
			}
		case OnOffCapabilityType: // IOT-1506 - relative is used for on/off toggle
			if state, ok := capability.State().(OnOffCapabilityState); ok {
				state.Relative = nil // ensure we don't save relative sign to device database
				capability.SetState(state)
			}
		}

		if deviceCapability.LastUpdated() < capability.LastUpdated() {
			deviceCapability.SetState(capability.State())
			deviceCapability.SetLastUpdated(capability.LastUpdated())
			updatedCapabilities[deviceCapability.Key()] = append(updatedCapabilities[deviceCapability.Key()], deviceCapability.Clone())
		}
	}
	for i := range d.Capabilities {
		if updatedCapability, found := deviceCapabilitiesMap[d.Capabilities[i].Key()]; found {
			d.Capabilities[i] = updatedCapability
		}
	}
	return updatedCapabilities
}

func (d *Device) updatePropertyStates(properties Properties) (UpdatedPropertiesMap, PropertiesChangedStates) {
	sort.SliceStable(properties, func(i, j int) bool {
		return properties[i].LastUpdated() < properties[j].LastUpdated()
	})

	updatedProperties := make(UpdatedPropertiesMap, len(d.Properties))
	changedPropertiesMap := make(map[string]PropertyChangedStates, len(d.Properties))

	devicePropertiesMap := d.Properties.Clone().AsMap()
	for _, property := range properties {
		if property.State() == nil {
			continue
		}
		deviceProperty, exists := devicePropertiesMap[property.Key()]
		if !exists {
			continue
		}
		if !canApplyNewPropertyStateUpdate(deviceProperty, property) {
			continue
		}
		changedStates, found := changedPropertiesMap[deviceProperty.Key()]
		if !found {
			changedStates = PropertyChangedStates{
				Previous: deviceProperty.State(),
				Current:  property.State(),
			}
			switch {
			case changedStates.Previous == nil:
				// on first new state StateChangedAt should be updated
				deviceProperty.SetStateChangedAt(property.LastUpdated())
			case !changedStates.Previous.Equals(changedStates.Current):
				// StateChangedAt should be updated to new state last update time
				deviceProperty.SetStateChangedAt(property.LastUpdated())
			default:
				// StateChangedAt is not updated on equal states
			}
		} else {
			// when we have 3 or more consecutive states, we should take into account situations like
			// 0 -> 17 -> 0, so we always compare the latest known change to incoming update
			if !changedStates.Current.Equals(property.State()) {
				deviceProperty.SetStateChangedAt(property.LastUpdated())
			}
		}
		changedStates.Current = property.State()
		changedPropertiesMap[deviceProperty.Key()] = changedStates

		deviceProperty.SetState(property.State())
		deviceProperty.SetLastUpdated(property.LastUpdated())
		updatedProperties[deviceProperty.Key()] = append(updatedProperties[deviceProperty.Key()], deviceProperty.Clone())
	}
	for i := range d.Properties {
		if updatedProperty, found := devicePropertiesMap[d.Properties[i].Key()]; found {
			d.Properties[i] = updatedProperty
		}
	}
	changedProperties := make(PropertiesChangedStates, 0, len(changedPropertiesMap))
	for _, states := range changedPropertiesMap {
		changedProperties = append(changedProperties, states)
	}
	return updatedProperties, changedProperties
}

// canApplyNewPropertyStateUpdate returns true if new property state can be applied over the old one
// the method checks lastUpdated timestamp monotonic grows, anti-flap logic, etc.
func canApplyNewPropertyStateUpdate(oldProperty IProperty, newProperty IProperty) bool {
	if oldProperty == nil {
		return true
	}

	if newProperty == nil {
		return false
	}

	if IsFlappingProperty(oldProperty, newProperty) {
		return false
	}

	return oldProperty.LastUpdated() < newProperty.LastUpdated()
}

// IsFlappingProperty check if new property is flapping and should not be applied to the state
func IsFlappingProperty(oldProperty IProperty, newProperty IProperty) bool {
	if oldProperty == nil || newProperty == nil {
		return false
	}

	// if state has the same value - use anti-flap for events delta for deduplication of same events
	if oldProperty.Type() == EventPropertyType &&
		oldProperty.State() != nil &&
		oldProperty.State().Equals(newProperty.State()) {
		diff := float64(oldProperty.LastUpdated() - newProperty.LastUpdated())
		return math.Abs(diff) < eventPropertyAntiFlapTimeout
	}

	return false
}

// TuyaACDeviceWeirdFix is a weird lawyard tuya ac fix
func TuyaACDeviceWeirdFix(newStateContainer Device, dbUserDevice Device) Device {
	device := dbUserDevice
	for _, capability := range newStateContainer.Capabilities {
		if capability.State() != nil {
			if deviceCapability, exists := device.GetCapabilityByTypeAndInstance(capability.Type(), capability.Instance()); exists && deviceCapability.Retrievable() {
				deviceCapability.SetState(capability.State())
			}
		}
	}
	device.FillNilStatesWithDefaults()
	if _, exists := newStateContainer.GetCapabilityByTypeAndInstance(OnOffCapabilityType, string(OnOnOffCapabilityInstance)); !exists {
		device.RemoveCapabilityByTypeAndInstance(OnOffCapabilityType, string(OnOnOffCapabilityInstance))
	}
	return device
}

//used to update states from scenario
func (d *Device) PopulateScenarioStates(sd ScenarioDevice) {
	for i := range d.Capabilities {
		state, ok := sd.GetStateByTypeAndInstance(d.Capabilities[i].Type(), d.Capabilities[i].Instance())
		if ok && state != nil {
			d.Capabilities[i].SetState(state)
		}
	}
}

// TODO: move to capability models with splitting to per capability functions
func (d *Device) FillNilStatesWithDefaults() {
	for i := range d.Capabilities {
		if d.Capabilities[i].State() == nil {
			d.Capabilities[i].SetState(d.Capabilities[i].DefaultState())
		}
	}
}

func (d *Device) RemoveCapabilityByTypeAndInstance(cType CapabilityType, cInstance string) {
	filteredCapabilities := make([]ICapability, 0, len(d.Capabilities))
	for i, c := range d.Capabilities {
		if c.Type() != cType && c.Instance() != cInstance {
			filteredCapabilities = append(filteredCapabilities, d.Capabilities[i])
		}
	}
	d.Capabilities = filteredCapabilities
}

func (d Device) Equals(other Device) bool {
	return cmp.Equal(d, other)
}

func (d Device) HasRetrievableState() bool {
	return d.Capabilities.HaveRetrievableState() || d.Properties.HaveRetrievableState()
}

func (d Device) HasReportableState() bool {
	return d.Capabilities.HaveReportableState() || d.Properties.HaveReportableState()
}

func (d Device) CanPersistState() bool {
	return d.HasRetrievableState() || d.HasReportableState()
}

func (d Device) Clone() Device {
	result, _ := copystructure.Copy(d)
	device := result.(Device)
	if len(d.Capabilities) != 0 {
		device.Capabilities = make([]ICapability, 0, len(d.Capabilities))
		for _, capability := range d.Capabilities {
			device.Capabilities = append(device.Capabilities, capability.Clone())
		}
	}
	if len(d.Properties) != 0 {
		device.Properties = make(Properties, 0, len(d.Properties))
		for _, property := range d.Properties {
			device.Properties = append(device.Properties, property.Clone())
		}
	}
	return device
}

func (d Device) ToProto() *protos.Device {
	pd := &protos.Device{
		Id:           d.ID,
		Name:         d.Name,
		ExternalId:   d.ExternalID,
		ExternalName: d.ExternalName,
		SkillId:      d.SkillID,
		Type:         *d.Type.toProto(),
		OriginalType: *d.OriginalType.toProto(),
		Updated:      float64(d.Updated),
		Sharing:      d.SharingInfo.ToProto(),
	}
	if d.Description != nil {
		pd.Description = *d.Description
	}
	if d.DeviceInfo != nil {
		pd.DeviceInfo = d.DeviceInfo.ToProto()
	}
	if d.Room != nil {
		pd.Room = d.Room.ToProto()
	}
	if d.CustomData != nil {
		bytes, _ := json.Marshal(d.CustomData)
		pd.CustomData = bytes
	}
	for _, g := range d.Groups {
		pd.Groups = append(pd.Groups, g.ToProto())
	}
	for _, c := range d.Capabilities {
		pd.Capabilities = append(pd.Capabilities, c.ToProto())
	}
	for _, p := range d.Properties {
		pd.Properties = append(pd.Properties, p.ToProto())
	}

	return pd
}

func (d *Device) FromProto(p *protos.Device) {
	d.ID = p.Id
	d.ExternalID = p.ExternalId
	d.Name = p.Name
	d.ExternalName = p.ExternalName
	d.SkillID = p.SkillId
	d.Updated = timestamp.PastTimestamp(p.Updated)

	var dType DeviceType
	dType.fromProto(&p.Type)
	d.Type = dType

	var odType DeviceType
	odType.fromProto(&p.OriginalType)
	d.OriginalType = odType

	if len(p.Description) > 0 {
		d.Description = &p.Description
	}

	if p.Room != nil {
		r := Room{}
		r.FromProto(p.Room)
		mr := r
		d.Room = &mr
	}

	for _, g := range p.Groups {
		gp := Group{}
		gp.FromProto(g)
		d.Groups = append(d.Groups, gp)
	}

	if p.DeviceInfo != nil {
		di := DeviceInfo{}
		di.FromProto(p.DeviceInfo)
		mdi := di
		d.DeviceInfo = &mdi
	}

	if p.CustomData != nil {
		var _ = json.Unmarshal(p.CustomData, &d.CustomData)
	}

	if p.Sharing != nil {
		d.SharingInfo = &SharingInfo{}
		d.SharingInfo.FromProto(p.Sharing)
	}

	for _, pc := range p.Capabilities {
		d.Capabilities = append(d.Capabilities, ProtoUnmarshalCapability(pc))
	}
	for _, pp := range p.Properties {
		d.Properties = append(d.Properties, ProtoUnmarshalProperty(pp))
	}
}

func (d *Device) IsAvailableForScenarios() bool {
	return len(d.Capabilities) > 0 && d.Type != YandexModuleDeviceType
}

func (d *Device) IsAvailableForTrigger() bool {
	return len(d.Properties) > 0 && !d.Type.IsSmartSpeakerOrModule() && !d.Type.IsRemoteCarDeviceType()
}

func (d *Device) GetModel() string {
	if d.DeviceInfo != nil && d.DeviceInfo.HasModel() {
		return *d.DeviceInfo.Model
	}
	return "unknown"
}

func (d *Device) GetManufacturer() string {
	if d.DeviceInfo != nil && d.DeviceInfo.HasManufacturer() {
		return *d.DeviceInfo.Manufacturer
	}
	return "unknown"
}

func (d *Device) IsShared() bool {
	return d.SharingInfo != nil
}

// ToScenarioLaunchDevice makes a ScenarioLaunchDevice from the device with its capabilities filtered by scenarioCapabilities.
// If scenarioCapabilities is nil, the ScenarioLaunchDevice will not have any capabilities.
func (d *Device) ToScenarioLaunchDevice(scenarioCapabilities ScenarioCapabilities) ScenarioLaunchDevice {
	capabilities := Capabilities(scenarioCapabilities.ToCapabilitiesStates())
	capabilities = capabilities.FilterByActualCapabilities(d.Capabilities)
	capabilities.PopulateInternals(d.Capabilities)
	return ScenarioLaunchDevice{
		ID:           d.ID,
		Name:         d.Name,
		Type:         d.Type,
		Capabilities: capabilities,
		CustomData:   d.CustomData,
		SkillID:      d.SkillID,
	}
}

func (d *Device) ToTimerScenarioLaunchDevice() ScenarioLaunchDevice {
	return ScenarioLaunchDevice{
		ID:           d.ID,
		Name:         d.Name,
		Type:         d.Type,
		Capabilities: d.Capabilities.Clone(),
		CustomData:   d.CustomData,
		SkillID:      d.SkillID,
	}
}

func (d *Device) ToUserInfoProto(ctx context.Context) *common.TIoTUserInfo_TDevice {
	protoDevice := &common.TIoTUserInfo_TDevice{
		Id:            d.ID,
		Name:          d.Name,
		Aliases:       d.Aliases,
		Type:          d.Type.ToUserInfoProto(),
		OriginalType:  d.OriginalType.ToUserInfoProto(),
		ExternalId:    d.ExternalID,
		ExternalName:  d.ExternalName,
		SkillId:       d.SkillID,
		RoomId:        d.RoomID(),
		GroupIds:      d.GroupsIDs(),
		Capabilities:  d.Capabilities.ToUserInfoProto(),
		Properties:    d.Properties.ToUserInfoProto(),
		IconURL:       d.Type.IconURL(RawIconFormat),
		Updated:       float64(d.Updated),
		Created:       float64(d.Created),
		HouseholdId:   d.HouseholdID,
		AnalyticsType: analyticsDeviceType(d.Type),
		AnalyticsName: analyticsDeviceTypeName(d.Type),
		Status:        d.Status.ToUserInfoProto(),
		StatusUpdated: float64(d.StatusUpdated),
		SharingInfo:   d.SharingInfo.ToUserInfoProto(),
	}
	if d.DeviceInfo != nil {
		protoDevice.DeviceInfo = d.DeviceInfo.ToUserInfoProto()
	}
	if d.CustomData != nil {
		rawCustomData, _ := json.Marshal(d.CustomData) // if we were smarter, we would be storing device.CustomData in json.RawMessage
		protoDevice.CustomData = rawCustomData
	}
	if d.IsQuasarDevice() || d.IsVirtualQuasarDevice() {
		quasarInfo, err := d.QuasarCustomData()
		if err == nil { // not sure that we should be quiet about this error tho
			protoDevice.QuasarInfo = &common.TIoTUserInfo_TDevice_TQuasarInfo{
				DeviceId: quasarInfo.DeviceID,
				Platform: quasarInfo.Platform,
			}
		}
	}
	if d.Type == YandexStationMidiDeviceType && experiments.MidiUserInfoColorSetting.IsEnabled(ctx) {
		// huge https://st.yandex-team.ru/IOT-1352
		protoDevice.Type = LightDeviceType.ToUserInfoProto()
		protoDevice.OriginalType = LightDeviceType.ToUserInfoProto()
		protoDevice.AnalyticsType = analyticsDeviceType(LightDeviceType)
		protoDevice.AnalyticsName = analyticsDeviceTypeName(LightDeviceType)

		colorScenes := make(ColorScenes, 0, len(KnownYandexmidiColorScenes))
		for _, colorSceneID := range KnownYandexmidiColorScenes {
			colorScenes = append(colorScenes, KnownColorScenes[ColorSceneID(colorSceneID)])
		}
		sort.Sort(ColorSceneSorting(colorScenes))
		midiColorSetting := MakeCapabilityByType(ColorSettingCapabilityType).
			WithParameters(ColorSettingCapabilityParameters{
				ColorSceneParameters: &ColorSceneParameters{
					Scenes: colorScenes,
				},
			})

		if len(d.GetCapabilitiesByType(OnOffCapabilityType)) == 0 {
			protoDevice.Capabilities = append(protoDevice.Capabilities, MakeCapabilityByType(OnOffCapabilityType).ToUserInfoProto())
		}

		if len(d.GetCapabilitiesByType(ColorSettingCapabilityType)) == 0 {
			protoDevice.Capabilities = append(protoDevice.Capabilities, midiColorSetting.ToUserInfoProto())
		}
	}
	return protoDevice
}

func (d *Device) FromUserInfoProtoSimple(p *common.TIoTUserInfo_TDevice) {
	d.ID = p.GetId()
	d.Name = p.GetName()
	d.Aliases = p.GetAliases()
	d.ExternalID = p.GetExternalId()
	d.ExternalName = p.GetExternalName()
	d.SkillID = p.GetSkillId()
	d.HouseholdID = p.GetHouseholdId()

	d.Type.fromUserInfoProto(p.GetType())
	d.OriginalType.fromUserInfoProto(p.GetOriginalType())

	if roomID := p.GetRoomId(); len(roomID) > 0 {
		d.Room = &Room{ID: p.GetRoomId()}
	}

	for _, id := range p.GetGroupIds() {
		d.Groups = append(d.Groups, Group{ID: id})
	}

	for _, pc := range p.GetCapabilities() {
		capability, _ := MakeCapabilityFromUserInfoProto(pc)
		d.Capabilities = append(d.Capabilities, capability)
	}

	for _, pp := range p.GetProperties() {
		property, _ := MakePropertyFromUserInfoProto(pp)
		d.Properties = append(d.Properties, property)
	}

	d.DeviceInfo = &DeviceInfo{}
	d.DeviceInfo.FromUserInfoProto(p.GetDeviceInfo())

	if customData := p.GetCustomData(); customData != nil {
		var _ = json.Unmarshal(customData, &d.CustomData)
	}

	if p.GetSharingInfo() != nil {
		var sharingInfo SharingInfo
		sharingInfo.fromUserInfoProto(p.GetSharingInfo())
		d.SharingInfo = &sharingInfo
	}

	d.Updated = timestamp.PastTimestamp(p.GetUpdated())
	d.Created = timestamp.PastTimestamp(p.GetCreated())
	d.Favorite = p.GetFavorite()
	d.Status.fromUserInfoProto(p.GetStatus())
	d.StatusUpdated = timestamp.PastTimestamp(p.GetStatusUpdated())
	d.InternalConfig.fromUserInfoProto(p.GetInternalConfig())
}

func (d *Device) ExternalKey() string {
	return fmt.Sprintf("%s:%s", d.SkillID, d.ExternalID)
}

type StoreResult string

var (
	StoreResultNew          StoreResult = "NEW"
	StoreResultUpdated      StoreResult = "UPDATED"
	StoreResultLimitReached StoreResult = "LIMIT_REACHED"
	StoreResultUnknownError StoreResult = "UNKNOWN_ERROR"
)

type DeviceStoreResult struct {
	Device
	Result StoreResult
}

func (dsr DeviceStoreResult) Clone() DeviceStoreResult {
	return DeviceStoreResult{
		Device: dsr.Device.Clone(),
		Result: dsr.Result,
	}
}

type DeviceStoreResults []DeviceStoreResult

func (dsrs DeviceStoreResults) Devices() Devices {
	devices := make(Devices, 0, len(dsrs))
	for _, device := range dsrs {
		devices = append(devices, device.Device)
	}
	return devices
}

func (dsrs DeviceStoreResults) ChooseNewOrUpdated() DeviceStoreResults {
	result := make(DeviceStoreResults, 0, len(dsrs))
	for _, r := range dsrs {
		if r.Result == StoreResultNew || r.Result == StoreResultUpdated {
			result = append(result, r)
		}
	}
	return result
}

func (dsrs DeviceStoreResults) IDs() []string {
	result := make([]string, 0, len(dsrs))
	for _, dsr := range dsrs {
		result = append(result, dsr.ID)
	}
	return result
}

func (dsrs DeviceStoreResults) ExternalIDs() []string {
	result := make([]string, 0, len(dsrs))
	for _, dsr := range dsrs {
		result = append(result, dsr.ExternalID)
	}
	return result
}

func (dsrs DeviceStoreResults) ToExternalIDMap() map[string]DeviceStoreResult {
	result := make(map[string]DeviceStoreResult)
	for _, dsr := range dsrs {
		result[dsr.ExternalID] = dsr.Clone()
	}
	return result
}

type Devices []Device

func (devices Devices) ExternalKeys() []string {
	result := make([]string, 0, len(devices))
	for _, device := range devices {
		result = append(result, device.ExternalKey())
	}
	return result
}

func (devices Devices) FilterByIDs(deviceIDs []string) Devices {
	filteredDevices := make(Devices, 0, len(devices))
	for _, device := range devices {
		if tools.Contains(device.ID, deviceIDs) {
			filteredDevices = append(filteredDevices, device)
		}
	}
	return filteredDevices
}

func (devices Devices) FilterByExternalIDs(externalIDs []string) Devices {
	result := make(Devices, 0, len(devices))
	for _, device := range devices {
		if tools.Contains(device.ExternalID, externalIDs) {
			result = append(result, device)
		}
	}
	return result
}

func (devices Devices) QuasarDevicesExternalIDs() []string {
	quasarIDs := make([]string, 0, len(devices))

	for _, device := range devices {
		quasarData, err := device.QuasarCustomData()
		if err != nil {
			continue
		}

		quasarIDs = append(quasarIDs, quasarData.DeviceID)
	}

	return quasarIDs
}

func (devices Devices) FilterByQuasarExternalIDs(quasarIDs ...string) Devices {
	quasarIDsMap := make(map[string]bool, len(quasarIDs))
	for _, id := range quasarIDs {
		quasarIDsMap[id] = true
	}

	result := make(Devices, 0, len(devices))
	for _, device := range devices {
		quasarData, err := device.QuasarCustomData()
		if err != nil {
			continue
		}

		if quasarIDsMap[quasarData.DeviceID] {
			result = append(result, device)
		}
	}

	return result
}

func (devices Devices) FilterByQuasarPlatforms(quasarPlatforms ...QuasarPlatform) Devices {
	quasarPlatformsMap := make(map[string]bool, len(quasarPlatforms))
	for _, platform := range quasarPlatforms {
		quasarPlatformsMap[string(platform)] = true
	}

	result := make(Devices, 0, len(devices))
	for _, device := range devices {
		quasarData, err := device.QuasarCustomData()
		if err != nil {
			continue
		}

		if quasarPlatformsMap[quasarData.Platform] {
			result = append(result, device)
		}
	}

	return result
}

func (devices Devices) FilterByHouseholdIDs(householdIDs []string) Devices {
	householdMap := make(map[string]bool, len(householdIDs))
	for _, householdID := range householdIDs {
		householdMap[householdID] = true
	}
	result := make(Devices, 0, len(devices))
	for _, device := range devices {
		if householdMap[device.HouseholdID] {
			result = append(result, device)
		}
	}
	return result
}

func (devices Devices) FilterByRoomIDs(roomIDs []string) Devices {
	filteredDevices := make(Devices, 0, len(devices))
	for _, device := range devices {
		if device.Room != nil && tools.Contains(device.RoomID(), roomIDs) {
			filteredDevices = append(filteredDevices, device)
		}
	}
	return filteredDevices
}

func (devices Devices) FilterByGroupIDs(groupIDs []string) Devices {
	filteredDevices := make(Devices, 0, len(devices))
	for _, device := range devices {
		for _, deviceGroupID := range device.GroupsIDs() {
			if tools.Contains(deviceGroupID, groupIDs) {
				filteredDevices = append(filteredDevices, device)
				break
			}
		}
	}
	return filteredDevices
}

func (devices Devices) FilterBySkillID(skillID string) Devices {
	filteredDevices := make(Devices, 0, len(devices))
	for _, device := range devices {
		if device.SkillID == skillID {
			filteredDevices = append(filteredDevices, device)
		}
	}
	return filteredDevices
}

func (devices Devices) FilterByFavorite(favorite bool) Devices {
	result := make(Devices, 0, len(devices))
	for _, scenario := range devices {
		if scenario.Favorite == favorite {
			result = append(result, scenario)
		}
	}
	return result
}

func (devices Devices) FilterByDeviceTypes(deviceTypes DeviceTypes) Devices {
	filteredDevices := make(Devices, 0, len(devices))
	for _, device := range devices {
		if deviceTypes.Contains(device.Type) {
			filteredDevices = append(filteredDevices, device)
		}
	}
	return filteredDevices
}

func (devices Devices) IsSameType() (DeviceType, bool) {
	if len(devices) == 0 {
		return "", true
	}
	var res DeviceType
	for i, d := range devices {
		if i == 0 {
			res = d.Type
			continue
		}
		if d.Type != res {
			return "", false
		}
	}
	return res, true
}

func (devices Devices) IsMultiroomSmartSpeakers() bool {
	for _, device := range devices {
		if !MultiroomSpeakers[device.Type] {
			return false
		}
	}
	return true
}

func (devices Devices) AggregatedDeviceType() DeviceType {
	if len(devices) == 0 {
		return ""
	}
	devicesType, isSameType := devices.IsSameType()
	isMultiroomSpeakers := devices.IsMultiroomSmartSpeakers()
	if isMultiroomSpeakers {
		return SmartSpeakerDeviceType
	}
	if isSameType {
		return devicesType
	}
	return ""
}

func (devices Devices) FilterByScenarioTriggerType(triggerType ScenarioTriggerType) Devices {
	if triggerType == TimerScenarioTriggerType {
		filteredDevices := make(Devices, 0, len(devices))
		for _, device := range devices {
			if device.IsQuasarDevice() || device.IsRemoteCar() {
				continue
			}
			filteredDevices = append(filteredDevices, device)
		}
		return filteredDevices
	}
	return devices
}

func (devices Devices) FilterByScenario(scenario Scenario) Devices {
	return devices.FilterByScenarioDevices(scenario.ScenarioSteps(devices).Devices().ToScenarioDevices())
}

func (devices Devices) FilterByScenarioDevices(scenarioDevices ScenarioDevices) Devices {
	var filteredDevices Devices

	for _, sd := range scenarioDevices {
		capabilitiesMap := make(map[string]struct{})
		for _, scenarioCapability := range sd.Capabilities {
			capability := MakeCapabilityByType(scenarioCapability.Type)
			capability.SetState(scenarioCapability.State)
			capabilitiesMap[capability.Key()] = struct{}{}
		}
		for _, ud := range devices {
			if sd.ID == ud.ID {
				updatedDevice := ud.Clone()

				// need to show only capabilities that present in scenario device
				capabilities := make(Capabilities, 0)
				for _, capability := range updatedDevice.Capabilities {
					if _, exist := capabilitiesMap[capability.Key()]; exist {
						capabilities = append(capabilities, capability)
					}
				}
				if len(capabilities) == 0 {
					continue
				}
				updatedDevice.Capabilities = capabilities
				updatedDevice.PopulateScenarioStates(sd)

				filteredDevices = append(filteredDevices, updatedDevice)
			}
		}
	}

	return filteredDevices
}

func (devices Devices) FilterByStereopairFollowers(stereopairs Stereopairs) Devices {
	result := make(Devices, 0, len(devices))
	for _, device := range devices {
		if stereopairs.GetDeviceRole(device.ID) == FollowerRole {
			continue
		}
		result = append(result, device.Clone())
	}
	return result
}

func (devices Devices) QuasarDevices() Devices {
	result := make(Devices, 0, len(devices))
	for _, device := range devices {
		if device.IsQuasarDevice() {
			result = append(result, device.Clone())
		}
	}
	return result
}

func (devices Devices) GetDeviceByScenarioDevice(sd ScenarioDevice) (Device, bool) {
	scenarioCapabilitiesMap := make(map[string]struct{})
	for _, scenarioCapability := range sd.Capabilities {
		capability := MakeCapabilityByType(scenarioCapability.Type)
		capability.SetState(scenarioCapability.State)
		scenarioCapabilitiesMap[capability.Key()] = struct{}{}
	}
	for _, ud := range devices {
		if sd.ID == ud.ID {
			for _, capability := range ud.Capabilities {
				if _, exist := scenarioCapabilitiesMap[capability.Key()]; exist {
					return ud, true
				}
			}
		}
	}
	return Device{}, false
}

func (devices Devices) GetDeviceByExtID(extID string) (Device, bool) {
	for _, device := range devices {
		if device.ExternalID == extID {
			return device, true
		}
	}

	return Device{}, false
}

func (devices Devices) GetDeviceByID(id string) (Device, bool) {
	for _, device := range devices {
		if device.ID == id {
			return device, true
		}
	}

	return Device{}, false
}

func (devices Devices) GetDeviceByQuasarExtID(extID string) (Device, bool) {
	for _, device := range devices {
		dExternalID, _ := device.GetExternalID()
		if dExternalID == extID {
			return device.Clone(), true
		}
	}

	return Device{}, false
}

func (devices Devices) GetRooms() Rooms {
	roomsMap := make(map[string]Room)
	// 1. Collect all rooms
	for _, device := range devices {
		if device.Room != nil {
			roomsMap[device.Room.ID] = *device.Room
		}
	}
	// 2. Populate them with Devices
	for _, device := range devices {
		if device.Room == nil {
			continue
		}
		room := roomsMap[device.Room.ID]
		if !slices.Contains(room.Devices, device.ID) {
			room.Devices = append(room.Devices, device.ID)
			roomsMap[room.ID] = room
		}
	}
	res := make(Rooms, 0, len(roomsMap))
	for _, room := range roomsMap {
		res = append(res, room)
	}
	return res
}

func (devices Devices) GetGroups() []Group {
	groupsMap := make(map[string]Group)
	for _, device := range devices {
		for _, group := range device.Groups {
			if _, ok := groupsMap[group.ID]; !ok {
				groupsMap[group.ID] = group.Clone()
			}
			resultGroup := groupsMap[group.ID]
			resultGroup.Devices = append(resultGroup.Devices, device.ID)
			groupsMap[group.ID] = resultGroup
		}
	}
	result := make([]Group, 0, len(groupsMap))
	for _, group := range groupsMap {
		group.Devices = yslices.Dedup(group.Devices)
		result = append(result, group)
	}
	return result
}

func (devices Devices) SharedDevices() Devices {
	result := make(Devices, 0, len(devices))
	for _, device := range devices {
		if device.IsShared() {
			result = append(result, device)
		}
	}
	return result
}

func (devices Devices) NonSharedDevices() Devices {
	result := make(Devices, 0, len(devices))
	for _, device := range devices {
		if !device.IsShared() {
			result = append(result, device)
		}
	}
	return result
}

// UpdateStatuses updates statuses in Devices slice and returns updated devices
func (devices Devices) UpdateStatuses(deviceStates DeviceStatusMap, updatedTimestamp timestamp.PastTimestamp) Devices {
	updatedDevices := make(Devices, 0, len(devices))
	for i := 0; i < len(devices); i++ {
		if state, ok := deviceStates[devices[i].ID]; ok {
			if devices[i].StatusUpdated >= updatedTimestamp {
				continue
			}
			devices[i].Status = state
			devices[i].StatusUpdated = updatedTimestamp
			updatedDevices = append(updatedDevices, devices[i])
		}
	}
	return updatedDevices
}

func (devices Devices) SetSharingInfo(sharingInfos SharingInfos) {
	sharingInfoMap := sharingInfos.ToHouseholdMap()
	for i := range devices {
		sharingInfo, ok := sharingInfoMap[devices[i].HouseholdID]
		if !ok {
			continue
		}
		devices[i].SharingInfo = &sharingInfo
		if devices[i].Room != nil {
			devices[i].Room.SharingInfo = &sharingInfo
		}
		for j := range devices[i].Groups {
			devices[i].Groups[j].SharingInfo = &sharingInfo
		}
	}
}

func (devices Devices) SetFavorite(favorite bool) {
	for i := range devices {
		devices[i].Favorite = favorite
	}
}

func (devices Devices) ToDevicesByOwnerIDMap(origin Origin) DevicesMapByOwnerID {
	result := make(DevicesMapByOwnerID)
	for _, device := range devices {
		if device.SharingInfo == nil {
			result[origin.User.ID] = append(result[origin.User.ID], device)
			continue
		}
		result[device.SharingInfo.OwnerID] = append(result[device.SharingInfo.OwnerID], device)
	}
	return result
}

type DevicesMapByID map[string]Device

func (devices Devices) ToMap() DevicesMapByID {
	devicesMap := make(DevicesMapByID)
	for _, device := range devices {
		devicesMap[device.ID] = device
	}
	return devicesMap
}

type DevicesMapByExternalID map[string]Device

func (devices Devices) ToExternalIDMap() DevicesMapByExternalID {
	devicesMap := make(DevicesMapByExternalID)
	for _, device := range devices {
		devicesMap[device.ExternalID] = device
	}
	return devicesMap
}

type QuasarCustomDataMapByID map[string]quasar.CustomData

func (devices Devices) ToQuasarCustomDataMap() QuasarCustomDataMapByID {
	res := make(QuasarCustomDataMapByID, len(devices))
	for _, device := range devices {
		quasarCustomData, err := device.QuasarCustomData()
		if err != nil {
			continue
		}
		res[device.ID] = *quasarCustomData
	}
	return res
}

func (dmbi DevicesMapByID) Flatten() Devices {
	devices := make(Devices, 0, len(dmbi))
	for _, device := range dmbi {
		devices = append(devices, device)
	}
	return devices
}

type ProviderDevicesMap map[string]Devices

func (devices Devices) GroupByProvider() ProviderDevicesMap {
	providerDevices := make(ProviderDevicesMap)
	for _, device := range devices {
		providerDevices[device.SkillID] = append(providerDevices[device.SkillID], device)
	}
	return providerDevices
}

func (devices Devices) ExternalIDs() []string {
	ids := make([]string, 0, len(devices))
	for _, device := range devices {
		ids = append(ids, device.ExternalID)
	}
	return ids
}

func (devices Devices) GetIDs() []string {
	deviceIDs := make([]string, 0, len(devices))
	for _, d := range devices {
		deviceIDs = append(deviceIDs, d.ID)
	}
	return deviceIDs
}

func (devices Devices) Names() []string {
	names := make([]string, 0, len(devices))
	for _, d := range devices {
		names = append(names, d.Name)
	}
	return names
}

func (devices Devices) ContainsSpeaker() bool {
	for _, device := range devices {
		if device.Type.IsSmartSpeaker() {
			return true
		}
	}
	return false
}

func (devices Devices) ContainsOnlySpeakerDevices() bool {
	for _, device := range devices {
		if !device.Type.IsSmartSpeakerOrModule() {
			return false
		}
	}
	return true
}

func (devices Devices) ToProto() []*protos.Device {
	result := make([]*protos.Device, 0, len(devices))
	for _, d := range devices {
		result = append(result, d.ToProto())
	}
	return result
}

func (devices Devices) ToUserInfoProto(ctx context.Context) []*common.TIoTUserInfo_TDevice {
	userInfoDevices := make([]*common.TIoTUserInfo_TDevice, 0, len(devices))
	for _, device := range devices {
		userInfoDevices = append(userInfoDevices, device.ToUserInfoProto(ctx))
	}
	return userInfoDevices
}

func (devices *Devices) FromUserInfoProtoSimple(p []*common.TIoTUserInfo_TDevice) {
	result := make(Devices, 0, len(p))
	for _, protoDevice := range p {
		var device Device
		device.FromUserInfoProtoSimple(protoDevice)
		result = append(result, device)
	}
	*devices = result
}

func (providerDevicesMap ProviderDevicesMap) Flatten() Devices {
	devices := make(Devices, 0)
	for _, providerDevices := range providerDevicesMap {
		devices = append(devices, providerDevices...)
	}
	return devices
}

type DeviceStatusMap map[string]DeviceStatus

func (deviceStatusMap DeviceStatusMap) FillNotSeenWithStatus(devices Devices, status DeviceStatus) DeviceStatusMap {
	for _, device := range devices {
		if _, found := deviceStatusMap[device.ID]; !found {
			deviceStatusMap[device.ID] = status
		}
	}
	return deviceStatusMap
}

type DevicesMapByHousehold map[string]Devices

func (devices Devices) GroupByHousehold() DevicesMapByHousehold {
	devicesPerHouseholds := make(DevicesMapByHousehold)
	for _, device := range devices {
		devicesPerHouseholds[device.HouseholdID] = append(devicesPerHouseholds[device.HouseholdID], device)
	}
	return devicesPerHouseholds
}

type DevicesMapByRoom map[string]Devices

func (devices Devices) GroupByRoom() DevicesMapByRoom {
	devicesPerRoom := make(DevicesMapByRoom)
	for _, device := range devices {
		devicesPerRoom[device.RoomID()] = append(devicesPerRoom[device.RoomID()], device)
	}
	return devicesPerRoom
}

type DevicesMapByGroupID map[string]Devices

func (devices Devices) GroupByGroupID() DevicesMapByGroupID {
	devicesPerGroup := make(DevicesMapByGroupID)
	for _, device := range devices {
		for _, group := range device.Groups {
			devicesPerGroup[group.ID] = append(devicesPerGroup[group.ID], device.Clone())
		}
	}
	return devicesPerGroup
}

type DevicesMapByType map[DeviceType]Devices

func (devices Devices) GroupByType() DevicesMapByType {
	devicesPerType := make(DevicesMapByType)
	for _, device := range devices {
		deviceType := device.Type
		devicesPerType[deviceType] = append(devicesPerType[deviceType], device.Clone())
	}
	return devicesPerType
}

type DevicesMapByQuasarExternalID map[string]Device

func (devices Devices) ToQuasarExternalIDMap() map[string]Device {
	result := make(DevicesMapByQuasarExternalID, len(devices))
	for _, device := range devices {
		quasarData, err := device.QuasarCustomData()
		if err != nil {
			continue
		}
		result[quasarData.DeviceID] = device
	}
	return result
}

func (devices Devices) ToTimerScenarioLaunchDevices() ScenarioLaunchDevices {
	launchDevices := make(ScenarioLaunchDevices, 0, len(devices))
	for _, device := range devices {
		launchDevices = append(launchDevices, device.ToTimerScenarioLaunchDevice())
	}
	return launchDevices
}

func (devices Devices) Clone() Devices {
	if devices == nil {
		return nil
	}
	result := make(Devices, 0, len(devices))
	for _, device := range devices {
		result = append(result, device.Clone())
	}
	return result
}

type QuasarDevice Device

// ChildDevices finds all yandexIO devices that belong to this quasar device
func (d QuasarDevice) ChildDevices(userDevices Devices) Devices {
	result := make(Devices, 0)
	quasarExternalID, err := (*Device)(&d).GetExternalID()
	if err != nil {
		return result
	}
	for _, userDevice := range userDevices.FilterBySkillID(YANDEXIO) {
		var yandexIOConfig yandexiocd.CustomData
		err := mapstructure.Decode(userDevice.CustomData, &yandexIOConfig)
		if err == nil && yandexIOConfig.ParentEndpointID == quasarExternalID {
			result = append(result, userDevice)
		}
	}
	sort.Sort(DevicesSorting(result))
	return result
}

type YandexIODevice Device

// GetParent finds parent quasar device of given yandexIO device
func (d YandexIODevice) GetParent(userDevices Devices) (Device, bool) {
	var yandexIOConfig yandexiocd.CustomData
	if err := mapstructure.Decode(d.CustomData, &yandexIOConfig); err != nil {
		return Device{}, false
	}
	if parentDevice, ok := userDevices.GetDeviceByQuasarExtID(yandexIOConfig.ParentEndpointID); ok {
		return parentDevice, true
	}
	return Device{}, false
}

type YandexIODevices Devices

// GetParentChildRelations groups receiver yandexIO devices by their parent speaker devices
//
// Returns
// - parentsMap, which maps parentDeviceID to parentDevice
// - parentRelationsMap, which maps parentDeviceID to its childDevices
func (devices YandexIODevices) GetParentChildRelations(userDevices Devices) (parentsMap map[string]Device, parentRelationsMap map[string]Devices) {
	parentsMap = userDevices.FilterBySkillID(QUASAR).ToMap()
	parentRelationsMap = make(map[string]Devices, len(devices))
	for _, childDevice := range Devices(devices).FilterBySkillID(YANDEXIO) {
		var yandexIOConfig yandexiocd.CustomData
		if err := mapstructure.Decode(childDevice.CustomData, &yandexIOConfig); err != nil {
			continue
		}
		parentDevice, ok := userDevices.GetDeviceByQuasarExtID(yandexIOConfig.ParentEndpointID)
		if ok {
			parentRelationsMap[parentDevice.ID] = append(parentRelationsMap[parentDevice.ID], childDevice)
		}
	}
	return parentsMap, parentRelationsMap
}

// GetChildParentRelations maps yandexIO deviceIDs to their parent speaker devices
func (devices YandexIODevices) GetChildParentRelations(userDevices Devices) (childrenMap map[string]Device, childRelationsMap map[string]Device) {
	childrenMap = Devices(devices).FilterBySkillID(YANDEXIO).ToMap()
	childRelationsMap = make(map[string]Device, len(devices))
	for _, childDevice := range childrenMap {
		var yandexIOConfig yandexiocd.CustomData
		if err := mapstructure.Decode(childDevice.CustomData, &yandexIOConfig); err != nil {
			continue
		}
		parentDevice, ok := userDevices.GetDeviceByQuasarExtID(yandexIOConfig.ParentEndpointID)
		if ok {
			childRelationsMap[childDevice.ID] = parentDevice
		}
	}
	return childrenMap, childRelationsMap
}

func (devices YandexIODevices) ParentToChildEndpointsMap() map[string]Devices {
	parentRelationsMap := make(map[string]Devices, len(devices))
	for _, childDevice := range Devices(devices).FilterBySkillID(YANDEXIO) {
		var yandexIOConfig yandexiocd.CustomData
		if err := mapstructure.Decode(childDevice.CustomData, &yandexIOConfig); err != nil {
			continue
		}
		parentRelationsMap[yandexIOConfig.ParentEndpointID] = append(parentRelationsMap[yandexIOConfig.ParentEndpointID], childDevice)
	}
	return parentRelationsMap
}

type DevicesMapByOwnerID map[uint64]Devices

func (m DevicesMapByOwnerID) Short() map[uint64][]string {
	result := make(map[uint64][]string)
	for k, v := range m {
		result[k] = v.GetIDs()
	}
	return result
}
