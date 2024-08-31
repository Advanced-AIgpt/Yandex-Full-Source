package tuya

import (
	"net/http"

	"a.yandex-team.ru/alice/iot/adapters/tuya_adapter/dto/mobile"
	"a.yandex-team.ru/alice/iot/adapters/tuya_adapter/middleware"
	"a.yandex-team.ru/alice/iot/adapters/tuya_adapter/tuya"
	"a.yandex-team.ru/alice/iot/bulbasaur/model"
	"a.yandex-team.ru/alice/iot/bulbasaur/provider"
	"a.yandex-team.ru/alice/library/go/requestid"
	"a.yandex-team.ru/library/go/core/log/ctxlog"
	"github.com/go-chi/chi/v5"
)

func (s *Server) CheckDeviceFirmwareVersionHandler(w http.ResponseWriter, r *http.Request) {
	user, err := model.GetUserFromContext(r.Context())
	if err != nil {
		ctxlog.Warnf(r.Context(), s.Logger, "failed to get user from context: %v", err)
		s.render.RenderMobileError(r.Context(), w, err)
		return
	}

	// simple user check
	skillID := middleware.GetSkillID(r.Context())
	tuyaUserID, err := s.db.GetTuyaUserID(r.Context(), user.ID, skillID)
	if err != nil {
		ctxlog.Warnf(r.Context(), s.Logger, "failed to get Tuya user for uid %d. Reason: %v", user.ID, err)
		s.render.RenderMobileError(r.Context(), w, err)
		return
	}

	// Check user rights on device
	userDevices, err := s.getUserDevices(r.Context(), tuyaUserID, skillID)
	if err != nil {
		ctxlog.Warnf(r.Context(), s.Logger, "failed to get Tuya devices for uid %d. Reason: %v", user.ID, err)
		s.render.RenderMobileError(r.Context(), w, err)
		return
	}

	deviceID := chi.URLParam(r, "device_id") // external (tuya) IR device id

	device, ok := userDevices[deviceID]
	if !ok {
		ctxlog.Warnf(r.Context(), s.Logger, "User %d has no rights to control device %s", user.ID, deviceID)
		s.render.RenderMobileError(r.Context(), w, provider.ErrorDeviceNotFound)
		return
	}

	response := mobile.DeviceFirmwareVersionResponse{
		Status:        "ok",
		RequestID:     requestid.GetRequestID(r.Context()),
		UpgradeStatus: mobile.FirmwareUnknown,
	}

	if deviceConfig := device.GetDeviceConfig(); deviceConfig.Manufacturer != tuya.YandexDevicesManufacturer {
		ctxlog.Infof(r.Context(), s.Logger, "device %s manufacturer is not Yandex, skip its firmware updates", device.ID)
		response.UpgradeStatus = mobile.FirmwareActual
		s.render.RenderJSON(r.Context(), w, response)
		return
	}

	deviceFirmwareInfo, err := s.tuyaClient.GetDeviceFirmwareInfo(r.Context(), deviceID)
	device.FirmwareInfo = deviceFirmwareInfo
	if err != nil {
		ctxlog.Warnf(r.Context(), s.Logger, "failed to get tuya device firmware info for device %s. Reason: %v", deviceID, err)
		s.render.RenderMobileError(r.Context(), w, err)
		return
	}

	// filling response upgrade status if it exists
	upgradeStatus, moduleType := deviceFirmwareInfo.GetStatusAndModuleType()
	ctxlog.Infof(r.Context(), s.Logger, "device %s current firmware module type - %d, upgrade status: %d", device.ID, moduleType, upgradeStatus)

	switch upgradeStatus {
	case tuya.FirmwareUpgradeExceptionStatus:
		ctxlog.Warnf(r.Context(), s.Logger, "failed to upgrade device %s firmware. Reason: %v. ", deviceID, &tuya.ErrFirmwareUpgradeException{ModuleType: moduleType})
		s.render.RenderMobileError(r.Context(), w, &tuya.ErrFirmwareUpgradeException{ModuleType: moduleType})
		return
	case tuya.FirmwareUnknownUpgradeStatus:
		ctxlog.Warnf(r.Context(), s.Logger, "device %s got no modules for firmware upgrade", device.ID)
		s.render.RenderMobileError(r.Context(), w, &tuya.ErrModuleTypeNotExist{})
		return
	case tuya.FirmwareNoNeedUpgradeStatus:
		if err = s.pushDiscovery(r.Context(), tuyaUserID, []tuya.UserDevice{device}); err != nil {
			// ignore error for now
			ctxlog.Warnf(r.Context(), s.Logger, "failed to push discovery device %s for user %d: %v", deviceID, user.ID, err)
		}
	}

	if _, exist := mobile.KnownFirmwareUpgradeStatuses[upgradeStatus]; !exist {
		ctxlog.Warnf(r.Context(), s.Logger, "device %s got unknown firmware upgrade status from tuya: %d", device.ID, upgradeStatus)
		s.render.RenderMobileError(r.Context(), w, &tuya.ErrUnknownFirmwareStatus{FirmwareStatus: upgradeStatus})
		return
	}

	response.UpgradeStatus = mobile.KnownFirmwareUpgradeStatuses[upgradeStatus]

	s.render.RenderJSON(r.Context(), w, response)
}

func (s *Server) UpgradeDeviceFirmwareHandler(w http.ResponseWriter, r *http.Request) {
	user, err := model.GetUserFromContext(r.Context())
	if err != nil {
		ctxlog.Warnf(r.Context(), s.Logger, "failed to get user from context: %v", err)
		s.render.RenderMobileError(r.Context(), w, err)
		return
	}

	// simple user check
	skillID := middleware.GetSkillID(r.Context())
	tuyaUserID, err := s.db.GetTuyaUserID(r.Context(), user.ID, skillID)
	if err != nil {
		ctxlog.Warnf(r.Context(), s.Logger, "failed to get Tuya user for uid %d. Reason: %v", user.ID, err)
		s.render.RenderMobileError(r.Context(), w, err)
		return
	}

	// Check user rights on device
	deviceID := chi.URLParam(r, "device_id") // external (tuya) IR device id

	userDevices, err := s.getUserDevices(r.Context(), tuyaUserID, skillID)
	if err != nil {
		ctxlog.Warnf(r.Context(), s.Logger, "failed to get Tuya devices for uid %d. Reason: %v", user.ID, err)
		s.render.RenderMobileError(r.Context(), w, err)
		return
	}

	device, ok := userDevices[deviceID]
	if !ok {
		ctxlog.Warnf(r.Context(), s.Logger, "User %d has no rights to control device %s", user.ID, deviceID)
		s.render.RenderMobileError(r.Context(), w, provider.ErrorDeviceNotFound)
		return
	}

	response := mobile.UpgradeDeviceFirmwareResponse{
		Status:    "ok",
		RequestID: requestid.GetRequestID(r.Context()),
	}
	if deviceConfig := device.GetDeviceConfig(); deviceConfig.Manufacturer != tuya.YandexDevicesManufacturer {
		ctxlog.Infof(r.Context(), s.Logger, "device %s manufacturer is not Yandex, skip its firmware updates", device.ID)
		s.render.RenderJSON(r.Context(), w, response)
		return
	}

	deviceFirmwareInfo, err := s.tuyaClient.GetDeviceFirmwareInfo(r.Context(), deviceID)
	if err != nil {
		ctxlog.Warnf(r.Context(), s.Logger, "failed to get tuya device firmware info for device %s. Reason: %v", deviceID, err)
		s.render.RenderMobileError(r.Context(), w, err)
		return
	}

	upgradeStatus, moduleType := deviceFirmwareInfo.GetStatusAndModuleType()
	ctxlog.Infof(r.Context(), s.Logger, "device %s current firmware module type - %d, upgrade status: %d", device.ID, moduleType, upgradeStatus)
	switch upgradeStatus {
	case tuya.FirmwareUpgradeExceptionStatus:
		ctxlog.Warnf(r.Context(), s.Logger, "failed to upgrade device %s firmware. Reason: %v. ", deviceID, &tuya.ErrFirmwareUpgradeException{ModuleType: moduleType})
	case tuya.FirmwareUpgradingStatus, tuya.FirmwareNoNeedUpgradeStatus, tuya.FirmwareUpgradeCompleteStatus, tuya.FirmwareUnknownUpgradeStatus:
	default:
		if err = s.tuyaClient.UpgradeDeviceFirmware(r.Context(), deviceID, moduleType); err != nil {
			ctxlog.Warnf(r.Context(), s.Logger, "failed to upgrade tuya device firmware on module type %d for device %s. Reason: %v", moduleType, deviceID, err)
			s.render.RenderMobileError(r.Context(), w, err)
			return
		}
	}
	s.render.RenderJSON(r.Context(), w, response)
}
