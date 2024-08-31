package mobile

import (
	"sort"
	"testing"

	"a.yandex-team.ru/alice/iot/bulbasaur/model"
	"github.com/stretchr/testify/assert"
)

func TestModesByName(t *testing.T) {
	auto := Mode{model.AutoMode, "Авто"}
	cool := Mode{model.CoolMode, "Охлаждение"}
	heat := Mode{model.HeatMode, "Нагрев"}
	fanOnly := Mode{model.FanOnlyMode, "Обдув"}
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
		{model.ModeValue("turbo"), "Турбо"},
	}

	sort.Sort(ModesSorting(list))

	expectedList = []Mode{
		cool,
		heat,
		dry,
		auto,
		{model.ModeValue("turbo"), "Турбо"},
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
		{model.ModeValue("turbo"), "Турбо"},
	}

	sort.Sort(ModesSorting(list))

	expectedList = []Mode{
		auto,
		low,
		medium,
		high,
		{model.ModeValue("turbo"), "Турбо"},
	}

	assert.Equal(t, expectedList, list)
}

func TestIRBrandsByName(t *testing.T) {
	brands := []IRBrand{
		{ID: "1", Name: "Samsung"},
		{ID: "2", Name: "AEC"},
		{ID: "3", Name: "Hitachi"},
		{ID: "4", Name: "siemens"},
		{ID: "5", Name: "Sony"},
		{ID: "6", Name: "aoc"},
	}

	sort.Sort(IRBrandsByName(brands))

	expectedBrands := []IRBrand{
		{ID: "2", Name: "AEC"},
		{ID: "6", Name: "aoc"},
		{ID: "3", Name: "Hitachi"},
		{ID: "1", Name: "Samsung"},
		{ID: "4", Name: "siemens"},
		{ID: "5", Name: "Sony"},
	}

	assert.Equal(t, expectedBrands, brands)
}

func TestIRCustomButtonSortingByName(t *testing.T) {
	buttons := []IRCustomButton{
		{
			Key:  "1",
			Name: "съешь еще",
		},
		{
			Key:  "2",
			Name: "этих мягких",
		},
		{
			Key:  "3",
			Name: "французских булок",
		},
		{
			Key:  "4",
			Name: "да выпей чаю",
		},
	}

	expectedButtons := []IRCustomButton{
		{
			Key:  "4",
			Name: "да выпей чаю",
		},
		{
			Key:  "1",
			Name: "съешь еще",
		},
		{
			Key:  "3",
			Name: "французских булок",
		},
		{
			Key:  "2",
			Name: "этих мягких",
		},
	}

	sort.Sort(IRCustomButtonSortingByName(buttons))
	assert.Equal(t, expectedButtons, buttons)
}
