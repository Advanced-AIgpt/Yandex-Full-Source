package settings

import (
	memento "a.yandex-team.ru/alice/memento/proto"
	"a.yandex-team.ru/alice/protos/data/scenario/music"
	"a.yandex-team.ru/library/go/core/xerrors"
	"a.yandex-team.ru/library/go/ptr"
	"google.golang.org/protobuf/types/known/anypb"
)

type MusicConfig struct {
	*music.TUserConfig
}

func NewMusicConfig() *MusicConfig {
	return &MusicConfig{TUserConfig: &music.TUserConfig{}}
}

func (c *MusicConfig) From(configPair *memento.TConfigKeyAnyPair) error {
	if configPair.Key != c.Key() {
		return xerrors.Errorf("invalid config key to build music config from: %s", configPair.Key.String())
	}
	var musicConfig music.TUserConfig
	if err := configPair.Value.UnmarshalTo(&musicConfig); err != nil {
		return xerrors.Errorf("cannot unmarshal music config: %v", err)
	}
	c.TUserConfig = &musicConfig
	return nil
}

func (c *MusicConfig) ExtractFrom(settings UserSettings) (bool, error) {
	if settings.Music == nil {
		return false, nil
	}
	if settings.Music.AnnounceTracks == nil {
		return false, nil
	}
	c.AnnounceTracks = *settings.Music.AnnounceTracks
	return true, nil
}

func (c MusicConfig) ExtractTo(settings UserSettings) (UserSettings, error) {
	result := settings.Clone()
	result.Music = &MusicSettings{AnnounceTracks: ptr.Bool(c.AnnounceTracks)}
	return *result, nil
}

func (c MusicConfig) Build() (*memento.TConfigKeyAnyPair, error) {
	serialized, err := anypb.New(c.TUserConfig)
	if err != nil {
		return nil, xerrors.Errorf("failed to serialize music config: %v", err)
	}
	return &memento.TConfigKeyAnyPair{
		Key:   c.Key(),
		Value: serialized,
	}, nil
}

func (c MusicConfig) Key() memento.EConfigKey {
	return memento.EConfigKey_CK_MUSIC
}
