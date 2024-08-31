package api

import (
	"encoding/json"

	"golang.org/x/xerrors"

	skillsAPI "a.yandex-team.ru/alice/gamma/sdk/api"
)

type Button struct {
	Title   string          `json:"title"`
	Payload json.RawMessage `json:"payload,omitempty"`
	URL     string          `json:"url,omitempty"`
	Hide    bool            `json:"hide"`
}

func buttonFromProto(protoButton *skillsAPI.Button) (button Button, err error) {
	if protoButton == nil {
		return button, xerrors.New("empty button proto")
	}
	button = Button{
		Title:   protoButton.Title,
		Payload: protoButton.Payload,
		URL:     protoButton.Url,
		Hide:    protoButton.Hide,
	}
	return button, nil
}

func buttonsFromProto(protoButtons []*skillsAPI.Button) (buttons []Button, err error) {
	buttons = make([]Button, len(protoButtons))
	for i, protoButton := range protoButtons {
		buttons[i], err = buttonFromProto(protoButton)
		if err != nil {
			return buttons, err
		}
	}
	return buttons, nil
}
