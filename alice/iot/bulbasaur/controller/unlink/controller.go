package unlink

import (
	"context"
	"fmt"
	"time"

	"a.yandex-team.ru/alice/iot/bulbasaur/controller/localscenarios"
	"a.yandex-team.ru/alice/iot/bulbasaur/controller/notificator"
	"a.yandex-team.ru/alice/iot/bulbasaur/controller/quasarconfig"
	"a.yandex-team.ru/alice/iot/bulbasaur/controller/updates"
	"a.yandex-team.ru/alice/iot/bulbasaur/db"
	"a.yandex-team.ru/alice/iot/bulbasaur/megamind/frames"
	"a.yandex-team.ru/alice/iot/bulbasaur/model"
	"a.yandex-team.ru/alice/iot/bulbasaur/provider"
	"a.yandex-team.ru/alice/library/go/dialogs"
	"a.yandex-team.ru/alice/library/go/socialism"
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/log/ctxlog"
	"a.yandex-team.ru/library/go/core/xerrors"
)

const unlinkProviderHandlerTimeout = 5 * time.Second

type controller struct {
	logger                   log.Logger
	dbClient                 db.DB
	notificatorController    notificator.IController
	quasarConfigController   quasarconfig.IController
	updatesController        updates.IController
	dialogs                  dialogs.Dialoger
	socialism                socialism.IClient
	providerFactory          provider.IProviderFactory
	localScenariosController localscenarios.Controller
}

func NewController(
	logger log.Logger,
	dbClient db.DB,
	notificatorController notificator.IController,
	quasarConfigController quasarconfig.IController,
	updatesController updates.IController,
	dialogs dialogs.Dialoger,
	socialism socialism.IClient,
	providerFactory provider.IProviderFactory,
	localScenariosController localscenarios.Controller,
) *controller {
	return &controller{
		logger,
		dbClient,
		notificatorController,
		quasarConfigController,
		updatesController,
		dialogs,
		socialism,
		providerFactory,
		localScenariosController,
	}
}

func (c *controller) DeleteChangedOwnerDevices(ctx context.Context, newOwnerUID uint64, skillID string, devices model.Devices) error {
	changedOwnerDevicesMap, err := c.findChangedOwnerDevices(ctx, newOwnerUID, skillID, devices)
	if err != nil {
		ctxlog.Warnf(ctx, c.logger, "failed to find changed owner devices: %s", err)
		return xerrors.Errorf("failed to find changed owner devices: %w", err)
	}
	if err := c.deleteChangedOwnerDevices(ctx, skillID, changedOwnerDevicesMap); err != nil {
		ctxlog.Warnf(ctx, c.logger, "failed to delete changed owner devices: %s", err)
		return xerrors.Errorf("failed to delete changed owner devices: %w", err)
	}
	return nil
}

func (c *controller) findChangedOwnerDevices(ctx context.Context, newOwnerUID uint64, skillID string, devices model.Devices) (changedOwnerDevicesMap model.DevicesMapByOwnerID, err error) {
	devices = devices.FilterBySkillID(skillID)
	switch skillID {
	case model.QUASAR, model.YANDEXIO, model.TUYA:
		return c.findSnatchedDevicesByExternalID(ctx, newOwnerUID, skillID, devices)
	}
	return nil, nil
}

func (c *controller) findSnatchedDevicesByExternalID(ctx context.Context, newOwnerUID uint64, skillID string, devices model.Devices) (changedOwnerDevicesMap model.DevicesMapByOwnerID, err error) {
	changedOwnerDevicesMap, err = c.dbClient.SelectDevicesSimpleByExternalIDs(ctx, devices.ExternalIDs())
	if err != nil {
		ctxlog.Warnf(ctx, c.logger, "failed to select devices by external ids: %v", err)
		return nil, xerrors.Errorf("failed to select other device owners: %w", err)
	}
	delete(changedOwnerDevicesMap, newOwnerUID) // we deliberately skip new owner

	// external id is not a unique key, so we should filter out devices by skillID
	for oldOwner := range changedOwnerDevicesMap {
		changedOwnerDevicesMap[oldOwner] = changedOwnerDevicesMap[oldOwner].FilterBySkillID(skillID)
	}
	return changedOwnerDevicesMap, nil
}

func (c *controller) deleteChangedOwnerDevices(ctx context.Context, skillID string, changedOwnerDevicesMap model.DevicesMapByOwnerID) error {
	ctxlog.Info(ctx, c.logger, "deleting snatched devices", log.Any("snatched_devices", changedOwnerDevicesMap.Short()))
	switch skillID {
	case model.QUASAR:
		for userID, devices := range changedOwnerDevicesMap {
			c.deleteSnatchedQuasarDevices(ctx, userID, devices)
			c.updatesController.AsyncNotifyAboutDeviceListUpdates(ctx, model.User{ID: userID}, updates.DeleteChangedOwnerDevicesSource)
		}
		return nil
	case model.YANDEXIO:
		for userID, devices := range changedOwnerDevicesMap {
			c.deleteSnatchedYandexIODevices(ctx, userID, devices)
			c.updatesController.AsyncNotifyAboutDeviceListUpdates(ctx, model.User{ID: userID}, updates.DeleteChangedOwnerDevicesSource)
		}
		return nil
	case model.TUYA:
		ctxlog.Infof(ctx, c.logger, "skip deleting tuya changed owner devices: provider deleted its data already")
		// todo: log tuya skip due to provider guard
		return nil
	}
	return xerrors.Errorf("devices of skillID %s can not implicitly change owner", skillID)
}

func (c *controller) deleteSnatchedQuasarDevices(ctx context.Context, userID uint64, snatchedDevices model.Devices) {
	userDevices, err := c.dbClient.SelectUserDevicesSimple(ctx, userID)
	if err != nil {
		ctxlog.Warnf(ctx, c.logger, "failed to select devices: %s", err)
	}
	stereopairs, err := c.dbClient.SelectStereopairsSimple(ctx, userID)
	if err != nil {
		ctxlog.Errorf(ctx, c.logger, "failed to select stereopairs: %s", err)
	}
	for _, device := range snatchedDevices {
		// delete child devices, if there are any
		childDevices := model.QuasarDevice(device).ChildDevices(userDevices)
		if childDeviceIDs := childDevices.GetIDs(); len(childDeviceIDs) > 0 {
			ctxlog.Info(ctx, c.logger, fmt.Sprintf("found child devices of %s, will delete them", device.ID), log.Any("child_device_ids", childDeviceIDs))
			if err := c.DeleteDevices(ctx, userID, childDeviceIDs); err != nil {
				ctxlog.Warnf(ctx, c.logger, "failed to delete devices: %s", err)
			}
		}

		// delete stereopair, if device is part of it
		if stereopair, exist := stereopairs.GetByDeviceID(device.ID); exist {
			ctxlog.Infof(ctx, c.logger, "device %s is in stereopair %s, will delete stereopair", device.ID, stereopair.ID)
			if err = c.dbClient.DeleteStereopair(ctx, userID, stereopair.ID); err != nil {
				ctxlog.Errorf(ctx, c.logger, "failed to delete stereopair %s: %s", stereopair.ID, err)
			}
		}
	}
	if err := c.DeleteDevices(ctx, userID, snatchedDevices.GetIDs()); err != nil {
		ctxlog.Warnf(ctx, c.logger, "failed to delete devices: %s", err)
	}
}

func (c *controller) deleteSnatchedYandexIODevices(ctx context.Context, userID uint64, snatchedDevices model.Devices) {
	if err := c.DeleteDevices(ctx, userID, snatchedDevices.GetIDs()); err != nil {
		ctxlog.Warnf(ctx, c.logger, "failed to delete devices: %s", err)
	}
	userDevices, err := c.dbClient.SelectUserDevicesSimple(ctx, userID)
	if err != nil {
		ctxlog.Warnf(ctx, c.logger, "failed to select devices: %s", err)
		return
	}
	parentsMap, parentChildRelations := model.YandexIODevices(snatchedDevices).GetParentChildRelations(userDevices)
	for _, parent := range parentsMap {
		parentEndpointID, err := parent.GetExternalID()
		if err != nil {
			ctxlog.Warnf(ctx, c.logger, "failed to get external id of yandex io parent: %s", err)
			continue
		}
		forgetEndpointsFrame := &frames.ForgetEndpointsFrame{
			EndpointIDs: parentChildRelations[parent.ID].ExternalIDs(),
		}
		if err := c.notificatorController.SendTypedSemanticFrame(ctx, userID, parentEndpointID, forgetEndpointsFrame); err != nil {
			ctxlog.Warnf(ctx, c.logger, "failed to send forget endpoint frame: %s", err)
			continue
		}
	}
}

func (c *controller) UnlinkProvider(
	ctx context.Context,
	skillID string,
	origin model.Origin,
	saveDevices bool,
) error {
	ctxlog.Infof(ctx, c.logger, "unlink provider skill_id %s, user_id: %d", skillID, origin.User.ID)
	user := origin.User
	if !saveDevices {
		providerDevices, err := c.dbClient.SelectUserProviderDevicesSimple(ctx, user.ID, skillID)
		if err != nil {
			return xerrors.Errorf("failed to obtain provider's devices: %w", err)
		}

		deviceIDList := make([]string, 0, len(providerDevices))
		for _, device := range providerDevices {
			deviceIDList = append(deviceIDList, device.ID)
		}

		if err := c.DeleteDevices(ctx, user.ID, deviceIDList); err != nil {
			return xerrors.Errorf("failed to delete devices: %w", err)
		}

		for _, device := range providerDevices {
			ctxlog.Info(ctx, c.logger, "Deleting device",
				log.String("skill_id", skillID),
				log.String("device_id", device.ID),
				log.String("device_external_id", device.ExternalID),
				log.String("device_type", string(device.OriginalType)),
				log.String("delete_reason", model.DeleteReasonUnlink))
		}

		c.updatesController.AsyncNotifyAboutDeviceListUpdates(ctx, user, updates.UnlinkSource)
	}

	// ignore error
	if err := c.dbClient.DeleteExternalUser(ctx, skillID, user); err != nil {
		ctxlog.Warnf(ctx, c.logger, "external user delete failed: %v", err)
	}

	// ignore error here too - what can we do?
	if err := c.dbClient.DeleteUserSkill(ctx, user.ID, skillID); err != nil {
		ctxlog.Warnf(ctx, c.logger, "user skill delete failed: %v", err)
	}

	if skillID == model.VIRTUAL || skillID == model.QUALITY || skillID == model.UIQUALITY || skillID == model.YANDEXIO {
		return nil // VIRTUAL, QUALITY and UIQUALITY unlink actions end here
	}

	skillInfo, err := c.dialogs.GetSkillInfo(ctx, skillID, user.Ticket)
	if err != nil {
		return xerrors.Errorf("Failed to get skill info: %w", err)
	}
	socialismSkillInfo := socialism.NewSkillInfo(skillInfo.SkillID, skillInfo.ApplicationName, skillInfo.Trusted)
	bounded, err := c.socialism.CheckUserAppTokenExists(ctx, user.ID, socialismSkillInfo)
	if err != nil {
		return xerrors.Errorf("failed to check token existence in socialism: %w", err)
	}

	if !bounded {
		return nil
	}

	iProvider, err := c.providerFactory.NewProviderClient(ctx, origin, skillID)
	if err != nil {
		return xerrors.Errorf("failed to obtain provider: %w", err)
	}
	unlinkCtx, cancel := context.WithTimeout(ctx, unlinkProviderHandlerTimeout)
	defer cancel()
	err = iProvider.Unlink(unlinkCtx)
	provider.RecordMetricsOnUnlink(err, iProvider.GetSkillSignals())
	if err != nil {
		// ignore error here also - what can we do?
		ctxlog.Warnf(ctx, c.logger, "provider unlink call failed: %v", err)
	}
	socialismSkillInfo = socialism.NewSkillInfo(skillInfo.SkillID, skillInfo.ApplicationName, skillInfo.Trusted)
	if err := c.socialism.DeleteUserToken(ctx, user.ID, socialismSkillInfo); err != nil {
		return xerrors.Errorf("failed to delete token: %w", err)
	}

	return nil
}

func (c *controller) DeleteDevices(ctx context.Context, userID uint64, deviceIDs []string) error {
	if err := c.dbClient.DeleteUserDevices(ctx, userID, deviceIDs); err != nil {
		ctxlog.Warnf(ctx, c.logger, "failed to delete devices: %s", err)
		return err
	}
	scenarios, err := c.dbClient.SelectUserScenariosSimple(ctx, userID)
	if err != nil {
		ctxlog.Warnf(ctx, c.logger, "failed to sync local scenarios after device deletion: %s", err)
		return nil
	}
	devices, err := c.dbClient.SelectUserDevicesSimple(ctx, userID)
	if err != nil {
		ctxlog.Warnf(ctx, c.logger, "failed to sync local scenarios after device deletion: %s", err)
		return nil
	}
	if err := c.localScenariosController.SyncLocalScenarios(ctx, userID, scenarios, devices); err != nil {
		ctxlog.Warnf(ctx, c.logger, "failed to sync local scenarios after device deletion: %s", err)
		return nil
	}
	return nil
}
