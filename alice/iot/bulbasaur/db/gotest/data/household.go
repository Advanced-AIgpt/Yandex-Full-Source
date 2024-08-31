package data

import (
	"math/rand"
	"strings"

	"a.yandex-team.ru/alice/iot/bulbasaur/model"
	"a.yandex-team.ru/alice/iot/bulbasaur/testing"
	"a.yandex-team.ru/alice/library/go/random"
)

func GenerateHousehold(name string) model.Household {
	result := model.Household{
		Name:     name,
		Location: generateHouseholdLocation(),
	}
	for strings.TrimSpace(result.Name) == "" {
		result.Name = testing.RandOnlyCyrillicString(random.RandRange(10, 20))
	}
	return result
}

func generateHouseholdLocation() *model.HouseholdLocation {
	address := testing.RandCyrillicWithNumbersString(random.RandRange(30, 100))
	return &model.HouseholdLocation{
		Longitude:    rand.Float64(),
		Latitude:     rand.Float64(),
		Address:      address,
		ShortAddress: address[:20],
	}
}
