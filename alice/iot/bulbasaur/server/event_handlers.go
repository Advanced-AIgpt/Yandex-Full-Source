package server

import (
	"context"

	"a.yandex-team.ru/alice/iot/bulbasaur/begemot"

	"a.yandex-team.ru/library/go/core/log/ctxlog"

	"a.yandex-team.ru/alice/iot/bulbasaur/dto/mobile"
	bulbasaur "a.yandex-team.ru/alice/iot/bulbasaur/errors"
	"a.yandex-team.ru/alice/iot/bulbasaur/model"
	"a.yandex-team.ru/library/go/core/xerrors"
)

func (s *Server) garlandEvent(ctx context.Context, user model.User, payload mobile.GarlandEventPayload) error {
	socketDeviceID := payload.DeviceID
	if err := s.db.UpdateUserDeviceName(ctx, user.ID, socketDeviceID, "Гирлянда"); err != nil {
		return xerrors.Errorf("Can't update name for device with id %s and user %d: %w", socketDeviceID, user.ID, err)
	}

	type scenarioBase struct {
		on      bool
		names   []model.ScenarioName
		phrases []string
		icon    model.ScenarioIcon
	}
	scenarioBases := []scenarioBase{
		{
			on: true,
			names: []model.ScenarioName{
				"Новогодний",
				"Елочка гори",
				"Новогоднее настроение",
				"Новый год",
				"Рождество",
				"Елочка гори 2",
				"Елочка гори 3",
			},
			phrases: []string{
				"Раз, два, три, елочка, гори!",
				"Елочка, гори!",
				"Включи новогоднее настроение",
			},
			icon: model.ScenarioIconTree,
		},
		{
			on: false,
			names: []model.ScenarioName{
				"Елочка не гори",
				"Гирлянда не гори",
				"Елочка не гори 2",
				"Елочка не гори 3",
			},
			phrases: []string{
				"Елочка, не гори",
				"Выключи новогоднее настроение",
			},
			icon: model.ScenarioIconSnowflake,
		},
	}

	var errors bulbasaur.Errors

	userInfo, err := s.repository.UserInfo(ctx, user)
	if err != nil {
		errors = append(errors, xerrors.Errorf("failed to select user info for user %d: %w", user.ID, err))
		return errors
	}

	existingScenarios, err := s.db.SelectUserScenarios(ctx, user.ID)
	if err != nil {
		errors = append(errors, xerrors.Errorf("failed to select existing scenarios for user %d: %w", user.ID, err))
		return errors
	}
	scenarioByID := model.Scenarios(existingScenarios).ToMap()

	for _, base := range scenarioBases {
		baseCapability := model.ScenarioCapability{
			Type: model.OnOffCapabilityType,
			State: model.OnOffCapabilityState{
				Instance: model.OnOnOffCapabilityInstance,
				Value:    base.on,
			},
		}
		baseDevice := model.ScenarioDevice{
			ID: socketDeviceID,
			Capabilities: []model.ScenarioCapability{
				baseCapability,
			},
		}

		notUsedPhrases := make([]model.ScenarioTrigger, 0, len(base.phrases))

		for _, phrase := range base.phrases {
			scenarioID, err := begemot.ScenarioIDByPushText(ctx, s.Logger, s.begemot, phrase, model.Scenarios(existingScenarios).GetIDs(), userInfo)
			if err != nil {
				return err
			}

			if scenarioID == "" {
				notUsedPhrases = append(notUsedPhrases, model.VoiceScenarioTrigger{Phrase: phrase})
				continue
			}

			existingScenario := scenarioByID[scenarioID]

			ctxlog.Infof(ctx, s.Logger, "User %d already has scenario with trigger %s (scenario id is %s), try merge it", user.ID, phrase, scenarioID)
			shouldBeUpdated := false
			shouldAddDevice := true
			for i, scenarioDevice := range existingScenario.Devices {
				if scenarioDevice.ID != socketDeviceID {
					continue
				}
				shouldAddDevice = false
				if _, found := scenarioDevice.GetCapabilityByTypeAndInstance(baseCapability.Type, string(model.OnOnOffCapabilityInstance)); !found {
					existingScenario.Devices[i].Capabilities = append(existingScenario.Devices[i].Capabilities, baseCapability)
					shouldBeUpdated = true
				}
			}
			if shouldAddDevice {
				existingScenario.Devices = append(existingScenario.Devices, baseDevice)
				shouldBeUpdated = true
			}
			if shouldBeUpdated {
				if err := s.db.UpdateScenario(ctx, user.ID, existingScenario); err != nil {
					errors = append(errors, xerrors.Errorf("error while merging device %s in new year scenarios for user %d: %w", socketDeviceID, user.ID, err))
				}
			} else {
				ctxlog.Infof(ctx, s.Logger, "User %d already has scenario with garland event on device %s, skipping creation", user.ID, socketDeviceID)
			}
		}

		if len(notUsedPhrases) == 0 {
			ctxlog.Infof(ctx, s.Logger, "All activation phrases are used in another scenarios, skipping creation")
			continue
		}

		var notUsedName model.ScenarioName
		for _, name := range base.names {
			if _, found := model.GetScenarioByName(name, existingScenarios); !found {
				notUsedName = name
				break
			}
		}

		if notUsedName == "" {
			// we do not expect it to happen, but just in case
			ctxlog.Warnf(ctx, s.Logger, "All names for scenario `%s` are used", base.names[0])
			continue
		}

		scenario := model.Scenario{
			Name:                         notUsedName,
			Icon:                         base.icon,
			Triggers:                     notUsedPhrases,
			Devices:                      []model.ScenarioDevice{baseDevice},
			RequestedSpeakerCapabilities: []model.ScenarioCapability{},
			IsActive:                     true,
		}

		if _, err := s.db.CreateScenario(ctx, user.ID, scenario); err != nil {
			errors = append(errors, xerrors.Errorf("Unknown error while creating new year scenarios for user %d: %w", user.ID, err))
		}
	}

	if len(errors) > 0 {
		return errors
	}

	return nil
}

func (s *Server) switchToDeviceTypeEvent(ctx context.Context, user model.User, payload mobile.SwitchToDeviceTypeEventPayload) error {
	var errors bulbasaur.Errors
	for _, deviceID := range payload.DevicesID {
		if err := s.db.UpdateUserDeviceType(ctx, user.ID, deviceID, payload.SwitchToType); err != nil {
			errors = append(errors, err)
		}
	}
	if len(errors) > 0 {
		return errors
	}
	return nil
}
