package mobile

import (
	"encoding/json"
	"testing"

	"a.yandex-team.ru/alice/iot/adapters/tuya_adapter/tuya"
	"a.yandex-team.ru/alice/iot/bulbasaur/model"
	"a.yandex-team.ru/alice/library/go/tools"
	"a.yandex-team.ru/library/go/valid"
	"github.com/stretchr/testify/assert"
)

func TestIrAcAction_ToAcStateView_FromJSON(t *testing.T) {
	jsonFromMobile := `{"preset_id":"7422","power":true,"temperature":18,"mode":"heat","fan_speed":"auto"}`
	var actionsView IrAcAction
	err := json.Unmarshal([]byte(jsonFromMobile), &actionsView)
	if err != nil {
		t.Error(err)
	}

	converted, err := actionsView.ToAcStateView()
	if err != nil {
		t.Error(err)
	}

	expectedView := tuya.AcStateView{
		RemoteIndex: "7422",
		Power:       tools.AOS("1"),
		Mode:        tools.AOS("1"),
		Wind:        tools.AOS("0"),
		Temp:        tools.AOS("18"),
	}

	assert.Equal(t, expectedView, converted)
}

func TestIrAcAction_ToAcStateView(t *testing.T) {
	// case 1: full IrAcAction
	fullActions := IrAcAction{
		Power:        true,
		PresetID:     "1122",
		Mode:         tools.AOS("heat"),
		FanSpeedMode: tools.AOS("auto"),
		Temperature:  tools.AOI(22),
	}

	expectedFullView := tuya.AcStateView{
		RemoteIndex: "1122",
		Power:       tools.AOS("1"),
		Mode:        tools.AOS("1"),
		Wind:        tools.AOS("0"),
		Temp:        tools.AOS("22"),
	}

	allConverted, err := fullActions.ToAcStateView()
	if err != nil {
		t.Error(err)
	}

	assert.Equal(t, expectedFullView, allConverted)

	// case 2: no fan IrAcAction
	noFanActions := IrAcAction{
		Power:       true,
		PresetID:    "1122",
		Mode:        tools.AOS("auto"),
		Temperature: tools.AOI(22),
	}

	expectedNoFanView := tuya.AcStateView{
		RemoteIndex: "1122",
		Power:       tools.AOS("1"),
		Mode:        tools.AOS("2"),
		Temp:        tools.AOS("22"),
	}

	noFanConverted, err := noFanActions.ToAcStateView()
	if err != nil {
		t.Error(err)
	}

	assert.Equal(t, expectedNoFanView, noFanConverted)
	assert.Nil(t, noFanConverted.Wind)

	// case 3: unsupported mode
	errorActions := IrAcAction{
		Power:       true,
		PresetID:    "1122",
		Mode:        tools.AOS("hell"),
		Temperature: tools.AOI(22),
	}

	_, err = errorActions.ToAcStateView()
	assert.EqualError(t, err, "unsupported ac work mode value \"hell\": INVALID_VALUE")

	// case 4: remove temperature param in dry mode
	for modeValue, modeTuyaID := range map[string]string{"dry": "4", "fan_only": "3"} {
		actions := IrAcAction{
			Power:        true,
			PresetID:     "1122",
			Mode:         tools.AOS(modeValue),
			FanSpeedMode: tools.AOS("auto"),
			Temperature:  tools.AOI(22),
		}

		expectedView := tuya.AcStateView{
			RemoteIndex: "1122",
			Power:       tools.AOS("1"),
			Mode:        tools.AOS(modeTuyaID),
			Wind:        tools.AOS("0"),
			Temp:        tools.AOS("22"),
		}

		converted, err := actions.ToAcStateView()
		if err != nil {
			t.Error(err)
		}

		assert.Equal(t, expectedView, converted)
	}
}

func TestIRCustomButtonName_Validate(t *testing.T) {
	name := IRCustomButtonName("большая красная кнопка")

	irButtons := []tuya.IRCustomButton{
		{
			Key:  "somebody once told me",
			Name: "маленькая синенькая",
		},
		{
			Key:  "The world is gonna roll me",
			Name: "БОЛЬШАЯ КРАСНАЯ КНОПКА!",
		},
	}

	vctx := valid.NewValidationCtx()
	vctx.Add(IRCustomButtonExistingNameValidator, IRCustomButtonExistingNameValidatorFunc(irButtons))

	_, err := name.Validate(vctx)
	verrs, ok := err.(valid.Errors)
	assert.True(t, ok)
	assert.True(t, verrs.Has(&tuya.ErrCustomButtonNameIsTaken{}))

	name = "небольшая кнопка"
	_, err = name.Validate(vctx)
	assert.NoError(t, err)
}

func TestIRCustomButtonConfigurationView(t *testing.T) {
	customControl := tuya.IRCustomControl{
		ID:         "1",
		Name:       "Мой пультик",
		DeviceType: model.OtherDeviceType,
		Buttons: []tuya.IRCustomButton{
			{
				Key:  "1",
				Name: "съешь еще",
			},
			{
				Key:  "2",
				Name: "этих мягких",
			},
			{
				Key:  "3",
				Name: "французских булок",
			},
			{
				Key:  "4",
				Name: "да выпей чаю",
			},
		},
	}

	var configurationView IRCustomControlConfigurationView
	configurationView.FromIRCustomControl(customControl)

	expectedConfigurationView := IRCustomControlConfigurationView{
		ID:         "1",
		Name:       "Мой пультик",
		DeviceType: model.OtherDeviceType,
		Buttons: []IRCustomButton{
			{
				Key:  "4",
				Name: "да выпей чаю",
			},
			{
				Key:  "1",
				Name: "съешь еще",
			},
			{
				Key:  "3",
				Name: "французских булок",
			},
			{
				Key:  "2",
				Name: "этих мягких",
			},
		},
	}

	assert.Equal(t, expectedConfigurationView, configurationView)
}
