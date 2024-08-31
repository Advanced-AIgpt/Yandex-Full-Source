package callback

import (
	"regexp"

	"golang.org/x/exp/slices"

	"a.yandex-team.ru/alice/iot/bulbasaur/controller/deferredevents"
	"a.yandex-team.ru/alice/iot/bulbasaur/dto/adapter"
	"a.yandex-team.ru/alice/iot/bulbasaur/model"
	"a.yandex-team.ru/alice/library/go/timestamp"
	"a.yandex-team.ru/library/go/core/xerrors"
	"a.yandex-team.ru/library/go/valid"
)

type UpdateStateRequest struct {
	Timestamp timestamp.PastTimestamp `json:"ts"`
	Payload   *UpdateStatePayload     `json:"payload" valid:"notNil"`
}

// SkillUpdateStateRequest wraps UpdateStateRequest
type SkillUpdateStateRequest struct {
	SkillID  string             `json:"skill_id"`
	Callback UpdateStateRequest `json:"callback"`
}

type UpdateStatePayload struct {
	UserID       string            `json:"user_id" valid:"required"`
	DeviceStates []DeviceStateView `json:"devices" valid:"greater=0"`
}

var usrRenaming = []struct {
	re      *regexp.Regexp
	replace string
}{
	{
		regexp.MustCompile(`^\.Timestamp$`),
		"ts",
	},
	{
		regexp.MustCompile(`^\.Payload$`),
		"payload",
	},
	{
		regexp.MustCompile(`^Payload\.UserID$`),
		"payload.user_id",
	},
	{
		regexp.MustCompile(`^Payload\.DeviceStates$`),
		"payload.devices",
	},
	{
		regexp.MustCompile(`^Payload\.DeviceStates\.(\d+)$`),
		"payload.devices.$1",
	},
	{
		regexp.MustCompile(`^Payload\.DeviceStates\.(\d+)\.ID$`),
		"payload.devices.$1.id",
	},
	{
		regexp.MustCompile(`^Payload\.DeviceStates\.(\d+)\.Capabilities$`),
		"payload.devices.$1.capabilities",
	},
	{
		regexp.MustCompile(`^Payload\.DeviceStates\.(\d+)\.Capabilities\.(\d+)$`),
		"payload.devices.$1.capabilities.$2",
	},
	{
		regexp.MustCompile(`^Payload\.DeviceStates\.(\d+)\.Capabilities\.(\d+)\.Type$`),
		"payload.devices.$1.capabilities.$2.type",
	},
	{
		regexp.MustCompile(`^Payload\.DeviceStates\.(\d+)\.Capabilities\.(\d+)\.State$`),
		"payload.devices.$1.capabilities.$2.state",
	},
	{
		regexp.MustCompile(`^Payload\.DeviceStates\.(\d+)\.Capabilities\.(\d+)\.State\.Instance$`),
		"payload.devices.$1.capabilities.$2.state.instance",
	},
	{
		regexp.MustCompile(`^Payload\.DeviceStates\.(\d+)\.Capabilities\.(\d+)\.State\.Value$`),
		"payload.devices.$1.capabilities.$2.state.value",
	},
	{
		regexp.MustCompile(`^Payload\.DeviceStates\.(\d+)\.Properties$`),
		"payload.devices.$1.properties",
	},
	{
		regexp.MustCompile(`^Payload\.DeviceStates\.(\d+)\.Properties\.(\d+)$`),
		"payload.devices.$1.properties.$2",
	},
	{
		regexp.MustCompile(`^Payload\.DeviceStates\.(\d+)\.Properties\.(\d+)\.Type$`),
		"payload.devices.$1.properties.$2.type",
	},
	{
		regexp.MustCompile(`^Payload\.DeviceStates\.(\d+)\.Properties\.(\d+)\.State$`),
		"payload.devices.$1.properties.$2.state",
	},
	{
		regexp.MustCompile(`^Payload\.DeviceStates\.(\d+)\.Properties\.(\d+)\.State\.Instance$`),
		"payload.devices.$1.properties.$2.state.instance",
	},
	{
		regexp.MustCompile(`^Payload\.DeviceStates\.(\d+)\.Properties\.(\d+)\.State\.Value$`),
		"payload.devices.$1.properties.$2.state.value",
	},
}

func (usr *UpdateStateRequest) MapFieldNameToJSONAddress(fieldName string) string {
	for _, rule := range usrRenaming {
		if res := rule.re.ReplaceAllString(fieldName, rule.replace); res != fieldName {
			return res
		}
	}
	return ""
}

var _ valid.Validator = new(DeviceStateView)

type DeviceStateView struct {
	ID           string                       `json:"id"`
	Status       model.DeviceStatus           `json:"status"`
	Capabilities adapter.CapabilityStateViews `json:"capabilities,omitempty"`
	Properties   adapter.PropertyStateViews   `json:"properties,omitempty"`
}

func (dsv *DeviceStateView) ToDevice() model.Device {
	adapterDSV := adapter.DeviceStateView{
		ID:           dsv.ID,
		Capabilities: dsv.Capabilities,
		Properties:   dsv.Properties,
	}
	return adapterDSV.ToDevice()
}

func (dsv *DeviceStateView) ToDeviceChangedProperties() deferredevents.DeviceUpdatedProperties {
	properties := make(model.Properties, 0, len(dsv.Properties))
	for _, adapterProperty := range dsv.Properties {
		properties = append(properties, adapterProperty.ToProperty())
	}
	return deferredevents.DeviceUpdatedProperties{
		ID:         dsv.ID,
		Properties: properties,
	}
}

func (dsv DeviceStateView) Validate(ctx *valid.ValidationCtx) (bool, error) {
	param := struct {
		ID string `valid:"required"`
	}{
		ID: dsv.ID,
	}

	if err := valid.Struct(ctx, param); err != nil {
		return false, err
	}

	if len(dsv.Capabilities) == 0 && len(dsv.Properties) == 0 {
		return false, xerrors.New("either capabilities or properties should be provided")
	}
	return true, nil
}

func (dsv *DeviceStateView) ValidateDSV(userDevice model.Device) error {
	adapterDSV := adapter.DeviceStateView{
		ID:           dsv.ID,
		Capabilities: dsv.Capabilities,
		Properties:   dsv.Properties,
	}
	if err := adapterDSV.ValidateDSV(userDevice); err != nil {
		return err
	}
	if dsv.Status != "" {
		if !slices.Contains(model.KnownDeviceStatuses, string(dsv.Status)) {
			return xerrors.Errorf("unknown device status: %q", dsv.Status)
		}
	}
	return nil
}

func (dsv *DeviceStateView) UpdateCapabilityTimestampIfEmpty(updated timestamp.PastTimestamp) {
	for i := range dsv.Capabilities {
		if dsv.Capabilities[i].Timestamp == 0 {
			dsv.Capabilities[i].Timestamp = updated
		}
	}
	for i := range dsv.Properties {
		if dsv.Properties[i].Timestamp == 0 {
			dsv.Properties[i].Timestamp = updated
		}
	}
}

func (dsv *DeviceStateView) FilterOutStaleStates(device model.Device) {
	deviceCapabilitiesMap := device.Capabilities.AsMap()
	dsvCapabilitiesMap := dsv.Capabilities.AsMap()
	for capabilityKey, capabilityStates := range dsvCapabilitiesMap {
		capability, ok := deviceCapabilitiesMap[capabilityKey]
		if !ok {
			continue
		}
		dsvCapabilitiesMap[capabilityKey] = capabilityStates.FilterOutStaleStates(capability)
	}
	dsv.Capabilities = dsvCapabilitiesMap.Flatten()

	devicePropertiesMap := device.Properties.AsMap()
	dsvPropertiesMap := dsv.Properties.AsMap()
	for propertyKey, propertyStates := range dsvPropertiesMap {
		property, ok := devicePropertiesMap[propertyKey]
		if !ok {
			continue
		}
		dsvPropertiesMap[propertyKey] = propertyStates.FilterOutStaleStates(property)
	}
	dsv.Properties = dsvPropertiesMap.Flatten()
}

func (dsv *DeviceStateView) IsEmpty() bool {
	return len(dsv.Capabilities) == 0 && len(dsv.Properties) == 0
}
