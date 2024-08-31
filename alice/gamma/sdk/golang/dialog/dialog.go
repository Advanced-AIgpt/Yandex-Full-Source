package dialoglib

import (
	"strings"

	sdk "a.yandex-team.ru/alice/gamma/sdk/golang"
)

type Dialog struct {
	text       []string
	voice      []string
	buttons    []sdk.Button
	endSession bool
	card       *sdk.Card
}

func (dialog *Dialog) EndSession() {
	dialog.endSession = true
}

func (dialog *Dialog) AddButtons(buttons ...sdk.Button) {
	dialog.buttons = append(dialog.buttons, buttons...)
}

func (dialog *Dialog) Say(cue *Cue) {
	if len(cue.Text) != 0 {
		dialog.text = append(dialog.text, cue.Text)
	}
	if len(cue.Voice) != 0 {
		dialog.voice = append(dialog.voice, cue.Voice)
	}
}

func (dialog *Dialog) SayTemplate(cueTemplate *CueTemplate, args interface{}) error {
	cue, err := cueTemplate.Execute(args)
	if err != nil {
		return err
	}
	dialog.Say(cue)
	return nil
}

func (dialog *Dialog) SetCard(card *sdk.Card) {
	dialog.card = card
}

func (dialog *Dialog) BuildResponse() (response *sdk.Response, err error) {
	response = &sdk.Response{
		Text:       strings.Join(dialog.text, "\n"),
		Tts:        strings.Join(dialog.voice, "\n"),
		Buttons:    dialog.buttons,
		EndSession: dialog.endSession,
		Card:       dialog.card,
	}
	return response, nil
}
