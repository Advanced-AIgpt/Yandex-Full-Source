package settings

import (
	"a.yandex-team.ru/alice/iot/bulbasaur/model"
	memento "a.yandex-team.ru/alice/memento/proto"
)

var modelReactionTypeToReactionTypes = map[model.AliceResponseReactionType]memento.TIoTResponseOptions_EIoTResponseReactionType{
	model.SoundAliceResponseReactionType: memento.TIoTResponseOptions_SOUND,
	model.NLGAliceResponseReactionType:   memento.TIoTResponseOptions_NLG,
}

var reactionTypeToModelReactionType map[memento.TIoTResponseOptions_EIoTResponseReactionType]model.AliceResponseReactionType

const (
	IoTScenarioIntent string = "iot" //memento-only
)

var KnownMementoKeys = []memento.EConfigKey{
	memento.EConfigKey_CK_IOT_RESPONSE_OPTIONS,
	memento.EConfigKey_CK_TTS_WHISPER,
	memento.EConfigKey_CK_MUSIC,
	memento.EConfigKey_CK_ORDER_STATUS,
}

func init() {
	reactionTypeToModelReactionType = make(map[memento.TIoTResponseOptions_EIoTResponseReactionType]model.AliceResponseReactionType)
	for modelRT, mementoRT := range modelReactionTypeToReactionTypes {
		reactionTypeToModelReactionType[mementoRT] = modelRT
	}
}
