package scenario

import (
	"context"
	"time"

	"a.yandex-team.ru/alice/iot/bulbasaur/controller/scenario/timetable"
	"a.yandex-team.ru/alice/iot/bulbasaur/model"
)

type IController interface {
	CreateScenario(ctx context.Context, userID uint64, scenario model.Scenario) (scenarioID string, err error)
	SelectScenario(ctx context.Context, user model.User, scenarioID string) (model.Scenario, error)
	UpdateScenario(ctx context.Context, user model.User, initialScenario, updatedScenario model.Scenario) error
	ReplaceDevicesInScenarios(ctx context.Context, user model.User, fromTo map[string]string) error
	DeleteScenario(ctx context.Context, user model.User, scenarioID string) error
	SelectScenarios(ctx context.Context, user model.User) (model.Scenarios, error)
	CancelLaunch(ctx context.Context, origin model.Origin, launchID string) error
	StoreScenarioLaunch(ctx context.Context, userID uint64, launch model.ScenarioLaunch) (launchID string, err error)
	UpdateScenarioLaunch(ctx context.Context, origin model.Origin, launch model.ScenarioLaunch) error
	GetLaunchByID(ctx context.Context, userID uint64, launchID string) (model.ScenarioLaunch, error)
	GetScheduledLaunches(ctx context.Context, user model.User, userDevices model.Devices) (model.ScenarioLaunches, error)
	GetHistoryLaunches(ctx context.Context, user model.User, statuses []model.ScenarioLaunchStatus, limit uint64) (model.ScenarioLaunches, error)
	CancelTimerLaunches(ctx context.Context, userID uint64) error
	CreateScheduledScenarioLaunch(ctx context.Context, userID uint64, launch model.ScenarioLaunch) (string, error)
	InvokeScenarioLaunch(ctx context.Context, origin model.Origin, scenarioLaunch model.ScenarioLaunch) (model.ScenarioLaunch, error)
	InvokeScheduledScenarioByLaunchID(ctx context.Context, origin model.Origin, launchID string) error
	InvokeScenariosByDeviceProperties(ctx context.Context, origin model.Origin, deviceID string, changedProperties model.PropertiesChangedStates) error
	InvokeScenarioAndCreateLaunch(ctx context.Context, origin model.Origin, trigger model.ScenarioTrigger, scenario model.Scenario, userDevices model.Devices) (model.ScenarioLaunch, error)
	SendActionsToScenarioLaunch(ctx context.Context, origin model.Origin, launch model.ScenarioLaunch, allUserDevices model.Devices) LaunchResult
	SendInvokedLaunchPush(ctx context.Context, user model.User, launch model.ScenarioLaunch) error
	// CalculateNextTimetableRun calculates next run for given timetable triggers.
	// return error if next run is impossible to calculate
	CalculateNextTimetableRun(ctx context.Context, user model.User, timetableTriggers []model.TimetableScenarioTrigger) (timetable.NextRun, error)
}

type Mock struct {
	InvokeScenarioByPropertiesEvents chan map[string]model.PropertiesChangedStates
}

func NewMock() Mock {
	return Mock{
		InvokeScenarioByPropertiesEvents: make(chan map[string]model.PropertiesChangedStates, 100),
	}
}

func (m Mock) CreateScenario(ctx context.Context, userID uint64, scenario model.Scenario) (scenarioID string, err error) {
	return "", nil
}

func (m Mock) SendInvokedLaunchPush(ctx context.Context, user model.User, launch model.ScenarioLaunch) error {
	return nil
}

func (m Mock) SelectScenario(ctx context.Context, user model.User, scenarioID string) (model.Scenario, error) {
	return model.Scenario{}, nil
}

func (m Mock) UpdateScenario(ctx context.Context, user model.User, initialScenario, updatedScenario model.Scenario) error {
	return nil
}

func (m Mock) ReplaceDevicesInScenarios(ctx context.Context, user model.User, fromTo map[string]string) error {
	return nil
}

func (m Mock) DeleteScenario(ctx context.Context, user model.User, scenarioID string) error {
	return nil
}

func (m Mock) SelectScenarios(ctx context.Context, user model.User) (model.Scenarios, error) {
	return model.Scenarios{}, nil
}

func (m Mock) CancelLaunch(ctx context.Context, origin model.Origin, launchID string) error {
	return nil
}

func (m Mock) StoreScenarioLaunch(ctx context.Context, userID uint64, launch model.ScenarioLaunch) (launchID string, err error) {
	return "", nil
}

func (m Mock) UpdateScenarioLaunch(ctx context.Context, origin model.Origin, launch model.ScenarioLaunch) error {
	return nil
}

func (m Mock) GetLaunchByID(ctx context.Context, userID uint64, launchID string) (model.ScenarioLaunch, error) {
	return model.ScenarioLaunch{}, nil
}

func (m Mock) GetScheduledLaunches(ctx context.Context, user model.User, userDevices model.Devices) (model.ScenarioLaunches, error) {
	return model.ScenarioLaunches{}, nil
}

func (m Mock) GetHistoryLaunches(ctx context.Context, user model.User, statuses []model.ScenarioLaunchStatus, limit uint64) (model.ScenarioLaunches, error) {
	return model.ScenarioLaunches{}, nil
}

func (m Mock) CancelTimerLaunches(ctx context.Context, userID uint64) error {
	return nil
}

func (m Mock) CreateScheduledScenarioLaunch(ctx context.Context, userID uint64, launch model.ScenarioLaunch) (string, error) {
	return "", nil
}

func (m Mock) InvokeScenarioLaunch(ctx context.Context, origin model.Origin, scenarioLaunch model.ScenarioLaunch) (model.ScenarioLaunch, error) {
	return model.ScenarioLaunch{}, nil
}

func (m Mock) InvokeScheduledScenarioByLaunchID(ctx context.Context, origin model.Origin, launchID string) error {
	return nil
}

func (m Mock) InvokeScenariosByDeviceProperties(ctx context.Context, origin model.Origin, deviceID string, changedProperties model.PropertiesChangedStates) error {
	m.InvokeScenarioByPropertiesEvents <- map[string]model.PropertiesChangedStates{deviceID: changedProperties}
	return nil
}

func (m Mock) InvokeScenarioAndCreateLaunch(ctx context.Context, origin model.Origin, trigger model.ScenarioTrigger, scenario model.Scenario, userDevices model.Devices) (model.ScenarioLaunch, error) {
	return model.ScenarioLaunch{}, nil
}

func (m Mock) SendActionsToScenarioLaunch(ctx context.Context, origin model.Origin, launch model.ScenarioLaunch, allUserDevices model.Devices) LaunchResult {
	return LaunchResult{}
}

func (m Mock) AssertInvokeScenariosByPropertiesEvents(timeout time.Duration, assertion func(m map[string]model.PropertiesChangedStates)) {
	events := map[string]model.PropertiesChangedStates{}
	for {
		select {
		case <-time.After(timeout):
			assertion(events)
			return
		case deviceEvents := <-m.InvokeScenarioByPropertiesEvents:
			for k, v := range deviceEvents {
				events[k] = v
			}
		}
	}
}
func (m Mock) CalculateNextTimetableRun(ctx context.Context, user model.User, timetableTriggers []model.TimetableScenarioTrigger) (timetable.NextRun, error) {
	return timetable.NextRun{}, nil
}
