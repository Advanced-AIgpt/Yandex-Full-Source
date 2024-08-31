package mobile

import (
	"strings"
	"time"

	"a.yandex-team.ru/alice/iot/bulbasaur/model"
	"a.yandex-team.ru/alice/library/go/timestamp"
)

// Sort devices by name
type deviceInfoViewByName []DeviceInfoView

func (s deviceInfoViewByName) Len() int {
	return len(s)
}
func (s deviceInfoViewByName) Swap(i, j int) {
	s[i], s[j] = s[j], s[i]
}
func (s deviceInfoViewByName) Less(i, j int) bool {
	iName := strings.ToLower(s[i].Name)
	jName := strings.ToLower(s[j].Name)

	switch {
	case iName != jName:
		return iName < jName
	default:
		return s[i].ID < s[j].ID
	}
}

type deviceTriggerInfoViewByName []DeviceTriggerInfoView

func (s deviceTriggerInfoViewByName) Len() int {
	return len(s)
}
func (s deviceTriggerInfoViewByName) Swap(i, j int) {
	s[i], s[j] = s[j], s[i]
}
func (s deviceTriggerInfoViewByName) Less(i, j int) bool {
	iName := strings.ToLower(s[i].Name)
	jName := strings.ToLower(s[j].Name)

	switch {
	case iName != jName:
		return iName < jName
	default:
		return s[i].ID < s[j].ID
	}
}

// Sort rooms by name
type roomInfoViewByName []RoomInfoView

func (s roomInfoViewByName) Len() int {
	return len(s)
}
func (s roomInfoViewByName) Swap(i, j int) {
	s[i], s[j] = s[j], s[i]
}
func (s roomInfoViewByName) Less(i, j int) bool {
	iName := strings.ToLower(s[i].Name)
	jName := strings.ToLower(s[j].Name)

	switch {
	case iName != jName:
		return iName < jName
	default:
		return s[i].ID < s[j].ID
	}
}

type deviceTriggerRoomInfoViewByName []DeviceTriggerRoomInfoView

func (s deviceTriggerRoomInfoViewByName) Len() int {
	return len(s)
}
func (s deviceTriggerRoomInfoViewByName) Swap(i, j int) {
	s[i], s[j] = s[j], s[i]
}
func (s deviceTriggerRoomInfoViewByName) Less(i, j int) bool {
	iName := strings.ToLower(s[i].Name)
	jName := strings.ToLower(s[j].Name)

	switch {
	case iName != jName:
		return iName < jName
	default:
		return s[i].ID < s[j].ID
	}
}

// Sort groups by name
type userGroupViewByName []UserGroupView

func (s userGroupViewByName) Len() int {
	return len(s)
}
func (s userGroupViewByName) Swap(i, j int) {
	s[i], s[j] = s[j], s[i]
}
func (s userGroupViewByName) Less(i, j int) bool {
	iName := strings.ToLower(s[i].Name)
	jName := strings.ToLower(s[j].Name)

	switch {
	case iName != jName:
		return iName < jName
	default:
		return s[i].ID < s[j].ID
	}
}

// Sort rooms by name
type userRoomViewByName []UserRoomView

func (s userRoomViewByName) Len() int {
	return len(s)
}
func (s userRoomViewByName) Swap(i, j int) {
	s[i], s[j] = s[j], s[i]
}
func (s userRoomViewByName) Less(i, j int) bool {
	iName := strings.ToLower(s[i].Name)
	jName := strings.ToLower(s[j].Name)

	switch {
	case iName != jName:
		return iName < jName
	default:
		return s[i].ID < s[j].ID
	}
}

// Sort ProviderSkillShortInfo by Trusted flag, AverageRating and provider Name
type ProviderByRating []ProviderSkillShortInfo

func (s ProviderByRating) Len() int {
	return len(s)
}
func (s ProviderByRating) Swap(i, j int) {
	s[i], s[j] = s[j], s[i]
}
func (s ProviderByRating) Less(i, j int) bool {
	if lir, rir := s[i].GetInnerRating(), s[j].GetInnerRating(); lir != rir {
		return lir < rir
	}
	if lt, rt := s[i].Trusted, s[j].Trusted; lt != rt {
		return lt && !rt
	}
	if lar, rar := s[i].AverageRating, s[j].AverageRating; lar != rar {
		return lar > rar
	}
	return strings.ToLower(s[i].Name) < strings.ToLower(s[j].Name)
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
	return model.CompareModes(model.KnownModes[s[i].Value], model.KnownModes[s[j].Value])
}

type CapabilityStateViewSorting []CapabilityStateView

func (s CapabilityStateViewSorting) Len() int {
	return len(s)
}
func (s CapabilityStateViewSorting) Swap(i, j int) {
	s[i], s[j] = s[j], s[i]
}
func (s CapabilityStateViewSorting) Less(i, j int) bool {
	first := model.CapabilityComparingInfo{
		Type:         s[i].Type,
		InstanceName: s[i].Parameters.GetInstanceName(),
	}
	second := model.CapabilityComparingInfo{
		Type:         s[j].Type,
		InstanceName: s[j].Parameters.GetInstanceName(),
	}
	return model.LessCapabilities(first, second)
}

type ScenarioListViewSorting []ScenarioListView

func (s ScenarioListViewSorting) Len() int {
	return len(s)
}

func (s ScenarioListViewSorting) Swap(i, j int) {
	s[i], s[j] = s[j], s[i]
}

func (s ScenarioListViewSorting) Less(i, j int) bool {
	siName := strings.ToLower(string(s[i].Name))
	sjName := strings.ToLower(string(s[j].Name))

	switch {
	case s[i].IsActive != s[j].IsActive:
		return s[i].IsActive
	case siName != sjName:
		return siName < sjName
	default:
		return s[i].ID < s[j].ID
	}
}

type StereopairListPossibleResponseDeviceInfosByName []StereopairListPossibleResponseDeviceInfo

func (s StereopairListPossibleResponseDeviceInfosByName) Len() int {
	return len(s)
}

func (s StereopairListPossibleResponseDeviceInfosByName) Less(i, j int) bool {
	switch {
	case s[i].Name != s[j].Name:
		return s[i].Name < s[j].Name
	default:
		return s[i].ID < s[j].ID
	}
}

func (s StereopairListPossibleResponseDeviceInfosByName) Swap(i, j int) {
	s[i], s[j] = s[j], s[i]
}

type HouseholdViewSorting []HouseholdView

func (s HouseholdViewSorting) Len() int {
	return len(s)
}

func (s HouseholdViewSorting) Swap(i, j int) {
	s[i], s[j] = s[j], s[i]
}

func (s HouseholdViewSorting) Less(i, j int) bool {
	siName := strings.ToLower(s[i].Name)
	sjName := strings.ToLower(s[j].Name)

	switch {
	case s[i].IsCurrent != s[j].IsCurrent:
		return s[i].IsCurrent
	case siName != sjName:
		return siName < sjName
	default:
		return s[i].ID < s[j].ID
	}
}

type HouseholdDeviceConfigurationViewSorting struct {
	Households         []HouseholdDeviceConfigurationView
	CurrentHouseholdID string
}

func (s *HouseholdDeviceConfigurationViewSorting) Len() int {
	return len(s.Households)
}

func (s *HouseholdDeviceConfigurationViewSorting) Swap(i, j int) {
	s.Households[i], s.Households[j] = s.Households[j], s.Households[i]
}

func (s *HouseholdDeviceConfigurationViewSorting) Less(i, j int) bool {
	siName := strings.ToLower(s.Households[i].Name)
	sjName := strings.ToLower(s.Households[j].Name)
	siCurrent := s.Households[i].ID == s.CurrentHouseholdID
	sjCurrent := s.Households[j].ID == s.CurrentHouseholdID

	switch {
	case siCurrent != sjCurrent:
		return siCurrent
	case siName != sjName:
		return siName < sjName
	default:
		return s.Households[i].ID < s.Households[j].ID
	}
}

type HouseholdWithDevicesInfoViewSorting []HouseholdWithDevicesView

func (s HouseholdWithDevicesInfoViewSorting) Len() int {
	return len(s)
}

func (s HouseholdWithDevicesInfoViewSorting) Swap(i, j int) {
	s[i], s[j] = s[j], s[i]
}

func (s HouseholdWithDevicesInfoViewSorting) Less(i, j int) bool {
	siName := strings.ToLower(s[i].Name)
	sjName := strings.ToLower(s[j].Name)

	switch {
	case s[i].IsCurrent != s[j].IsCurrent:
		return s[i].IsCurrent
	case siName != sjName:
		return siName < sjName
	default:
		return s[i].ID < s[j].ID
	}
}

type HouseholdWithDeviceTriggersInfoViewSorting []HouseholdWithDeviceTriggersView

func (s HouseholdWithDeviceTriggersInfoViewSorting) Len() int {
	return len(s)
}

func (s HouseholdWithDeviceTriggersInfoViewSorting) Swap(i, j int) {
	s[i], s[j] = s[j], s[i]
}

func (s HouseholdWithDeviceTriggersInfoViewSorting) Less(i, j int) bool {
	siName := strings.ToLower(s[i].Name)
	sjName := strings.ToLower(s[j].Name)

	switch {
	case s[i].IsCurrent != s[j].IsCurrent:
		return s[i].IsCurrent
	case siName != sjName:
		return siName < sjName
	default:
		return s[i].ID < s[j].ID
	}
}

type DeviceAvailableForGroupViewSorting []DeviceAvailableForGroupView

func (s DeviceAvailableForGroupViewSorting) Len() int {
	return len(s)
}

func (s DeviceAvailableForGroupViewSorting) Swap(i, j int) {
	s[i], s[j] = s[j], s[i]
}

func (s DeviceAvailableForGroupViewSorting) Less(i, j int) bool {
	siName := strings.ToLower(s[i].Name)
	sjName := strings.ToLower(s[j].Name)

	switch {
	case siName != sjName:
		return siName < sjName
	default:
		return s[i].ID < s[j].ID
	}
}

type RoomDevicesAvailableForGroupViewSorting []RoomDevicesAvailableForGroupView

func (s RoomDevicesAvailableForGroupViewSorting) Len() int {
	return len(s)
}

func (s RoomDevicesAvailableForGroupViewSorting) Swap(i, j int) {
	s[i], s[j] = s[j], s[i]
}

func (s RoomDevicesAvailableForGroupViewSorting) Less(i, j int) bool {
	siName := strings.ToLower(s[i].Name)
	sjName := strings.ToLower(s[j].Name)

	switch {
	case siName != sjName:
		return siName < sjName
	default:
		return s[i].ID < s[j].ID
	}
}

type GroupStateDeviceViewSorting []GroupStateDeviceView

func (s GroupStateDeviceViewSorting) Len() int {
	return len(s)
}

func (s GroupStateDeviceViewSorting) Swap(i, j int) {
	s[i], s[j] = s[j], s[i]
}

func (s GroupStateDeviceViewSorting) Less(i, j int) bool {
	siName := strings.ToLower(s[i].Name)
	sjName := strings.ToLower(s[j].Name)

	switch {
	case siName != sjName:
		return siName < sjName
	default:
		return s[i].ID < s[j].ID
	}
}

type DeviceRoomEditViewSorting []DeviceRoomEditView

func (s DeviceRoomEditViewSorting) Len() int {
	return len(s)
}

func (s DeviceRoomEditViewSorting) Swap(i, j int) {
	s[i], s[j] = s[j], s[i]
}

func (s DeviceRoomEditViewSorting) Less(i, j int) bool {
	siName := strings.ToLower(s[i].Name)
	sjName := strings.ToLower(s[j].Name)

	switch {
	case siName != sjName:
		return siName < sjName
	default:
		return s[i].ID < s[j].ID
	}
}

type RoomAvailableDevicesViewSorting []RoomAvailableDevicesView

func (s RoomAvailableDevicesViewSorting) Len() int {
	return len(s)
}

func (s RoomAvailableDevicesViewSorting) Swap(i, j int) {
	s[i], s[j] = s[j], s[i]
}

func (s RoomAvailableDevicesViewSorting) Less(i, j int) bool {
	siName := strings.ToLower(s[i].Name)
	sjName := strings.ToLower(s[j].Name)

	switch {
	case siName != sjName:
		return siName < sjName
	default:
		return s[i].ID < s[j].ID
	}
}

type DeviceAvailableForRoomViewSorting []DeviceAvailableForRoomView

func (s DeviceAvailableForRoomViewSorting) Len() int {
	return len(s)
}

func (s DeviceAvailableForRoomViewSorting) Swap(i, j int) {
	s[i], s[j] = s[j], s[i]
}

func (s DeviceAvailableForRoomViewSorting) Less(i, j int) bool {
	siName := strings.ToLower(s[i].Name)
	sjName := strings.ToLower(s[j].Name)

	switch {
	case siName != sjName:
		return siName < sjName
	default:
		return s[i].ID < s[j].ID
	}
}

type HouseholdRoomAvailableDevicesViewSorting []HouseholdRoomAvailableDevicesView

func (s HouseholdRoomAvailableDevicesViewSorting) Len() int {
	return len(s)
}

func (s HouseholdRoomAvailableDevicesViewSorting) Swap(i, j int) {
	s[i], s[j] = s[j], s[i]
}

func (s HouseholdRoomAvailableDevicesViewSorting) Less(i, j int) bool {
	siName := strings.ToLower(s[i].Name)
	sjName := strings.ToLower(s[j].Name)

	switch {
	case s[i].IsCurrent != s[j].IsCurrent:
		return s[i].IsCurrent
	case siName != sjName:
		return siName < sjName
	default:
		return s[i].ID < s[j].ID
	}
}

type ProviderSkillDeviceViewSorting []ProviderSkillDeviceView

func (s ProviderSkillDeviceViewSorting) Len() int {
	return len(s)
}

func (s ProviderSkillDeviceViewSorting) Swap(i, j int) {
	s[i], s[j] = s[j], s[i]
}

func (s ProviderSkillDeviceViewSorting) Less(i, j int) bool {
	siName := strings.ToLower(s[i].Name)
	sjName := strings.ToLower(s[j].Name)

	switch {
	case siName != sjName:
		return siName < sjName
	default:
		return s[i].ID < s[j].ID
	}
}

type ItemInfoViewSorting []ItemInfoView

func (s ItemInfoViewSorting) Len() int {
	return len(s)
}

func (s ItemInfoViewSorting) Swap(i, j int) {
	s[i], s[j] = s[j], s[i]
}

func (s ItemInfoViewSorting) Less(i, j int) bool {
	siName := strings.ToLower(s[i].Name)
	sjName := strings.ToLower(s[j].Name)

	switch {
	case s[i].Unconfigured != s[j].Unconfigured:
		return s[i].Unconfigured
	case siName != sjName:
		return siName < sjName
	default:
		return s[i].ID < s[j].ID
	}
}

type RoomInfoViewV3Sorting []RoomInfoViewV3

func (s RoomInfoViewV3Sorting) Len() int {
	return len(s)
}

func (s RoomInfoViewV3Sorting) Swap(i, j int) {
	s[i], s[j] = s[j], s[i]
}

func (s RoomInfoViewV3Sorting) Less(i, j int) bool {
	siName := strings.ToLower(s[i].Name)
	sjName := strings.ToLower(s[j].Name)

	switch {
	case siName != sjName:
		return siName < sjName
	default:
		return s[i].ID < s[j].ID
	}
}

type HouseholdWithDevicesInfoViewV3Sorting []HouseholdWithDevicesViewV3

func (s HouseholdWithDevicesInfoViewV3Sorting) Len() int {
	return len(s)
}

func (s HouseholdWithDevicesInfoViewV3Sorting) Swap(i, j int) {
	s[i], s[j] = s[j], s[i]
}

func (s HouseholdWithDevicesInfoViewV3Sorting) Less(i, j int) bool {
	siName := strings.ToLower(s[i].Name)
	sjName := strings.ToLower(s[j].Name)

	switch {
	case s[i].IsCurrent != s[j].IsCurrent:
		return s[i].IsCurrent
	case siName != sjName:
		return siName < sjName
	default:
		return s[i].ID < s[j].ID
	}
}

type GroupStateRoomViewSorting []GroupStateRoomView

func (s GroupStateRoomViewSorting) Len() int {
	return len(s)
}

func (s GroupStateRoomViewSorting) Swap(i, j int) {
	s[i], s[j] = s[j], s[i]
}

func (s GroupStateRoomViewSorting) Less(i, j int) bool {
	siName := strings.ToLower(s[i].Name)
	sjName := strings.ToLower(s[j].Name)

	switch {
	case siName != sjName:
		return siName < sjName
	default:
		return s[i].ID < s[j].ID
	}
}

type FavoriteScenarioAvailableViewSorting []FavoriteScenarioAvailableView

func (s FavoriteScenarioAvailableViewSorting) Len() int {
	return len(s)
}

func (s FavoriteScenarioAvailableViewSorting) Swap(i, j int) {
	s[i], s[j] = s[j], s[i]
}

func (s FavoriteScenarioAvailableViewSorting) Less(i, j int) bool {
	siName := strings.ToLower(s[i].Name)
	sjName := strings.ToLower(s[j].Name)

	switch {
	case siName != sjName:
		return siName < sjName
	default:
		return s[i].ID < s[j].ID
	}
}

type RoomFavoriteDevicesAvailableViewSorting []RoomFavoriteDevicesAvailableView

func (s RoomFavoriteDevicesAvailableViewSorting) Len() int {
	return len(s)
}

func (s RoomFavoriteDevicesAvailableViewSorting) Swap(i, j int) {
	s[i], s[j] = s[j], s[i]
}

func (s RoomFavoriteDevicesAvailableViewSorting) Less(i, j int) bool {
	siName := strings.ToLower(s[i].Name)
	sjName := strings.ToLower(s[j].Name)

	switch {
	case siName != sjName:
		return siName < sjName
	default:
		return s[i].ID < s[j].ID
	}
}

type FavoriteDeviceAvailableViewSorting []FavoriteDeviceAvailableView

func (s FavoriteDeviceAvailableViewSorting) Len() int {
	return len(s)
}

func (s FavoriteDeviceAvailableViewSorting) Swap(i, j int) {
	s[i], s[j] = s[j], s[i]
}

func (s FavoriteDeviceAvailableViewSorting) Less(i, j int) bool {
	siName := strings.ToLower(s[i].Name)
	sjName := strings.ToLower(s[j].Name)

	switch {
	case siName != sjName:
		return siName < sjName
	default:
		return s[i].ID < s[j].ID
	}
}

type HouseholdFavoriteDevicesAvailableViewSorting []HouseholdFavoriteDevicesAvailableView

func (s HouseholdFavoriteDevicesAvailableViewSorting) Len() int {
	return len(s)
}

func (s HouseholdFavoriteDevicesAvailableViewSorting) Swap(i, j int) {
	s[i], s[j] = s[j], s[i]
}

func (s HouseholdFavoriteDevicesAvailableViewSorting) Less(i, j int) bool {
	siName := strings.ToLower(s[i].Name)
	sjName := strings.ToLower(s[j].Name)

	switch {
	case s[i].IsCurrent != s[j].IsCurrent:
		return s[i].IsCurrent
	case siName != sjName:
		return siName < sjName
	default:
		return s[i].ID < s[j].ID
	}
}

type FavoritePropertyAvailableViewSorting []FavoritePropertyAvailableView

func (s FavoritePropertyAvailableViewSorting) Len() int {
	return len(s)
}

func (s FavoritePropertyAvailableViewSorting) Swap(i, j int) {
	s[i], s[j] = s[j], s[i]
}

func (s FavoritePropertyAvailableViewSorting) Less(i, j int) bool {
	siDeviceName := strings.ToLower(s[i].DeviceName)
	sjDeviceName := strings.ToLower(s[j].DeviceName)

	siKey := strings.ToLower(s[i].Property.PropertyKey())
	sjKey := strings.ToLower(s[j].Property.PropertyKey())

	switch {
	case siDeviceName != sjDeviceName:
		return siDeviceName < sjDeviceName
	case siKey != sjKey:
		return siKey < sjKey
	default:
		return s[i].DeviceID < s[j].DeviceID
	}
}

type RoomFavoritePropertiesAvailableViewSorting []RoomFavoritePropertiesAvailableView

func (s RoomFavoritePropertiesAvailableViewSorting) Len() int {
	return len(s)
}

func (s RoomFavoritePropertiesAvailableViewSorting) Swap(i, j int) {
	s[i], s[j] = s[j], s[i]
}

func (s RoomFavoritePropertiesAvailableViewSorting) Less(i, j int) bool {
	siName := strings.ToLower(s[i].Name)
	sjName := strings.ToLower(s[j].Name)

	switch {
	case siName != sjName:
		return siName < sjName
	default:
		return s[i].ID < s[j].ID
	}
}

type HouseholdFavoritePropertiesAvailableViewSorting []HouseholdFavoritePropertiesAvailableView

func (s HouseholdFavoritePropertiesAvailableViewSorting) Len() int {
	return len(s)
}

func (s HouseholdFavoritePropertiesAvailableViewSorting) Swap(i, j int) {
	s[i], s[j] = s[j], s[i]
}

func (s HouseholdFavoritePropertiesAvailableViewSorting) Less(i, j int) bool {
	siName := strings.ToLower(s[i].Name)
	sjName := strings.ToLower(s[j].Name)

	switch {
	case s[i].IsCurrent != s[j].IsCurrent:
		return s[i].IsCurrent
	case siName != sjName:
		return siName < sjName
	default:
		return s[i].ID < s[j].ID
	}
}

type FavoriteListItemViewSorting []FavoriteListItemView

func (s FavoriteListItemViewSorting) Len() int {
	return len(s)
}

func (s FavoriteListItemViewSorting) Swap(i, j int) {
	s[i], s[j] = s[j], s[i]
}

func (s FavoriteListItemViewSorting) Less(i, j int) bool {
	siName := strings.ToLower(s[i].Parameters.GetName())
	sjName := strings.ToLower(s[j].Parameters.GetName())

	siTypeSorting := favoriteTypesSortingMap[s[i].Type]
	sjTypeSorting := favoriteTypesSortingMap[s[j].Type]

	switch {
	case siTypeSorting != sjTypeSorting:
		return siTypeSorting < sjTypeSorting
	case siName != sjName:
		return siName < sjName
	default:
		return s[i].Parameters.GetID() < s[j].Parameters.GetID()
	}
}

type FavoriteDevicePropertyListViewSorting []FavoriteDevicePropertyListView

func (s FavoriteDevicePropertyListViewSorting) Len() int {
	return len(s)
}

func (s FavoriteDevicePropertyListViewSorting) Swap(i, j int) {
	s[i], s[j] = s[j], s[i]
}

func (s FavoriteDevicePropertyListViewSorting) Less(i, j int) bool {
	siRoomName := strings.ToLower(s[i].RoomName)
	sjRoomName := strings.ToLower(s[j].RoomName)

	siHouseholdName := strings.ToLower(s[i].HouseholdName)
	sjHouseholdName := strings.ToLower(s[j].HouseholdName)

	switch {
	case siHouseholdName != sjHouseholdName:
		return siHouseholdName < sjHouseholdName
	case siRoomName != sjRoomName:
		return siRoomName < sjRoomName
	default:
		return s[i].Property.PropertyKey() < s[j].Property.PropertyKey()
	}
}

type HouseholdFavoriteGroupsAvailableViewSorting []HouseholdFavoriteGroupsAvailableView

func (s HouseholdFavoriteGroupsAvailableViewSorting) Len() int {
	return len(s)
}

func (s HouseholdFavoriteGroupsAvailableViewSorting) Swap(i, j int) {
	s[i], s[j] = s[j], s[i]
}

func (s HouseholdFavoriteGroupsAvailableViewSorting) Less(i, j int) bool {
	siName := strings.ToLower(s[i].Name)
	sjName := strings.ToLower(s[j].Name)

	switch {
	case s[i].IsCurrent != s[j].IsCurrent:
		return s[i].IsCurrent
	case siName != sjName:
		return siName < sjName
	default:
		return s[i].ID < s[j].ID
	}
}

type FavoriteGroupAvailableViewSorting []FavoriteGroupAvailableView

func (s FavoriteGroupAvailableViewSorting) Len() int {
	return len(s)
}

func (s FavoriteGroupAvailableViewSorting) Swap(i, j int) {
	s[i], s[j] = s[j], s[i]
}

func (s FavoriteGroupAvailableViewSorting) Less(i, j int) bool {
	siName := strings.ToLower(s[i].Name)
	sjName := strings.ToLower(s[j].Name)

	switch {
	case siName != sjName:
		return siName < sjName
	default:
		return s[i].ID < s[j].ID
	}
}

type SpeakerNewsTopicViewSorting []SpeakerNewsTopicView

func (s SpeakerNewsTopicViewSorting) Len() int {
	return len(s)
}

func (s SpeakerNewsTopicViewSorting) Swap(i, j int) {
	s[i], s[j] = s[j], s[i]
}

func (s SpeakerNewsTopicViewSorting) Less(i, j int) bool {
	siName := strings.ToLower(s[i].Name)
	sjName := strings.ToLower(s[j].Name)

	switch {
	case s[i].ID == model.IndexSpeakerNewsTopic:
		return true
	default:
		return siName < sjName
	}
}

type TandemAvailableDeviceViewSorting []TandemAvailableDeviceView

func (s TandemAvailableDeviceViewSorting) Len() int {
	return len(s)
}

func (s TandemAvailableDeviceViewSorting) Swap(i, j int) {
	s[i], s[j] = s[j], s[i]
}

func (s TandemAvailableDeviceViewSorting) Less(i, j int) bool {
	siName := strings.ToLower(s[i].Name)
	sjName := strings.ToLower(s[j].Name)
	switch {
	case siName != sjName:
		return siName < sjName
	default:
		return s[i].ID < s[j].ID
	}
}

type PropertyStateViewSorting []PropertyStateView

func (s PropertyStateViewSorting) Len() int {
	return len(s)
}

func (s PropertyStateViewSorting) Swap(i, j int) {
	s[i], s[j] = s[j], s[i]
}

func (s PropertyStateViewSorting) Less(i, j int) bool {
	siComparingInfo := model.PropertyComparingInfo{
		Type:         s[i].Type,
		InstanceName: s[i].Parameters.GetInstanceName(),
	}
	sjComparingInfo := model.PropertyComparingInfo{
		Type:         s[j].Type,
		InstanceName: s[j].Parameters.GetInstanceName(),
	}
	return model.LessProperties(siComparingInfo, sjComparingInfo)
}

type SpeakerSoundCategoryViewSorting []SpeakerSoundCategoryView

func (s SpeakerSoundCategoryViewSorting) Len() int {
	return len(s)
}

func (s SpeakerSoundCategoryViewSorting) Swap(i, j int) {
	s[i], s[j] = s[j], s[i]
}

func (s SpeakerSoundCategoryViewSorting) Less(i, j int) bool {
	siName := strings.ToLower(s[i].Name)
	sjName := strings.ToLower(s[j].Name)
	switch {
	case siName != sjName:
		return siName < sjName
	default:
		return s[i].ID < s[j].ID
	}
}

type SpeakerSoundViewSorting []SpeakerSoundView

func (s SpeakerSoundViewSorting) Len() int {
	return len(s)
}

func (s SpeakerSoundViewSorting) Swap(i, j int) {
	s[i], s[j] = s[j], s[i]
}

func (s SpeakerSoundViewSorting) Less(i, j int) bool {
	siName := strings.ToLower(s[i].Name)
	sjName := strings.ToLower(s[j].Name)
	switch {
	case siName != sjName:
		return siName < sjName
	default:
		return s[i].ID < s[j].ID
	}
}

type DeviceConfigureV2ViewSorting []DeviceConfigureV2View

func (s DeviceConfigureV2ViewSorting) Len() int {
	return len(s)
}

func (s DeviceConfigureV2ViewSorting) Swap(i, j int) {
	s[i], s[j] = s[j], s[i]
}

func (s DeviceConfigureV2ViewSorting) Less(i, j int) bool {
	siName := strings.ToLower(s[i].Name)
	sjName := strings.ToLower(s[j].Name)
	switch {
	case siName != sjName:
		return siName < sjName
	default:
		return s[i].ID < s[j].ID
	}
}

type ScenarioLaunchesHistoryViewSorting []ScenarioLaunchesHistoryView

func (s ScenarioLaunchesHistoryViewSorting) Len() int {
	return len(s)
}

func (s ScenarioLaunchesHistoryViewSorting) Swap(i, j int) {
	s[i], s[j] = s[j], s[i]
}

func (s ScenarioLaunchesHistoryViewSorting) Less(i, j int) bool {
	siName := strings.ToLower(string(s[i].Name))
	sjName := strings.ToLower(string(s[j].Name))
	switch {
	case s[i].LaunchTime != s[j].LaunchTime:
		siTime, _ := time.Parse(time.RFC3339, s[i].LaunchTime)
		sjTime, _ := time.Parse(time.RFC3339, s[j].LaunchTime)
		return timestamp.FromTime(siTime) > timestamp.FromTime(sjTime)
	default:
		return siName < sjName
	}
}

type HouseholdInvitationShortViewSorting []HouseholdInvitationShortView

func (s HouseholdInvitationShortViewSorting) Len() int {
	return len(s)
}

func (s HouseholdInvitationShortViewSorting) Swap(i, j int) {
	s[i], s[j] = s[j], s[i]
}

func (s HouseholdInvitationShortViewSorting) Less(i, j int) bool {
	switch {
	case s[i].Sender.ID != s[j].Sender.ID:
		return s[i].Sender.ID < s[j].Sender.ID
	default:
		return s[i].ID < s[j].ID
	}

}

type HouseholdResidentViewSorting []HouseholdResidentView

func (s HouseholdResidentViewSorting) Len() int {
	return len(s)
}

func (s HouseholdResidentViewSorting) Swap(i, j int) {
	s[i], s[j] = s[j], s[i]
}

func (s HouseholdResidentViewSorting) Less(i, j int) bool {
	switch {
	case s[i].Role == model.OwnerHouseholdRole:
		return true
	default:
		return s[i].DisplayName < s[j].DisplayName
	}
}
