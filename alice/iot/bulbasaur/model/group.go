package model

import (
	"a.yandex-team.ru/alice/iot/bulbasaur/protos"
	"a.yandex-team.ru/alice/protos/data/location"
)

type Group struct {
	ID          string       `json:"id"`
	Name        string       `json:"name"`
	Aliases     []string     `json:"aliases"`
	Type        DeviceType   `json:"type"`
	Devices     []string     `json:"devices,omitempty"`
	HouseholdID string       `json:"household_id"`
	Favorite    bool         `json:"favorite"`
	SharingInfo *SharingInfo `json:"sharing_info,omitempty"`
}

//means there is no devices in group
func (g *Group) IsEmpty() bool {
	return len(g.Devices) == 0
}

func (g *Group) AssertName() error {
	return validName(g.Name, GroupNameLength)
}

func (g *Group) AssertAliases() error {
	if len(g.Aliases) > GroupNameAliasesLimit {
		return &GroupAliasesLimitReachedError{}
	}

	for _, alias := range g.Aliases {
		if err := validName(alias, GroupNameLength); err != nil {
			return err
		}
	}
	return nil
}

func (g Group) CompatibleWithDeviceType(dt DeviceType) bool {
	if g.Type == "" {
		if dt.IsSmartSpeaker() && !MultiroomSpeakers[dt] {
			return false
		}
		return true
	}
	if g.Type == SmartSpeakerDeviceType {
		return MultiroomSpeakers[dt]
	}
	return g.Type == dt
}

func (g Group) CompatibleWith(d Device) bool {
	return g.CompatibleWithDeviceType(d.Type)
}

func (g Group) CompatibleWithDevices(devices Devices) bool {
	if len(devices) == 0 {
		return true
	}
	devicesType, devicesIsSameType := devices.IsSameType()
	isMultiroomSpeakers := devices.IsMultiroomSmartSpeakers()
	switch {
	case !devicesIsSameType && !isMultiroomSpeakers:
		return false
	case isMultiroomSpeakers && g.Type != "" && g.Type != SmartSpeakerDeviceType:
		return false
	case devicesIsSameType && g.Type != "" && g.Type != devicesType:
		return false
	default:
		return true
	}
}

func (g Group) Clone() Group {
	copiedAliases := make([]string, 0, len(g.Aliases))
	copiedAliases = append(copiedAliases, g.Aliases...)
	copiedDevices := make([]string, 0, len(g.Devices))
	copiedDevices = append(copiedDevices, g.Devices...)
	return Group{
		ID:          g.ID,
		Name:        g.Name,
		Aliases:     copiedAliases,
		Type:        g.Type,
		Devices:     copiedDevices,
		HouseholdID: g.HouseholdID,
		Favorite:    g.Favorite,
		SharingInfo: g.SharingInfo.Clone(),
	}
}

func (g Group) ToProto() *protos.Group {
	p := &protos.Group{
		Name:    g.Name,
		Id:      g.ID,
		Devices: g.Devices,
	}

	//group type can be empty
	if len(g.Type) > 0 {
		p.Type = *g.Type.toProto()
	}

	return p
}

func (g *Group) FromProto(pg *protos.Group) {
	g.Name = pg.Name
	g.ID = pg.Id
	g.Devices = append(g.Devices, pg.Devices...)

	if len(pg.Type.String()) > 0 {
		var gType DeviceType
		gType.fromProto(&pg.Type)
		g.Type = gType
	}
}

func (g *Group) ToUserInfoProto() *location.TUserGroup {
	return &location.TUserGroup{
		Id:            g.ID,
		Name:          g.Name,
		Type:          g.Type.ToUserInfoProto(),
		Aliases:       g.Aliases,
		HouseholdId:   g.HouseholdID,
		SharingInfo:   g.SharingInfo.ToUserInfoProto(),
		AnalyticsType: analyticsDeviceType(g.Type),
		AnalyticsName: analyticsGroupTypeName(g.Type),
	}
}

type Groups []Group

func (gs Groups) ToProto() []*protos.Group {
	result := make([]*protos.Group, 0, len(gs))
	for _, g := range gs {
		result = append(result, g.ToProto())
	}
	return result
}

func (gs Groups) GetGroupByID(id string) (Group, bool) {
	for _, g := range gs {
		if g.ID == id {
			return g, true
		}
	}
	return Group{}, false
}

func (gs Groups) ToMap() map[string]Group {
	result := make(map[string]Group)
	for _, g := range gs {
		result[g.ID] = g
	}
	return result
}

func (gs Groups) GroupByHousehold() map[string]Groups {
	groupsPerHousehold := make(map[string]Groups)
	for _, group := range gs {
		groupsPerHousehold[group.HouseholdID] = append(groupsPerHousehold[group.HouseholdID], group)
	}
	return groupsPerHousehold
}

func (gs Groups) FilterByFavorite(favorite bool) Groups {
	result := make(Groups, 0, len(gs))
	for _, scenario := range gs {
		if scenario.Favorite == favorite {
			result = append(result, scenario)
		}
	}
	return result
}

func (gs Groups) SetSharingInfo(sharingInfos SharingInfos) {
	sharingInfoMap := sharingInfos.ToHouseholdMap()
	for i := range gs {
		sharingInfo, ok := sharingInfoMap[gs[i].HouseholdID]
		if !ok {
			continue
		}
		gs[i].SharingInfo = &sharingInfo
	}
}

func (gs Groups) SetFavorite(favorite bool) {
	for i := range gs {
		gs[i].Favorite = favorite
	}
}

func (gs Groups) FilterByHouseholdIDs(householdIDs []string) Groups {
	householdMap := make(map[string]bool, len(householdIDs))
	for _, householdID := range householdIDs {
		householdMap[householdID] = true
	}
	result := make(Groups, 0, len(gs))
	for _, group := range gs {
		if householdMap[group.HouseholdID] {
			result = append(result, group)
		}
	}
	return result
}

func GroupCapabilitiesFromDevices(devices []Device) CapabilitiesMap {
	groupedCapabilities := make(map[string][]ICapability)
	for _, device := range devices {
		for _, capability := range device.Capabilities {
			key := capability.Key()
			groupedCapabilities[key] = append(groupedCapabilities[key], capability)
		}
	}

	groupResult := make(map[string]ICapability)
	for key, capabilities := range groupedCapabilities {
		if len(capabilities) == len(devices) {
			if mergedCapability, ok := Capabilities(capabilities).Merge(); ok {
				groupResult[key] = mergedCapability
			}
		}
	}
	return groupResult
}

// TODO: change to GroupCapabilitiesFromDevices
func GetGroupCapabilityFromDevicesByTypeAndInstance(devices []Device, cType CapabilityType, cInstance string) (ICapability, bool) {
	var groupCapability ICapability

	for _, device := range devices {
		if deviceCapability, ok := device.GetCapabilityByTypeAndInstance(cType, cInstance); ok {
			if groupCapability == nil {
				groupCapability = deviceCapability
			} else {
				if mergedCapability, merged := groupCapability.Merge(deviceCapability); merged {
					groupCapability = mergedCapability
				}
			}
		}
	}

	return groupCapability, groupCapability != nil
}

func GroupOnOffStateFromDevices(devices []Device) DeviceStatus {
	var firstCapability ICapability

	for _, device := range devices {
		if deviceCapability, ok := device.GetCapabilityByTypeAndInstance(OnOffCapabilityType, string(OnOnOffCapabilityInstance)); ok {
			// TODO: поддержать UnknownState?
			if firstCapability == nil {
				firstCapability = deviceCapability
			} else {
				if firstCapability.State() != deviceCapability.State() {
					return SplitStatus
				}
			}
		}
	}

	return OnlineDeviceStatus
}
