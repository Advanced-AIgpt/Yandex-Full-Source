package main

import (
	"encoding/json"
	"fmt"
	"strconv"

	"a.yandex-team.ru/alice/iot/bulbasaur/dto/adapter"
	"a.yandex-team.ru/alice/iot/bulbasaur/dto/mobile"
	"a.yandex-team.ru/alice/iot/bulbasaur/model"
	"a.yandex-team.ru/alice/library/go/timestamp"
	"a.yandex-team.ru/alice/library/go/tools"
	"a.yandex-team.ru/kikimr/public/sdk/go/ydb"
	"a.yandex-team.ru/library/go/core/xerrors"
)

type deviceConfig struct {
	UserID     string `json:"account_id"`
	DeviceID   string `json:"device_id"`
	JSONConfig string `json:"json_config"`
}

func (dc deviceConfig) UserDeviceKey() userDeviceKey {
	return userDeviceKey{
		userID:   dc.UserID,
		deviceID: dc.DeviceID,
	}
}

type userDeviceKey struct {
	userID, deviceID string
}

type device struct {
	DeviceID   string     `json:"device_id"`
	PlatformID platformID `json:"platform_id"`
	UserID     string     `json:"account_id"`
}

func (d device) UserDeviceKey() userDeviceKey {
	return userDeviceKey{
		userID:   d.UserID,
		deviceID: d.DeviceID,
	}
}

func (d device) ToDiscoveryInfoView(deviceConfigs map[userDeviceKey]deviceConfig) adapter.DeviceInfoView {
	config := deviceConfigs[d.UserDeviceKey()]
	return adapter.DeviceInfoView{
		ID:           d.GetIOTDeviceID(),
		Name:         d.GetDeviceName(config),
		Capabilities: []adapter.CapabilityInfoView{},
		Type:         d.GetDeviceType(),
		CustomData: mobile.QuasarInfo{
			DeviceID: d.DeviceID,
			Platform: d.GetPlatform(),
		},
	}
}

func (d device) GetDeviceType() model.DeviceType {
	return speakerDeviceTypes.GetDeviceType(d.PlatformID)
}

func (d device) GetIOTDeviceID() string {
	return fmt.Sprintf("%s.%s", d.DeviceID, d.GetPlatform())
}

func (d device) GetPlatform() string {
	return platform[d.PlatformID]
}

func (d device) GetDeviceName(dc deviceConfig) string {
	var config map[string]interface{}
	if err := json.Unmarshal([]byte(dc.JSONConfig), &config); err != nil {
		return defaultNames.GetName(d.PlatformID)
	}
	name, isString := config["name"].(string)
	if !isString {
		return defaultNames.GetName(d.PlatformID)
	}
	if len(name) == 0 {
		return defaultNames.GetName(d.PlatformID)
	}
	return name
}

type quasarUser struct {
	ID, Login string
}

func (u quasarUser) GetID() uint64 {
	ID, _ := strconv.Atoi(u.ID)
	return uint64(ID)
}

func (u quasarUser) ToYDBValue(iotUser IotUser) (ydb.Value, error) {
	var created ydb.Value
	if iotUser.Created > 0 {
		created = ydb.TimestampValue(iotUser.Created.YdbTimestamp())
	} else {
		created = ydb.TimestampValue(timestamp.Now().YdbTimestamp())
	}

	value := ydb.StructValue(
		ydb.StructFieldValue("hid", ydb.Uint64Value(tools.Huidify(u.GetID()))),
		ydb.StructFieldValue("id", ydb.Uint64Value(u.GetID())),
		ydb.StructFieldValue("login", ydb.StringValue([]byte(u.Login))),
		ydb.StructFieldValue("created", created),
	)
	return value, nil
}

type quasarUserDevice struct {
	user   quasarUser
	device model.Device
}

func (userDevice quasarUserDevice) ToYDBValue(iotDevice IotDevice) (ydb.Value, error) {
	user, device := userDevice.user, userDevice.device

	//capabilities
	var capabilitiesB []byte
	var capabilities model.Capabilities
	if len(iotDevice.capabilities) > 0 {
		capabilities = iotDevice.capabilities
	} else {
		capabilities = device.Capabilities
	}
	capabilitiesB, err := json.Marshal(capabilities)
	if err != nil {
		err := xerrors.Errorf("cannot marshal capabilities: %w", err)
		return nil, err
	}

	//properties
	var propertiesB []byte
	var properties model.Properties
	if len(iotDevice.properties) > 0 {
		properties = iotDevice.properties
	} else {
		properties = device.Properties
	}
	propertiesB, err = json.Marshal(properties)
	if err != nil {
		err := xerrors.Errorf("cannot marshal properties: %w", err)
		return nil, err
	}

	//device_info
	var deviceInfoB []byte
	var deviceInfoI interface{}
	if iotDevice.deviceInfo != nil {
		deviceInfoI = iotDevice.deviceInfo
	} else {
		deviceInfoI = device.DeviceInfo
	}
	deviceInfoB, err = json.Marshal(deviceInfoI)
	if err != nil {
		err := xerrors.Errorf("cannot marshal device_info: %w", err)
		return nil, err
	}

	//custom_data
	var customDataB []byte
	var customDataI interface{}
	if iotDevice.customData != nil {
		customDataI = iotDevice.customData
	} else {
		customDataI = device.CustomData
	}
	customDataB, err = json.Marshal(customDataI)
	if err != nil {
		err := xerrors.Errorf("cannot marshal custom_data: %w", err)
		return nil, err
	}

	//room
	var roomID ydb.Value
	if len(iotDevice.roomID) > 0 {
		roomID = ydb.OptionalValue(ydb.StringValue([]byte(iotDevice.roomID)))
	} else {
		roomID = ydb.NullValue(ydb.TypeString)
	}

	//created
	var created ydb.Value
	if iotDevice.created > 0 {
		created = ydb.TimestampValue(iotDevice.created.YdbTimestamp())
	} else {
		created = ydb.TimestampValue(timestamp.Now().YdbTimestamp())
	}

	//updated
	var updated ydb.Value
	if iotDevice.updated > 0 {
		updated = ydb.TimestampValue(iotDevice.updated.YdbTimestamp())
	} else {
		updated = ydb.TimestampValue(timestamp.Now().YdbTimestamp())
	}

	value := ydb.StructValue(
		ydb.StructFieldValue("id", ydb.StringValue([]byte(iotDevice.ID))),
		ydb.StructFieldValue("huid", ydb.Uint64Value(tools.Huidify(user.GetID()))),
		ydb.StructFieldValue("user_id", ydb.Uint64Value(user.GetID())),
		ydb.StructFieldValue("name", ydb.StringValue([]byte(device.Name))),
		ydb.StructFieldValue("external_id", ydb.StringValue([]byte(device.ExternalID))),
		ydb.StructFieldValue("external_name", ydb.StringValue([]byte(device.ExternalName))),
		ydb.StructFieldValue("skill_id", ydb.StringValue([]byte(device.SkillID))),
		ydb.StructFieldValue("type", ydb.StringValue([]byte(device.Type))),
		ydb.StructFieldValue("original_type", ydb.StringValue([]byte(device.OriginalType))),
		ydb.StructFieldValue("room_id", roomID),
		ydb.StructFieldValue("capabilities", ydb.JSONValue(string(capabilitiesB))),
		ydb.StructFieldValue("properties", ydb.JSONValue(string(propertiesB))),
		ydb.StructFieldValue("custom_data", ydb.JSONValue(string(customDataB))),
		ydb.StructFieldValue("device_info", ydb.JSONValue(string(deviceInfoB))),
		ydb.StructFieldValue("created", created),
		ydb.StructFieldValue("updated", updated),
	)
	return value, nil
}
