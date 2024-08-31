package settings

import (
	"google.golang.org/protobuf/types/known/anypb"

	memento "a.yandex-team.ru/alice/memento/proto"
	"a.yandex-team.ru/library/go/core/xerrors"
)

type IoTResponseOptionsConfig struct {
	*memento.TIoTResponseOptions
}

func NewIoTResponseOptionsConfig() *IoTResponseOptionsConfig {
	return &IoTResponseOptionsConfig{TIoTResponseOptions: &memento.TIoTResponseOptions{}}
}

func (c *IoTResponseOptionsConfig) From(configPair *memento.TConfigKeyAnyPair) error {
	if configPair.Key != c.Key() {
		return xerrors.Errorf("invalid config key to build iot options config from: %s", configPair.Key.String())
	}
	var responseOptions memento.TIoTResponseOptions
	if err := configPair.Value.UnmarshalTo(&responseOptions); err != nil {
		return xerrors.Errorf("cannot unmarshal iot response reaction config: %w", err)
	}
	c.TIoTResponseOptions = &responseOptions
	return nil
}

func (c *IoTResponseOptionsConfig) ExtractFrom(settings UserSettings) (bool, error) {
	if settings.IoT == nil {
		return false, nil
	}
	if settings.IoT.ResponseReactionType == nil {
		return false, nil
	}
	rt, exist := modelReactionTypeToReactionTypes[*settings.IoT.ResponseReactionType]
	if !exist {
		return false, xerrors.Errorf("unknown reaction type: %s", *settings.IoT.ResponseReactionType)
	}
	c.ReactionType = rt
	return true, nil
}

func (c IoTResponseOptionsConfig) ExtractTo(settings UserSettings) (UserSettings, error) {
	if c.TIoTResponseOptions == nil {
		return settings, nil
	}
	result := settings
	if result.IoT == nil {
		result.IoT = &IoTSettings{}
	}
	rt, exist := reactionTypeToModelReactionType[c.ReactionType]
	if !exist {
		return settings, xerrors.Errorf("reaction type is not known: %s", c.ReactionType.String())
	}
	result.IoT.ResponseReactionType = &rt
	return result, nil
}

func (c IoTResponseOptionsConfig) Build() (*memento.TConfigKeyAnyPair, error) {
	serialized, err := anypb.New(c.TIoTResponseOptions)
	if err != nil {
		return nil, xerrors.Errorf("failed to serialize iot response options: %w", err)
	}
	return &memento.TConfigKeyAnyPair{
		Key:   c.Key(),
		Value: serialized,
	}, nil
}

func (c IoTResponseOptionsConfig) Key() memento.EConfigKey {
	return memento.EConfigKey_CK_IOT_RESPONSE_OPTIONS
}
