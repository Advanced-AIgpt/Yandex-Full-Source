package levels

import (
	"encoding/json"

	"golang.org/x/xerrors"

	"a.yandex-team.ru/library/go/core/resource"
)

type Type struct {
	Name       string
	PluralForm string
	Words      []string
}

type Level struct {
	Types        []Type
	PairsOfTypes [][]int
}

func GetLevels() (levels []Level, err error) {
	if err = json.Unmarshal(resource.Get("levels.json"), &levels); err != nil {
		return nil, xerrors.Errorf("Resource is an invalid json resource")
	}
	for i := range levels {
		for j := 0; j < len(levels[i].Types); j++ {
			for k := j + 1; k < len(levels[i].Types); k++ {
				levels[i].PairsOfTypes = append(levels[i].PairsOfTypes, []int{j, k})
			}
		}
	}
	return
}
