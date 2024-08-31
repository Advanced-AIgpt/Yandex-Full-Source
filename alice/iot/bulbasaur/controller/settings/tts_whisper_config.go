package settings

import (
	memento "a.yandex-team.ru/alice/memento/proto"
	"a.yandex-team.ru/library/go/core/xerrors"
	"a.yandex-team.ru/library/go/ptr"
	"google.golang.org/protobuf/types/known/anypb"
)

type TTSWhisperConfig struct {
	*memento.TTtsWhisperConfig
}

func NewTTSWhisperConfig() *TTSWhisperConfig {
	return &TTSWhisperConfig{TTtsWhisperConfig: &memento.TTtsWhisperConfig{}}
}

func (c *TTSWhisperConfig) From(configPair *memento.TConfigKeyAnyPair) error {
	if configPair.Key != c.Key() {
		return xerrors.Errorf("invalid config key to build tts whisper config from: %s", configPair.Key.String())
	}
	var ttsWhisper memento.TTtsWhisperConfig
	if err := configPair.Value.UnmarshalTo(&ttsWhisper); err != nil {
		return xerrors.Errorf("cannot unmarshal tts whisper config: %v", err)
	}
	c.TTtsWhisperConfig = &ttsWhisper
	return nil
}

func (c *TTSWhisperConfig) ExtractFrom(settings UserSettings) (bool, error) {
	if settings.TTSWhisper == nil {
		return false, nil
	}
	c.Enabled = *settings.TTSWhisper
	return true, nil
}

func (c TTSWhisperConfig) ExtractTo(settings UserSettings) (UserSettings, error) {
	result := settings.Clone()
	result.TTSWhisper = ptr.Bool(c.Enabled)
	return *result, nil
}

func (c TTSWhisperConfig) Build() (*memento.TConfigKeyAnyPair, error) {
	serialized, err := anypb.New(c.TTtsWhisperConfig)
	if err != nil {
		return nil, xerrors.Errorf("failed to serialize tts whisper options: %v", err)
	}
	return &memento.TConfigKeyAnyPair{
		Key:   c.Key(),
		Value: serialized,
	}, nil
}

func (c TTSWhisperConfig) Key() memento.EConfigKey {
	return memento.EConfigKey_CK_TTS_WHISPER
}
