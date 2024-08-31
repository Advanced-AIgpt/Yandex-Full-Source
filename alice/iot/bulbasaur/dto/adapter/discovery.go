package adapter

import (
	"context"
	"encoding/json"
	"fmt"
	"reflect"

	"a.yandex-team.ru/alice/iot/bulbasaur/model"
	"a.yandex-team.ru/alice/library/go/timestamp"
	"a.yandex-team.ru/alice/library/go/tools"
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/log/ctxlog"
	"a.yandex-team.ru/library/go/core/xerrors"
	"a.yandex-team.ru/library/go/valid"
)

var _ valid.Validator = new(DiscoveryResult)
var _ valid.Validator = new(DiscoveryPayload)
var _ valid.Validator = new(DeviceInfoView)
var _ valid.Validator = new(CapabilityInfoView)
var _ valid.Validator = new(PropertyInfoView)

type DiscoveryResult struct {
	RequestID string                  `json:"request_id"`
	Timestamp timestamp.PastTimestamp `json:"ts"`
	Payload   DiscoveryPayload        `json:"payload"`
}

func (dr *DiscoveryResult) ToDevices(skillID string) []model.Device {
	devices := make([]model.Device, 0, len(dr.Payload.Devices))
	for _, deviceInfoView := range dr.Payload.Devices {
		device := deviceInfoView.ToDevice(skillID)
		device.Updated = dr.Timestamp
		devices = append(devices, device)
	}
	return devices
}

func (dr DiscoveryResult) Validate(vctx *valid.ValidationCtx) (bool, error) {
	var err valid.Errors
	if len(dr.RequestID) == 0 {
		err = append(err, fmt.Errorf("request_id is empty"))
	}

	if _, e := dr.Payload.Validate(vctx); e != nil {
		if ves, ok := e.(valid.Errors); ok {
			err = append(err, ves...)
		} else {
			err = append(err, e)
		}
	}

	if len(err) > 0 {
		return false, err
	}
	return false, nil
}

type DiscoveryPayload struct {
	UserID  string           `json:"user_id"`
	Devices []DeviceInfoView `json:"devices"`
}

func (dp DiscoveryPayload) Validate(_ *valid.ValidationCtx) (bool, error) {
	var err valid.Errors
	if len(dp.UserID) == 0 {
		err = append(err, fmt.Errorf("user_id is empty"))
	}

	idMap := make(map[string]bool)
	for _, device := range dp.Devices {
		if _, found := idMap[device.ID]; found {
			err = append(err, fmt.Errorf("device ID is not unique in discovery %s", device.ID))
		} else {
			idMap[device.ID] = true
		}
	}

	//do not validate devices here
	//we need to filter invalid ones further
	if len(err) > 0 {
		return false, err
	}
	return false, nil
}

func (dp *DiscoveryPayload) IsEmpty() bool {
	return dp == nil || len(dp.Devices) == 0
}

type DeviceInfoView struct {
	ID           string               `json:"id"`
	Name         string               `json:"name"`
	Description  string               `json:"description,omitempty"`
	HouseholdID  string               `json:"-"`
	Room         string               `json:"room,omitempty"`
	Capabilities []CapabilityInfoView `json:"capabilities"`
	Properties   []PropertyInfoView   `json:"properties"`
	Type         model.DeviceType     `json:"type"`
	DeviceInfo   *model.DeviceInfo    `json:"device_info,omitempty"`
	CustomData   interface{}          `json:"custom_data,omitempty"`
}

func NewDiscoveryInfoViewValidationContext(context context.Context, logger log.Logger, skillID string, isTrusted bool) *valid.ValidationCtx {
	vctx := valid.NewValidationCtx()
	vctx.Add("specific_capability_remote_car", func(value reflect.Value, _ string) error {
		if skillID == model.REMOTECAR {
			return nil
		}
		var errs valid.Errors
		if capability, ok := value.Interface().(CapabilityInfoView); !ok {
			return valid.ErrBadParams
		} else if capability.Type == model.ToggleCapabilityType && tools.Contains(capability.GetInstance(), model.KnownRemoteCarToggleInstances) {
			errs = append(errs, xerrors.Errorf("unsupported toggle capability instance: %s", capability.GetInstance()))
		}
		if errs != nil {
			return xerrors.Errorf("invalid capabilities: %w", errs)
		}
		return nil
	})
	vctx.Add("specific_capability_quasar", func(value reflect.Value, _ string) error {
		if skillID == model.QUASAR {
			return nil
		}
		var errs valid.Errors
		if capability, ok := value.Interface().(CapabilityInfoView); !ok {
			return valid.ErrBadParams
		} else if model.KnownQuasarCapabilityTypes.Contains(capability.Type) {
			errs = append(errs, xerrors.Errorf("unsupported capability type: %s", capability.Type))
		}
		if errs != nil {
			return xerrors.Errorf("invalid capabilities: %w", errs)
		}
		return nil
	})
	vctx.Add("specific_capability_tuya", func(value reflect.Value, _ string) error {
		if skillID == model.TUYA {
			return nil
		}
		var errs valid.Errors
		if capability, ok := value.Interface().(CapabilityInfoView); !ok {
			return valid.ErrBadParams
		} else if capability.Type == model.CustomButtonCapabilityType {
			errs = append(errs, xerrors.Errorf("unsupported capability type: %s", capability.Type))
		}
		if errs != nil {
			return xerrors.Errorf("invalid capabilities: %w", errs)
		}
		return nil
	})
	vctx.Add("specific_type", func(value reflect.Value, _ string) error {
		if value.Kind() != reflect.String {
			return valid.ErrBadParams
		}
		var errs valid.Errors
		deviceType := value.Interface().(model.DeviceType)
		if (skillID != model.QUASAR && deviceType.IsSmartSpeakerOrModule()) || (skillID != model.REMOTECAR && deviceType.IsRemoteCarDeviceType()) {
			errs = append(errs, xerrors.Errorf("unsupported device type: %s", deviceType))
		}
		if errs != nil {
			return xerrors.Errorf("invalid device type: %w", errs)
		}
		return nil
	})
	vctx.Add("trusted_device_info", func(value reflect.Value, param string) error {
		deviceInfo, ok := value.Interface().(*model.DeviceInfo)
		if !ok {
			return valid.ErrBadParams
		}
		var errs valid.Errors
		if isTrusted {
			if deviceInfo == nil {
				errs = append(errs, xerrors.New("device does not supply device info"))
			} else {
				if !deviceInfo.HasManufacturer() {
					errs = append(errs, xerrors.New("device does not supply manufacturer"))
				} else if *deviceInfo.Manufacturer == "unknown" {
					errs = append(errs, xerrors.Errorf("%q is not a valid manufacturer value", *deviceInfo.Manufacturer))
				}
				if !deviceInfo.HasModel() {
					errs = append(errs, xerrors.New("device does not supply model"))
				} else if *deviceInfo.Model == "unknown" || *deviceInfo.Model == "all" {
					errs = append(errs, xerrors.Errorf("%q is not a valid model value", *deviceInfo.Model))
				}
			}
		}
		if errs != nil {
			ctxlog.Warnf(context,
				log.With(logger, log.Any("skill_id", skillID)),
				"device validation failed, will skip it after 01.03.2020: %v", errs)
			//// don't skip devices from trusted providers for now
			//return xerrors.Errorf("invalid device info: %w", errs)
		}
		return nil
	})
	vctx.Add("has_capabilities_or_properties", func(value reflect.Value, param string) error {
		deviceInfoView, ok := value.Interface().(DeviceInfoView)
		if !ok {
			return valid.ErrBadParams
		}
		var errs valid.Errors
		totalCapabilitiesAndProperties := len(deviceInfoView.Capabilities) + len(deviceInfoView.Properties)
		isQuasar := skillID == model.QUASAR
		isTuyaHub := skillID == model.TUYA && deviceInfoView.Type == model.HubDeviceType
		canHaveZeroObjects := isQuasar || isTuyaHub
		if !canHaveZeroObjects && totalCapabilitiesAndProperties == 0 {
			errs = append(errs, xerrors.New("device must contain at least one property or capability"))
		}
		if errs != nil {
			return xerrors.Errorf("invalid device: %w", errs)
		}
		return nil
	})
	return vctx
}

func (d DeviceInfoView) Validate(vctx *valid.ValidationCtx) (bool, error) {
	var err valid.Errors

	//id
	if len(d.ID) == 0 {
		err = append(err, fmt.Errorf("device id is empty"))
	}
	if len(d.ID) > 1024 {
		err = append(err, fmt.Errorf("device id is too long, should be less than 1024"))
	}

	//name
	if len(d.Name) == 0 {
		err = append(err, fmt.Errorf("device name is empty"))
	}
	if len(d.Name) > 1024 {
		err = append(err, fmt.Errorf("device name is too long, should be less than 1024"))
	}

	//description
	if len(d.Description) > 1024 {
		err = append(err, fmt.Errorf("description is too long, should be less than 1024"))
	}

	//room
	if len(d.Room) > 1024 {
		err = append(err, fmt.Errorf("room is too long, should be less than 1024"))
	}

	//capabilities
	capabilityMap := make(map[string]bool)
	for _, c := range d.Capabilities {
		if _, e := c.Validate(vctx); e != nil {
			if ves, ok := e.(valid.Errors); ok {
				err = append(err, ves...)
			} else {
				err = append(err, e)
			}
			continue
		}
		if validFunc, ok := vctx.Get("specific_capability_remote_car"); ok {
			if validErr := validFunc(reflect.ValueOf(c), ""); validErr != nil {
				var verrs valid.Errors
				switch {
				case xerrors.As(validErr, &verrs):
					err = append(err, verrs...)
				default:
					err = append(err, validErr)
				}
			}
		}
		if validFunc, ok := vctx.Get("specific_capability_quasar"); ok {
			if validErr := validFunc(reflect.ValueOf(c), ""); validErr != nil {
				var verrs valid.Errors
				switch {
				case xerrors.As(validErr, &verrs):
					err = append(err, verrs...)
				default:
					err = append(err, validErr)
				}
			}
		}
		if validFunc, ok := vctx.Get("specific_capability_tuya"); ok {
			if validErr := validFunc(reflect.ValueOf(c), ""); validErr != nil {
				var verrs valid.Errors
				switch {
				case xerrors.As(validErr, &verrs):
					err = append(err, verrs...)
				default:
					err = append(err, validErr)
				}
			}
		}
		key := c.GetKey()
		if _, found := capabilityMap[key]; found {
			err = append(err, fmt.Errorf("duplicated capability found - %s", key))
		} else {
			capabilityMap[key] = true
		}
	}

	//properties
	propertyMap := make(map[string]bool)
	for _, p := range d.Properties {
		if _, e := p.Validate(vctx); e != nil {
			if ves, ok := e.(valid.Errors); ok {
				err = append(err, ves...)
			} else {
				err = append(err, e)
			}
			continue
		}
		key := p.GetKey()
		if _, found := propertyMap[key]; found {
			err = append(err, fmt.Errorf("duplicated property found - %s", key))
		} else {
			propertyMap[key] = true
		}
	}

	if validFunc, ok := vctx.Get("has_capabilities_or_properties"); ok {
		if validErr := validFunc(reflect.ValueOf(d), ""); validErr != nil {
			var verrs valid.Errors
			switch {
			case xerrors.As(validErr, &verrs):
				err = append(err, verrs...)
			default:
				err = append(err, validErr)
			}
		}
	}

	//type
	if _, e := d.Type.Validate(vctx); e != nil {
		err = append(err, e)
	}
	if validFunc, ok := vctx.Get("specific_type"); ok {
		if validErr := validFunc(reflect.ValueOf(d.Type), ""); validErr != nil {
			var verrs valid.Errors
			switch {
			case xerrors.As(validErr, &verrs):
				err = append(err, verrs...)
			default:
				err = append(err, validErr)
			}
		}
	}

	//device_info
	if d.DeviceInfo != nil {
		if _, e := d.DeviceInfo.Validate(vctx); e != nil {
			if ves, ok := e.(valid.Errors); ok {
				err = append(err, ves...)
			} else {
				err = append(err, e)
			}
		}
	}
	if validFunc, ok := vctx.Get("trusted_device_info"); ok {
		if validErr := validFunc(reflect.ValueOf(d.DeviceInfo), ""); validErr != nil {
			var verrs valid.Errors
			switch {
			case xerrors.As(validErr, &verrs):
				err = append(err, verrs...)
			default:
				err = append(err, validErr)
			}
		}
	}
	//custom_data
	if d.CustomData != nil {
		b, _ := json.Marshal(d.CustomData)
		if len(b) > 32*1024 {
			err = append(err, fmt.Errorf("custom_data cannot be more than 32 kbytes"))
		}
	}

	if len(err) == 0 {
		return false, nil
	}
	return false, err
}

func (d *DeviceInfoView) ToDevice(skillID string) model.Device {
	device := model.Device{}

	device.ExternalID = d.ID
	device.HouseholdID = d.HouseholdID
	device.Name = d.Name
	device.ExternalName = d.Name
	if d.Description != "" {
		device.Description = &d.Description
	}
	device.SkillID = skillID
	device.Type = d.Type
	device.OriginalType = d.Type
	device.DeviceInfo = d.DeviceInfo
	device.CustomData = d.CustomData

	if d.Room != "" {
		device.Room = &model.Room{Name: d.Room}
	}

	device.Capabilities = make([]model.ICapability, 0, len(d.Capabilities))
	for _, c := range d.Capabilities {
		device.Capabilities = append(device.Capabilities, c.ToCapability())
	}

	device.Properties = make([]model.IProperty, 0, len(d.Properties))
	for _, p := range d.Properties {
		device.Properties = append(device.Properties, p.ToProperty())
	}

	return device
}

type DeviceInfoViews []DeviceInfoView

func (deviceInfoViews DeviceInfoViews) GetIDs() []string {
	result := make([]string, 0, len(deviceInfoViews))
	for _, view := range deviceInfoViews {
		result = append(result, view.ID)
	}
	return result
}

type CapabilityInfoView struct {
	Reportable  bool                        `json:"reportable"`
	Retrievable bool                        `json:"retrievable"`
	Type        model.CapabilityType        `json:"type"`
	Parameters  model.ICapabilityParameters `json:"parameters,omitempty"`
	State       model.ICapabilityState      `json:"state,omitempty"`
}

func (c CapabilityInfoView) Validate(vctx *valid.ValidationCtx) (bool, error) {
	var err valid.Errors

	//type
	if _, e := c.Type.Validate(vctx); e != nil {
		err = append(err, e)
		return false, err // do not process further, as we dont know how to validate unknown types
	}

	//parameters
	if c.Type != model.OnOffCapabilityType {
		if c.Parameters == nil {
			err = append(err, fmt.Errorf("capability parameters cannot be empty for type: %s", c.Type))
		} else {
			if _, e := c.Parameters.Validate(vctx); e != nil {
				if ves, ok := e.(valid.Errors); ok {
					err = append(err, ves...)
				} else {
					err = append(err, e)
				}
			}
		}
	}

	if len(err) == 0 {
		return false, nil
	}
	return false, err
}

func (c *CapabilityInfoView) UnmarshalJSON(b []byte) (err error) {
	cRaw := struct {
		Reportable  bool
		Retrievable bool
		Type        model.CapabilityType
		Parameters  json.RawMessage
		State       json.RawMessage
	}{Retrievable: true}
	if err := json.Unmarshal(b, &cRaw); err != nil {
		return err
	}

	c.Reportable = cRaw.Reportable
	c.Retrievable = cRaw.Retrievable
	c.Type = cRaw.Type

	switch cRaw.Type {
	case model.OnOffCapabilityType:
		p := model.OnOffCapabilityParameters{}
		if cRaw.Parameters != nil {
			if err = json.Unmarshal(cRaw.Parameters, &p); err != nil {
				break
			}
		}
		c.Parameters = p

		if cRaw.State != nil {
			s := model.OnOffCapabilityState{}
			if err = json.Unmarshal(cRaw.State, &s); err != nil {
				break
			}
			c.State = s
		}

	case model.ColorSettingCapabilityType:
		if cRaw.Parameters == nil {
			err = xerrors.New("parameters for color setting capability type is missing")
			break
		}
		p := model.ColorSettingCapabilityParameters{}
		if err = json.Unmarshal(cRaw.Parameters, &p); err != nil {
			break
		}

		c.Parameters = p

		if cRaw.State != nil {
			s := model.ColorSettingCapabilityState{}
			if err = json.Unmarshal(cRaw.State, &s); err != nil {
				break
			}
			c.State = s
		}

	case model.ModeCapabilityType:
		if cRaw.Parameters == nil {
			err = xerrors.New("parameters for mode capability type is missing")
			break
		}
		p := model.ModeCapabilityParameters{}
		if err = json.Unmarshal(cRaw.Parameters, &p); err != nil {
			break
		}

		c.Parameters = p

		if cRaw.State != nil {
			s := model.ModeCapabilityState{}
			if err = json.Unmarshal(cRaw.State, &s); err != nil {
				break
			}
			c.State = s
		}

	case model.RangeCapabilityType:
		if cRaw.Parameters == nil {
			err = xerrors.New("parameters for range capability type is missing")
			break
		}
		p := model.RangeCapabilityParameters{}
		if err = json.Unmarshal(cRaw.Parameters, &p); err != nil {
			break
		}

		c.Parameters = p

		if cRaw.State != nil {
			s := model.RangeCapabilityState{}
			if err = json.Unmarshal(cRaw.State, &s); err != nil {
				break
			}
			c.State = s
		}

	case model.ToggleCapabilityType:
		if cRaw.Parameters == nil {
			err = xerrors.New("parameters for toggle capability type is missing")
			break
		}
		p := model.ToggleCapabilityParameters{}
		if err = json.Unmarshal(cRaw.Parameters, &p); err != nil {
			break
		}

		c.Parameters = p

		if cRaw.State != nil {
			s := model.ToggleCapabilityState{}
			if err = json.Unmarshal(cRaw.State, &s); err != nil {
				break
			}
			c.State = s
		}

	case model.CustomButtonCapabilityType:
		if cRaw.Parameters == nil {
			err = xerrors.New("parameters for custom button capability type is missing")
			break
		}
		p := model.CustomButtonCapabilityParameters{}
		if err = json.Unmarshal(cRaw.Parameters, &p); err != nil {
			break
		}

		c.Parameters = p

		if cRaw.State != nil {
			s := model.CustomButtonCapabilityState{}
			if err = json.Unmarshal(cRaw.State, &s); err != nil {
				break
			}
			c.State = s
		}

	case model.QuasarServerActionCapabilityType:
		p := model.QuasarServerActionCapabilityParameters{}
		if cRaw.Parameters != nil {
			if err = json.Unmarshal(cRaw.Parameters, &p); err != nil {
				break
			}
		}

		c.Parameters = p

		if cRaw.State != nil {
			s := model.QuasarServerActionCapabilityState{}
			if err = json.Unmarshal(cRaw.State, &s); err != nil {
				break
			}
			c.State = s
		}

	case model.QuasarCapabilityType:
		p := model.QuasarCapabilityParameters{}
		if cRaw.Parameters != nil {
			if err = json.Unmarshal(cRaw.Parameters, &p); err != nil {
				break
			}
		}

		c.Parameters = p

		if cRaw.State != nil {
			s := model.QuasarCapabilityState{}
			if err = json.Unmarshal(cRaw.State, &s); err != nil {
				break
			}
			c.State = s
		}

	case model.VideoStreamCapabilityType:
		if cRaw.Parameters == nil {
			err = xerrors.New("parameters for video stream capability type is missing")
			break
		}
		p := model.VideoStreamCapabilityParameters{}
		if err = json.Unmarshal(cRaw.Parameters, &p); err != nil {
			break
		}

		c.Parameters = p

		if cRaw.State != nil {
			s := model.VideoStreamCapabilityState{}
			if err = json.Unmarshal(cRaw.State, &s); err != nil {
				break
			}
			c.State = s
		}
	}

	if err != nil {
		return xerrors.Errorf("cannot parse capability: %w", err)
	}
	return nil
}

func (c *CapabilityInfoView) ToCapability() model.ICapability {
	result := model.MakeCapabilityByType(c.Type)
	result.SetReportable(c.Reportable)
	result.SetRetrievable(c.Retrievable)
	result.SetParameters(c.Parameters)
	if c.State != nil {
		if err := c.State.ValidateState(result); err == nil {
			result.SetState(c.State)
		}
	}
	return result
}

type parameterInstance interface {
	GetInstance() string
}

func (c *CapabilityInfoView) GetInstance() string {
	if params, ok := c.Parameters.(parameterInstance); ok {
		return params.GetInstance()
	}

	panic("capabilityInfoView has no known instance")
}

func (c CapabilityInfoView) GetKey() string {
	capability := model.MakeCapabilityByType(c.Type)
	if c.Parameters != nil {
		capability.SetParameters(c.Parameters)
	}
	return capability.Key()
}

type CapabilityInfoViews []CapabilityInfoView

func (capabilities CapabilityInfoViews) GetByTypeAndInstance(cType model.CapabilityType, cInstance string) (CapabilityInfoView, bool) {
	for _, capability := range capabilities {
		if capability.Type == cType && capability.GetInstance() == cInstance {
			return capability, true
		}
	}
	return CapabilityInfoView{}, false
}

type PropertyInfoView struct {
	Type        model.PropertyType        `json:"type"`
	Reportable  bool                      `json:"reportable"`
	Retrievable bool                      `json:"retrievable"`
	Parameters  model.IPropertyParameters `json:"parameters"`
	State       model.IPropertyState      `json:"state,omitempty"`
}

func (piv PropertyInfoView) GetKey() string {
	property := model.MakePropertyByType(piv.Type)
	if piv.Parameters != nil {
		property.SetParameters(piv.Parameters)
	}
	return property.Key()
}

func (piv PropertyInfoView) GetInstance() string {
	if piv.Parameters != nil {
		return piv.Parameters.GetInstance()
	} else if piv.State != nil {
		return piv.State.GetInstance()
	}
	return ""
}

func (piv *PropertyInfoView) ToProperty() model.IProperty {
	p := model.MakePropertyByType(piv.Type)
	p.SetReportable(piv.Reportable)
	p.SetRetrievable(piv.Retrievable)
	p.SetParameters(piv.Parameters)
	p.SetState(piv.State)
	return p
}

func (piv PropertyInfoView) Validate(ctx *valid.ValidationCtx) (bool, error) {
	var verrs valid.Errors

	if _, err := piv.Type.Validate(ctx); err != nil {
		verrs = append(verrs, err.(valid.Errors)...)
		return false, verrs
	}

	if piv.Parameters == nil {
		verrs = append(verrs, xerrors.New("property parameters cannot be empty"))
	} else {
		if err := valid.Struct(ctx, piv.Parameters); err != nil {
			if verr, ok := err.(valid.Errors); ok {
				verrs = append(verrs, verr...)
			} else {
				verrs = append(verrs, err)
			}
		}
	}

	if len(verrs) == 0 {
		return true, nil
	}
	return false, verrs
}

func (piv *PropertyInfoView) UnmarshalJSON(b []byte) (err error) {
	commonFields := struct {
		Type        model.PropertyType
		Retrievable bool
		Reportable  bool
	}{Retrievable: true}
	if err := json.Unmarshal(b, &commonFields); err != nil {
		return err
	}

	piv.Type = commonFields.Type
	piv.Retrievable = commonFields.Retrievable
	piv.Reportable = commonFields.Reportable

	switch commonFields.Type {
	case model.EventPropertyType:
		var specialFields struct {
			Parameters model.EventPropertyParameters
			State      *model.EventPropertyState
		}
		if err := json.Unmarshal(b, &specialFields); err != nil {
			return err
		}
		piv.Parameters = specialFields.Parameters
		if specialFields.State != nil {
			piv.State = *specialFields.State
		}
	case model.FloatPropertyType:
		var specialFields struct {
			Parameters model.FloatPropertyParameters
			State      *model.FloatPropertyState
		}
		if err := json.Unmarshal(b, &specialFields); err != nil {
			return err
		}
		piv.Parameters = specialFields.Parameters
		if specialFields.State != nil {
			piv.State = *specialFields.State
		}
	}

	return nil
}

type PropertyInfoViews []PropertyInfoView

func (properties PropertyInfoViews) GetByTypeAndInstance(pType model.PropertyType, pInstance string) (PropertyInfoView, bool) {
	for _, property := range properties {
		if property.Type == pType && property.GetInstance() == pInstance {
			return property, true
		}
	}
	return PropertyInfoView{}, false
}
