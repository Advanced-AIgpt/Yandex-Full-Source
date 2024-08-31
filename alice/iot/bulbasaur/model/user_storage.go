package model

import (
	"encoding/json"

	"a.yandex-team.ru/alice/library/go/timestamp"
	"a.yandex-team.ru/library/go/core/xerrors"
)

type UserStorageValueType string

const (
	BoolUserStorageValueType   UserStorageValueType = "storage.value.bool"
	StringUserStorageValueType UserStorageValueType = "storage.value.string"
	FloatUserStorageValueType  UserStorageValueType = "storage.value.float"
	StructUserStorageValueType UserStorageValueType = "storage.value.struct"
)

type UserStorageValue struct {
	Type    UserStorageValueType    `json:"type"`
	Created timestamp.PastTimestamp `json:"created"`
	Updated timestamp.PastTimestamp `json:"updated"`
	Value   IUserStorageValue       `json:"value"`
}

func (v UserStorageValue) Merge(userStorageValue UserStorageValue) (UserStorageValue, error) {
	if v.Type != userStorageValue.Type {
		return UserStorageValue{}, xerrors.Errorf("incompatible types to merge: has %s and %s", v.Type, userStorageValue.Type)
	}
	if v.Value == nil || userStorageValue.Value == nil {
		return UserStorageValue{}, xerrors.New("cannot merge nil storage values")
	}
	if v.Updated > userStorageValue.Updated {
		return v.Clone(), nil
	}
	return userStorageValue.Clone(), nil
}

func (v UserStorageValue) Clone() UserStorageValue {
	return UserStorageValue{
		Type:    v.Type,
		Created: v.Created,
		Updated: v.Updated,
		Value:   v.Value.Clone(),
	}
}

func (v *UserStorageValue) UnmarshalJSON(b []byte) error {
	vRaw := rawUserStorageValue{}
	if err := json.Unmarshal(b, &vRaw); err != nil {
		return err
	}
	v.Created = vRaw.Created
	v.Updated = vRaw.Updated
	v.Type = vRaw.Type

	switch v.Type {
	case BoolUserStorageValueType:
		var value BoolUserStorageValue
		if err := json.Unmarshal(vRaw.Value, &value); err != nil {
			return xerrors.Errorf("failed to unmarshal bool storage value: %w", err)
		}
		v.Value = value
	case StringUserStorageValueType:
		var value StringUserStorageValue
		if err := json.Unmarshal(vRaw.Value, &value); err != nil {
			return xerrors.Errorf("failed to unmarshal string storage value: %w", err)
		}
		v.Value = value
	case FloatUserStorageValueType:
		var value FloatUserStorageValue
		if err := json.Unmarshal(vRaw.Value, &value); err != nil {
			return xerrors.Errorf("failed to unmarshal float storage value: %w", err)
		}
		v.Value = value
	case StructUserStorageValueType:
		v.Value = StructUserStorageValue(vRaw.Value)
	default:
		return xerrors.Errorf("unknown storage value type: %q", vRaw.Type)
	}
	return nil
}

type rawUserStorageValue struct {
	Type    UserStorageValueType    `json:"type"`
	Created timestamp.PastTimestamp `json:"created"`
	Updated timestamp.PastTimestamp `json:"updated"`
	Value   json.RawMessage         `json:"value"`
}

type IUserStorageValue interface {
	isUserStorageValue()
	Clone() IUserStorageValue
}

type BoolUserStorageValue bool

func (v BoolUserStorageValue) isUserStorageValue() {}

func (v BoolUserStorageValue) Clone() IUserStorageValue {
	return v
}

type StringUserStorageValue string

func (v StringUserStorageValue) isUserStorageValue() {}

func (v StringUserStorageValue) Clone() IUserStorageValue {
	return v
}

type FloatUserStorageValue float64

func (v FloatUserStorageValue) isUserStorageValue() {}

func (v FloatUserStorageValue) Clone() IUserStorageValue {
	return v
}

type StructUserStorageValue json.RawMessage

func (v StructUserStorageValue) isUserStorageValue() {}

func (v StructUserStorageValue) Clone() IUserStorageValue {
	return append(StructUserStorageValue(nil), v...)
}

func (v StructUserStorageValue) MarshalJSON() ([]byte, error) {
	res := json.RawMessage(v)
	return json.Marshal(&res)
}

type UserStorageConfig map[string]UserStorageValue

func (c UserStorageConfig) MergeConfig(other UserStorageConfig) (UserStorageConfig, error) {
	result := make(UserStorageConfig)
	for key, storageValue := range c {
		result[key] = storageValue.Clone()
	}
	for key, storageValue := range other {
		valueInResult, exist := result[key]
		if !exist {
			result[key] = storageValue.Clone()
			continue
		}
		mergedValue, err := valueInResult.Merge(storageValue)
		if err != nil {
			return nil, xerrors.Errorf("failed to merge user storage config: %w", err)
		}
		result[key] = mergedValue
	}
	return result, nil
}

func (c UserStorageConfig) AddValue(key string, storageValue UserStorageValue) error {
	valueInConfig, exist := c[key]
	if !exist {
		c[key] = storageValue
		return nil
	}
	mergedValue, err := valueInConfig.Merge(storageValue)
	if err != nil {
		return xerrors.Errorf("failed to add value to user storage config: %w", err)
	}
	c[key] = mergedValue
	return nil
}
