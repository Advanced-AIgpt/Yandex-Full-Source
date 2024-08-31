package model

import (
	"fmt"

	"golang.org/x/exp/slices"

	"a.yandex-team.ru/alice/iot/bulbasaur/protos"
	"a.yandex-team.ru/alice/megamind/protos/common"
	"a.yandex-team.ru/alice/megamind/protos/scenarios"
	"a.yandex-team.ru/library/go/core/xerrors"
	"a.yandex-team.ru/library/go/valid"
)

type QuasarCapabilityValue interface {
	isQuasarCapabilityValue()
	ToTypedSemanticFrame() *common.TTypedSemanticFrame
	Validate(vctx *valid.ValidationCtx) (bool, error)
	HasResultTTS() bool
}

type quasarCapabilityValue struct{}

func (quasarCapabilityValue) isQuasarCapabilityValue() {}

type WeatherQuasarCapabilityValue struct {
	quasarCapabilityValue
	Where     *HouseholdLocation    `json:"where,omitempty"`
	Household *WeatherHouseholdInfo `json:"household,omitempty"`
}

func (v WeatherQuasarCapabilityValue) toProto() *protos.QuasarCapabilityState_Weather {
	var where *protos.QuasarCapabilityState_WeatherValue_Location
	if v.Where != nil {
		where = &protos.QuasarCapabilityState_WeatherValue_Location{
			Longitude:    v.Where.Longitude,
			Latitude:     v.Where.Latitude,
			Address:      v.Where.Address,
			ShortAddress: v.Where.ShortAddress,
		}
	}
	var householdInfo *protos.QuasarCapabilityState_WeatherValue_HouseholdInfo
	if v.Household != nil {
		householdInfo = &protos.QuasarCapabilityState_WeatherValue_HouseholdInfo{
			Id:   v.Household.ID,
			Name: v.Household.Name,
		}
	}
	return &protos.QuasarCapabilityState_Weather{
		Weather: &protos.QuasarCapabilityState_WeatherValue{
			Where:     where,
			Household: householdInfo,
		},
	}
}

func (v WeatherQuasarCapabilityValue) toUserInfoProto() *common.TIoTUserInfo_TCapability_TQuasarCapabilityState_WeatherValue {
	var where *common.TIoTUserInfo_TCapability_TQuasarCapabilityState_TWeatherValue_TLocation
	if v.Where != nil {
		where = &common.TIoTUserInfo_TCapability_TQuasarCapabilityState_TWeatherValue_TLocation{
			Longitude:    v.Where.Longitude,
			Latitude:     v.Where.Latitude,
			Address:      v.Where.Address,
			ShortAddress: v.Where.ShortAddress,
		}
	}
	var householdInfo *common.TIoTUserInfo_TCapability_TQuasarCapabilityState_TWeatherValue_THouseholdInfo
	if v.Household != nil {
		householdInfo = &common.TIoTUserInfo_TCapability_TQuasarCapabilityState_TWeatherValue_THouseholdInfo{
			Id:   v.Household.ID,
			Name: v.Household.Name,
		}
	}

	return &common.TIoTUserInfo_TCapability_TQuasarCapabilityState_WeatherValue{
		WeatherValue: &common.TIoTUserInfo_TCapability_TQuasarCapabilityState_TWeatherValue{
			Where:     where,
			Household: householdInfo,
		},
	}
}

func (v *WeatherQuasarCapabilityValue) fromUserInfoProto(weatherValue *common.TIoTUserInfo_TCapability_TQuasarCapabilityState_TWeatherValue) {
	if weatherValue.GetWhere() != nil {
		v.Where = &HouseholdLocation{
			Latitude:     weatherValue.GetWhere().GetLatitude(),
			Longitude:    weatherValue.GetWhere().GetLongitude(),
			Address:      weatherValue.GetWhere().GetAddress(),
			ShortAddress: weatherValue.GetWhere().GetShortAddress(),
		}
	}
	if weatherValue.GetHousehold() != nil {
		v.Household = &WeatherHouseholdInfo{
			ID:   weatherValue.GetHousehold().GetId(),
			Name: weatherValue.GetHousehold().GetName(),
		}
	}
}

func (v WeatherQuasarCapabilityValue) ToTypedSemanticFrame() *common.TTypedSemanticFrame {
	return &common.TTypedSemanticFrame{
		Type: &common.TTypedSemanticFrame_WeatherSemanticFrame{
			WeatherSemanticFrame: &common.TWeatherSemanticFrame{},
		},
	}
}

func (v WeatherQuasarCapabilityValue) Validate(vctx *valid.ValidationCtx) (bool, error) {
	return false, nil
}

func (v WeatherQuasarCapabilityValue) HasResultTTS() bool {
	return true
}

type WeatherHouseholdInfo struct {
	ID   string `json:"id"`
	Name string `json:"name"`
}

type VolumeQuasarCapabilityValue struct {
	quasarCapabilityValue
	Value    int  `json:"value"`
	Relative bool `json:"relative,omitempty"`
}

func (v VolumeQuasarCapabilityValue) toProto() *protos.QuasarCapabilityState_Volume {
	return &protos.QuasarCapabilityState_Volume{
		Volume: &protos.QuasarCapabilityState_VolumeValue{
			Value:    int32(v.Value),
			Relative: v.Relative,
		},
	}
}

func (v VolumeQuasarCapabilityValue) toUserInfoProto() *common.TIoTUserInfo_TCapability_TQuasarCapabilityState_VolumeValue {
	return &common.TIoTUserInfo_TCapability_TQuasarCapabilityState_VolumeValue{
		VolumeValue: &common.TIoTUserInfo_TCapability_TQuasarCapabilityState_TVolumeValue{
			Value:    int32(v.Value),
			Relative: v.Relative,
		},
	}
}

func (v *VolumeQuasarCapabilityValue) fromUserInfoProto(volumeValue *common.TIoTUserInfo_TCapability_TQuasarCapabilityState_TVolumeValue) {
	v.Value = int(volumeValue.GetValue())
	v.Relative = volumeValue.GetRelative()
}

func (v VolumeQuasarCapabilityValue) ToTypedSemanticFrame() *common.TTypedSemanticFrame {
	switch {
	case v.Relative && v.Value > 0:
		return &common.TTypedSemanticFrame{
			Type: &common.TTypedSemanticFrame_SoundLouderSemanticFrame{
				SoundLouderSemanticFrame: &common.TSoundLouderSemanticFrame{},
			},
		}
	case v.Relative && v.Value <= 0:
		return &common.TTypedSemanticFrame{
			Type: &common.TTypedSemanticFrame_SoundQuiterSemanticFrame{
				SoundQuiterSemanticFrame: &common.TSoundQuiterSemanticFrame{},
			},
		}
	default:
		return &common.TTypedSemanticFrame{
			Type: &common.TTypedSemanticFrame_SoundSetLevelSemanticFrame{
				SoundSetLevelSemanticFrame: &common.TSoundSetLevelSemanticFrame{
					Level: &common.TSoundLevelSlot{
						Value: &common.TSoundLevelSlot_NumLevelValue{NumLevelValue: int32(v.Value)},
					},
				},
			},
		}
	}
}

func (v VolumeQuasarCapabilityValue) ToDirective() *scenarios.TDirective {
	switch {
	case v.Relative && v.Value > 0:
		return &scenarios.TDirective{
			Directive: &scenarios.TDirective_SoundLouderDirective{
				SoundLouderDirective: &scenarios.TSoundLouderDirective{},
			},
		}
	case v.Relative && v.Value < 0:
		return &scenarios.TDirective{
			Directive: &scenarios.TDirective_SoundQuiterDirective{
				SoundQuiterDirective: &scenarios.TSoundQuiterDirective{},
			},
		}
	default:
		return &scenarios.TDirective{
			Directive: &scenarios.TDirective_SoundSetLevelDirective{
				SoundSetLevelDirective: &scenarios.TSoundSetLevelDirective{
					NewLevel: int32(v.Value),
				},
			},
		}
	}
}

func (v VolumeQuasarCapabilityValue) Validate(vctx *valid.ValidationCtx) (bool, error) {
	var err valid.Errors

	switch {
	case v.Relative:
		if v.Value < -10 || v.Value > 10 {
			err = append(err, fmt.Errorf("unsupported quasar capability state value: volume out of range"))
		}
	default:
		if v.Value < 0 || v.Value > 10 {
			err = append(err, fmt.Errorf("unsupported quasar capability state value: volume out of range"))
		}
	}

	if len(err) > 0 {
		return false, err
	}

	return false, nil
}

func (v VolumeQuasarCapabilityValue) HasResultTTS() bool {
	return false
}

type MusicPlayQuasarCapabilityValue struct {
	quasarCapabilityValue
	Object           *MusicPlayObject `json:"object,omitempty"`
	SearchText       string           `json:"search_text,omitempty"`
	PlayInBackground bool             `json:"play_in_background,omitempty"`
}

func (v MusicPlayQuasarCapabilityValue) toProto() *protos.QuasarCapabilityState_MusicPlay {
	value := &protos.QuasarCapabilityState_MusicPlayValue{
		PlayInBackground: v.PlayInBackground,
	}
	switch {
	case v.Object != nil:
		value.Value = &protos.QuasarCapabilityState_MusicPlayValue_Object{
			Object: &protos.QuasarCapabilityState_MusicPlayValue_MusicPlayObject{
				Id:   v.Object.ID,
				Type: string(v.Object.Type),
				Name: v.Object.Name,
			},
		}
	default:
		value.Value = &protos.QuasarCapabilityState_MusicPlayValue_SearchText{
			SearchText: v.SearchText,
		}
	}
	return &protos.QuasarCapabilityState_MusicPlay{
		MusicPlay: value,
	}
}

func (v MusicPlayQuasarCapabilityValue) toUserInfoProto() *common.TIoTUserInfo_TCapability_TQuasarCapabilityState_MusicPlayValue {
	value := &common.TIoTUserInfo_TCapability_TQuasarCapabilityState_TMusicPlayValue{
		PlayInBackground: v.PlayInBackground,
	}
	switch {
	case v.Object != nil:
		value.Value = &common.TIoTUserInfo_TCapability_TQuasarCapabilityState_TMusicPlayValue_Object{
			Object: &common.TIoTUserInfo_TCapability_TQuasarCapabilityState_TMusicPlayValue_TMusicPlayObject{
				Id:   v.Object.ID,
				Type: string(v.Object.Type),
				Name: v.Object.Name,
			},
		}
	default:
		value.Value = &common.TIoTUserInfo_TCapability_TQuasarCapabilityState_TMusicPlayValue_SearchText{
			SearchText: v.SearchText,
		}
	}
	return &common.TIoTUserInfo_TCapability_TQuasarCapabilityState_MusicPlayValue{
		MusicPlayValue: value,
	}
}

func (v *MusicPlayQuasarCapabilityValue) fromUserInfoProto(musicValue *common.TIoTUserInfo_TCapability_TQuasarCapabilityState_TMusicPlayValue) {
	if musicValue.GetObject() != nil {
		v.Object = &MusicPlayObject{
			ID:   musicValue.GetObject().GetId(),
			Name: musicValue.GetObject().GetName(),
			Type: MusicPlayObjectType(musicValue.GetObject().GetType()),
		}
	}
	v.SearchText = musicValue.GetSearchText()
	v.PlayInBackground = musicValue.GetPlayInBackground()
}

func (v MusicPlayQuasarCapabilityValue) ToTypedSemanticFrame() *common.TTypedSemanticFrame {
	frame := &common.TTypedSemanticFrame{
		Type: &common.TTypedSemanticFrame_MusicPlaySemanticFrame{
			MusicPlaySemanticFrame: &common.TMusicPlaySemanticFrame{
				PlaySingleTrack: &common.TBoolSlot{Value: &common.TBoolSlot_BoolValue{BoolValue: true}},
				DisableAutoflow: &common.TBoolSlot{Value: &common.TBoolSlot_BoolValue{BoolValue: true}},
				DisableHistory:  &common.TBoolSlot{Value: &common.TBoolSlot_BoolValue{BoolValue: true}},
				DisableNlg:      &common.TBoolSlot{Value: &common.TBoolSlot_BoolValue{BoolValue: true}},
			},
		},
	}
	if v.Object != nil {
		frame.GetMusicPlaySemanticFrame().ObjectId = &common.TStringSlot{Value: &common.TStringSlot_StringValue{StringValue: v.Object.ID}}
		frame.GetMusicPlaySemanticFrame().ObjectType = &common.TMusicPlayObjectTypeSlot{Value: &common.TMusicPlayObjectTypeSlot_EnumValue{EnumValue: mmMusicPlayObjectTypeToProtoMap[v.Object.Type]}}
		return frame
	}
	frame.GetMusicPlaySemanticFrame().SearchText = &common.TStringSlot{Value: &common.TStringSlot_StringValue{StringValue: v.SearchText}}
	return frame
}

func (v MusicPlayQuasarCapabilityValue) Validate(vctx *valid.ValidationCtx) (bool, error) {
	var err valid.Errors
	// only one of the fields should be filled
	if (v.Object != nil) == (len(v.SearchText) > 0) {
		err = append(err, xerrors.New("unsupported quasar capability state music play value: should be object or search text"))
	}

	if v.Object != nil {
		if !slices.Contains(KnownSpeakerMusicPlayObjectTypes, string(v.Object.Type)) {
			err = append(err, xerrors.Errorf("unsupported by current device quasar capability state music play object type: %q", v.Object.Type))
		}
	}
	if len(v.SearchText) > 0 {
		if e := validQuasarCapabilityValue(v.SearchText, 100); e != nil {
			err = append(err, e)
		}
	}
	if len(err) > 0 {
		return false, err
	}
	return false, nil
}

func (v MusicPlayQuasarCapabilityValue) HasResultTTS() bool {
	return true
}

type MusicPlayObjectType string

const (
	TrackMusicPlayObjectType          MusicPlayObjectType = "track"
	AlbumMusicPlayObjectType          MusicPlayObjectType = "album"
	ArtistMusicPlayObjectType         MusicPlayObjectType = "artist"
	PlaylistMusicPlayObjectType       MusicPlayObjectType = "playlist"
	RadioMusicPlayObjectType          MusicPlayObjectType = "radio"
	GenerativeMusicPlayObjectType     MusicPlayObjectType = "generative"
	PodcastMusicPlayObjectType        MusicPlayObjectType = "podcast"
	PodcastEpisodeMusicPlayObjectType MusicPlayObjectType = "podcast-episode"
)

type MusicPlayObject struct {
	ID   string              `json:"id"`
	Type MusicPlayObjectType `json:"type"`
	Name string              `json:"name"`
}

type NewsQuasarCapabilityValue struct {
	quasarCapabilityValue
	Topic            SpeakerNewsTopic `json:"topic"`
	Provider         string           `json:"provider,omitempty"`
	PlayInBackground bool             `json:"play_in_background,omitempty"`
}

func (v NewsQuasarCapabilityValue) toProto() *protos.QuasarCapabilityState_News {
	return &protos.QuasarCapabilityState_News{
		News: &protos.QuasarCapabilityState_NewsValue{
			Provider:         v.Provider,
			Topic:            string(v.Topic),
			PlayInBackground: v.PlayInBackground,
		},
	}
}

func (v NewsQuasarCapabilityValue) toUserInfoProto() *common.TIoTUserInfo_TCapability_TQuasarCapabilityState_NewsValue {
	return &common.TIoTUserInfo_TCapability_TQuasarCapabilityState_NewsValue{
		NewsValue: &common.TIoTUserInfo_TCapability_TQuasarCapabilityState_TNewsValue{
			Provider:         v.Provider,
			Topic:            string(v.Topic),
			PlayInBackground: v.PlayInBackground,
		},
	}
}

func (v *NewsQuasarCapabilityValue) fromUserInfoProto(newsValue *common.TIoTUserInfo_TCapability_TQuasarCapabilityState_TNewsValue) {
	v.Topic = SpeakerNewsTopic(newsValue.GetTopic())
	v.Provider = newsValue.GetProvider()
	v.PlayInBackground = newsValue.GetPlayInBackground()
}

func (v NewsQuasarCapabilityValue) ToTypedSemanticFrame() *common.TTypedSemanticFrame {
	return &common.TTypedSemanticFrame{
		Type: &common.TTypedSemanticFrame_NewsSemanticFrame{
			NewsSemanticFrame: &common.TNewsSemanticFrame{
				Topic:               &common.TStringSlot{Value: &common.TStringSlot_NewsTopicValue{NewsTopicValue: string(v.Topic)}},
				SkipIntroAndEnding:  &common.TBoolSlot{Value: &common.TBoolSlot_BoolValue{BoolValue: true}},
				DisableVoiceButtons: &common.TBoolSlot{Value: &common.TBoolSlot_BoolValue{BoolValue: true}},
				// slots are similar to alice show
				SingleNews: &common.TBoolSlot{Value: &common.TBoolSlot_BoolValue{BoolValue: true}},
				MaxCount:   &common.TNumSlot{Value: &common.TNumSlot_NumValue{NumValue: 1}},
				// FIXME: for now that code does not support provider
			},
		},
	}
}

func (v NewsQuasarCapabilityValue) Validate(vctx *valid.ValidationCtx) (bool, error) {
	var err valid.Errors

	if !slices.Contains(KnownSpeakerNewsTopics, string(v.Topic)) {
		err = append(err, fmt.Errorf("unsupported quasar capability state news value topic: %q", v.Topic))
	}

	if len(err) > 0 {
		return false, err
	}
	return false, nil
}

func (v NewsQuasarCapabilityValue) HasResultTTS() bool {
	return true
}

type SoundPlayQuasarCapabilityValue struct {
	quasarCapabilityValue
	Sound SpeakerSoundID `json:"sound"`
}

func (v SoundPlayQuasarCapabilityValue) toProto() *protos.QuasarCapabilityState_SoundPlay {
	return &protos.QuasarCapabilityState_SoundPlay{SoundPlay: &protos.QuasarCapabilityState_SoundPlayValue{Sound: string(v.Sound)}}
}

func (v SoundPlayQuasarCapabilityValue) toUserInfoProto() *common.TIoTUserInfo_TCapability_TQuasarCapabilityState_SoundPlayValue {
	return &common.TIoTUserInfo_TCapability_TQuasarCapabilityState_SoundPlayValue{
		SoundPlayValue: &common.TIoTUserInfo_TCapability_TQuasarCapabilityState_TSoundPlayValue{
			Sound: string(v.Sound),
		},
	}
}

func (v *SoundPlayQuasarCapabilityValue) fromUserInfoProto(soundValue *common.TIoTUserInfo_TCapability_TQuasarCapabilityState_TSoundPlayValue) {
	v.Sound = SpeakerSoundID(soundValue.GetSound())
}

func (v SoundPlayQuasarCapabilityValue) ToTypedSemanticFrame() *common.TTypedSemanticFrame {
	return &common.TTypedSemanticFrame{
		Type: &common.TTypedSemanticFrame_RepeatAfterMeSemanticFrame{
			RepeatAfterMeSemanticFrame: &common.TRepeatAfterMeSemanticFrame{
				Text:  &common.TStringSlot{Value: &common.TStringSlot_StringValue{StringValue: ""}},
				Voice: &common.TStringSlot{Value: &common.TStringSlot_StringValue{StringValue: KnownSpeakerSounds[v.Sound].Opus()}},
			},
		},
	}
}

func (v SoundPlayQuasarCapabilityValue) Validate(vctx *valid.ValidationCtx) (bool, error) {
	var err valid.Errors

	if _, exist := KnownSpeakerSounds[v.Sound]; !exist {
		err = append(err, fmt.Errorf("unsupported quasar capability state sound play value: %q", v.Sound))
	}

	if len(err) > 0 {
		return false, err
	}
	return false, nil
}

func (v SoundPlayQuasarCapabilityValue) HasResultTTS() bool {
	return true
}

type StopEverythingQuasarCapabilityValue struct {
	quasarCapabilityValue
}

func (v StopEverythingQuasarCapabilityValue) toProto() *protos.QuasarCapabilityState_StopEverything {
	return &protos.QuasarCapabilityState_StopEverything{StopEverything: &protos.QuasarCapabilityState_StopEverythingValue{}}
}

func (v StopEverythingQuasarCapabilityValue) toUserInfoProto() *common.TIoTUserInfo_TCapability_TQuasarCapabilityState_StopEverythingValue {
	return &common.TIoTUserInfo_TCapability_TQuasarCapabilityState_StopEverythingValue{
		StopEverythingValue: &common.TIoTUserInfo_TCapability_TQuasarCapabilityState_TStopEverythingValue{},
	}
}

func (v *StopEverythingQuasarCapabilityValue) fromUserInfoProto(stopValue *common.TIoTUserInfo_TCapability_TQuasarCapabilityState_TStopEverythingValue) {
}

func (v StopEverythingQuasarCapabilityValue) ToTypedSemanticFrame() *common.TTypedSemanticFrame {
	return &common.TTypedSemanticFrame{
		Type: &common.TTypedSemanticFrame_PlayerPauseSemanticFrame{
			PlayerPauseSemanticFrame: &common.TPlayerPauseSemanticFrame{},
		},
	}
}

func (v StopEverythingQuasarCapabilityValue) Validate(vctx *valid.ValidationCtx) (bool, error) {
	return false, nil
}

func (v StopEverythingQuasarCapabilityValue) HasResultTTS() bool {
	return false
}

func (v StopEverythingQuasarCapabilityValue) UnmarshalJSON(b []byte) error {
	return nil
}

func (v StopEverythingQuasarCapabilityValue) MarshalJSON() ([]byte, error) {
	return []byte("{}"), nil
}

type TTSQuasarCapabilityValue struct {
	quasarCapabilityValue
	Text string `json:"text"`
}

func (v TTSQuasarCapabilityValue) toProto() *protos.QuasarCapabilityState_TTS {
	return &protos.QuasarCapabilityState_TTS{TTS: &protos.QuasarCapabilityState_TTSValue{Text: v.Text}}
}

func (v TTSQuasarCapabilityValue) toUserInfoProto() *common.TIoTUserInfo_TCapability_TQuasarCapabilityState_TtsValue {
	return &common.TIoTUserInfo_TCapability_TQuasarCapabilityState_TtsValue{
		TtsValue: &common.TIoTUserInfo_TCapability_TQuasarCapabilityState_TTtsValue{
			Text: v.Text,
		},
	}
}

func (v *TTSQuasarCapabilityValue) fromUserInfoProto(ttsValue *common.TIoTUserInfo_TCapability_TQuasarCapabilityState_TTtsValue) {
	v.Text = ttsValue.GetText()
}

func (v TTSQuasarCapabilityValue) ToTypedSemanticFrame() *common.TTypedSemanticFrame {
	return &common.TTypedSemanticFrame{
		Type: &common.TTypedSemanticFrame_RepeatAfterMeSemanticFrame{
			RepeatAfterMeSemanticFrame: &common.TRepeatAfterMeSemanticFrame{
				Text:  &common.TStringSlot{Value: &common.TStringSlot_StringValue{StringValue: v.Text}},
				Voice: &common.TStringSlot{Value: &common.TStringSlot_StringValue{StringValue: v.Text}},
			},
		},
	}
}

func (v TTSQuasarCapabilityValue) Validate(vctx *valid.ValidationCtx) (bool, error) {
	var err valid.Errors

	if e := validQuasarCapabilityValue(v.Text, 100); e != nil {
		err = append(err, e)
	}

	if len(err) > 0 {
		return false, err
	}

	return false, nil
}

func (v TTSQuasarCapabilityValue) HasResultTTS() bool {
	return true
}

type AliceShowQuasarCapabilityValue struct {
	quasarCapabilityValue
}

func (v AliceShowQuasarCapabilityValue) toProto() *protos.QuasarCapabilityState_AliceShow {
	return &protos.QuasarCapabilityState_AliceShow{AliceShow: &protos.QuasarCapabilityState_AliceShowValue{}}
}

func (v AliceShowQuasarCapabilityValue) toUserInfoProto() *common.TIoTUserInfo_TCapability_TQuasarCapabilityState_AliceShowValue {
	return &common.TIoTUserInfo_TCapability_TQuasarCapabilityState_AliceShowValue{
		AliceShowValue: &common.TIoTUserInfo_TCapability_TQuasarCapabilityState_TAliceShowValue{},
	}
}

func (v *AliceShowQuasarCapabilityValue) fromUserInfoProto(aliceShowValue *common.TIoTUserInfo_TCapability_TQuasarCapabilityState_TAliceShowValue) {
}

func (v AliceShowQuasarCapabilityValue) ToTypedSemanticFrame() *common.TTypedSemanticFrame {
	return &common.TTypedSemanticFrame{
		Type: &common.TTypedSemanticFrame_AliceShowActivateSemanticFrame{
			AliceShowActivateSemanticFrame: &common.TAliceShowActivateSemanticFrame{},
		},
	}
}

func (v AliceShowQuasarCapabilityValue) Validate(vctx *valid.ValidationCtx) (bool, error) {
	return false, nil
}

func (v AliceShowQuasarCapabilityValue) HasResultTTS() bool {
	return true
}

func (v AliceShowQuasarCapabilityValue) UnmarshalJSON(b []byte) error {
	return nil
}

func (v AliceShowQuasarCapabilityValue) MarshalJSON() ([]byte, error) {
	return []byte("{}"), nil
}
