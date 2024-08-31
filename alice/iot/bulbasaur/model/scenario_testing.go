package model

import (
	"time"

	"a.yandex-team.ru/alice/library/go/timestamp"
)

func NewScenario(name string) *Scenario {
	return &Scenario{
		Name:     ScenarioName(name),
		Devices:  []ScenarioDevice{},
		IsActive: true,
	}
}

func (s *Scenario) WithIcon(icon ScenarioIcon) *Scenario {
	s.Icon = icon
	return s
}

func (s *Scenario) WithIsActive(isActive bool) *Scenario {
	s.IsActive = isActive
	return s
}

func (s *Scenario) WithTriggers(triggers ...ScenarioTrigger) *Scenario {
	s.Triggers = append(s.Triggers, triggers...)
	return s
}

func (s *Scenario) WithDevices(devices ...ScenarioDevice) *Scenario {
	s.Devices = append(s.Devices, devices...)
	return s
}

func (s *Scenario) WithRequestedSpeakerCapabilities(capabilities ...ScenarioCapability) *Scenario {
	s.RequestedSpeakerCapabilities = append(s.RequestedSpeakerCapabilities, capabilities...)
	return s
}

func (s *Scenario) WithEffectiveTime(startTimeOffset, endTimeOffset int, weekdays ...time.Weekday) *Scenario {
	s.EffectiveTime = &EffectiveTime{
		StartTimeOffset: startTimeOffset,
		EndTimeOffset:   endTimeOffset,
		DaysOfWeek:      weekdays,
	}
	return s
}

func (s *Scenario) WithSteps(steps ...IScenarioStep) *Scenario {
	s.Steps = append(s.Steps, steps...)
	return s
}

func NewScenarioLaunch() *ScenarioLaunch {
	return &ScenarioLaunch{Status: ScenarioLaunchScheduled}
}

func (l *ScenarioLaunch) WithName(name string) *ScenarioLaunch {
	l.ScenarioName = ScenarioName(name)
	return l
}

func (l *ScenarioLaunch) WithScenarioID(scenarioID string) *ScenarioLaunch {
	l.ScenarioID = scenarioID
	return l
}

func (l *ScenarioLaunch) WithTriggerType(triggerType ScenarioTriggerType) *ScenarioLaunch {
	l.LaunchTriggerType = triggerType
	return l
}

func (l *ScenarioLaunch) WithDevices(devices ...ScenarioLaunchDevice) *ScenarioLaunch {
	parameters := ScenarioStepActionsParameters{Devices: devices}
	l.Steps = append(l.Steps, MakeScenarioStepFromOldData(parameters))
	return l
}

func (l *ScenarioLaunch) WithCreatedTime(created timestamp.PastTimestamp) *ScenarioLaunch {
	l.Created = created
	return l
}

func (l *ScenarioLaunch) WithFinishedTime(finished timestamp.PastTimestamp) *ScenarioLaunch {
	l.Finished = finished
	return l
}

func (l *ScenarioLaunch) WithScheduledTime(scheduled timestamp.PastTimestamp) *ScenarioLaunch {
	l.Scheduled = scheduled
	return l
}

func (l *ScenarioLaunch) WithIcon(iconID ScenarioIcon) *ScenarioLaunch {
	l.Icon = iconID
	return l
}

func (l *ScenarioLaunch) WithStatus(status ScenarioLaunchStatus) *ScenarioLaunch {
	l.Status = status
	return l
}
