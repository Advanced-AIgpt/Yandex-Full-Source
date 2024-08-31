package mobile

import (
	"encoding/json"
	"testing"

	"a.yandex-team.ru/alice/iot/bulbasaur/model"
	"github.com/stretchr/testify/assert"
)

func TestCapabilityStateViewFrom(t *testing.T) {
	newsCapability := model.MakeCapabilityByType(model.QuasarCapabilityType)
	newsCapability.SetParameters(model.QuasarCapabilityParameters{Instance: model.NewsCapabilityInstance})
	newsCapability.SetState(model.QuasarCapabilityState{
		Instance: model.NewsCapabilityInstance,
		Value: model.NewsQuasarCapabilityValue{
			Topic: model.IndexSpeakerNewsTopic,
		},
	})

	expected := CapabilityStateView{
		Retrievable: false,
		Type:        model.QuasarCapabilityType,
		State: QuasarCapabilityStateView{
			Instance: model.NewsCapabilityInstance,
			Value: NewsQuasarCapabilityValue{
				NewsQuasarCapabilityValue: model.NewsQuasarCapabilityValue{
					Topic: model.IndexSpeakerNewsTopic,
				},
				TopicName: "Главное",
			},
		},
		Parameters: QuasarCapabilityParameters{Instance: model.NewsCapabilityInstance},
	}
	var view CapabilityStateView
	view.FromCapability(newsCapability)
	assert.Equal(t, expected, view)

	soundCapability := model.MakeCapabilityByType(model.QuasarCapabilityType)
	soundCapability.SetParameters(model.QuasarCapabilityParameters{Instance: model.SoundPlayCapabilityInstance})
	soundCapability.SetState(model.QuasarCapabilityState{
		Instance: model.SoundPlayCapabilityInstance,
		Value: model.SoundPlayQuasarCapabilityValue{
			Sound: "chainsaw-1",
		},
	})
	expected = CapabilityStateView{
		Retrievable: false,
		Type:        model.QuasarCapabilityType,
		State: QuasarCapabilityStateView{
			Instance: model.SoundPlayCapabilityInstance,
			Value: SoundPlayQuasarCapabilityValue{
				SoundPlayQuasarCapabilityValue: model.SoundPlayQuasarCapabilityValue{
					Sound: "chainsaw-1",
				},
				SoundName: "Бензопила",
			},
		},
		Parameters: QuasarCapabilityParameters{Instance: model.SoundPlayCapabilityInstance},
	}
	view = CapabilityStateView{}
	view.FromCapability(soundCapability)
	assert.Equal(t, expected, view)
}

func TestQuasarCapabilityStateViewMarshalling(t *testing.T) {
	stateView := QuasarCapabilityStateView{
		Instance: model.NewsCapabilityInstance,
		Value: NewsQuasarCapabilityValue{
			NewsQuasarCapabilityValue: model.NewsQuasarCapabilityValue{
				Topic: model.IndexSpeakerNewsTopic,
			},
			TopicName: "Главное",
		},
	}
	bytes, err := json.Marshal(stateView)
	assert.NoError(t, err)
	assert.Equal(t, `{"instance":"news","value":{"topic":"index","topic_name":"Главное"}}`, string(bytes))

	stateView = QuasarCapabilityStateView{
		Instance: model.SoundPlayCapabilityInstance,
		Value: SoundPlayQuasarCapabilityValue{
			SoundPlayQuasarCapabilityValue: model.SoundPlayQuasarCapabilityValue{
				Sound: "chainsaw-1",
			},
			SoundName: "Бензопила",
		},
	}
	bytes, err = json.Marshal(stateView)
	assert.NoError(t, err)
	assert.Equal(t, `{"instance":"sound_play","value":{"sound":"chainsaw-1","sound_name":"Бензопила"}}`, string(bytes))
}
