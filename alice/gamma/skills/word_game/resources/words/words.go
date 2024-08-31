package words

import (
	"encoding/json"

	"golang.org/x/xerrors"

	"a.yandex-team.ru/alice/gamma/sdk/golang/dialog"
	"a.yandex-team.ru/library/go/core/resource"
)

type RoundContent struct {
	MainWord dialoglib.Cue `json:"mainword"`
	Words    []string      `json:"words"`
}

func GetRoundContent() (rc []RoundContent, err error) {
	if err = json.Unmarshal(resource.Get("words.json"), &rc); err != nil {
		return rc, xerrors.Errorf("Resource is an invalid json resource")
	}
	return rc, nil
}
