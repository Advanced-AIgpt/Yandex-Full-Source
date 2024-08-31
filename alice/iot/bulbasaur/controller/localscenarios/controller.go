package localscenarios

import (
	"context"

	"a.yandex-team.ru/alice/iot/bulbasaur/controller/experiments"
	"a.yandex-team.ru/alice/iot/bulbasaur/controller/notificator"
	"a.yandex-team.ru/alice/iot/bulbasaur/model"
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/log/ctxlog"
)

type controller struct {
	logger                log.Logger
	notificatorController notificator.IController
}

func NewController(
	logger log.Logger,
	notificatorController notificator.IController,
) *controller {
	return &controller{
		logger,
		notificatorController,
	}
}

func (c *controller) SyncLocalScenarios(ctx context.Context, userID uint64, scenarios model.Scenarios, userDevices model.Devices) error {
	if !experiments.EnableLocalScenarios.IsEnabled(ctx) {
		return nil
	}
	for _, speaker := range userDevices.FilterBySkillID(model.QUASAR) {
		if !model.ZigbeeSpeakers[speaker.Type] {
			continue
		}
		quasarExternalID, _ := speaker.GetExternalID()
		syncScenariosDirective := SyncScenariosSpeechkitDirective{
			endpointID: quasarExternalID,
			Scenarios:  scenarios.ToLocalScenarios(userDevices),
		}
		if err := c.notificatorController.SendSpeechkitDirective(ctx, userID, quasarExternalID, &syncScenariosDirective); err != nil {
			ctxlog.Infof(ctx, c.logger, "failed to sync local scenarios to speaker %s: %v", quasarExternalID, err)
			return err
		}
	}
	return nil
}

func (c *controller) AddLocalScenariosToSpeaker(ctx context.Context, userID uint64, endpointID string, scenarios model.Scenarios, userDevices model.Devices) error {
	if !experiments.EnableLocalScenarios.IsEnabled(ctx) {
		return nil
	}

	addScenariosDirective := AddScenariosSpeechkitDirective{
		endpointID: endpointID,
		Scenarios:  scenarios.ToLocalScenarios(userDevices),
	}
	if err := c.notificatorController.SendSpeechkitDirective(ctx, userID, endpointID, &addScenariosDirective); err != nil {
		ctxlog.Infof(ctx, c.logger, "failed to add local scenarios to speaker %s: %v", endpointID, err)
		return err
	}
	return nil
}

func (c *controller) RemoveLocalScenarios(ctx context.Context, userID uint64, scenarioIDs []string, userDevices model.Devices) error {
	if len(scenarioIDs) == 0 {
		return nil
	}
	if !experiments.EnableLocalScenarios.IsEnabled(ctx) {
		return nil
	}
	for _, speaker := range userDevices.FilterBySkillID(model.QUASAR) {
		if !model.ZigbeeSpeakers[speaker.Type] {
			continue
		}
		quasarExternalID, _ := speaker.GetExternalID()
		if err := c.RemoveLocalScenariosFromSpeaker(ctx, userID, quasarExternalID, scenarioIDs); err != nil {
			return err
		}
	}
	return nil
}

func (c *controller) RemoveLocalScenariosFromSpeaker(ctx context.Context, userID uint64, endpointID string, scenarioIDs []string) error {
	if len(scenarioIDs) == 0 {
		return nil
	}
	removeScenariosDirective := RemoveScenariosSpeechkitDirective{
		endpointID: endpointID,
		IDs:        scenarioIDs,
	}
	if err := c.notificatorController.SendSpeechkitDirective(ctx, userID, endpointID, &removeScenariosDirective); err != nil {
		ctxlog.Infof(ctx, c.logger, "failed to remove local scenarios from speaker %s: %v", endpointID, err)
		return err
	}
	return nil
}
