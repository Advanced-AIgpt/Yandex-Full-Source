package api

import (
	"github.com/mitchellh/mapstructure"

	"a.yandex-team.ru/alice/iot/bulbasaur/model"
	"a.yandex-team.ru/alice/iot/bulbasaur/model/quasar"
	"a.yandex-team.ru/alice/iot/bulbasaur/model/tuya"
	"a.yandex-team.ru/library/go/ptr"
)

type UserInfoResult struct {
	Status    string `json:"status"`
	RequestID string `json:"request_id"`
	UserInfoView
}

type UserInfoView struct {
	Rooms       []RoomInfoView       `json:"rooms"`
	Groups      []GroupInfoView      `json:"groups"`
	Devices     []DeviceInfoView     `json:"devices"`
	Scenarios   []ScenarioInfoView   `json:"scenarios"`
	Households  []HouseholdInfoView  `json:"households"`
	Stereopairs []StereopairInfoView `json:"stereopairs,omitempty"`
}

func NewUserInfoView(userInfo model.UserInfo) UserInfoView {
	uiv := UserInfoView{
		Rooms:       make([]RoomInfoView, 0),
		Groups:      make([]GroupInfoView, 0),
		Devices:     make([]DeviceInfoView, 0),
		Scenarios:   make([]ScenarioInfoView, 0),
		Households:  make([]HouseholdInfoView, 0),
		Stereopairs: make([]StereopairInfoView, 0),
	}
	for _, room := range userInfo.Rooms {
		uiv.Rooms = append(uiv.Rooms, RoomInfoView{
			ID:          room.ID,
			Name:        room.Name,
			HouseholdID: room.HouseholdID,
			Devices:     room.Devices,
		})
	}
	deviceMapByID := make(map[string]model.Device)
	for _, device := range userInfo.Devices {
		if device.SkillID == model.TUYA {
			var cd tuya.CustomData
			if err := mapstructure.Decode(device.CustomData, &cd); err != nil {
				continue
			}
			if irData := cd.InfraredData; irData != nil && irData.Learned {
				continue // learned devices with custom buttons are also skipped
			}
		}

		div := DeviceInfoView{
			ID:           device.ID,
			Name:         device.Name,
			ExternalID:   device.ExternalID,
			SkillID:      device.SkillID,
			HouseholdID:  device.HouseholdID,
			Aliases:      device.Aliases,
			Type:         device.Type,
			Groups:       device.GroupsIDs(),
			Capabilities: make(model.Capabilities, 0, len(device.Capabilities)),
			Properties:   make(model.Properties, 0, len(device.Properties)),
		}
		if device.Room != nil {
			div.Room = ptr.String(device.Room.ID)
		}
		for _, capability := range device.Capabilities {
			if capability.IsInternal() {
				continue // internal capabilities are not shown in api
			}
			div.Capabilities = append(div.Capabilities, capability)
		}
		div.Properties = append(div.Properties, device.Properties...)
		if device.SkillID == model.QUASAR { // haha classic
			var customData quasar.CustomData
			if err := mapstructure.Decode(device.CustomData, &customData); err == nil {
				div.QuasarInfo = &QuasarInfoView{
					CustomData: customData,
				}
			}
		}
		deviceMapByID[device.ID] = device
		uiv.Devices = append(uiv.Devices, div)
	}
	for _, group := range userInfo.Groups {
		groupDevices := make(model.Devices, 0, len(group.Devices))
		groupDeviceIDs := make([]string, 0, len(group.Devices))
		for _, deviceID := range group.Devices {
			device, ok := deviceMapByID[deviceID]
			if !ok {
				continue
			}
			groupDevices = append(groupDevices, device)
			groupDeviceIDs = append(groupDeviceIDs, device.ID)
		}
		if len(groupDeviceIDs) == 0 {
			continue
		}
		groupCapabilities := model.GroupCapabilitiesFromDevices(groupDevices)
		groupCapabilitiesView := make([]GroupCapabilityStateView, 0, len(groupCapabilities))
		for _, groupCapability := range groupCapabilities {
			if groupCapability.IsInternal() {
				continue
			}
			groupCapabilitiesView = append(groupCapabilitiesView, GroupCapabilityStateView{
				Retrievable: groupCapability.Retrievable(),
				Type:        groupCapability.Type(),
				Parameters:  groupCapability.Parameters(),
				State:       groupCapability.State(),
			})
		}
		uiv.Groups = append(uiv.Groups, GroupInfoView{
			ID:           group.ID,
			Name:         group.Name,
			Aliases:      group.Aliases,
			HouseholdID:  group.HouseholdID,
			Type:         group.Type,
			Devices:      groupDeviceIDs,
			Capabilities: groupCapabilitiesView,
		})
	}
	for _, scenario := range userInfo.Scenarios {
		if !scenario.IsExecutable(userInfo.Devices) {
			continue // we skip devices without device actions
		}
		uiv.Scenarios = append(uiv.Scenarios, ScenarioInfoView{
			ID:       scenario.ID,
			Name:     scenario.Name.String(),
			IsActive: scenario.IsActive,
		})
	}
	for _, household := range userInfo.Households {
		uiv.Households = append(uiv.Households, HouseholdInfoView{
			ID:   household.ID,
			Name: household.Name,
		})
	}
	for _, stereopair := range userInfo.Stereopairs {
		spv := StereopairInfoView{
			ID:   stereopair.ID,
			Name: stereopair.Name,
		}
		for _, stereopairDevice := range stereopair.Config.Devices {
			spv.Devices = append(spv.Devices, StereopairDeviceView{
				ID:      stereopairDevice.ID,
				Role:    stereopairDevice.Role,
				Channel: stereopairDevice.Channel,
			})
		}
		uiv.Stereopairs = append(uiv.Stereopairs, spv)
	}
	return uiv
}

type DeviceInfoView struct {
	ID           string             `json:"id"`
	Name         string             `json:"name"`
	Aliases      []string           `json:"aliases"`
	Type         model.DeviceType   `json:"type"`
	ExternalID   string             `json:"external_id"`
	SkillID      string             `json:"skill_id"`
	HouseholdID  string             `json:"household_id"`
	Room         *string            `json:"room"`
	Groups       []string           `json:"groups"`
	Capabilities model.Capabilities `json:"capabilities"`
	Properties   model.Properties   `json:"properties"`
	QuasarInfo   *QuasarInfoView    `json:"quasar_info,omitempty"`
}

type QuasarInfoView struct {
	quasar.CustomData
}

type GroupInfoView struct {
	ID           string                     `json:"id"`
	Name         string                     `json:"name"`
	Aliases      []string                   `json:"aliases"`
	HouseholdID  string                     `json:"household_id"`
	Type         model.DeviceType           `json:"type"`
	Devices      []string                   `json:"devices"`
	Capabilities []GroupCapabilityStateView `json:"capabilities"`
}

type GroupCapabilityStateView struct {
	Retrievable bool                        `json:"retrievable"`
	Type        model.CapabilityType        `json:"type"`
	Parameters  model.ICapabilityParameters `json:"parameters"`
	State       model.ICapabilityState      `json:"state"`
}

type RoomInfoView struct {
	ID          string   `json:"id"`
	Name        string   `json:"name"`
	HouseholdID string   `json:"household_id"`
	Devices     []string `json:"devices"`
}

type ScenarioInfoView struct {
	ID       string `json:"id"`
	Name     string `json:"name"`
	IsActive bool   `json:"is_active"`
}

type HouseholdInfoView struct {
	ID   string `json:"id"`
	Name string `json:"name"`
}

type StereopairInfoView struct {
	ID      string                 `json:"id"`
	Name    string                 `json:"name"`
	Devices []StereopairDeviceView `json:"devices"`
}
type StereopairDeviceView struct {
	ID      string                        `json:"id"`
	Role    model.StereopairDeviceRole    `json:"role"`
	Channel model.StereopairDeviceChannel `json:"channel"`
}
