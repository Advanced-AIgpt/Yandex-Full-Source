package api

import (
	"encoding/json"
	"testing"

	"github.com/stretchr/testify/assert"

	"a.yandex-team.ru/alice/gamma/sdk/api"
)

func TestButtonFromProto(t *testing.T) {
	protoButton := &api.Button{Title: "123", Payload: []byte(`{"test":"bar"}`)}
	button, err := buttonFromProto(protoButton)
	if err != nil {
		t.Errorf("Bad buttonFromProto: %v", err)
	}
	protoPayload, _ := json.Marshal(map[string]interface{}{
		"test": "bar",
	})
	expectedButton := Button{
		Title:   "123",
		Payload: protoPayload,
	}
	assert.Equal(t, expectedButton, button, "Buttons should be equal")
}
