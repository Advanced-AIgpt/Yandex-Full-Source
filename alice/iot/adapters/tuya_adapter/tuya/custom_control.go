package tuya

import (
	"a.yandex-team.ru/alice/iot/bulbasaur/model"
	"a.yandex-team.ru/alice/library/go/tools"
)

type IRCustomControl struct {
	ID         string           `json:"id"`
	Name       string           `json:"name"`
	DeviceType model.DeviceType `json:"device_type"`
	Buttons    IRCustomButtons  `json:"buttons"`
}

type IRCustomControls []IRCustomControl

func (ircc IRCustomControls) AsMap() map[string]IRCustomControl {
	result := make(map[string]IRCustomControl)
	for _, preset := range ircc {
		result[preset.ID] = preset
	}
	return result
}

type IRCustomButton struct {
	Key  string `json:"key"`
	Name string `json:"name"`
}

type IRCustomButtons []IRCustomButton

func (ircb IRCustomButtons) Normalize() {
	for i := range ircb {
		ircb[i].Name = tools.StandardizeSpaces(ircb[i].Name)
	}
}
