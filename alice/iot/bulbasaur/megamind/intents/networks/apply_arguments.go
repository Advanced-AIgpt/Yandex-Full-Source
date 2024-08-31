package networks

import (
	"a.yandex-team.ru/alice/iot/bulbasaur/model"
)

type RestoreNetworksApplyArguments struct {
	SpeakerID string `json:"speaker_id"`
}

func (*RestoreNetworksApplyArguments) ProcessorName() string {
	return RestoreNetworksProcessorName
}

func (*RestoreNetworksApplyArguments) IsUniversalApplyArguments() {}

type SaveNetworksApplyArguments struct {
	SpeakerID string                `json:"speaker_id"`
	Networks  model.SpeakerNetworks `json:"networks"`
}

func (*SaveNetworksApplyArguments) ProcessorName() string {
	return SaveNetworksProcessorName
}

func (*SaveNetworksApplyArguments) IsUniversalApplyArguments() {}

type DeleteNetworksApplyArguments struct {
	SpeakerID string `json:"speaker_id"`
}

func (*DeleteNetworksApplyArguments) ProcessorName() string {
	return DeleteNetworksProcessorName
}

func (*DeleteNetworksApplyArguments) IsUniversalApplyArguments() {}
