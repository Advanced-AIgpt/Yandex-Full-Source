package quasarconfig

import (
	"fmt"

	"a.yandex-team.ru/alice/iot/bulbasaur/model"
	"a.yandex-team.ru/alice/library/go/libquasar"
)

type DeviceChannel struct {
	DeviceID string
	Channel  model.StereopairDeviceChannel
}

type DeviceConfig struct {
	Config  libquasar.Config
	Version string
}

func (dc *DeviceConfig) Clone() *DeviceConfig {
	if dc == nil {
		return nil
	}
	return &DeviceConfig{
		Config:  dc.Config.Clone(),
		Version: dc.Version,
	}
}

func (dc *DeviceConfig) FromVersionedConfig(config libquasar.IotDeviceInfoVersionedConfig) {
	dc.Version = config.Version
	dc.Config = config.Content.Clone()
}

type DeviceInfo struct {
	ID             string
	QuasarID       string
	QuasarPlatform string
	Config         DeviceConfig
	Tandem         *TandemDeviceInfo
}

func (di *DeviceInfo) FromIotDeviceInfo(deviceInfo libquasar.IotDeviceInfo, userDevices model.Devices) {
	di.QuasarID = deviceInfo.ID
	di.QuasarPlatform = deviceInfo.Platform
	currentDevice, _ := userDevices.GetDeviceByQuasarExtID(di.QuasarID)
	di.ID = currentDevice.ID
	di.Config.FromVersionedConfig(deviceInfo.Config)

	if deviceInfo.Group != nil {
		var tandem TandemDeviceInfo
		tandem.FromGroupInfo(di.QuasarID, *deviceInfo.Group, userDevices)
		di.Tandem = &tandem
	}
}

type DeviceInfos []DeviceInfo

func (deviceInfos DeviceInfos) ToMap() map[string]DeviceInfo {
	result := make(map[string]DeviceInfo)
	for _, deviceInfo := range deviceInfos {
		result[deviceInfo.ID] = deviceInfo
	}
	return result
}

func (deviceInfos DeviceInfos) TandemPartnerID(deviceID string) string {
	if tandemInfo := deviceInfos.TandemInfo(deviceID); tandemInfo != nil {
		return tandemInfo.Partner.ID
	}
	return ""
}

func (deviceInfos DeviceInfos) TandemInfo(deviceID string) *TandemDeviceInfo {
	for _, di := range deviceInfos {
		if di.ID != deviceID {
			continue
		}
		if di.Tandem != nil {
			return di.Tandem
		}
	}
	return nil
}

func (deviceInfos DeviceInfos) GetByDeviceID(deviceID string) *DeviceInfo {
	for _, di := range deviceInfos {
		if di.ID == deviceID {
			return &di
		}
	}
	return nil
}

func (deviceInfos DeviceInfos) EncodeVersions(stereopairs model.Stereopairs) {
	deviceInfoMap := deviceInfos.ToMap()
	for i := range deviceInfos {
		if stereopair, ok := stereopairs.GetByDeviceID(deviceInfos[i].ID); ok {
			stereopairDeviceInfos := make(DeviceInfos, 0, 2)
			for _, device := range stereopair.Devices {
				if deviceInfo, ok := deviceInfoMap[device.ID]; ok {
					stereopairDeviceInfos = append(stereopairDeviceInfos, deviceInfo)
				}
			}
			versions := make(libquasar.DeviceVersions, 0, 2)
			for _, stereopairDeviceInfo := range stereopairDeviceInfos {
				versions = append(versions, libquasar.DeviceVersion{
					DeviceID: stereopairDeviceInfo.QuasarID,
					Version:  stereopairDeviceInfo.Config.Version,
				})
			}
			deviceInfos[i].Config.Version = versions.JoinVersions()
		} else {
			deviceInfos[i].Config.Version = libquasar.DeviceVersions{
				libquasar.DeviceVersion{
					DeviceID: deviceInfos[i].QuasarID,
					Version:  deviceInfos[i].Config.Version,
				},
			}.JoinVersions()
		}
	}
}

type TandemDeviceInfo struct {
	GroupID uint64
	Partner model.Device
	Role    libquasar.GroupDeviceRole
}

func (tdi *TandemDeviceInfo) FromGroupInfo(currentDeviceQuasarID string, groupInfo libquasar.GroupInfo, userDevices model.Devices) {
	tdi.GroupID = groupInfo.ID
	for _, groupDevice := range groupInfo.Devices {
		if groupDevice.ID == currentDeviceQuasarID {
			tdi.Role = groupDevice.Role
		} else {
			tdi.Partner, _ = userDevices.GetDeviceByQuasarExtID(groupDevice.ID)
		}
	}
}

func newGroupCreateRequest(display model.Device, speaker model.Device) libquasar.GroupCreateRequest {
	defaultGroupConfig := make(libquasar.Config)
	// https://st.yandex-team.ru/QUASARUI-1825#60d20a17b0bb5e42ced96841
	var playToRole model.TandemRole
	switch {
	case display.Type == model.YandexModuleDeviceType && speaker.Type == model.YandexStationDeviceType:
		playToRole = model.LeaderTandemRole
	case display.Type == model.YandexModuleDeviceType:
		playToRole = model.FollowerTandemRole
	default:
		playToRole = model.LeaderTandemRole
	}
	defaultGroupConfig.SetDefaultTandemConfig(string(playToRole))

	displayQuasarCustomData, _ := display.QuasarCustomData()
	speakerQuasarCustomData, _ := speaker.QuasarCustomData()

	return libquasar.GroupCreateRequest{
		Name:   fmt.Sprintf("%s-%s", speakerQuasarCustomData.DeviceID, displayQuasarCustomData.DeviceID),
		Secret: fmt.Sprintf("%s-%s", speakerQuasarCustomData.DeviceID, displayQuasarCustomData.DeviceID),
		Config: defaultGroupConfig,
		Devices: libquasar.GroupDevices{
			{
				ID:       speakerQuasarCustomData.DeviceID,
				Platform: speakerQuasarCustomData.Platform,
				Role:     libquasar.LeaderGroupDeviceRole,
			},
			{
				ID:       displayQuasarCustomData.DeviceID,
				Platform: displayQuasarCustomData.Platform,
				Role:     libquasar.FollowerGroupDeviceRole,
			},
		},
	}
}

func newDeviceConfig(device model.Device, partner model.Device, tandemGroupID uint64) model.DeviceConfig {
	config := model.DeviceConfig{
		Tandem: &model.TandemDeviceConfig{
			Partner: model.TandemDeviceConfigPartner{
				ID: partner.ID,
			},
			Group: model.TandemGroup{
				ID: tandemGroupID,
			},
		},
	}
	if device.IsTandemSpeaker() {
		config.Tandem.Role = model.LeaderTandemRole
	} else {
		config.Tandem.Role = model.FollowerTandemRole
	}
	return config
}

func deleteTandemFromDeviceConfig(config model.DeviceConfig) model.DeviceConfig {
	result := config.Clone()
	result.Tandem = nil
	return result
}
