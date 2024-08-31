package levels

import (
	"encoding/json"

	"golang.org/x/xerrors"

	dialoglib "a.yandex-team.ru/alice/gamma/sdk/golang/dialog"
	"a.yandex-team.ru/library/go/core/resource"
)

type Word struct {
	Word  dialoglib.Cue
	Types map[int]string
}

func GetWords() (words map[string]Word, err error) {
	if err = json.Unmarshal(resource.Get("words.json"), &words); err != nil {
		return nil, xerrors.Errorf("Resource is an invalid json resource")
	}
	return
}
