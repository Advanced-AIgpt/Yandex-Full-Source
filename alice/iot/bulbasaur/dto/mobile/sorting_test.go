package mobile

import (
	"math/rand"
	"sort"
	"testing"

	"github.com/stretchr/testify/assert"

	"a.yandex-team.ru/alice/iot/bulbasaur/model"
	"a.yandex-team.ru/library/go/ptr"
)

func TestSortByRating(t *testing.T) {
	p1 := ProviderSkillShortInfo{
		SkillID:       model.XiaomiSkill,
		Name:          "Xiaomi",
		Trusted:       true,
		AverageRating: 0,
	}
	p2 := ProviderSkillShortInfo{
		SkillID:       model.AqaraSkill,
		Name:          "Aqara",
		Trusted:       true,
		AverageRating: 0,
	}
	p3 := ProviderSkillShortInfo{
		SkillID:       model.RedmondSkill,
		Name:          "Redmond",
		Trusted:       true,
		AverageRating: 0,
	}
	p4 := ProviderSkillShortInfo{
		SkillID:       model.SamsungSkill,
		Name:          "Samsung",
		Trusted:       false,
		AverageRating: 0,
	}
	p5 := ProviderSkillShortInfo{
		SkillID:       model.LGSkill,
		Name:          "LG",
		Trusted:       false,
		AverageRating: 0,
	}
	p6 := ProviderSkillShortInfo{
		SkillID:       model.NewPhilipsSkill,
		Name:          "Philips",
		Trusted:       true,
		AverageRating: 3.5,
	}
	p7 := ProviderSkillShortInfo{
		SkillID:       model.LegrandSkill,
		Name:          "Legrand",
		Trusted:       true,
		AverageRating: 5,
	}
	p8 := ProviderSkillShortInfo{
		SkillID:       model.RubetekSkill,
		Name:          "Rubetek",
		Trusted:       true,
		AverageRating: 3.5,
	}
	p9 := ProviderSkillShortInfo{
		SkillID:       model.ElariSkill,
		Name:          "Elari",
		Trusted:       true,
		AverageRating: 5,
	}
	p10 := ProviderSkillShortInfo{
		SkillID:       model.DigmaSkill,
		Name:          "Digma",
		Trusted:       true,
		AverageRating: 5,
	}
	i1 := ProviderSkillShortInfo{
		SkillID:       "1",
		Name:          "f",
		Trusted:       true,
		AverageRating: 5,
	}
	i2 := ProviderSkillShortInfo{
		SkillID:       "2",
		Name:          "e",
		Trusted:       true,
		AverageRating: 3.5,
	}
	i3 := ProviderSkillShortInfo{
		SkillID:       "3",
		Name:          "c",
		Trusted:       true,
		AverageRating: 0,
	}
	i4 := ProviderSkillShortInfo{
		SkillID:       "4",
		Name:          "d",
		Trusted:       true,
		AverageRating: 0,
	}
	i5 := ProviderSkillShortInfo{
		SkillID:       "5",
		Name:          "a",
		Trusted:       false,
		AverageRating: 0,
	}
	i6 := ProviderSkillShortInfo{
		SkillID:       "6",
		Name:          "b",
		Trusted:       false,
		AverageRating: 0,
	}

	t.Run("byRating", func(t *testing.T) {
		p := []ProviderSkillShortInfo{i4, i6, i5, i3, i2, i1}
		sort.Sort(ProviderByRating(p))
		assert.Equal(t, []ProviderSkillShortInfo{i1, i2, i3, i4, i5, i6}, p)
	})

	t.Run("byInnerRating", func(t *testing.T) {
		p := []ProviderSkillShortInfo{p8, p9, p2, p1, p3, p4, p5, p6, p7, p10}
		sort.Stable(ProviderByRating(p))
		assert.Equal(t, []ProviderSkillShortInfo{p1, p2, p3, p4, p5, p6, p7, p8, p9, p10}, p)
	})

	t.Run("byCombinedRating", func(t *testing.T) {
		p := []ProviderSkillShortInfo{i4, i6, i5, p8, p9, p2, p1, p3, p4, p5, p6, p7, i3, i2, i1, p10}
		sort.Sort(ProviderByRating(p))
		assert.Equal(t, []ProviderSkillShortInfo{p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, i1, i2, i3, i4, i5, i6}, p)
	})
}

func TestModesByName(t *testing.T) {
	auto := Mode{model.AutoMode, "Авто"}
	cool := Mode{model.CoolMode, "Охлаждение"}
	heat := Mode{model.HeatMode, "Нагрев"}
	fanOnly := Mode{model.FanOnlyMode, "Вентиляция"}
	dry := Mode{model.DryMode, "Осушение"}
	// case 1: list with known modes
	list := []Mode{
		cool,
		auto,
		heat,
		dry,
		fanOnly,
	}

	sort.Sort(ModesSorting(list))

	expectedList := []Mode{
		cool,
		heat,
		fanOnly,
		dry,
		auto,
	}
	assert.Equal(t, expectedList, list)
	// case 2: list with 1 unknown mode
	list = []Mode{
		cool,
		auto,
		heat,
		dry,
		{"turbo", "Турбо"},
	}

	sort.Sort(ModesSorting(list))

	expectedList = []Mode{
		cool,
		heat,
		dry,
		auto,
		{"turbo", "Турбо"},
	}
	assert.Equal(t, expectedList, list)
}

func TestFanModesBySpeed(t *testing.T) {
	auto := Mode{model.AutoMode, "Авто"}
	low := Mode{model.LowMode, "Низкий"}
	medium := Mode{model.MediumMode, "Средний"}
	high := Mode{model.HighMode, "Высокий"}
	// case 1: list with known modes
	list := []Mode{
		high,
		auto,
		low,
		medium,
	}

	sort.Sort(ModesSorting(list))

	expectedList := []Mode{
		auto,
		low,
		medium,
		high,
	}
	assert.Equal(t, expectedList, list)
	// case 2: list with 1 unknown mode
	list = []Mode{
		high,
		auto,
		low,
		medium,
		{"turbo", "Турбо"},
	}

	sort.Sort(ModesSorting(list))

	expectedList = []Mode{
		auto,
		low,
		medium,
		high,
		{"turbo", "Турбо"},
	}
	assert.Equal(t, expectedList, list)
}

func TestDeviceInfoViewByName(t *testing.T) {
	device1 := DeviceInfoView{DeviceShortInfoView: DeviceShortInfoView{Name: "lamp", ID: "1"}}
	device2 := DeviceInfoView{DeviceShortInfoView: DeviceShortInfoView{Name: "Lamp", ID: "2"}}
	device3 := DeviceInfoView{DeviceShortInfoView: DeviceShortInfoView{Name: "lamp", ID: "3"}}
	device4 := DeviceInfoView{DeviceShortInfoView: DeviceShortInfoView{Name: "Lamp_UNIQUE", ID: "4"}}
	listProper := []DeviceInfoView{
		device1,
		device2,
		device3,
		device4,
	}
	expectedList := []DeviceInfoView{
		device1,
		device2,
		device3,
		device4,
	}
	rand.Shuffle(len(listProper), func(i, j int) {
		listProper[i], listProper[j] = listProper[j], listProper[i]
	})
	sort.Sort(deviceInfoViewByName(listProper))
	assert.Equal(t, expectedList, listProper)
}

func TestRoomInfoViewByName(t *testing.T) {
	room1 := RoomInfoView{Name: "room", ID: "1"}
	room2 := RoomInfoView{Name: "Room", ID: "2"}
	room3 := RoomInfoView{Name: "room", ID: "3"}
	room4 := RoomInfoView{Name: "Room_UNIQUE", ID: "4"}
	listProper := []RoomInfoView{
		room1,
		room2,
		room3,
		room4,
	}
	expectedList := []RoomInfoView{
		room1,
		room2,
		room3,
		room4,
	}
	rand.Shuffle(len(listProper), func(i, j int) {
		listProper[i], listProper[j] = listProper[j], listProper[i]
	})
	sort.Sort(roomInfoViewByName(listProper))
	assert.Equal(t, expectedList, listProper)
}

func TestUserGroupViewByName(t *testing.T) {
	userGroup1 := UserGroupView{Name: "userGroup", ID: "1"}
	userGroup2 := UserGroupView{Name: "UserGroup", ID: "2"}
	userGroup3 := UserGroupView{Name: "userGroup", ID: "3"}
	userGroup4 := UserGroupView{Name: "UserGroup_UNIQUE", ID: "4"}
	listProper := []UserGroupView{
		userGroup1,
		userGroup2,
		userGroup3,
		userGroup4,
	}
	expectedList := []UserGroupView{
		userGroup1,
		userGroup2,
		userGroup3,
		userGroup4,
	}
	rand.Shuffle(len(listProper), func(i, j int) {
		listProper[i], listProper[j] = listProper[j], listProper[i]
	})
	sort.Sort(userGroupViewByName(listProper))
	assert.Equal(t, expectedList, listProper)
}

func TestUserRoomViewByName(t *testing.T) {
	room1 := UserRoomView{Name: "userRoom", ID: "1"}
	room2 := UserRoomView{Name: "UserRoom", ID: "2"}
	room3 := UserRoomView{Name: "userRoom", ID: "3"}
	room4 := UserRoomView{Name: "UserRoom_UNIQUE", ID: "4"}
	listProper := []UserRoomView{
		room1,
		room2,
		room3,
		room4,
	}
	expectedList := []UserRoomView{
		room1,
		room2,
		room3,
		room4,
	}
	rand.Shuffle(len(listProper), func(i, j int) {
		listProper[i], listProper[j] = listProper[j], listProper[i]
	})
	sort.Sort(userRoomViewByName(listProper))
	assert.Equal(t, expectedList, listProper)
}

func TestCapabilityStateViewSorting(t *testing.T) {
	onOff := CapabilityStateView{
		Retrievable: true,
		Type:        model.OnOffCapabilityType,
		Split:       false,
		Parameters:  OnOffCapabilityParameters{},
	}

	colorSetting1 := CapabilityStateView{
		Retrievable: true,
		Type:        model.ColorSettingCapabilityType,
		Split:       false,
		Parameters: ColorSettingCapabilityParameters{
			Instance:     "color",
			InstanceName: "цвет",
		},
	}
	colorSetting2 := CapabilityStateView{
		Retrievable: true,
		Type:        model.ColorSettingCapabilityType,
		Split:       false,
		Parameters: ColorSettingCapabilityParameters{
			Instance:     "color",
			InstanceName: "цветочек",
		},
	}

	rangeCapability1 := CapabilityStateView{
		Retrievable: true,
		Type:        model.RangeCapabilityType,
		Split:       false,
		Parameters: RangeCapabilityParameters{
			Instance:     model.VolumeRangeInstance,
			InstanceName: "звук",
			Unit:         model.UnitPercent,
		},
	}
	rangeCapability2 := CapabilityStateView{
		Retrievable: true,
		Type:        model.RangeCapabilityType,
		Split:       false,
		Parameters: RangeCapabilityParameters{
			Instance:     model.BrightnessRangeInstance,
			InstanceName: "яркость",
			Unit:         model.UnitPercent,
		},
	}

	modeCapability1 := CapabilityStateView{
		Retrievable: true,
		Type:        model.ModeCapabilityType,
		Split:       false,
		Parameters: ModeCapabilityParameters{
			Instance:     model.FanSpeedModeInstance,
			InstanceName: "скорость вентиляции",
		},
	}
	modeCapability2 := CapabilityStateView{
		Retrievable: true,
		Type:        model.ModeCapabilityType,
		Split:       false,
		Parameters: ModeCapabilityParameters{
			Instance:     model.ThermostatModeInstance,
			InstanceName: "термостат",
		},
	}

	toggleCapability1 := CapabilityStateView{
		Retrievable: true,
		Type:        model.ToggleCapabilityType,
		Split:       false,
		Parameters: ToggleCapabilityParameters{
			Instance:     model.PauseToggleCapabilityInstance,
			InstanceName: "пауза",
		},
	}
	toggleCapability2 := CapabilityStateView{
		Retrievable: true,
		Type:        model.ToggleCapabilityType,
		Split:       false,
		Parameters: ToggleCapabilityParameters{
			Instance:     model.BacklightToggleCapabilityInstance,
			InstanceName: "подсветка",
		},
	}

	customButton1 := CapabilityStateView{
		Retrievable: true,
		Type:        model.CustomButtonCapabilityType,
		Split:       false,
		Parameters: CustomButtonCapabilityParameters{
			Instance:     "1_instance",
			InstanceName: "первАя кНопка",
		},
	}

	customButton2 := CapabilityStateView{
		Retrievable: true,
		Type:        model.CustomButtonCapabilityType,
		Split:       false,
		Parameters: CustomButtonCapabilityParameters{
			Instance:     "simple_instance",
			InstanceName: "Простая кнопка",
		},
	}

	rawList := []CapabilityStateView{
		customButton1,
		customButton2,
		toggleCapability1,
		onOff,
		toggleCapability2,
		rangeCapability2,
		modeCapability2,
		colorSetting1,
		rangeCapability1,
		modeCapability1,
		colorSetting2,
	}

	expectedList := []CapabilityStateView{
		onOff,
		colorSetting1,
		colorSetting2,
		rangeCapability1,
		rangeCapability2,
		modeCapability1,
		modeCapability2,
		toggleCapability1,
		toggleCapability2,
		customButton1,
		customButton2,
	}

	sort.Sort(CapabilityStateViewSorting(rawList))
	assert.Equal(t, expectedList, rawList)
}

func TestScenarioListViewSorting(t *testing.T) {
	scenarios := []ScenarioListView{
		{
			ID:       "1",
			Name:     "c",
			IsActive: true,
		},
		{
			ID:       "2",
			Name:     "z",
			IsActive: false,
		},
		{
			ID:       "3",
			Name:     "z",
			IsActive: true,
		},
		{
			ID:       "4",
			Name:     "a",
			IsActive: true,
		},
		{
			ID:       "5",
			Name:     "z",
			IsActive: true,
		},
		{
			ID:       "6",
			Name:     "a",
			IsActive: false,
		},
		{
			ID:       "7",
			Name:     "b",
			IsActive: false,
		},
		{
			ID:       "8",
			Name:     "b",
			IsActive: false,
		},
	}
	expected := []ScenarioListView{
		{
			ID:       "4",
			Name:     "a",
			IsActive: true,
		},
		{
			ID:       "1",
			Name:     "c",
			IsActive: true,
		},
		{
			ID:       "3",
			Name:     "z",
			IsActive: true,
		},
		{
			ID:       "5",
			Name:     "z",
			IsActive: true,
		},
		{
			ID:       "6",
			Name:     "a",
			IsActive: false,
		},
		{
			ID:       "7",
			Name:     "b",
			IsActive: false,
		},
		{
			ID:       "8",
			Name:     "b",
			IsActive: false,
		},
		{
			ID:       "2",
			Name:     "z",
			IsActive: false,
		},
	}

	sort.Sort(ScenarioListViewSorting(scenarios))
	assert.Equal(t, expected, scenarios)
}

func TestStereopairListPossibleResponseDeviceInfosByName(t *testing.T) {
	items := []StereopairListPossibleResponseDeviceInfo{
		{ItemInfoView: ItemInfoView{Name: "B", ID: "4"}},
		{ItemInfoView: ItemInfoView{Name: "A", ID: "3"}},
		{ItemInfoView: ItemInfoView{Name: "A", ID: "2"}},
		{ItemInfoView: ItemInfoView{Name: "C", ID: "1"}},
	}
	sort.Sort(StereopairListPossibleResponseDeviceInfosByName(items))
	expected := []StereopairListPossibleResponseDeviceInfo{
		{ItemInfoView: ItemInfoView{Name: "A", ID: "2"}},
		{ItemInfoView: ItemInfoView{Name: "A", ID: "3"}},
		{ItemInfoView: ItemInfoView{Name: "B", ID: "4"}},
		{ItemInfoView: ItemInfoView{Name: "C", ID: "1"}},
	}
	assert.Equal(t, expected, items)
}

func TestHouseholdViewSorting(t *testing.T) {
	households := []HouseholdView{
		{
			ID:        "dom-3",
			Name:      "Конура",
			IsCurrent: true,
		},
		{
			ID:   "dom-2",
			Name: "Конура 2",
		},
		{
			ID:   "dom-1",
			Name: "Конура",
		},
	}
	sort.Sort(HouseholdViewSorting(households))
	expected := []HouseholdView{
		{
			ID:        "dom-3",
			Name:      "Конура",
			IsCurrent: true,
		},
		{
			ID:   "dom-1",
			Name: "Конура",
		},
		{
			ID:   "dom-2",
			Name: "Конура 2",
		},
	}
	assert.Equal(t, expected, households)
}

func TestHouseholdInfoViewSorting(t *testing.T) {
	households := []HouseholdWithDevicesView{
		{
			HouseholdInfoView: HouseholdInfoView{
				ID:        "dom-3",
				Name:      "Конура",
				IsCurrent: true,
			},
		},
		{
			HouseholdInfoView: HouseholdInfoView{
				ID:   "dom-2",
				Name: "Конура 2",
			},
		},
		{
			HouseholdInfoView: HouseholdInfoView{
				ID:   "dom-1",
				Name: "Конура",
			},
		},
	}
	sort.Sort(HouseholdWithDevicesInfoViewSorting(households))
	expected := []HouseholdWithDevicesView{
		{
			HouseholdInfoView: HouseholdInfoView{
				ID:        "dom-3",
				Name:      "Конура",
				IsCurrent: true,
			},
		},
		{
			HouseholdInfoView: HouseholdInfoView{
				ID:   "dom-1",
				Name: "Конура",
			},
		},
		{
			HouseholdInfoView: HouseholdInfoView{
				ID:   "dom-2",
				Name: "Конура 2",
			},
		},
	}
	assert.Equal(t, expected, households)
}

func TestHouseholdDeviceConfigurationViewSorting(t *testing.T) {
	households := []HouseholdDeviceConfigurationView{
		{
			ID:   "dom-3",
			Name: "Конура",
		},
		{
			ID:   "dom-2",
			Name: "Конура 2",
		},
		{
			ID:   "dom-1",
			Name: "Конура",
		},
	}
	householdSorting := HouseholdDeviceConfigurationViewSorting{
		Households:         households,
		CurrentHouseholdID: "dom-3",
	}
	sort.Sort(&householdSorting)
	expected := []HouseholdDeviceConfigurationView{
		{
			ID:   "dom-3",
			Name: "Конура",
		},
		{
			ID:   "dom-1",
			Name: "Конура",
		},
		{
			ID:   "dom-2",
			Name: "Конура 2",
		},
	}
	assert.Equal(t, expected, householdSorting.Households)
}

func TestDeviceRoomEditViewSorting(t *testing.T) {
	devices := []DeviceRoomEditView{
		{
			ID:   "1",
			Name: "Аюшка",
			Type: model.LightDeviceType,
		},
		{
			ID:   "2",
			Name: "Аяшка",
			Type: model.LightDeviceType,
		},
		{
			ID:   "3",
			Name: "Абшка",
			Type: model.LightDeviceType,
		},
		{
			ID:   "4",
			Name: "Абшка",
			Type: model.LightDeviceType,
		},
	}
	sort.Sort(DeviceRoomEditViewSorting(devices))
	expected := []DeviceRoomEditView{
		{
			ID:   "3",
			Name: "Абшка",
			Type: model.LightDeviceType,
		},
		{
			ID:   "4",
			Name: "Абшка",
			Type: model.LightDeviceType,
		},
		{
			ID:   "1",
			Name: "Аюшка",
			Type: model.LightDeviceType,
		},
		{
			ID:   "2",
			Name: "Аяшка",
			Type: model.LightDeviceType,
		},
	}
	assert.Equal(t, expected, devices)
}

func TestRoomAvailableDevicesViewSorting(t *testing.T) {
	rooms := []RoomAvailableDevicesView{
		{
			ID:   "room-3",
			Name: "Конура",
		},
		{
			ID:   "room-2",
			Name: "Конура 2",
		},
		{
			ID:   "room-1",
			Name: "Конура",
		},
	}
	sort.Sort(RoomAvailableDevicesViewSorting(rooms))
	expected := []RoomAvailableDevicesView{
		{
			ID:   "room-1",
			Name: "Конура",
		},
		{
			ID:   "room-3",
			Name: "Конура",
		},
		{
			ID:   "room-2",
			Name: "Конура 2",
		},
	}
	assert.Equal(t, expected, rooms)
}

func TestDeviceAvailableForRoomViewSorting(t *testing.T) {
	devices := []DeviceAvailableForRoomView{
		{
			ID:   "device-3",
			Name: "Конура",
		},
		{
			ID:   "device-2",
			Name: "Конура 2",
		},
		{
			ID:   "device-1",
			Name: "Конура",
		},
	}
	sort.Sort(DeviceAvailableForRoomViewSorting(devices))
	expected := []DeviceAvailableForRoomView{
		{
			ID:   "device-1",
			Name: "Конура",
		},
		{
			ID:   "device-3",
			Name: "Конура",
		},
		{
			ID:   "device-2",
			Name: "Конура 2",
		},
	}
	assert.Equal(t, expected, devices)
}

func TestHouseholdRoomAvailableDevicesViewSorting(t *testing.T) {
	households := []HouseholdRoomAvailableDevicesView{
		{
			ID:        "dom-3",
			Name:      "Конура",
			IsCurrent: true,
		},
		{
			ID:   "dom-2",
			Name: "Конура 2",
		},
		{
			ID:   "dom-1",
			Name: "Конура",
		},
	}
	sort.Sort(HouseholdRoomAvailableDevicesViewSorting(households))
	expected := []HouseholdRoomAvailableDevicesView{
		{
			ID:        "dom-3",
			Name:      "Конура",
			IsCurrent: true,
		},
		{
			ID:   "dom-1",
			Name: "Конура",
		},
		{
			ID:   "dom-2",
			Name: "Конура 2",
		},
	}
	assert.Equal(t, expected, households)
}

func TestProviderSkillDeviceViewSorting(t *testing.T) {
	devices := []ProviderSkillDeviceView{
		{
			ID:   "device-2",
			Name: "Дачку",
		},
		{
			ID:   "device-1",
			Name: "Дачку",
		},
		{
			ID:   "device-3",
			Name: "Дачка",
		},
	}
	sort.Sort(ProviderSkillDeviceViewSorting(devices))
	expected := []ProviderSkillDeviceView{
		{
			ID:   "device-3",
			Name: "Дачка",
		},
		{
			ID:   "device-1",
			Name: "Дачку",
		},
		{
			ID:   "device-2",
			Name: "Дачку",
		},
	}
	assert.Equal(t, expected, devices)
}

func TestItemInfoViewSorting(t *testing.T) {
	items := []ItemInfoView{
		{
			ID:   "2",
			Name: "А",
		},
		{
			ID:           "3",
			Name:         "Б",
			Unconfigured: true,
		},
		{
			ID:   "1",
			Name: "А",
		},
	}
	sort.Sort(ItemInfoViewSorting(items))
	expected := []ItemInfoView{
		{
			ID:           "3",
			Name:         "Б",
			Unconfigured: true,
		},
		{
			ID:   "1",
			Name: "А",
		},
		{
			ID:   "2",
			Name: "А",
		},
	}
	assert.Equal(t, expected, items)
}

func TestRoomInfoViewV3Sorting(t *testing.T) {
	items := []RoomInfoViewV3{
		{
			ID:   "2",
			Name: "А",
		},
		{
			ID:   "3",
			Name: "Б",
		},
		{
			ID:   "1",
			Name: "А",
		},
	}
	sort.Sort(RoomInfoViewV3Sorting(items))
	expected := []RoomInfoViewV3{
		{
			ID:   "1",
			Name: "А",
		},
		{
			ID:   "2",
			Name: "А",
		},
		{
			ID:   "3",
			Name: "Б",
		},
	}
	assert.Equal(t, expected, items)
}

func TestHouseholdWithDevicesViewV3Sorting(t *testing.T) {
	households := []HouseholdWithDevicesViewV3{
		{
			HouseholdInfoView: HouseholdInfoView{
				ID:        "dom-3",
				Name:      "Конура",
				IsCurrent: true,
			},
		},
		{
			HouseholdInfoView: HouseholdInfoView{
				ID:   "dom-2",
				Name: "Конура 2",
			},
		},
		{
			HouseholdInfoView: HouseholdInfoView{
				ID:   "dom-1",
				Name: "Конура",
			},
		},
	}
	sort.Sort(HouseholdWithDevicesInfoViewV3Sorting(households))
	expected := []HouseholdWithDevicesViewV3{
		{
			HouseholdInfoView: HouseholdInfoView{
				ID:        "dom-3",
				Name:      "Конура",
				IsCurrent: true,
			},
		},
		{
			HouseholdInfoView: HouseholdInfoView{
				ID:   "dom-1",
				Name: "Конура",
			},
		},
		{
			HouseholdInfoView: HouseholdInfoView{
				ID:   "dom-2",
				Name: "Конура 2",
			},
		},
	}
	assert.Equal(t, expected, households)
}

func TestGroupStateRoomViewSorting(t *testing.T) {
	items := []GroupStateRoomView{
		{
			ID:   "2",
			Name: "А",
		},
		{
			ID:   "3",
			Name: "Б",
		},
		{
			ID:   "1",
			Name: "А",
		},
	}
	sort.Sort(GroupStateRoomViewSorting(items))
	expected := []GroupStateRoomView{
		{
			ID:   "1",
			Name: "А",
		},
		{
			ID:   "2",
			Name: "А",
		},
		{
			ID:   "3",
			Name: "Б",
		},
	}
	assert.Equal(t, expected, items)
}

func TestFavoriteScenarioAvailableViewSorting(t *testing.T) {
	items := []FavoriteScenarioAvailableView{
		{
			ID:   "2",
			Name: "А",
		},
		{
			ID:   "3",
			Name: "Б",
		},
		{
			ID:   "1",
			Name: "А",
		},
	}
	sort.Sort(FavoriteScenarioAvailableViewSorting(items))
	expected := []FavoriteScenarioAvailableView{
		{
			ID:   "1",
			Name: "А",
		},
		{
			ID:   "2",
			Name: "А",
		},
		{
			ID:   "3",
			Name: "Б",
		},
	}
	assert.Equal(t, expected, items)
}

func TestRoomFavoriteDevicesAvailableViewSorting(t *testing.T) {
	items := []RoomFavoriteDevicesAvailableView{
		{
			ID:   "2",
			Name: "А",
		},
		{
			ID:   "3",
			Name: "Б",
		},
		{
			ID:   "1",
			Name: "А",
		},
	}
	sort.Sort(RoomFavoriteDevicesAvailableViewSorting(items))
	expected := []RoomFavoriteDevicesAvailableView{
		{
			ID:   "1",
			Name: "А",
		},
		{
			ID:   "2",
			Name: "А",
		},
		{
			ID:   "3",
			Name: "Б",
		},
	}
	assert.Equal(t, expected, items)
}

func TestFavoriteDeviceAvailableViewSorting(t *testing.T) {
	items := []FavoriteDeviceAvailableView{
		{
			ID:   "2",
			Name: "А",
		},
		{
			ID:   "3",
			Name: "Б",
		},
		{
			ID:   "1",
			Name: "А",
		},
	}
	sort.Sort(FavoriteDeviceAvailableViewSorting(items))
	expected := []FavoriteDeviceAvailableView{
		{
			ID:   "1",
			Name: "А",
		},
		{
			ID:   "2",
			Name: "А",
		},
		{
			ID:   "3",
			Name: "Б",
		},
	}
	assert.Equal(t, expected, items)
}

func TestHouseholdFavoriteDevicesAvailableViewSorting(t *testing.T) {
	households := []HouseholdFavoriteDevicesAvailableView{
		{
			HouseholdFavoriteView: HouseholdFavoriteView{
				ID:        "dom-3",
				Name:      "Конура",
				IsCurrent: true,
			},
		},
		{
			HouseholdFavoriteView: HouseholdFavoriteView{
				ID:   "dom-2",
				Name: "Конура 2",
			},
		},
		{
			HouseholdFavoriteView: HouseholdFavoriteView{
				ID:   "dom-2",
				Name: "Конура",
			},
		},
	}
	sort.Sort(HouseholdFavoriteDevicesAvailableViewSorting(households))
	expected := []HouseholdFavoriteDevicesAvailableView{
		{
			HouseholdFavoriteView: HouseholdFavoriteView{
				ID:        "dom-3",
				Name:      "Конура",
				IsCurrent: true,
			},
		},
		{
			HouseholdFavoriteView: HouseholdFavoriteView{
				ID:   "dom-2",
				Name: "Конура",
			},
		},
		{
			HouseholdFavoriteView: HouseholdFavoriteView{
				ID:   "dom-2",
				Name: "Конура 2",
			},
		},
	}
	assert.Equal(t, expected, households)
}

func TestFavoritePropertyAvailableViewSorting(t *testing.T) {
	propertyA := PropertyStateView{
		Type: model.FloatPropertyType,
		Parameters: FloatPropertyParameters{
			Instance:     model.HumidityPropertyInstance,
			InstanceName: model.KnownPropertyInstanceNames[model.HumidityPropertyInstance],
			Unit:         model.UnitPercent,
		},
		State: FloatPropertyState{
			Percent: ptr.Float64(33),
			Status:  model.PS(model.WarningStatus),
			Value:   33,
		},
	}
	propertyB := PropertyStateView{
		Type: model.FloatPropertyType,
		Parameters: FloatPropertyParameters{
			Instance:     model.WaterLevelPropertyInstance,
			InstanceName: model.KnownPropertyInstanceNames[model.WaterLevelPropertyInstance],
			Unit:         model.UnitPercent,
		},
		State: FloatPropertyState{
			Percent: ptr.Float64(21),
			Status:  model.PS(model.DangerStatus),
			Value:   21,
		},
	}
	items := []FavoritePropertyAvailableView{
		{
			DeviceID:   "2",
			DeviceName: "А",
			Property:   propertyA,
		},
		{
			DeviceID:   "3",
			DeviceName: "Б",
			Property:   propertyA,
		},
		{
			DeviceID:   "1",
			DeviceName: "А",
			Property:   propertyB,
		},
		{
			DeviceID:   "1",
			DeviceName: "А",
			Property:   propertyA,
		},
	}
	sort.Sort(FavoritePropertyAvailableViewSorting(items))
	expected := []FavoritePropertyAvailableView{
		{
			DeviceID:   "1",
			DeviceName: "А",
			Property:   propertyA,
		},
		{
			DeviceID:   "2",
			DeviceName: "А",
			Property:   propertyA,
		},
		{
			DeviceID:   "1",
			DeviceName: "А",
			Property:   propertyB,
		},
		{
			DeviceID:   "3",
			DeviceName: "Б",
			Property:   propertyA,
		},
	}
	assert.Equal(t, expected, items)
}

func TestRoomFavoritePropertiesAvailableViewSorting(t *testing.T) {
	items := []RoomFavoritePropertiesAvailableView{
		{
			ID:   "2",
			Name: "А",
		},
		{
			ID:   "3",
			Name: "Б",
		},
		{
			ID:   "1",
			Name: "А",
		},
	}
	sort.Sort(RoomFavoritePropertiesAvailableViewSorting(items))
	expected := []RoomFavoritePropertiesAvailableView{
		{
			ID:   "1",
			Name: "А",
		},
		{
			ID:   "2",
			Name: "А",
		},
		{
			ID:   "3",
			Name: "Б",
		},
	}
	assert.Equal(t, expected, items)
}

func TestHouseholdFavoritePropertiesAvailableViewSorting(t *testing.T) {
	households := []HouseholdFavoritePropertiesAvailableView{
		{
			HouseholdFavoriteView: HouseholdFavoriteView{
				ID:        "dom-3",
				Name:      "Конура",
				IsCurrent: true,
			},
		},
		{
			HouseholdFavoriteView: HouseholdFavoriteView{
				ID:   "dom-2",
				Name: "Конура 2",
			},
		},
		{
			HouseholdFavoriteView: HouseholdFavoriteView{
				ID:   "dom-2",
				Name: "Конура",
			},
		},
	}
	sort.Sort(HouseholdFavoritePropertiesAvailableViewSorting(households))
	expected := []HouseholdFavoritePropertiesAvailableView{
		{
			HouseholdFavoriteView: HouseholdFavoriteView{
				ID:        "dom-3",
				Name:      "Конура",
				IsCurrent: true,
			},
		},
		{
			HouseholdFavoriteView: HouseholdFavoriteView{
				ID:   "dom-2",
				Name: "Конура",
			},
		},
		{
			HouseholdFavoriteView: HouseholdFavoriteView{
				ID:   "dom-2",
				Name: "Конура 2",
			},
		},
	}
	assert.Equal(t, expected, households)
}

func TestFavoriteListItemViewSorting(t *testing.T) {
	scenario := FavoriteListItemView{
		Type: model.ScenarioFavoriteType,
		Parameters: ScenarioListView{
			ID:   "scenario-id",
			Name: "Сценарий",
		},
	}
	device := FavoriteListItemView{
		Type: model.DeviceFavoriteType,
		Parameters: ItemInfoView{
			ID:   "device-id",
			Name: "Девайс",
		},
	}
	groupA := FavoriteListItemView{
		Type: model.GroupFavoriteType,
		Parameters: ItemInfoView{
			ID:   "group-id",
			Name: "Группа",
		},
	}
	groupB := FavoriteListItemView{
		Type: model.GroupFavoriteType,
		Parameters: ItemInfoView{
			ID:   "group-id-2",
			Name: "Группочка",
		},
	}
	list := []FavoriteListItemView{groupA, groupB, device, scenario}
	expected := []FavoriteListItemView{scenario, device, groupA, groupB}
	sort.Sort(FavoriteListItemViewSorting(list))
	assert.Equal(t, expected, list)
}

func TestFavoriteDevicePropertyListViewSorting(t *testing.T) {
	propertyA := PropertyStateView{
		Type: model.FloatPropertyType,
		Parameters: FloatPropertyParameters{
			Instance:     model.HumidityPropertyInstance,
			InstanceName: model.KnownPropertyInstanceNames[model.HumidityPropertyInstance],
			Unit:         model.UnitPercent,
		},
		State: FloatPropertyState{
			Percent: ptr.Float64(33),
			Status:  model.PS(model.WarningStatus),
			Value:   33,
		},
	}
	propertyB := PropertyStateView{
		Type: model.FloatPropertyType,
		Parameters: FloatPropertyParameters{
			Instance:     model.WaterLevelPropertyInstance,
			InstanceName: model.KnownPropertyInstanceNames[model.WaterLevelPropertyInstance],
			Unit:         model.UnitPercent,
		},
		State: FloatPropertyState{
			Percent: ptr.Float64(21),
			Status:  model.PS(model.DangerStatus),
			Value:   21,
		},
	}
	aRoomAHouseholdAProperty := FavoriteDevicePropertyListView{
		DeviceID:      "1",
		Property:      propertyA,
		RoomName:      "Комната",
		HouseholdName: "Дом",
	}
	aRoomBHouseholdAProperty := FavoriteDevicePropertyListView{
		DeviceID:      "1",
		Property:      propertyA,
		RoomName:      "Комната",
		HouseholdName: "Дача",
	}
	aRoomAHouseholdBProperty := FavoriteDevicePropertyListView{
		DeviceID:      "1",
		Property:      propertyB,
		RoomName:      "Комната",
		HouseholdName: "Дом",
	}
	list := []FavoriteDevicePropertyListView{aRoomBHouseholdAProperty, aRoomAHouseholdAProperty, aRoomAHouseholdBProperty}
	expected := []FavoriteDevicePropertyListView{aRoomBHouseholdAProperty, aRoomAHouseholdAProperty, aRoomAHouseholdBProperty}
	sort.Sort(FavoriteDevicePropertyListViewSorting(list))
	assert.Equal(t, expected, list)
}

func TestHouseholdFavoriteGroupsAvailableViewSorting(t *testing.T) {
	households := []HouseholdFavoriteGroupsAvailableView{
		{
			HouseholdFavoriteView: HouseholdFavoriteView{
				ID:        "dom-3",
				Name:      "Конура",
				IsCurrent: true,
			},
		},
		{
			HouseholdFavoriteView: HouseholdFavoriteView{
				ID:   "dom-2",
				Name: "Конура 2",
			},
		},
		{
			HouseholdFavoriteView: HouseholdFavoriteView{
				ID:   "dom-2",
				Name: "Конура",
			},
		},
	}
	sort.Sort(HouseholdFavoriteGroupsAvailableViewSorting(households))
	expected := []HouseholdFavoriteGroupsAvailableView{
		{
			HouseholdFavoriteView: HouseholdFavoriteView{
				ID:        "dom-3",
				Name:      "Конура",
				IsCurrent: true,
			},
		},
		{
			HouseholdFavoriteView: HouseholdFavoriteView{
				ID:   "dom-2",
				Name: "Конура",
			},
		},
		{
			HouseholdFavoriteView: HouseholdFavoriteView{
				ID:   "dom-2",
				Name: "Конура 2",
			},
		},
	}
	assert.Equal(t, expected, households)
}

func TestFavoriteGroupAvailableViewSorting(t *testing.T) {
	items := []FavoriteGroupAvailableView{
		{
			ID:   "2",
			Name: "А",
		},
		{
			ID:   "3",
			Name: "Б",
		},
		{
			ID:   "1",
			Name: "А",
		},
	}
	sort.Sort(FavoriteGroupAvailableViewSorting(items))
	expected := []FavoriteGroupAvailableView{
		{
			ID:   "1",
			Name: "А",
		},
		{
			ID:   "2",
			Name: "А",
		},
		{
			ID:   "3",
			Name: "Б",
		},
	}
	assert.Equal(t, expected, items)
}

func TestSpeakerNewsTopicViewSorting(t *testing.T) {
	topics := []SpeakerNewsTopicView{
		{
			ID:   model.PoliticsSpeakerNewsTopic,
			Name: "Политика",
		},
		{
			ID:   model.AutoSpeakerNewsTopic,
			Name: "Авто",
		},
		{
			ID:   model.IndexSpeakerNewsTopic,
			Name: "Главное",
		},
	}
	expected := []SpeakerNewsTopicView{
		{
			ID:   model.IndexSpeakerNewsTopic,
			Name: "Главное",
		},
		{
			ID:   model.AutoSpeakerNewsTopic,
			Name: "Авто",
		},
		{
			ID:   model.PoliticsSpeakerNewsTopic,
			Name: "Политика",
		},
	}
	sort.Sort(SpeakerNewsTopicViewSorting(topics))
	assert.Equal(t, expected, topics)
}

func TestTandemAvailableDeviceViewSorting(t *testing.T) {
	items := []TandemAvailableDeviceView{
		{
			ID:   "2",
			Name: "А",
		},
		{
			ID:   "3",
			Name: "Б",
		},
		{
			ID:   "1",
			Name: "А",
		},
	}
	sort.Sort(TandemAvailableDeviceViewSorting(items))
	expected := []TandemAvailableDeviceView{
		{
			ID:   "1",
			Name: "А",
		},
		{
			ID:   "2",
			Name: "А",
		},
		{
			ID:   "3",
			Name: "Б",
		},
	}
	assert.Equal(t, expected, items)
}

func TestPropertyStateViewSorting(t *testing.T) {
	properties := []PropertyStateView{
		{
			Type: model.FloatPropertyType,
			Parameters: FloatPropertyParameters{
				Instance:     model.CO2LevelPropertyInstance,
				InstanceName: model.KnownPropertyInstanceNames[model.CO2LevelPropertyInstance],
				Unit:         model.UnitPPM,
			},
		},
		{
			Type: model.FloatPropertyType,
			Parameters: FloatPropertyParameters{
				Instance:     model.HumidityPropertyInstance,
				InstanceName: model.KnownPropertyInstanceNames[model.HumidityPropertyInstance],
				Unit:         model.UnitPercent,
			},
		},
		{
			Type: model.EventPropertyType,
			Parameters: EventPropertyParameters{
				Instance:     model.MotionPropertyInstance,
				InstanceName: model.KnownPropertyInstanceNames[model.MotionPropertyInstance],
				Events: []model.Event{
					model.KnownEvents[model.EventKey{Instance: model.MotionPropertyInstance, Value: model.DetectedEvent}],
					model.KnownEvents[model.EventKey{Instance: model.MotionPropertyInstance, Value: model.NotDetectedEvent}],
				},
			},
		},
	}
	expected := []PropertyStateView{
		{
			Type: model.EventPropertyType,
			Parameters: EventPropertyParameters{
				Instance:     model.MotionPropertyInstance,
				InstanceName: model.KnownPropertyInstanceNames[model.MotionPropertyInstance],
				Events: []model.Event{
					model.KnownEvents[model.EventKey{Instance: model.MotionPropertyInstance, Value: model.DetectedEvent}],
					model.KnownEvents[model.EventKey{Instance: model.MotionPropertyInstance, Value: model.NotDetectedEvent}],
				},
			},
		},
		{
			Type: model.FloatPropertyType,
			Parameters: FloatPropertyParameters{
				Instance:     model.HumidityPropertyInstance,
				InstanceName: model.KnownPropertyInstanceNames[model.HumidityPropertyInstance],
				Unit:         model.UnitPercent,
			},
		},
		{
			Type: model.FloatPropertyType,
			Parameters: FloatPropertyParameters{
				Instance:     model.CO2LevelPropertyInstance,
				InstanceName: model.KnownPropertyInstanceNames[model.CO2LevelPropertyInstance],
				Unit:         model.UnitPPM,
			},
		},
	}
	sort.Sort(PropertyStateViewSorting(properties))
	assert.Equal(t, expected, properties)
}

func TestSpeakerSoundCategoryViewSorting(t *testing.T) {
	items := []SpeakerSoundCategoryView{
		{
			ID:   "2",
			Name: "А",
		},
		{
			ID:   "3",
			Name: "Б",
		},
		{
			ID:   "1",
			Name: "А",
		},
	}
	sort.Sort(SpeakerSoundCategoryViewSorting(items))
	expected := []SpeakerSoundCategoryView{
		{
			ID:   "1",
			Name: "А",
		},
		{
			ID:   "2",
			Name: "А",
		},
		{
			ID:   "3",
			Name: "Б",
		},
	}
	assert.Equal(t, expected, items)
}

func TestSpeakerSoundViewSorting(t *testing.T) {
	items := []SpeakerSoundView{
		{
			ID:   "2",
			Name: "А",
		},
		{
			ID:   "3",
			Name: "Б",
		},
		{
			ID:   "1",
			Name: "А",
		},
	}
	sort.Sort(SpeakerSoundViewSorting(items))
	expected := []SpeakerSoundView{
		{
			ID:   "1",
			Name: "А",
		},
		{
			ID:   "2",
			Name: "А",
		},
		{
			ID:   "3",
			Name: "Б",
		},
	}
	assert.Equal(t, expected, items)
}

func TestDeviceConfigureV2ViewSorting(t *testing.T) {
	items := []DeviceConfigureV2View{
		{
			DeviceConfigureView: DeviceConfigureView{
				ID:   "2",
				Name: "А",
			},
		},
		{
			DeviceConfigureView: DeviceConfigureView{
				ID:   "3",
				Name: "Б",
			},
		},
		{
			DeviceConfigureView: DeviceConfigureView{
				ID:   "1",
				Name: "А",
			},
		},
	}
	sort.Sort(DeviceConfigureV2ViewSorting(items))
	expected := []DeviceConfigureV2View{
		{
			DeviceConfigureView: DeviceConfigureView{
				ID:   "1",
				Name: "А",
			},
		},
		{
			DeviceConfigureView: DeviceConfigureView{
				ID:   "2",
				Name: "А",
			},
		},
		{
			DeviceConfigureView: DeviceConfigureView{
				ID:   "3",
				Name: "Б",
			},
		},
	}
	assert.Equal(t, expected, items)
}

func TestScenarioLaunchesHistoryViewSorting(t *testing.T) {
	items := []ScenarioLaunchesHistoryView{
		{
			LaunchTime: formatTimestamp(1.0),
			Name:       "A",
		},
		{
			LaunchTime: formatTimestamp(3.0),
			Name:       "B",
		},
		{
			LaunchTime: formatTimestamp(3.0),
			Name:       "A",
		},
	}
	sort.Sort(ScenarioLaunchesHistoryViewSorting(items))
	expected := []ScenarioLaunchesHistoryView{
		{
			LaunchTime: formatTimestamp(3.0),
			Name:       "A",
		},
		{
			LaunchTime: formatTimestamp(3.0),
			Name:       "B",
		},
		{
			LaunchTime: formatTimestamp(1.0),
			Name:       "A",
		},
	}
	assert.Equal(t, expected, items)
}

func TestHouseholdInvitationShortViewSorting(t *testing.T) {
	items := []HouseholdInvitationShortView{
		{
			ID: "1",
			Sender: SharingUserView{
				ID: 1,
			},
		},
		{
			ID: "2",
			Sender: SharingUserView{
				ID: 2,
			},
		},
		{
			ID: "1",
			Sender: SharingUserView{
				ID: 2,
			},
		},
	}
	expected := []HouseholdInvitationShortView{
		{
			ID: "1",
			Sender: SharingUserView{
				ID: 1,
			},
		},
		{
			ID: "1",
			Sender: SharingUserView{
				ID: 2,
			},
		},
		{
			ID: "2",
			Sender: SharingUserView{
				ID: 2,
			},
		},
	}
	sort.Sort(HouseholdInvitationShortViewSorting(items))
	assert.Equal(t, expected, items)
}

func TestHouseholdResidentViewSorting(t *testing.T) {
	items := []HouseholdResidentView{
		{
			SharingUserView: SharingUserView{
				DisplayName: "Марат М.",
			},
			Role: model.GuestHouseholdRole,
		},
		{
			SharingUserView: SharingUserView{
				DisplayName: "Аюка Э.",
			},
			Role: model.GuestHouseholdRole,
		},
		{
			SharingUserView: SharingUserView{
				DisplayName: "Тимофей Ш.",
			},
			Role: model.OwnerHouseholdRole,
		},
	}
	expected := []HouseholdResidentView{
		{
			SharingUserView: SharingUserView{
				DisplayName: "Тимофей Ш.",
			},
			Role: model.OwnerHouseholdRole,
		},
		{
			SharingUserView: SharingUserView{
				DisplayName: "Аюка Э.",
			},
			Role: model.GuestHouseholdRole,
		},
		{
			SharingUserView: SharingUserView{
				DisplayName: "Марат М.",
			},
			Role: model.GuestHouseholdRole,
		},
	}
	sort.Sort(HouseholdResidentViewSorting(items))
	assert.Equal(t, expected, items)
}
