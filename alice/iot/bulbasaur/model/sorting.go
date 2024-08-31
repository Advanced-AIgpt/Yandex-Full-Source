package model

import (
	"strings"
)

// Sort devices by name and id
type DevicesSorting []Device

func (s DevicesSorting) Len() int {
	return len(s)
}
func (s DevicesSorting) Swap(i, j int) {
	s[i], s[j] = s[j], s[i]
}
func (s DevicesSorting) Less(i, j int) bool {
	iName, jName := strings.ToLower(s[i].Name), strings.ToLower(s[j].Name)

	switch {
	case iName != jName:
		return iName < jName
	default:
		return s[i].ID < s[j].ID
	}
}

// Sort modes by name, fans by speed, number by order
type ModesSorting []Mode

func (s ModesSorting) Len() int {
	return len(s)
}
func (s ModesSorting) Swap(i, j int) {
	s[i], s[j] = s[j], s[i]
}
func (s ModesSorting) Less(i, j int) bool {
	return CompareModes(KnownModes[s[i].Value], KnownModes[s[j].Value])
}

type CapabilityComparingInfo struct {
	Type         CapabilityType
	InstanceName string
}

func LessCapabilities(first CapabilityComparingInfo, second CapabilityComparingInfo) bool {
	firstTypePriority := 999
	secondTypePriority := 999
	if priority, ok := capabilitySortingMap[first.Type]; ok {
		firstTypePriority = priority
	}
	if priority, ok := capabilitySortingMap[second.Type]; ok {
		secondTypePriority = priority
	}
	switch {
	case firstTypePriority != secondTypePriority:
		return firstTypePriority < secondTypePriority
	default:
		return strings.ToLower(first.InstanceName) < strings.ToLower(second.InstanceName)
	}
}

type PropertyComparingInfo struct {
	Type         PropertyType
	InstanceName string
}

func LessProperties(first PropertyComparingInfo, second PropertyComparingInfo) bool {
	firstTypePriority := 999
	secondTypePriority := 999
	if priority, ok := propertySortingMap[first.Type]; ok {
		firstTypePriority = priority
	}
	if priority, ok := propertySortingMap[second.Type]; ok {
		secondTypePriority = priority
	}
	switch {
	case firstTypePriority != secondTypePriority:
		return firstTypePriority < secondTypePriority
	default:
		return strings.ToLower(first.InstanceName) < strings.ToLower(second.InstanceName)
	}
}

type CapabilitiesSorting Capabilities

func (s CapabilitiesSorting) Len() int {
	return len(s)
}

func (s CapabilitiesSorting) Swap(i, j int) {
	s[i], s[j] = s[j], s[i]
}

func (s CapabilitiesSorting) Less(i, j int) bool {
	first := CapabilityComparingInfo{
		Type:         s[i].Type(),
		InstanceName: s[i].Instance(),
	}
	second := CapabilityComparingInfo{
		Type:         s[j].Type(),
		InstanceName: s[j].Instance(),
	}
	return LessCapabilities(first, second)
}

type PropertiesSorting Properties

func (s PropertiesSorting) Len() int {
	return len(s)
}

func (s PropertiesSorting) Swap(i, j int) {
	s[i], s[j] = s[j], s[i]
}

func (s PropertiesSorting) Less(i, j int) bool {
	first := PropertyComparingInfo{
		Type:         s[i].Type(),
		InstanceName: s[i].Instance(),
	}
	second := PropertyComparingInfo{
		Type:         s[j].Type(),
		InstanceName: s[j].Instance(),
	}
	return LessProperties(first, second)
}

type ColorSceneSorting []ColorScene

func (s ColorSceneSorting) Len() int {
	return len(s)
}

func (s ColorSceneSorting) Swap(i, j int) {
	s[i], s[j] = s[j], s[i]
}

func (s ColorSceneSorting) Less(i, j int) bool {
	var siName string
	if scene, exist := KnownColorScenes[s[i].ID]; exist {
		siName = strings.ToLower(scene.Name)
	}
	var sjName string
	if scene, exist := KnownColorScenes[s[j].ID]; exist {
		sjName = strings.ToLower(scene.Name)
	}

	switch {
	case siName != sjName:
		return siName < sjName
	default:
		return s[i].ID < s[j].ID
	}
}

type RoomsSorting Rooms

func (s RoomsSorting) Len() int {
	return len(s)
}
func (s RoomsSorting) Swap(i, j int) {
	s[i], s[j] = s[j], s[i]
}
func (s RoomsSorting) Less(i, j int) bool {
	iName, jName := strings.ToLower(s[i].Name), strings.ToLower(s[j].Name)

	switch {
	case iName != jName:
		return iName < jName
	default:
		return s[i].ID < s[j].ID
	}
}

type HouseholdsSorting []Household

func (s HouseholdsSorting) Len() int {
	return len(s)
}
func (s HouseholdsSorting) Swap(i, j int) {
	s[i], s[j] = s[j], s[i]
}
func (s HouseholdsSorting) Less(i, j int) bool {
	iName, jName := strings.ToLower(s[i].Name), strings.ToLower(s[j].Name)

	switch {
	case iName != jName:
		return iName < jName
	default:
		return s[i].ID < s[j].ID
	}
}

type FavoritesDevicePropertySorting []FavoritesDeviceProperty

func (s FavoritesDevicePropertySorting) Len() int {
	return len(s)
}
func (s FavoritesDevicePropertySorting) Swap(i, j int) {
	s[i], s[j] = s[j], s[i]
}
func (s FavoritesDevicePropertySorting) Less(i, j int) bool {
	switch {
	case s[i].DeviceID != s[j].DeviceID:
		return s[i].DeviceID < s[j].DeviceID
	default:
		return s[i].PropertyKey() < s[j].PropertyKey()
	}
}

type SharingInfoSorting SharingInfos

func (si SharingInfoSorting) Len() int {
	return len(si)
}
func (si SharingInfoSorting) Swap(i, j int) {
	si[i], si[j] = si[j], si[i]
}
func (si SharingInfoSorting) Less(i, j int) bool {
	switch {
	case si[i].OwnerID != si[j].OwnerID:
		return si[i].OwnerID < si[j].OwnerID
	default:
		return si[i].HouseholdID < si[j].HouseholdID
	}
}
