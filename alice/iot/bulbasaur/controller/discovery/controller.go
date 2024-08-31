package discovery

import (
	"context"
	"encoding/json"
	"fmt"
	"golang.org/x/sync/errgroup"
	"runtime/debug"
	"strings"

	"github.com/gofrs/uuid"
	"github.com/mitchellh/mapstructure"
	"golang.org/x/exp/slices"

	"a.yandex-team.ru/alice/iot/bulbasaur/controller/experiments"
	"a.yandex-team.ru/alice/iot/bulbasaur/controller/notificator"
	"a.yandex-team.ru/alice/iot/bulbasaur/controller/quasarconfig"
	"a.yandex-team.ru/alice/iot/bulbasaur/controller/sup"
	"a.yandex-team.ru/alice/iot/bulbasaur/controller/unlink"
	"a.yandex-team.ru/alice/iot/bulbasaur/controller/updates"
	"a.yandex-team.ru/alice/iot/bulbasaur/db"
	"a.yandex-team.ru/alice/iot/bulbasaur/dto/adapter"
	"a.yandex-team.ru/alice/iot/bulbasaur/dto/callback"
	"a.yandex-team.ru/alice/iot/bulbasaur/megamind/frames"
	"a.yandex-team.ru/alice/iot/bulbasaur/model"
	"a.yandex-team.ru/alice/iot/bulbasaur/model/tuya"
	"a.yandex-team.ru/alice/iot/bulbasaur/provider"
	"a.yandex-team.ru/alice/library/go/contexter"
	"a.yandex-team.ru/alice/library/go/goroutines"
	"a.yandex-team.ru/alice/library/go/recorder"
	"a.yandex-team.ru/alice/library/go/requestsource"
	"a.yandex-team.ru/alice/library/go/timestamp"
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/log/ctxlog"
	"a.yandex-team.ru/library/go/core/xerrors"
)

type Controller struct {
	Logger                log.Logger
	ProviderFactory       provider.IProviderFactory
	Database              db.DB
	Sup                   sup.IController
	AppLinksGenerator     sup.AppLinksGenerator
	UpdatesController     updates.IController
	NotificatorController notificator.IController
	UnlinkController      unlink.Controller
	QuasarController      quasarconfig.IController
}

func NewController(
	logger log.Logger,
	providerFactory provider.IProviderFactory,
	database db.DB,
	sup sup.IController,
	appLinksGenerator sup.AppLinksGenerator,
	updatesController updates.IController,
	notificatorController notificator.IController,
	unlinkController unlink.Controller,
	quasarController quasarconfig.IController,
) *Controller {
	return &Controller{
		logger,
		providerFactory,
		database,
		sup,
		appLinksGenerator,
		updatesController,
		notificatorController,
		unlinkController,
		quasarController,
	}
}

func (s *Controller) ProviderDiscovery(ctx context.Context, origin model.Origin, skillID string) (model.Devices, error) {
	iProvider, err := s.ProviderFactory.NewProviderClient(ctx, origin, skillID)
	if err != nil {
		return nil, xerrors.Errorf("cannot get user device provider %q: %w", skillID, err)
	}

	if iProvider == nil {
		return nil, &model.SkillNotFoundError{}
	}

	// Create new context to ensure that the process succeeds when the client connection is closed (499 error)
	// cause current context will be closed in that case
	ctx = contexter.NoCancel(ctx)
	devConsoleLogger := recorder.GetLoggerWithDebugInfoBySkillID(s.Logger, ctx, skillID)

	//discover
	discoveryResult, err := iProvider.Discover(ctx)
	provider.RecordMetricsOnDiscovery(discoveryResult, err, iProvider.GetSkillSignals())
	if err != nil {
		ctxlog.Warnf(ctx, devConsoleLogger, "Error during discovery, user: %d, skillId: %s, error: %s", origin.User.ID, skillID, err)
		switch {
		case xerrors.Is(err, &provider.HTTPAuthorizationError{}):
			return nil, xerrors.Errorf("discover request failed: %w", &model.DiscoveryAuthorizationError{})
		default:
			return nil, xerrors.Errorf("discover request failed: %w", &model.DiscoveryErrorError{})
		}
	}

	// postprocess devices
	skillInfo := iProvider.GetSkillInfo()
	return s.DiscoveryPostprocessing(ctx, skillInfo, origin, discoveryResult)
}

func (s *Controller) StartDiscoveryFromSurface(ctx context.Context, origin model.Origin, targetSurface model.SurfaceParameters, discoveryType DiscoveryType) error {
	var speakerDevice model.Device
	switch targetSurface.SurfaceType() {
	case model.SpeakerSurfaceType:
		speakerID := targetSurface.(model.SpeakerSurfaceParameters).ID
		device, err := s.Database.SelectUserDevice(ctx, origin.User.ID, speakerID)
		if err != nil {
			ctxlog.Warnf(ctx, s.Logger, "failed to get speaker %s for user %d: %s", speakerID, origin.User.ID, err)
			return &model.SpeakerDiscoveryInternalError{}
		}
		speakerDevice = device
	case model.StereopairSurfaceType:
		// frontend uses leader speaker id as stereopair id
		speakerID := targetSurface.(model.StereopairSurfaceParameters).ID
		stereopairs, err := s.Database.SelectStereopairs(ctx, origin.User.ID)
		if err != nil {
			ctxlog.Warnf(ctx, s.Logger, "failed to get stereopairs for user %d: %s", origin.User.ID, err)
			return &model.SpeakerDiscoveryInternalError{}
		}
		if stereopairs.GetDeviceRole(speakerID) == model.FollowerRole {
			ctxlog.Warnf(ctx, s.Logger, "cannot start discovery for stereopair follower %s: %s", speakerID, err)
			return &model.SpeakerDiscoveryInternalError{}
		}
		stereopair, ok := stereopairs.GetByDeviceID(speakerID)
		if !ok {
			ctxlog.Warnf(ctx, s.Logger, "failed to get stereopair for user %d and device %s: %v", origin.User.ID, speakerID, err)
			return &model.SpeakerDiscoveryInternalError{}
		}
		speakerDevice = stereopair.GetLeaderDevice()
	default:
		return &model.SpeakerDiscoveryInternalError{}
	}

	quasarSpeakerID, err := speakerDevice.GetExternalID()
	if err != nil {
		ctxlog.Warnf(ctx, s.Logger, "failed to get quasar speaker id, %s: %v", quasarSpeakerID, err)
		return &model.SpeakerDiscoveryInternalError{}
	}

	if !s.NotificatorController.IsDeviceOnline(ctx, origin.User.ID, quasarSpeakerID) {
		ctxlog.Warnf(ctx, s.Logger, "quasar speaker id %s: offline", quasarSpeakerID)
		return &model.DeviceUnreachableError{}
	}

	sessionID := uuid.Must(uuid.NewV4()).String()
	intentState := IntentState{
		DiscoveryType: discoveryType,
	}
	rawIntentState, err := json.Marshal(intentState)
	if err != nil {
		ctxlog.Warnf(ctx, s.Logger, "failed to marshal intent state: %v", err)
		return err
	}
	if err := s.Database.StoreUserIntentState(ctx, origin.User.ID, model.NewIntentStateKey(quasarSpeakerID, sessionID, "discovery"), rawIntentState); err != nil {
		ctxlog.Warnf(ctx, s.Logger, "failed to store user intent state: %v", err)
		return err
	}
	ctxlog.Info(ctx, s.Logger, "created session id for discovery", log.Any("session_id", sessionID))

	frame := &frames.StartDiscoveryFrame{
		Protocols: model.Protocols{model.ZigbeeProtocol},
		SessionID: sessionID,
	}
	if err := s.NotificatorController.SendTypedSemanticFrame(ctx, origin.User.ID, quasarSpeakerID, frame); err != nil {
		switch {
		case xerrors.Is(err, notificator.DeviceOfflineError):
			ctxlog.Warnf(ctx, s.Logger, "quasar speaker id %s: offline", quasarSpeakerID)
			return &model.DeviceUnreachableError{}
		default:
			ctxlog.Warnf(ctx, s.Logger, "quasar speaker id %s: %v", quasarSpeakerID, err)
			return &model.SpeakerDiscoveryInternalError{}
		}
	}
	return nil
}

// DiscoveryPostprocessing performs discovery side effects and validation
func (s *Controller) DiscoveryPostprocessing(
	ctx context.Context,
	skillInfo provider.SkillInfo,
	origin model.Origin,
	discoveryResult adapter.DiscoveryResult,
) (model.Devices, error) {
	user := origin.User
	discoveryResult.Payload = s.filterSupportedDevices(ctx, skillInfo, user, discoveryResult.Payload)

	validDevices, invalidDevices := s.validateDiscoveredDevices(ctx, skillInfo, discoveryResult)

	// record discovery metrics
	s.recordDiscoveryMetrics(ctx, skillInfo, validDevices, invalidDevices)

	// get list of domain devices
	discoveryResult.Payload.Devices = validDevices
	if discoveryResult.Timestamp == 0 {
		discoveryResult.Timestamp = timestamp.CurrentTimestampCtx(ctx)
	}

	// create User with Household
	err := s.Database.StoreUser(ctx, user)
	if err != nil {
		return nil, xerrors.Errorf("failed to create new user with household: %w", err)
	}
	devices := discoveryResult.ToDevices(skillInfo.SkillID)

	// store External user for external skills
	isInternalSkill := slices.Contains(model.KnownInternalProviders, skillInfo.SkillID)
	if !isInternalSkill {
		if len(discoveryResult.Payload.UserID) > 0 {
			if err := s.Database.StoreExternalUser(ctx, discoveryResult.Payload.UserID, skillInfo.SkillID, user); err != nil {
				ctxlog.Warnf(ctx, s.Logger, "can't store external id of user: %d, skillId: %s, error: %s", user.ID, skillInfo.SkillID, err)
			}
			// Draft skills must not be stored in the UserSkills table: IOT-1034
			if !skillInfo.IsDraft() {
				if err := s.Database.StoreUserSkill(ctx, user.ID, skillInfo.SkillID); err != nil {
					ctxlog.Warnf(ctx, s.Logger, "Can't store user skill. UserId: %d, skillId: %s, error: %s",
						user.ID, skillInfo.SkillID, err)
				}
			}
		} else {
			ctxlog.Warnf(ctx, s.Logger, "During discovery of provider %s external id of user %d was empty", skillInfo.SkillID, user.ID)
		}
	}

	if len(devices) == 0 {
		return nil, nil
	}

	// Sync IR controls rooms with IR Hub room
	// https://st.yandex-team.ru/QUASAR-4001
	userDevices := make(model.Devices, 0)
	for i, device := range devices {
		if device.SkillID != model.TUYA {
			continue
		}
		var cd tuya.CustomData
		if err := mapstructure.Decode(device.CustomData, &cd); err != nil {
			continue
		}
		if hubID, isInfraredDevice := cd.GetIRParentHubExtID(); isInfraredDevice {
			if len(userDevices) == 0 { // prevent database spamming
				uDevices, err := s.Database.SelectUserDevices(ctx, user.ID)
				if err != nil {
					break
				}
				userDevices = uDevices
			}
			if hubDevice, hubExists := userDevices.GetDeviceByExtID(hubID); hubExists {
				device.Room = hubDevice.Room
				devices[i] = device
			}
		}
	}

	//replace non-latin and non-cyrillic names with sane defaults
	if !isInternalSkill {
		for i := range devices {
			trimmedStr := strings.TrimSpace(devices[i].Name)
			words := strings.Split(trimmedStr, " ")
			for _, word := range words {
				if !discoveryNameRegex.MatchString(word) {
					name := devices[i].Type.GenerateDeviceName()
					devices[i].ExternalName = name
					devices[i].Name = name
					break
				}
			}
		}
	}

	//run async postprocessing
	backgroundCtx := contexter.NoCancel(ctx)
	s.postprocessingAsync(backgroundCtx, origin, skillInfo, devices, discoveryResult)

	return devices, nil
}

// postprocessingAsync runs after discovery in background
func (s *Controller) postprocessingAsync(
	ctx context.Context,
	origin model.Origin,
	skillInfo provider.SkillInfo,
	devices model.Devices,
	discoveryResult adapter.DiscoveryResult,
) {
	go func() {
		defer func() {
			if r := recover(); r != nil {
				msg := fmt.Sprintf("caught panic in async postprocessing: %v", r)
				ctxlog.Info(ctx, s.Logger, msg, log.Any("stacktrace", string(debug.Stack())))
			}
		}()
		if err := s.UnlinkController.DeleteChangedOwnerDevices(ctx, origin.User.ID, skillInfo.SkillID, devices); err != nil {
			ctxlog.Warnf(ctx, s.Logger, "failed to delete changed owner devices: %s", err)
		}

		if err := s.unlinkOtherXiaomiUsers(ctx, origin, skillInfo.SkillID, discoveryResult); err != nil {
			ctxlog.Warnf(ctx, s.Logger, "failed to unlink other users for skill %s: %v", skillInfo.SkillID, err)
		}
	}()
}

func (s *Controller) unlinkOtherXiaomiUsers(
	ctx context.Context,
	origin model.Origin,
	skillID string,
	discoveryResult adapter.DiscoveryResult) error {
	// unlink other users only for xiaomi
	// xiaomi allows to have only one active client https://st.yandex-team.ru/IOT-1526
	if skillID != model.XiaomiSkill {
		return nil
	}

	if origin.SurfaceParameters.SurfaceType() != model.SearchAppSurfaceType {
		return nil
	}

	if !experiments.UnlinkOtherXiaomiAccountsAfterDiscovery.IsEnabledForUser(ctx, origin.User.ToUserctxUser()) {
		return nil
	}

	// select all yandex users connected to the given external account
	skillUsers, err := s.Database.SelectExternalUsers(ctx, discoveryResult.Payload.UserID, skillID)
	if err != nil {
		return xerrors.Errorf("failed to load external users by external_user_id: %s, skill_id: %s, %w", discoveryResult.Payload.UserID, skillID, err)
	}

	wg := errgroup.Group{}
	for _, unlinkUser := range skillUsers {
		if unlinkUser.ID == origin.User.ID { // unlink other users from xiaomi
			continue
		}
		unlinkOrigin := origin.ToSharedOrigin(unlinkUser.ID)
		wg.Go(func() error {
			if err := s.sendUnlinkToProvider(ctx, skillID, unlinkOrigin); err != nil {
				return fmt.Errorf("failed to unlink xiaomi for user: %d", unlinkOrigin.User.ID)
			}
			return nil
		})
	}

	return wg.Wait()
}

func (s *Controller) sendUnlinkToProvider(ctx context.Context, skillID string, unlinkOrigin model.Origin) error {
	if err := s.UnlinkController.UnlinkProvider(ctx, skillID, unlinkOrigin, true); err != nil {
		return fmt.Errorf("failed to unlink user from skill %s: %w", skillID, err)
	}

	// send push on unlink only for xiaomi
	if skillID != model.XiaomiSkill {
		return nil
	}

	providerDevices, err := s.Database.SelectUserProviderDevicesSimple(ctx, unlinkOrigin.User.ID, skillID)
	if err != nil {
		return fmt.Errorf("failed to select devices from db: %w", err)
	}

	if len(providerDevices.NonSharedDevices()) == 0 {
		ctxlog.Infof(ctx, s.Logger, "user %d has no %s devices, skip sending push after unlink", unlinkOrigin.User.ID, skillID)
		return nil
	}

	err = s.Sup.SendPushToUser(ctx, unlinkOrigin.User, sup.PushInfo{
		ID:               sup.SkillsForcedUnlinked,
		Text:             "Ваша интеграция с аккаунтом Xiaomi отключена, потому что управлять им может только один пользователь.",
		Link:             s.AppLinksGenerator.BuildSkillIntegrationLink(skillID),
		ThrottlePolicyID: sup.SkillsForcedUnlinkedThrottlePolicy,
		Tag:              skillID,
	})
	if err != nil {
		return xerrors.Errorf("failed to send push for unlinked user %d from skill %s: %w",
			unlinkOrigin.User.ID,
			skillID,
			err)
	}

	return nil
}

func (s *Controller) validateDiscoveredDevices(ctx context.Context, skillInfo provider.SkillInfo, result adapter.DiscoveryResult) (validDevices, invalidDevices []adapter.DeviceInfoView) {
	skillID := skillInfo.SkillID
	devConsoleLogger := recorder.GetLoggerWithDebugInfoBySkillID(s.Logger, ctx, skillID)
	for _, device := range result.Payload.Devices {
		vctx := adapter.NewDiscoveryInfoViewValidationContext(ctx, s.Logger, skillID, skillInfo.Trusted)
		if _, err := device.Validate(vctx); err != nil { // todo: valid.Struct or even separate method as in other dto objects
			ctxlog.Warnf(ctx, devConsoleLogger, "device %s from provider %s validation failed, will skip it: %s", device.ID, skillID, err)
			invalidDevices = append(invalidDevices, device)
		} else {
			validDevices = append(validDevices, device)
		}
	}
	return validDevices, invalidDevices
}

func (s *Controller) recordDiscoveryMetrics(ctx context.Context, skillInfo provider.SkillInfo, validDevices, invalidDevices []adapter.DeviceInfoView) {
	if signals := s.getSignals(ctx, skillInfo); signals != nil {
		signals.IncDiscoverySuccess(len(validDevices))
		signals.IncDiscoveryValidationError(len(invalidDevices))
	}
}

func (s *Controller) getSignals(ctx context.Context, skillInfo provider.SkillInfo) *provider.Signals {
	if skillSignals := s.ProviderFactory.GetSignalsRegistry(); skillSignals != nil {
		signals := skillSignals.GetSignals(ctx, requestsource.GetRequestSource(ctx), skillInfo)
		return &signals
	}
	return nil
}

func (s *Controller) StoreDiscoveredDevices(ctx context.Context, user model.User, devices model.Devices) (model.DeviceStoreResults, error) {
	var dsrs model.DeviceStoreResults
	for _, device := range devices {
		storedDevice, storeResult, err := s.Database.StoreUserDevice(ctx, user, device)
		if err != nil {
			ctxlog.Warnf(ctx, s.Logger, "failed to store discovered device (deviceId:%s, userId:%d, skillId:%s): %s", device.ExternalID, user.ID, device.SkillID, err)
		}
		dsrs = append(dsrs, model.DeviceStoreResult{
			Device: storedDevice,
			Result: storeResult,
		})
	}
	go goroutines.SafeBackground(contexter.NoCancel(ctx), s.Logger, func(ctx context.Context) {
		if err := s.QuasarController.UpdateDevicesLocation(ctx, user, dsrs.ChooseNewOrUpdated().Devices()); err != nil {
			ctxlog.Warnf(ctx, s.Logger, "failed to update devices location for user %d: %v", user.ID, err)
		}
	})

	s.UpdatesController.AsyncNotifyAboutDeviceListUpdates(ctx, user, updates.DiscoverySource)
	return dsrs, nil
}

func (s *Controller) SendNewDevicesPush(ctx context.Context, user model.User, skillInfo provider.SkillInfo, newDevices []DeviceDiffInfo) error {
	return s.Sup.SendPushToUser(ctx, user, getNewDevicesPushInfo(newDevices, skillInfo, s.AppLinksGenerator.BuildDeviceListPageLink()))
}

func (s *Controller) filterSupportedDevices(ctx context.Context, skillInfo provider.SkillInfo, user model.User, payload adapter.DiscoveryPayload) adapter.DiscoveryPayload {
	filteredPayload := adapter.DiscoveryPayload{
		UserID:  payload.UserID,
		Devices: make([]adapter.DeviceInfoView, 0, len(payload.Devices)),
	}

	for _, device := range payload.Devices {
		supportedProperties := make([]adapter.PropertyInfoView, 0, len(device.Properties))
		for _, property := range device.Properties {
			if !s.isSupportedProperty(ctx, skillInfo, user, property) {
				ctxlog.Infof(ctx, s.Logger, "Device property `%s` (skill_id: `%s`) is skipped for user `%d`",
					property.Type, skillInfo.SkillID, user.ID)

				devConsoleLogger := recorder.GetLoggerWithDebugInfoBySkillID(s.Logger, ctx, skillInfo.SkillID)
				ctxlog.Warnf(ctx, devConsoleLogger,
					"property %s for device %s from provider %s validation failed, will skip it",
					property.Type, device.ID, skillInfo.SkillID)

				continue
			}
			supportedProperties = append(supportedProperties, property)
		}
		device.Properties = supportedProperties

		filteredPayload.Devices = append(filteredPayload.Devices, device)
	}

	return filteredPayload
}

func (s *Controller) isSupportedProperty(ctx context.Context, _ provider.SkillInfo, user model.User, property adapter.PropertyInfoView) bool {
	switch property.Type {
	case model.FloatPropertyType, model.EventPropertyType:
		return true
	}
	return false
}

func (s *Controller) PushDiscovery(ctx context.Context, skillID string, origin model.Origin, result adapter.DiscoveryResult) (model.DeviceStoreResults, error) {
	ctxlog.Infof(ctx, s.Logger, "starting push discovery for user %d and skill %s", origin.User.ID, skillID)
	skillInfo, err := s.ProviderFactory.SkillInfo(ctx, skillID, "") // todo: use skillOwner ticket here
	if err != nil {
		return nil, err
	}

	if skillID == model.QUASAR {
		// populate speakers with text action and phrase capabilities
		result = adapter.PopulateDiscoveryResultWithQuasarCapabilities(ctx, result)
	}

	oldDevices, err := s.Database.SelectUserProviderDevicesSimple(ctx, origin.User.ID, skillID)
	if err != nil {
		return nil, xerrors.Errorf("failed to select provider %s devices for user %d: %w", skillID, origin.User.ID, err)
	}
	devices, err := s.DiscoveryPostprocessing(ctx, skillInfo, origin, result)
	if err != nil {
		return nil, err
	}

	storeResults, err := s.StoreDiscoveredDevices(ctx, origin.User, devices)
	if err != nil {
		return nil, nil
	}

	failedDevices := make([]string, 0)
	discoveredDevices := make([]string, 0)
	updatedDevices := make([]string, 0)
	for _, storeResult := range storeResults {
		switch storeResult.Result {
		case model.StoreResultNew:
			discoveredDevices = append(discoveredDevices, storeResult.ID)
		case model.StoreResultUpdated:
			updatedDevices = append(updatedDevices, storeResult.ID)
		case model.StoreResultLimitReached, model.StoreResultUnknownError:
			failedDevices = append(failedDevices, storeResult.ID)
		}
	}
	ctxlog.Infof(ctx, log.With(s.Logger, log.Any("push_discovery_result", map[string]interface{}{
		"failed_devices_count":     len(failedDevices),
		"updated_devices_count":    len(updatedDevices),
		"discovered_devices_count": len(discoveredDevices),
		"failed_devices":           failedDevices,
		"updated_devices":          updatedDevices,
		"discovered_devices":       discoveredDevices,
	})), "push discovery for user %d finished", origin.User.ID)

	var userDiffInfo UserDiffInfo
	userDiffInfo.FromOldAndDiscoveredDevices(oldDevices, devices, storeResults)
	newDevicesDiffInfo := userDiffInfo.GetDevicesWithDiffStatus(NewDiffStatus)
	filteredDevicesDiffInfo := newDevicesDiffInfo.FilterDevicesForNotPushing(skillID)
	if len(filteredDevicesDiffInfo) != 0 {
		if err := s.SendNewDevicesPush(ctx, origin.User, skillInfo, filteredDevicesDiffInfo); err != nil {
			ctxlog.Warnf(ctx, s.Logger, "failed to send push about new devices from provider %s to user %d: %v", skillID, origin.User.ID, err)
		}
	}
	return storeResults, nil
}

func (s *Controller) CallbackDiscovery(ctx context.Context, skillID string, origin model.Origin, devices model.Devices, filter callback.IDiscoveryFilter) (model.DeviceStoreResults, error) {
	ctxlog.Infof(ctx, s.Logger, "trying to callback discovery for user %d and skill %s", origin.User.ID, skillID)

	archivedDevicesMap, err := s.Database.SelectUserProviderArchivedDevicesSimple(ctx, origin.User.ID, skillID)
	if err != nil {
		ctxlog.Warnf(ctx, s.Logger, "failed to get archived devices for user %d and skill %s from db: %v", origin.User.ID, skillID, err)
		return nil, err
	}
	existDevices, err := s.Database.SelectUserProviderDevicesSimple(ctx, origin.User.ID, skillID)
	if err != nil {
		ctxlog.Warnf(ctx, s.Logger, "failed to get devices for user %d and skill %s from db: %v", origin.User.ID, skillID, err)
		return nil, err
	}
	existDevicesMap := existDevices.ToExternalIDMap()
	deviceStoreResults := make(model.DeviceStoreResults, 0)

	failedDevices := make([]string, 0)
	discoveredDevices := make([]string, 0)
	updatedDevices := make([]string, 0)
	skippedDevices := make([]string, 0)
	filteredDevices := make([]string, 0)
	archivedDevices := make([]string, 0)
	for _, device := range devices {
		// before storing we only know external id
		if filtered, msg := filter.FilterOut(device); filtered {
			ctxlog.Infof(ctx, s.Logger, "device %s discovered, but filtered: %s", device.ExternalID, msg)
			filteredDevices = append(filteredDevices, device.ExternalID)
			continue
		}
		_, archivedPreviously := archivedDevicesMap[device.ExternalID]
		_, existNow := existDevicesMap[device.ExternalID]
		if !existNow && archivedPreviously {
			ctxlog.Infof(ctx, s.Logger, "device %s discovered, but was archived previously, skip it", device.ExternalID)
			archivedDevices = append(archivedDevices, device.ExternalID)
			continue
		}
		storedDevice, storeResult, _ := s.Database.StoreUserDevice(ctx, origin.User, device)
		switch storeResult {
		case model.StoreResultNew:
			ctxlog.Infof(ctx, s.Logger, "Discovered new device %s for user %d", storedDevice.ID, origin.User.ID)
			discoveredDevices = append(discoveredDevices, device.ExternalID)
		case model.StoreResultUpdated:
			ctxlog.Infof(ctx, s.Logger, "Updated device %s for user %d", storedDevice.ID, origin.User.ID)
			updatedDevices = append(updatedDevices, device.ExternalID)
		case model.StoreResultLimitReached:
			ctxlog.Warnf(ctx, s.Logger, "Error during storing discovered device (deviceId:%s, userId:%d, skillId:%s): %v", device.ExternalID, origin.User.ID, device.SkillID, &model.DeviceLimitReachedError{})
			failedDevices = append(failedDevices, device.ExternalID)
		default:
			ctxlog.Warnf(ctx, s.Logger, "Error during storing discovered device (deviceId:%s, userId:%d, skillId:%s): %v", device.ExternalID, origin.User.ID, device.SkillID, err)
			failedDevices = append(failedDevices, device.ExternalID)
		}
		deviceStoreResults = append(deviceStoreResults, model.DeviceStoreResult{
			Device: storedDevice,
			Result: storeResult,
		})
	}

	if err := s.QuasarController.UpdateDevicesLocation(ctx, origin.User, deviceStoreResults.ChooseNewOrUpdated().Devices()); err != nil {
		ctxlog.Warnf(ctx, s.Logger, "Error during updating location for devices %v: %s", deviceStoreResults.ChooseNewOrUpdated().IDs(), err)
	}

	ctxlog.Info(ctx, s.Logger, "Discovery finished",
		log.Any("callback_discovery_result", map[string]interface{}{
			"archived_devices_count":   len(archivedDevices),
			"failed_devices_count":     len(failedDevices),
			"discovered_devices_count": len(discoveredDevices),
			"updated_devices_count":    len(updatedDevices),
			"skipped_devices_count":    len(skippedDevices),
			"filtered_devices_count":   len(filteredDevices),
			"archived_devices":         archivedDevices,
			"failed_devices":           failedDevices,
			"discovered_devices":       discoveredDevices,
			"updated_devices":          updatedDevices,
			"skipped_devices":          skippedDevices,
			"filtered_devices":         filteredDevices,
		}))

	oldDevices, err := s.Database.SelectUserProviderDevicesSimple(ctx, origin.User.ID, skillID)
	if err != nil {
		ctxlog.Warnf(ctx, s.Logger, "failed to select provider %s devices for user %d: %v", skillID, origin.User.ID, err)
		return nil, err
	}

	var userDiffInfo UserDiffInfo
	userDiffInfo.FromOldAndDiscoveredDevices(oldDevices, devices, deviceStoreResults)
	newDevicesDiffInfo := userDiffInfo.GetDevicesWithDiffStatus(NewDiffStatus)
	filteredDevicesDiffInfo := newDevicesDiffInfo.FilterDevicesForNotPushing(skillID)
	if len(filteredDevicesDiffInfo) != 0 {
		skillInfo, err := s.ProviderFactory.SkillInfo(ctx, skillID, "") // todo: use skillOwner ticket here
		if err != nil {
			ctxlog.Warnf(ctx, s.Logger, "failed to get skillInfo for skill %s and user %d: %v", skillID, origin.User.ID, err)
			return nil, err
		}
		if err := s.SendNewDevicesPush(ctx, origin.User, skillInfo, filteredDevicesDiffInfo); err != nil {
			ctxlog.Warnf(ctx, s.Logger, "failed to send push about new devices from provider %s to user %d: %v", skillID, origin.User.ID, err)
			return nil, err
		}
	}
	return deviceStoreResults, nil
}
