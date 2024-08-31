package animals

import (
	"encoding/json"

	"golang.org/x/xerrors"

	dialoglib "a.yandex-team.ru/alice/gamma/sdk/golang/dialog"
	"a.yandex-team.ru/library/go/core/resource"
)

type Property struct {
	Value interface{}   `json:"value"`
	Cue   dialoglib.Cue `json:"cue"`
}

type Animal struct {
	Properties   map[string]Property `json:"properties"`
	ID           string              `json:"id"`
	Name         dialoglib.Cue       `json:"name"`
	GenitiveForm dialoglib.Cue       `json:"genitive"`
}

func (animal *Animal) HasProperty(property string) bool {
	_, ok := animal.Properties[property]
	return ok
}

func GetAnimals() (animals []Animal, err error) {
	if err = json.Unmarshal(resource.Get("animals.json"), &animals); err != nil {
		return nil, xerrors.Errorf("Resource is an invalid json resource")
	}
	return
}

func GetPatterns() (patterns map[string]map[string][]string, err error) {
	if err = json.Unmarshal(resource.Get("patterns.json"), &patterns); err != nil {
		return nil, xerrors.Errorf("Resource is an invalid json resource")
	}
	return
}

func GetParts() map[string]Property {
	return map[string]Property{
		"Wings": {Cue: dialoglib.Cue{Text: "крылья", Voice: "крылья"}, Value: "0"},
		"Fins":  {Cue: dialoglib.Cue{Text: "плавники", Voice: "плавники"}, Value: "1"},
		"Horns": {Cue: dialoglib.Cue{Text: "рога", Voice: "рог+а"}, Value: "2"},
		"Hoof":  {Cue: dialoglib.Cue{Text: "копыта", Voice: "копыта"}, Value: "3"},
		"Penny": {Cue: dialoglib.Cue{Text: "пятачок", Voice: "пятачок"}, Value: "4"},
		"Tusks": {Cue: dialoglib.Cue{Text: "бивни", Voice: "бивни"}, Value: "6"},
	}
}
