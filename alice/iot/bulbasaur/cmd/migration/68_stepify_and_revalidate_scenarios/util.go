package main

import (
	"fmt"
	"sync"
	"time"

	"go.uber.org/atomic"

	"a.yandex-team.ru/alice/iot/bulbasaur/model"
)

type User struct{ ID uint64 }

// recursive text action utils
func moveTextActionsToPhraseActions(parameters model.ScenarioStepActionsParameters) model.ScenarioStepActionsParameters {
	for i, capability := range parameters.RequestedSpeakerCapabilities {
		if !isScenarioTextActionCapability(capability) {
			continue
		}
		capability.State = model.QuasarServerActionCapabilityState{
			Instance: model.PhraseActionCapabilityInstance,
			Value:    capability.State.(model.QuasarServerActionCapabilityState).Value,
		}
		parameters.RequestedSpeakerCapabilities[i] = capability
	}
	for i, device := range parameters.Devices {
		for j, capability := range device.Capabilities {
			if !isTextActionCapability(capability) {
				continue
			}
			capability.SetParameters(model.QuasarServerActionCapabilityParameters{
				Instance: model.PhraseActionCapabilityInstance,
			})
			capability.SetState(model.QuasarServerActionCapabilityState{
				Instance: model.PhraseActionCapabilityInstance,
				Value:    capability.State().(model.QuasarServerActionCapabilityState).Value,
			})
			device.Capabilities[j] = capability
		}
		parameters.Devices[i] = device
	}
	return parameters
}

func isTextActionCapability(capability model.ICapability) bool {
	return capability.Type() == model.QuasarServerActionCapabilityType && capability.Instance() == string(model.TextActionCapabilityInstance)
}

func isScenarioTextActionCapability(capability model.ScenarioCapability) bool {
	return capability.Type == model.QuasarServerActionCapabilityType && capability.State.GetInstance() == string(model.TextActionCapabilityInstance)
}

// Stats hold migration progress counters
type Stats struct {
	StartTime        time.Time
	TotalUsersCount  atomic.Uint64
	FailedUsersCount atomic.Uint64

	TotalScenariosCount  atomic.Uint64
	FailedScenariosCount atomic.Uint64

	MigratedStepsScenariosCount     atomic.Uint64
	MigratedRecursiveScenariosCount atomic.Uint64
	FailedRecursiveScenariosCount   atomic.Uint64

	m                 sync.Mutex
	FailedScenarioIDs []string
}

func newStats() Stats {
	return Stats{StartTime: time.Now()}
}

func (s *Stats) String() string {
	return fmt.Sprintf(`
		Time elapsed: %v
		Users migrated: %d
		Failed users: %d
	
		Total read scenarios count: %d
		Failed scenarios count: %d

		Migrated scenarios count: %d
		Recursive scenarios count: %d
		Failed scenarios count: %d
		Failed scenarios: %v
	`,
		time.Since(s.StartTime),
		s.TotalUsersCount.Load(), s.FailedUsersCount.Load(),
		s.TotalScenariosCount.Load(), s.FailedScenariosCount.Load(),
		s.MigratedStepsScenariosCount.Load(), s.MigratedRecursiveScenariosCount.Load(), s.FailedRecursiveScenariosCount.Load(),
		s.FailedScenarioIDs,
	)
}

func (s *Stats) AddFailedRecursiveScenario(id string) {
	s.m.Lock()
	defer s.m.Unlock()
	s.FailedScenarioIDs = append(s.FailedScenarioIDs, id)
	s.FailedScenariosCount.Inc()
	s.FailedRecursiveScenariosCount.Inc()
}
