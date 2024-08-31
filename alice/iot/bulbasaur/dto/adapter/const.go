package adapter

import "a.yandex-team.ru/alice/iot/bulbasaur/model"

type ActionStatus string

const (
	DONE       ActionStatus = "DONE"
	ERROR      ActionStatus = "ERROR"
	INPROGRESS ActionStatus = "IN_PROGRESS"
)

func (as ActionStatus) Priority() int {
	switch as {
	case ERROR:
		return 3
	case INPROGRESS:
		return 2
	case DONE:
		return 1
	default:
		return 0
	}
}

type ErrorCode model.ErrorCode

const (
	DeviceUnreachable = ErrorCode(model.DeviceUnreachable)
	DeviceBusy        = ErrorCode(model.DeviceBusy)
	DeviceNotFound    = ErrorCode(model.DeviceNotFound)
	InternalError     = ErrorCode(model.InternalError)

	InvalidAction             = ErrorCode(model.InvalidAction)
	InvalidValue              = ErrorCode(model.InvalidValue)
	NotSupportedInCurrentMode = ErrorCode(model.NotSupportedInCurrentMode)

	DoorOpen               = ErrorCode(model.DoorOpen)
	LidOpen                = ErrorCode(model.LidOpen)
	RemoteControlDisabled  = ErrorCode(model.RemoteControlDisabled)
	NotEnoughWater         = ErrorCode(model.NotEnoughWater)
	LowChargeLevel         = ErrorCode(model.LowChargeLevel)
	ContainerFull          = ErrorCode(model.ContainerFull)
	ContainerEmpty         = ErrorCode(model.ContainerEmpty)
	DripTrayFull           = ErrorCode(model.DripTrayFull)
	DeviceStuck            = ErrorCode(model.DeviceStuck)
	DeviceOff              = ErrorCode(model.DeviceOff)
	FirmwareOutOfDate      = ErrorCode(model.FirmwareOutOfDate)
	NotEnoughDetergent     = ErrorCode(model.NotEnoughDetergent)
	AccountLinkingError    = ErrorCode(model.AccountLinkingError)
	HumanInvolvementNeeded = ErrorCode(model.HumanInvolvementNeeded)
)

const InternalProviderUserIDHeader = "X-Ya-User-ID"
