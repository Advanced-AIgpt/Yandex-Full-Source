package voiceprint

import (
	"a.yandex-team.ru/alice/iot/bulbasaur/model"
	"a.yandex-team.ru/alice/protos/data/scenario/iot"
)

type StatusContinueArguments struct {
	PUID          uint64                        `json:"puid"`
	DeviceID      string                        `json:"device_id"` // smart home device id of speaker
	Source        iot.TEnrollmentStatus_ESource `json:"source"`
	StatusFailure *StatusFailure                `json:"status_failure,omitempty"`
}

func (l *StatusContinueArguments) ProcessorName() string {
	return StatusProcessorName
}

func (l *StatusContinueArguments) IsUniversalContinueArguments() {}

func NewStatusContinueArguments(statusFrame StatusFrame, originDevice model.Device) *StatusContinueArguments {
	return &StatusContinueArguments{
		PUID:          statusFrame.PUID,
		DeviceID:      originDevice.ID,
		Source:        statusFrame.Source,
		StatusFailure: statusFrame.StatusFailure,
	}
}
