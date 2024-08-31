package model

import (
	"encoding/json"
	"testing"

	"a.yandex-team.ru/alice/megamind/protos/common"
	"github.com/stretchr/testify/assert"
)

func TestQuasarCapabilityState_Validate(t *testing.T) {
	type testCase struct {
		State          QuasarCapabilityState
		ExpectedErrStr string
	}
	testCases := []testCase{
		{
			State: QuasarCapabilityState{
				Instance: "hehehe",
			},
			ExpectedErrStr: `unsupported by current device quasar capability state instance: "hehehe"`,
		},
		{
			State: QuasarCapabilityState{
				Instance: StopEverythingCapabilityInstance,
				Value:    StopEverythingQuasarCapabilityValue{},
			},
		},
		{
			State: QuasarCapabilityState{
				Instance: WeatherCapabilityInstance,
				Value:    WeatherQuasarCapabilityValue{},
			},
		},
		{
			State: QuasarCapabilityState{
				Instance: MusicPlayCapabilityInstance,
				Value: MusicPlayQuasarCapabilityValue{
					SearchText: "Metallica - Enter Sandman",
				},
			},
		},
		{
			State: QuasarCapabilityState{
				Instance: MusicPlayCapabilityInstance,
				Value: MusicPlayQuasarCapabilityValue{
					Object: &MusicPlayObject{
						ID:   "1",
						Type: TrackMusicPlayObjectType,
						Name: "some-name",
					},
				},
			},
		},
		{
			State: QuasarCapabilityState{
				Instance: MusicPlayCapabilityInstance,
				Value: MusicPlayQuasarCapabilityValue{
					Object: &MusicPlayObject{
						ID:   "1",
						Type: "lol",
						Name: "some-name",
					},
				},
			},
			ExpectedErrStr: `unsupported by current device quasar capability state music play object type: "lol"`,
		},
		{
			State: QuasarCapabilityState{
				Instance: MusicPlayCapabilityInstance,
				Value:    MusicPlayQuasarCapabilityValue{},
			},
			ExpectedErrStr: "unsupported quasar capability state music play value: should be object or search text",
		},
		{
			State: QuasarCapabilityState{
				Instance: VolumeCapabilityInstance,
				Value: VolumeQuasarCapabilityValue{
					Value: 9,
				},
			},
		},
		{
			State: QuasarCapabilityState{
				Instance: VolumeCapabilityInstance,
				Value: VolumeQuasarCapabilityValue{
					Value:    -1,
					Relative: true,
				},
			},
		},
		{
			State: QuasarCapabilityState{
				Instance: VolumeCapabilityInstance,
				Value: VolumeQuasarCapabilityValue{
					Value: 11,
				},
			},
			ExpectedErrStr: "unsupported quasar capability state value: volume out of range",
		},
		{
			State: QuasarCapabilityState{
				Instance: NewsCapabilityInstance,
				Value: NewsQuasarCapabilityValue{
					Topic: IndexSpeakerNewsTopic,
				},
			},
		},
		{
			State: QuasarCapabilityState{
				Instance: NewsCapabilityInstance,
				Value: NewsQuasarCapabilityValue{
					Topic: "unknown-topic",
				},
			},
			ExpectedErrStr: `unsupported quasar capability state news value topic: "unknown-topic"`,
		},
		{
			State: QuasarCapabilityState{
				Instance: SoundPlayCapabilityInstance,
				Value:    SoundPlayQuasarCapabilityValue{Sound: "chainsaw-1"},
			},
		},
		{
			State: QuasarCapabilityState{
				Instance: SoundPlayCapabilityInstance,
				Value:    SoundPlayQuasarCapabilityValue{Sound: "unknown-sound"},
			},
			ExpectedErrStr: `unsupported quasar capability state sound play value: "unknown-sound"`,
		},
	}
	capability := MakeCapabilityByType(QuasarCapabilityType)
	for _, tc := range testCases {
		capability.SetParameters(QuasarCapabilityParameters{Instance: tc.State.Instance})
		if tc.ExpectedErrStr != "" {
			assert.EqualError(t, tc.State.ValidateState(capability), tc.ExpectedErrStr)
		} else {
			assert.NoError(t, tc.State.ValidateState(capability))
		}
	}
}

func TestQuasarCapabilityValueToTypedSemanticFrame(t *testing.T) {
	musicValue := MusicPlayQuasarCapabilityValue{
		SearchText: "Metallica - Enter Sandman",
	}
	expected := &common.TTypedSemanticFrame{
		Type: &common.TTypedSemanticFrame_MusicPlaySemanticFrame{
			MusicPlaySemanticFrame: &common.TMusicPlaySemanticFrame{
				SearchText:      &common.TStringSlot{Value: &common.TStringSlot_StringValue{StringValue: "Metallica - Enter Sandman"}},
				PlaySingleTrack: &common.TBoolSlot{Value: &common.TBoolSlot_BoolValue{BoolValue: true}},
				DisableAutoflow: &common.TBoolSlot{Value: &common.TBoolSlot_BoolValue{BoolValue: true}},
				DisableHistory:  &common.TBoolSlot{Value: &common.TBoolSlot_BoolValue{BoolValue: true}},
				DisableNlg:      &common.TBoolSlot{Value: &common.TBoolSlot_BoolValue{BoolValue: true}},
			},
		},
	}
	assert.Equal(t, expected, musicValue.ToTypedSemanticFrame())
}

func TestQuasarCapabilityFromUserInfoProto(t *testing.T) {
	protoValue := &common.TIoTUserInfo_TCapability_TQuasarCapabilityState_TMusicPlayValue{
		Value: &common.TIoTUserInfo_TCapability_TQuasarCapabilityState_TMusicPlayValue_SearchText{
			SearchText: "Metallica - Enter Sandman",
		},
	}
	expected := MusicPlayQuasarCapabilityValue{
		SearchText: "Metallica - Enter Sandman",
	}
	var musicValue MusicPlayQuasarCapabilityValue
	musicValue.fromUserInfoProto(protoValue)
	assert.Equal(t, expected, musicValue)
}

func TestQuasarCapabilityStateUnmarshalling(t *testing.T) {
	rawState := `{"instance":"stop_everything","value":{}}`
	var state QuasarCapabilityState
	err := json.Unmarshal([]byte(rawState), &state)
	assert.NoError(t, err)

	marshalResult, err := json.Marshal(state)
	assert.NoError(t, err)
	assert.Equal(t, rawState, string(marshalResult))
}

func TestSpeakerSounds(t *testing.T) {
	type testCase struct {
		soundID  SpeakerSoundID
		opus     string
		audioURL string
		name     string
	}
	testCases := []testCase{
		{
			soundID:  "chainsaw-1",
			opus:     `<speaker audio="alice-sounds-things-chainsaw-1.opus">`,
			audioURL: `https://dialogs.s3.yandex.net/sounds/alice-sounds-things-chainsaw-1.mp3`,
			name:     "Бензопила",
		},
		{
			soundID:  "ship-horn-1",
			opus:     `<speaker audio="alice-sounds-transport-ship-horn-1.opus">`,
			audioURL: `https://dialogs.s3.yandex.net/sounds/alice-sounds-transport-ship-horn-1.mp3`,
			name:     "Гудок корабля №1",
		},
		{
			soundID:  "piano-c-1",
			opus:     `<speaker audio="alice-music-piano-c-1.opus">`,
			audioURL: `https://dialogs.s3.yandex.net/sounds/alice-music-piano-c-1.mp3`,
			name:     "Фортепиано (до)",
		},
	}
	for _, tc := range testCases {
		sound := KnownSpeakerSounds[tc.soundID]
		assert.Equal(t, tc.opus, sound.Opus())
		assert.Equal(t, tc.audioURL, sound.AudioURL())
		assert.Equal(t, tc.name, sound.Name)
	}
}

func TestSpeakerSoundsMapConsistency(t *testing.T) {
	for ID, speakerSound := range KnownSpeakerSounds {
		assert.Equal(t, ID, speakerSound.ID, "inconsistent speaker sound ids in speaker sound map: %q != %q", ID, speakerSound.ID)
		_, exist := KnownSpeakerSoundCategories[speakerSound.CategoryID]
		assert.True(t, exist, "unknown speaker sound category of sound %q", speakerSound.ID)
	}
}

func TestNeedCompletionCallback(t *testing.T) {
	type testCase struct {
		state    QuasarCapabilityState
		expected bool
	}

	testCases := []testCase{
		{
			state: QuasarCapabilityState{
				Instance: MusicPlayCapabilityInstance,
				Value: MusicPlayQuasarCapabilityValue{
					SearchText:       "Queen - Show Must Go On",
					PlayInBackground: true,
				},
			},
			expected: false,
		},
		{
			state: QuasarCapabilityState{
				Instance: MusicPlayCapabilityInstance,
				Value: MusicPlayQuasarCapabilityValue{
					SearchText: "Queen - Bohemian Rhapsody",
				},
			},
			expected: true,
		},
		{
			state: QuasarCapabilityState{
				Instance: NewsCapabilityInstance,
				Value: NewsQuasarCapabilityValue{
					Topic:            IndexSpeakerNewsTopic,
					PlayInBackground: true,
				},
			},
			expected: false,
		},
		{
			state: QuasarCapabilityState{
				Instance: NewsCapabilityInstance,
				Value: NewsQuasarCapabilityValue{
					Topic: IndexSpeakerNewsTopic,
				},
			},
			expected: true,
		},
		{
			state: QuasarCapabilityState{
				Instance: StopEverythingCapabilityInstance,
				Value:    StopEverythingQuasarCapabilityValue{},
			},
			expected: true,
		},
	}
	for _, tc := range testCases {
		assert.Equal(t, tc.expected, tc.state.NeedCompletionCallback())
	}
}

func TestQuasarCapabilityValueHasResultTTS(t *testing.T) {
	type testCase struct {
		value    QuasarCapabilityValue
		expected bool
	}

	testCases := []testCase{
		{
			value:    MusicPlayQuasarCapabilityValue{},
			expected: true,
		},
		{
			value:    SoundPlayQuasarCapabilityValue{},
			expected: true,
		},
		{
			value:    NewsQuasarCapabilityValue{},
			expected: true,
		},
		{
			value:    WeatherQuasarCapabilityValue{},
			expected: true,
		},
		{
			value:    TTSQuasarCapabilityValue{},
			expected: true,
		},
		{
			value:    StopEverythingQuasarCapabilityValue{},
			expected: false,
		},
		{
			value:    VolumeQuasarCapabilityValue{},
			expected: false,
		},
		{
			value:    AliceShowQuasarCapabilityValue{},
			expected: true,
		},
	}
	for _, tc := range testCases {
		assert.Equal(t, tc.expected, tc.value.HasResultTTS())
	}
}
