package dialoglib

import (
	"testing"

	"github.com/stretchr/testify/assert"

	sdk "a.yandex-team.ru/alice/gamma/sdk/golang"
)

func TestDialogManager_BuildResponse(t *testing.T) {
	dialog := Dialog{}
	cueTemplates := []*CueTemplate{
		{Text: "Чтож, тестируем!", Voice: "Штош, тестируем!"},
		{Text: "Начали"},
		{Text: "И хваатит", Voice: "И хва+атит"},
	}
	buttons := []sdk.Button{
		{Title: "Cool button title here"},
	}

	for _, cueTemplate := range cueTemplates {
		cue, err := cueTemplate.Execute(nil)
		assert.NoError(t, err)
		dialog.Say(cue)
	}
	dialog.AddButtons(buttons...)

	expectedResponse := &sdk.Response{
		Text:    "Чтож, тестируем!\nНачали\nИ хваатит",
		Tts:     "Штош, тестируем!\nИ хва+атит",
		Buttons: buttons,
	}
	response, err := dialog.BuildResponse()
	assert.NoError(t, err)
	assert.Equal(t, expectedResponse, response)
}

func TestDialogManager_BuildResponseWithArgs(t *testing.T) {
	dialog := Dialog{}
	args := map[string]interface{}{
		"TextNumber": 42,
		"TtsNumber":  70,
		"Name":       "Corvo Attano",
	}

	cues := []CueTemplate{
		{Text: "Число в тексте {{.TextNumber}}", Voice: "Число в голосе {{.TtsNumber}}"},
		{Text: "Имя {{.Name}}"},
	}

	for i := range cues {
		assert.NoError(t, cues[i].Compile())
		resultCue, err := cues[i].Execute(args)
		assert.NoError(t, err)
		dialog.Say(resultCue)
	}

	expectedResponse := &sdk.Response{
		Text: "Число в тексте 42\nИмя Corvo Attano",
		Tts:  "Число в голосе 70",
	}
	response, err := dialog.BuildResponse()
	assert.NoError(t, err)
	assert.Equal(t, expectedResponse, response)
}
