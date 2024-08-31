package api

import (
	"a.yandex-team.ru/alice/iot/bulbasaur/model"
	"a.yandex-team.ru/alice/library/go/timestamp"
	"a.yandex-team.ru/library/go/ptr"
)

type GroupStateResult struct {
	Status    string `json:"status"`
	RequestID string `json:"request_id"`
	GroupStateView
}

type GroupStateView struct {
	ID           string                     `json:"id"`
	Name         string                     `json:"name"`
	Aliases      []string                   `json:"aliases"`
	Type         model.DeviceType           `json:"type"`
	State        model.DeviceStatus         `json:"state"`
	Capabilities []GroupCapabilityStateView `json:"capabilities"`
	Devices      []ShortDeviceInfoView      `json:"devices"`
}

type ShortDeviceInfoView struct {
	ID   string           `json:"id"`
	Name string           `json:"name"`
	Type model.DeviceType `json:"type"`
}

func NewGroupStateView(group model.Group, groupDevices model.Devices) GroupStateView {
	gsv := GroupStateView{
		ID:           group.ID,
		Name:         group.Name,
		Aliases:      group.Aliases,
		Type:         group.Type,
		State:        model.GroupOnOffStateFromDevices(groupDevices),
		Capabilities: make([]GroupCapabilityStateView, 0),
	}
	for _, groupedCapability := range model.GroupCapabilitiesFromDevices(groupDevices) {
		if groupedCapability.IsInternal() {
			continue
		}
		gsv.Capabilities = append(gsv.Capabilities, GroupCapabilityStateView{
			Retrievable: groupedCapability.Retrievable(),
			Type:        groupedCapability.Type(),
			Parameters:  groupedCapability.Parameters(),
			State:       groupedCapability.State(),
		})
	}
	for _, device := range groupDevices {
		shortDeviceInfoView := ShortDeviceInfoView{
			ID:   device.ID,
			Name: device.Name,
			Type: device.Type,
		}
		gsv.Devices = append(gsv.Devices, shortDeviceInfoView)
	}
	return gsv
}

type DeviceStateResult struct {
	Status    string `json:"status"`
	RequestID string `json:"request_id"`
	DeviceStateView
}

type DeviceStateView struct {
	ID           string                `json:"id"`
	Name         string                `json:"name"`
	Aliases      []string              `json:"aliases"`
	Type         model.DeviceType      `json:"type"`
	ExternalID   string                `json:"external_id"`
	SkillID      string                `json:"skill_id"`
	State        model.DeviceStatus    `json:"state"`
	Groups       []string              `json:"groups"`
	Room         *string               `json:"room"`
	Capabilities []CapabilityStateView `json:"capabilities"`
	Properties   []PropertyStateView   `json:"properties"`
}

func NewDeviceStateView(device model.Device, state model.DeviceStatus) DeviceStateView {
	dsv := DeviceStateView{
		ID:           device.ID,
		Name:         device.Name,
		Aliases:      device.Aliases,
		Type:         device.Type,
		ExternalID:   device.ExternalID,
		SkillID:      device.SkillID,
		State:        state,
		Groups:       device.GroupsIDs(),
		Capabilities: make([]CapabilityStateView, 0, len(device.Capabilities)),
		Properties:   make([]PropertyStateView, 0, len(device.Properties)),
	}
	if device.Room != nil {
		dsv.Room = ptr.String(device.Room.ID)
	}
	for _, c := range device.Capabilities {
		if c.IsInternal() {
			continue // internal capabilities are not shown externally
		}
		capabilityStateView := CapabilityStateView{
			Retrievable: c.Retrievable(),
			Type:        c.Type(),
			Parameters:  c.Parameters(),
			State:       c.State(),
			LastUpdated: c.LastUpdated(),
		}
		dsv.Capabilities = append(dsv.Capabilities, capabilityStateView)
	}
	for _, p := range device.Properties {
		propertyStateView := PropertyStateView{
			Retrievable: p.Retrievable(),
			Type:        p.Type(),
			Parameters:  p.Parameters(),
			State:       p.State(),
			LastUpdated: p.LastUpdated(),
		}
		dsv.Properties = append(dsv.Properties, propertyStateView)
	}
	return dsv
}

type CapabilityStateView struct {
	Retrievable bool                        `json:"retrievable"`
	Type        model.CapabilityType        `json:"type"`
	Parameters  model.ICapabilityParameters `json:"parameters"`
	State       model.ICapabilityState      `json:"state"`
	LastUpdated timestamp.PastTimestamp     `json:"last_updated"`
}

type PropertyStateView struct {
	Retrievable bool                      `json:"retrievable"`
	Type        model.PropertyType        `json:"type"`
	Parameters  model.IPropertyParameters `json:"parameters"`
	State       model.IPropertyState      `json:"state"`
	LastUpdated timestamp.PastTimestamp   `json:"last_updated"`
}
