package dictionary

import (
	"encoding/json"

	"golang.org/x/xerrors"

	"a.yandex-team.ru/library/go/core/resource"
)

func GetDictionary() (vc []string, err error) {
	if err = json.Unmarshal(resource.Get("dictionary.json"), &vc); err != nil {
		return vc, xerrors.Errorf("Resource is an invalid json resource")
	}
	return vc, nil
}
