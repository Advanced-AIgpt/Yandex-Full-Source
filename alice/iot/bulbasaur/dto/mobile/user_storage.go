package mobile

import (
	"encoding/json"

	"a.yandex-team.ru/alice/iot/bulbasaur/model"
	"a.yandex-team.ru/alice/library/go/timestamp"
	xerrors "a.yandex-team.ru/library/go/core/xerrors"
)

type UserStorageConfigResponse struct {
	Status    string                `json:"status"`
	RequestID string                `json:"request_id"`
	Config    UserStorageConfigView `json:"config"`
}

func (r *UserStorageConfigResponse) FromUserStorageConfig(config model.UserStorageConfig) {
	r.Config = make(UserStorageConfigView)
	for key, value := range config {
		var storageValueView UserStorageValueView
		storageValueView.FromUserStorageValue(value)
		r.Config[key] = storageValueView
	}
}

type UserStorageConfigView map[string]UserStorageValueView

type UserStorageValueView struct {
	Type  model.UserStorageValueType `json:"type"`
	Value model.IUserStorageValue    `json:"value"`
}

func (v *UserStorageValueView) UnmarshalJSON(b []byte) error {
	vRaw := struct {
		Type  model.UserStorageValueType `json:"type"`
		Value json.RawMessage            `json:"value"`
	}{}
	if err := json.Unmarshal(b, &vRaw); err != nil {
		return err
	}
	v.Type = vRaw.Type

	switch v.Type {
	case model.BoolUserStorageValueType:
		var value model.BoolUserStorageValue
		if err := json.Unmarshal(vRaw.Value, &value); err != nil {
			return xerrors.Errorf("failed to unmarshal bool storage value: %w", err)
		}
		v.Value = value
	case model.StringUserStorageValueType:
		var value model.StringUserStorageValue
		if err := json.Unmarshal(vRaw.Value, &value); err != nil {
			return xerrors.Errorf("failed to unmarshal string storage value: %w", err)
		}
		v.Value = value
	case model.FloatUserStorageValueType:
		var value model.FloatUserStorageValue
		if err := json.Unmarshal(vRaw.Value, &value); err != nil {
			return xerrors.Errorf("failed to unmarshal float storage value: %w", err)
		}
		v.Value = value
	case model.StructUserStorageValueType:
		v.Value = model.StructUserStorageValue(vRaw.Value)
	default:
		return xerrors.Errorf("unknown storage value type: %q", vRaw.Type)
	}
	return nil
}

func (v *UserStorageValueView) FromUserStorageValue(storageValue model.UserStorageValue) {
	v.Type = storageValue.Type
	v.Value = storageValue.Value.Clone()
}

func (v UserStorageValueView) ToUserStorageValue(now timestamp.PastTimestamp) model.UserStorageValue {
	return model.UserStorageValue{
		Type:    v.Type,
		Created: now,
		Updated: now,
		Value:   v.Value.Clone(),
	}
}

type UserStorageUpdateRequest map[string]UserStorageValueView

func (r UserStorageUpdateRequest) ToUserStorageConfig(now timestamp.PastTimestamp) model.UserStorageConfig {
	result := make(model.UserStorageConfig)
	for key, value := range r {
		result[key] = value.ToUserStorageValue(now)
	}
	return result
}
