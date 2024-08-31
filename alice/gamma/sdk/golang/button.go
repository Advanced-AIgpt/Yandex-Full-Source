package sdk

import (
	"encoding/json"

	"a.yandex-team.ru/alice/gamma/sdk/api"
)

type Button struct {
	Title   string
	Payload interface{}
	URL     string
	Hide    bool
}

func (button *Button) toProto() (protoButton *api.Button, err error) {
	protoButton = &api.Button{
		Title: button.Title,
		Url:   button.URL,
		Hide:  button.Hide,
	}
	if button.Payload != nil {
		protoButton.Payload, err = json.Marshal(button.Payload)
		if err != nil {
			return nil, err
		}
	}
	return protoButton, nil
}

func buttonsToProto(buttons []Button) (protoButtons []*api.Button, err error) {
	protoButtons = make([]*api.Button, len(buttons))
	for i, button := range buttons {
		protoButtons[i], err = button.toProto()
		if err != nil {
			return protoButtons, err
		}
	}
	return protoButtons, nil
}
