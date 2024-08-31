package data

import (
	"a.yandex-team.ru/alice/iot/adapters/tuya_adapter/tuya"
	"a.yandex-team.ru/alice/iot/bulbasaur/model"
	"a.yandex-team.ru/alice/iot/bulbasaur/testing"
	"a.yandex-team.ru/alice/library/go/random"
)

func GenerateIRCustomControl() tuya.IRCustomControl {
	control := tuya.IRCustomControl{
		ID:         testing.RandLatinString(20),
		Name:       testing.RandCyrillicWithNumbersString(20),
		DeviceType: model.OtherDeviceType,
	}

	buttons := make([]tuya.IRCustomButton, 0)
	amount := random.FlipCube(3) + 1
	for len(buttons) < amount {
		checkButton := GenerateIRCustomButton()
		shouldGen := false
		for _, button := range buttons {
			if checkButton.Key == button.Key || checkButton.Name == button.Name {
				shouldGen = true
				break
			}
		}
		if !shouldGen {
			buttons = append(buttons, checkButton)
		}
	}
	control.Buttons = buttons
	return control
}

func GenerateIRCustomButton() tuya.IRCustomButton {
	return tuya.IRCustomButton{
		Key:  testing.RandLatinString(20),
		Name: testing.RandCyrillicWithNumbersString(20),
	}
}
