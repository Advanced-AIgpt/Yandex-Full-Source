package query

import (
	"a.yandex-team.ru/alice/iot/bulbasaur/megamind/common"
	"a.yandex-team.ru/alice/iot/bulbasaur/model"
)

type ApplyArguments struct {
	Devices          model.Devices                     `json:"devices"`
	IntentParameters common.QueryIntentParametersSlice `json:"intent_parameters"`
}

func (a *ApplyArguments) ProcessorName() string {
	return processorName
}

func (a *ApplyArguments) IsUniversalApplyArguments() {}
