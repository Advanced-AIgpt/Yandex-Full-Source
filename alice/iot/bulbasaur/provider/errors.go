package provider

import (
	"fmt"
	"net/http"

	"a.yandex-team.ru/alice/iot/bulbasaur/dto/adapter"
	"a.yandex-team.ru/alice/iot/bulbasaur/model"
)

var (
	ErrorDeviceUnreachable         = Error{code: model.DeviceUnreachable}
	ErrorDeviceBusy                = Error{code: model.DeviceBusy}
	ErrorDeviceNotFound            = Error{code: model.DeviceNotFound}
	ErrorInternalError             = Error{code: model.InternalError}
	ErrorInvalidAction             = Error{code: model.InvalidAction}
	ErrorInvalidValue              = Error{code: model.InvalidValue}
	ErrorNotSupportedInCurrentMode = Error{code: model.NotSupportedInCurrentMode}

	ErrorDoorOpen               = Error{code: model.DoorOpen}
	ErrorLidOpen                = Error{code: model.LidOpen}
	ErrorRemoteControlDisabled  = Error{code: model.RemoteControlDisabled}
	ErrorNotEnoughWater         = Error{code: model.NotEnoughWater}
	ErrorLowChargeLevel         = Error{code: model.LowChargeLevel}
	ErrorContainerFull          = Error{code: model.ContainerFull}
	ErrorContainerEmpty         = Error{code: model.ContainerEmpty}
	ErrorDripTrayFull           = Error{code: model.DripTrayFull}
	ErrorDeviceStuck            = Error{code: model.DeviceStuck}
	ErrorDeviceOff              = Error{code: model.DeviceOff}
	ErrorFirmwareOutOfDate      = Error{code: model.FirmwareOutOfDate}
	ErrorNotEnoughDetergent     = Error{code: model.NotEnoughDetergent}
	ErrorAccountLinking         = Error{code: model.AccountLinkingError}
	ErrorHumanInvolvementNeeded = Error{code: model.HumanInvolvementNeeded}

	ErrorUnknownError = Error{code: model.UnknownError}
)

type Error struct {
	code    model.ErrorCode
	message string
}

func NewError(code model.ErrorCode, message string) Error {
	switch code {
	case model.DeviceUnreachable,
		model.DeviceBusy,
		model.DeviceNotFound,
		model.InternalError,
		model.InvalidAction,
		model.InvalidValue,
		model.NotSupportedInCurrentMode,
		model.DoorOpen,
		model.LidOpen,
		model.RemoteControlDisabled,
		model.NotEnoughWater,
		model.LowChargeLevel,
		model.ContainerFull,
		model.ContainerEmpty,
		model.DripTrayFull,
		model.DeviceStuck,
		model.DeviceOff,
		model.FirmwareOutOfDate,
		model.NotEnoughDetergent,
		model.AccountLinkingError,
		model.HumanInvolvementNeeded:
		return Error{code: code, message: message}
	default:
		return Error{code: model.UnknownError, message: message}
	}
}

func (e Error) Error() string {
	return fmt.Sprintf("[%s] %s", e.code, e.message)
}

func (e Error) HTTPStatus() int {
	switch e.code {
	case model.DeviceNotFound:
		return http.StatusNotFound
	default:
		return http.StatusOK
	}
}

func (e Error) ErrorCode() model.ErrorCode {
	return e.code
}

func (e Error) MobileErrorMessage() string {
	switch e.code {
	case model.DeviceUnreachable:
		return model.DeviceUnreachableErrorMessage
	case model.DeviceBusy:
		return model.DeviceBusyErrorMessage
	case model.InvalidAction:
		return model.DeviceInvalidActionErrorMessage
	case model.InvalidValue:
		return model.DeviceInvalidValueErrorMessage
	case model.NotSupportedInCurrentMode:
		return model.DeviceNotSupportedInCurrentModeMessage
	case model.DoorOpen:
		return model.DoorOpenErrorMessage
	case model.LidOpen:
		return model.LidOpenErrorMessage
	case model.RemoteControlDisabled:
		return model.RemoteControlDisabledErrorMessage
	case model.NotEnoughWater:
		return model.NotEnoughWaterErrorMessage
	case model.LowChargeLevel:
		return model.LowChargeLevelErrorMessage
	case model.ContainerFull:
		return model.ContainerFullErrorMessage
	case model.ContainerEmpty:
		return model.ContainerEmptyErrorMessage
	case model.DripTrayFull:
		return model.DripTrayFullErrorMessage
	case model.DeviceStuck:
		return model.DeviceStuckErrorMessage
	case model.DeviceOff:
		return model.DeviceOffErrorMessage
	case model.FirmwareOutOfDate:
		return model.FirmwareOutOfDateErrorMessage
	case model.NotEnoughDetergent:
		return model.NotEnoughDetergentErrorMessage
	case model.AccountLinkingError:
		return model.AccountLinkingErrorErrorMessage
	case model.HumanInvolvementNeeded:
		return model.HumanInvolvementNeededErrorMessage
	case model.DeviceNotFound:
		return model.DeviceNotFoundErrorMessage
	case model.InternalError:
		return model.DeviceInternalErrorMessage
	case model.UnknownError:
		return model.UnknownErrorMessage
	default:
		return ""
	}
}

func (e Error) Is(target error) bool {
	targetError, ok := target.(Error)
	if !ok {
		return false
	}
	return targetError.code == e.code || targetError.code == ""
}

type StateError struct{ Err Error }

func NewStateError(dsv adapter.DeviceStateView) StateError {
	return StateError{Err: NewError(model.ErrorCode(dsv.ErrorCode), dsv.ErrorMessage)}
}

func (se StateError) Error() string {
	return fmt.Sprintf("State error: %s", se.Err.Error())
}

func (se StateError) IsKnown() bool {
	return se.Err.code != model.UnknownError
}

func (se StateError) Is(target error) bool {
	return se.Err.Is(target)
}

func (se StateError) DeviceState() model.DeviceStatus {
	if se.IsKnown() {
		switch {
		case se.Is(ErrorDeviceBusy) || se.Is(ErrorInvalidValue):
			return model.OnlineDeviceStatus
		case se.Is(ErrorDeviceUnreachable):
			return model.OfflineDeviceStatus
		case se.Is(ErrorDeviceNotFound):
			return model.NotFoundDeviceStatus
		}
	}
	return model.UnknownDeviceStatus
}

type BadResponseError struct {
	err error
}

func (bre BadResponseError) Error() string {
	return fmt.Sprintf("provider bad response error: %v", bre.err)
}

func (bre BadResponseError) Is(target error) bool {
	_, ok := target.(BadResponseError)
	return ok
}

func (bre BadResponseError) Unwrap() error {
	return bre.err
}

type HTTPAuthorizationError struct{}

func (hae *HTTPAuthorizationError) Error() string {
	return "Authorization Error"
}
