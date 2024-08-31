package sdk

import (
	"encoding/json"

	"a.yandex-team.ru/alice/library/go/libmegamind"
	"a.yandex-team.ru/library/go/core/xerrors"
)

type Callback interface {
	Name() libmegamind.CallbackName
}

func UnmarshalCallback(input Input, callback Callback) error {
	inputCallback := input.GetCallback()
	if inputCallback == nil {
		return xerrors.Errorf("failed to unmarshal callback: input callback is nil")
	}
	if inputCallback.Name != callback.Name() {
		return xerrors.Errorf("failed to unmarshal callback: name mismatch: %s != %s", inputCallback.Name, callback.Name())
	}
	return json.Unmarshal(inputCallback.Payload, callback)
}
