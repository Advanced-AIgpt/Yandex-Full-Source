package settings

import (
	"a.yandex-team.ru/alice/iot/bulbasaur/model"
	memento "a.yandex-team.ru/alice/memento/proto"
	"a.yandex-team.ru/library/go/ptr"
)

type GetUserObjectsRequest struct {
	*memento.TReqGetUserObjects
}

func NewGetUserObjectsRequest() GetUserObjectsRequest {
	return GetUserObjectsRequest{&memento.TReqGetUserObjects{}}
}

func (r GetUserObjectsRequest) WithScenario(name string) GetUserObjectsRequest {
	r.ScenarioKeys = append(r.ScenarioKeys, name)
	return r
}

func (r GetUserObjectsRequest) WithConfigKeys(configKeys ...memento.EConfigKey) GetUserObjectsRequest {
	r.Keys = append(r.Keys, configKeys...)
	return r
}

func (r GetUserObjectsRequest) WithDevicesKeys(devicesKeys ...*memento.TDeviceKeys) GetUserObjectsRequest {
	r.DevicesKeys = append(r.DevicesKeys, devicesKeys...)
	return r
}

func (r GetUserObjectsRequest) Build() *memento.TReqGetUserObjects {
	return r.TReqGetUserObjects
}

type ChangeUserObjectsRequest struct {
	*memento.TReqChangeUserObjects
}

func NewChangeUserObjectsRequest() ChangeUserObjectsRequest {
	return ChangeUserObjectsRequest{&memento.TReqChangeUserObjects{}}
}

func (r ChangeUserObjectsRequest) WithConfigs(configs ...*memento.TConfigKeyAnyPair) ChangeUserObjectsRequest {
	r.UserConfigs = append(r.UserConfigs, configs...)
	return r
}

func (r ChangeUserObjectsRequest) Build() *memento.TReqChangeUserObjects {
	return r.TReqChangeUserObjects
}

type UserSettings struct {
	IoT        *IoTSettings   `json:"iot,omitempty"`
	Music      *MusicSettings `json:"music,omitempty"`
	Order      *OrderSettings `json:"order,omitempty"`
	TTSWhisper *bool          `json:"tts_whisper,omitempty"`
}

func (s *UserSettings) Clone() *UserSettings {
	if s == nil {
		return nil
	}
	var clonedTTSWhisper *bool
	if s.TTSWhisper != nil {
		clonedTTSWhisper = ptr.Bool(*s.TTSWhisper)
	}
	return &UserSettings{
		IoT:        s.IoT.Clone(),
		Music:      s.Music.Clone(),
		Order:      s.Order.Clone(),
		TTSWhisper: clonedTTSWhisper,
	}
}

func (s UserSettings) IoTResponseReactionType() model.AliceResponseReactionType {
	var reactionType model.AliceResponseReactionType
	if s.IoT != nil && s.IoT.ResponseReactionType != nil {
		reactionType = *s.IoT.ResponseReactionType
	} else {
		iotSettings := getDefaultIoTSettings()
		reactionType = *iotSettings.ResponseReactionType
	}
	return reactionType
}

func DefaultSettings() UserSettings {
	defaultIoTSettings := getDefaultIoTSettings()
	defaultMusicSettings := getDefaultMusicSettings()
	defaultOrderSettings := getDefaultOrderSettings()
	return UserSettings{
		IoT:        &defaultIoTSettings,
		Music:      &defaultMusicSettings,
		Order:      &defaultOrderSettings,
		TTSWhisper: ptr.Bool(false),
	}
}

type IoTSettings struct {
	ResponseReactionType *model.AliceResponseReactionType `json:"response_reaction_type,omitempty"`
}

func (s *IoTSettings) Clone() *IoTSettings {
	if s == nil {
		return nil
	}
	var reactionType *model.AliceResponseReactionType
	if s.ResponseReactionType != nil {
		reactionTypeValue := *s.ResponseReactionType
		reactionType = &reactionTypeValue
	}
	return &IoTSettings{
		ResponseReactionType: reactionType,
	}
}

func getDefaultIoTSettings() IoTSettings {
	responseReactionType := model.SoundAliceResponseReactionType
	return IoTSettings{ResponseReactionType: &responseReactionType}
}

type MusicSettings struct {
	AnnounceTracks *bool `json:"announce_tracks,omitempty"`
}

func (s *MusicSettings) Clone() *MusicSettings {
	if s == nil {
		return nil
	}
	var announceTracks *bool
	if s.AnnounceTracks != nil {
		announceTracks = ptr.Bool(*s.AnnounceTracks)
	}
	return &MusicSettings{AnnounceTracks: announceTracks}
}

func getDefaultMusicSettings() MusicSettings {
	return MusicSettings{AnnounceTracks: ptr.Bool(false)}
}

type OrderSettings struct {
	HideItemNames *bool `json:"hide_item_names,omitempty"`
}

func (s *OrderSettings) Clone() *OrderSettings {
	if s == nil {
		return nil
	}
	var hideItemNames *bool
	if s.HideItemNames != nil {
		hideItemNames = ptr.Bool(*s.HideItemNames)
	}
	return &OrderSettings{HideItemNames: hideItemNames}
}

func getDefaultOrderSettings() OrderSettings {
	return OrderSettings{HideItemNames: ptr.Bool(false)}
}
