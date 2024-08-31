package data

import (
	"a.yandex-team.ru/alice/iot/bulbasaur/model"
	"a.yandex-team.ru/alice/iot/bulbasaur/testing"
	"a.yandex-team.ru/alice/library/go/random"
	"a.yandex-team.ru/alice/library/go/tools"

	"strings"
)

func GenerateGroup() (group model.Group) {
	for strings.TrimSpace(group.Name) == "" {
		group.Name = tools.StandardizeSpaces(testing.RandCyrillicWithNumbersString(random.RandRange(1, 100)))
	}
	group.Aliases = make([]string, 0)

	return group
}
