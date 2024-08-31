package model

import (
	"sort"
	"testing"

	"a.yandex-team.ru/alice/library/go/tools"
	"github.com/stretchr/testify/assert"
)

func TestModesByName(t *testing.T) {
	auto := Mode{Value: AutoMode, Name: tools.AOS("Авто")}
	cool := Mode{Value: CoolMode, Name: tools.AOS("Охлаждение")}
	heat := Mode{Value: HeatMode, Name: tools.AOS("Нагрев")}
	fanOnly := Mode{Value: FanOnlyMode, Name: tools.AOS("Вентиляция")}
	dry := Mode{Value: DryMode, Name: tools.AOS("Осушение")}

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
		{Value: ModeValue("turbo"), Name: tools.AOS("Турбо")},
	}

	sort.Sort(ModesSorting(list))

	expectedList = []Mode{
		cool,
		heat,
		dry,
		auto,
		{Value: ModeValue("turbo"), Name: tools.AOS("Турбо")},
	}

	assert.Equal(t, expectedList, list)
}

func TestFanModesBySpeed(t *testing.T) {
	auto := Mode{Value: AutoMode, Name: tools.AOS("Авто")}
	low := Mode{Value: LowMode, Name: tools.AOS("Низкий")}
	medium := Mode{Value: MediumMode, Name: tools.AOS("Средний")}
	high := Mode{Value: HighMode, Name: tools.AOS("Высокий")}

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
		{Value: ModeValue("turbo"), Name: tools.AOS("Турбо")},
	}

	sort.Sort(ModesSorting(list))

	expectedList = []Mode{
		auto,
		low,
		medium,
		high,
		{Value: ModeValue("turbo"), Name: tools.AOS("Турбо")},
	}

	assert.Equal(t, expectedList, list)
}

func TestColorScenesSorting(t *testing.T) {
	scenes := ColorScenes{
		{
			ID:   ColorSceneIDFantasy,
			Name: KnownColorScenes[ColorSceneIDFantasy].Name,
		},
		{
			ID:   ColorSceneIDParty,
			Name: KnownColorScenes[ColorSceneIDParty].Name,
		},
		{
			ID:   ColorSceneIDCandle,
			Name: KnownColorScenes[ColorSceneIDCandle].Name,
		},
	}
	sort.Sort(ColorSceneSorting(scenes))
	expected := ColorScenes{
		{
			ID:   ColorSceneIDParty,
			Name: KnownColorScenes[ColorSceneIDParty].Name,
		},
		{
			ID:   ColorSceneIDCandle,
			Name: KnownColorScenes[ColorSceneIDCandle].Name,
		},
		{
			ID:   ColorSceneIDFantasy,
			Name: KnownColorScenes[ColorSceneIDFantasy].Name,
		},
	}
	assert.Equal(t, expected, scenes)
}

func TestFavoritesDevicePropertySorting(t *testing.T) {
	humidity := MakePropertyByType(FloatPropertyType)
	humidity.SetParameters(FloatPropertyParameters{
		Instance: HumidityPropertyInstance,
		Unit:     UnitPercent,
	})

	voltage := MakePropertyByType(FloatPropertyType)
	voltage.SetParameters(FloatPropertyParameters{
		Instance: VoltagePropertyInstance,
		Unit:     UnitVolt,
	})

	power := MakePropertyByType(FloatPropertyType)
	power.SetParameters(FloatPropertyParameters{
		Instance: PowerPropertyInstance,
		Unit:     UnitWatt,
	})

	properties := []FavoritesDeviceProperty{
		{
			DeviceID: "2",
			Property: power,
		},
		{
			DeviceID: "2",
			Property: humidity,
		},
		{
			DeviceID: "1",
			Property: voltage,
		},
	}
	sort.Sort(FavoritesDevicePropertySorting(properties))
	expected := []FavoritesDeviceProperty{
		{
			DeviceID: "1",
			Property: voltage,
		},
		{
			DeviceID: "2",
			Property: humidity,
		},
		{
			DeviceID: "2",
			Property: power,
		},
	}
	assert.Equal(t, expected, properties)
}

func TestSharingInfoSorting(t *testing.T) {
	infos := SharingInfos{
		{
			OwnerID:     1,
			HouseholdID: "1",
		},
		{
			OwnerID:     2,
			HouseholdID: "3",
		},
		{
			OwnerID:     2,
			HouseholdID: "2",
		},
	}

	expected := SharingInfos{
		{
			OwnerID:     1,
			HouseholdID: "1",
		},
		{
			OwnerID:     2,
			HouseholdID: "2",
		},
		{
			OwnerID:     2,
			HouseholdID: "3",
		},
	}

	sort.Sort(SharingInfoSorting(infos))
	assert.Equal(t, expected, infos)
}
