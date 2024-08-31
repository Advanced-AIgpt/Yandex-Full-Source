package model

import (
	"math/rand"
	"strconv"

	"a.yandex-team.ru/alice/library/go/timestamp"
)

func NewDevice(name string) *Device {
	externalID := uint64(rand.Uint32())<<32 + uint64(rand.Uint32())
	return &Device{
		Name:         name,
		Aliases:      make([]string, 0),
		ExternalName: name,
		SkillID:      VIRTUAL,
		ExternalID:   strconv.FormatUint(externalID, 10),
		OriginalType: OtherDeviceType,
		Capabilities: []ICapability{},
		Properties:   Properties{},
		Status:       UnknownDeviceStatus,
	}
}

func (d *Device) WithID(id string) *Device {
	d.ID = id
	return d
}

func (d *Device) WithDeviceType(deviceType DeviceType) *Device {
	d.Type = deviceType
	return d
}

func (d *Device) WithHouseholdID(householdID string) *Device {
	d.HouseholdID = householdID
	return d
}

func (d *Device) WithOriginalDeviceType(deviceType DeviceType) *Device {
	d.OriginalType = deviceType
	return d
}

func (d *Device) WithSkillID(skillID string) *Device {
	d.SkillID = skillID
	return d
}

func (d *Device) WithExternalID(externalID string) *Device {
	d.ExternalID = externalID
	return d
}

func (d *Device) WithDescription(description string) *Device {
	d.Description = &description
	return d
}

func (d *Device) WithGroups(groups ...Group) *Device {
	d.Groups = append(d.Groups, groups...)
	return d
}

func (d *Device) WithRoom(room Room) *Device {
	d.Room = &room
	return d
}

func (d *Device) WithCapabilities(capabilities ...ICapability) *Device {
	d.Capabilities = append(d.Capabilities, capabilities...)
	return d
}

func (d *Device) WithProperties(properties ...IProperty) *Device {
	d.Properties = append(d.Properties, properties...)
	return d
}

func (d *Device) WithCustomData(customData interface{}) *Device {
	d.CustomData = customData
	return d
}

func (d *Device) UpdatedAt(timestamp timestamp.PastTimestamp) *Device {
	d.Updated = timestamp
	return d
}

func (d *Device) WithAliases(aliases ...string) *Device {
	d.Aliases = append(d.Aliases, aliases...)
	return d
}

func (d *Device) WithUpdated(ts timestamp.PastTimestamp) *Device {
	d.Updated = ts
	return d
}

func (d *Device) WithCreated(ts timestamp.PastTimestamp) *Device {
	d.Created = ts
	return d
}

func (d *Device) WithStatus(status DeviceStatus) *Device {
	d.Status = status
	return d
}

func (d *Device) GroupsIDs() []string {
	groups := make([]string, 0, len(d.Groups))
	for _, group := range d.Groups {
		groups = append(groups, group.ID)
	}
	return groups
}
