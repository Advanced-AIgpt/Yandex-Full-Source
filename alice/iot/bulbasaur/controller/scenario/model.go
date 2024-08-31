package scenario

import (
	"a.yandex-team.ru/alice/iot/bulbasaur/controller/action"
)

type LaunchResult struct {
	StepsResult                 []StepResult
	RequestedSpeakerActionsSent bool
	ScheduleDelayMs             int
	CurrentStepIndex            int
}

func (r LaunchResult) HasDevicesResult() bool {
	for _, stepResult := range r.StepsResult {
		if stepResult.HasDevicesResult {
			return true
		}
	}
	return false
}

func (r LaunchResult) Err() error {
	for _, stepResult := range r.StepsResult {
		if err := stepResult.Err(); err != nil {
			return err
		}
	}
	return nil
}

type LaunchResults []LaunchResult

func (r LaunchResults) RequestedSpeakerActionsSent() bool {
	for _, result := range r {
		if result.RequestedSpeakerActionsSent {
			return true
		}
	}
	return false
}

type StepResult struct {
	HasDevicesResult bool
	DevicesResult    action.DevicesResult
	Error            error
}

func (r StepResult) Err() error {
	if r.Error != nil {
		return r.Error
	}
	return r.DevicesResult.Err()
}
