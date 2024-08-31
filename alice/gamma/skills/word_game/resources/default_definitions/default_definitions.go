package defaultdefinitions

import (
	"encoding/json"

	"golang.org/x/xerrors"

	"a.yandex-team.ru/library/go/core/resource"
)

func GetDefaultDefinitions() (defs map[string]string, err error) {
	if err = json.Unmarshal(resource.Get("default_definitions.json"), &defs); err != nil {
		return defs, xerrors.Errorf("Resource is an invalid json resource")
	}
	return defs, nil
}
