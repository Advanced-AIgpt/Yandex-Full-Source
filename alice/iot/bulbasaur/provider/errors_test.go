package provider

import (
	"testing"

	"a.yandex-team.ru/alice/iot/bulbasaur/model"
	"github.com/stretchr/testify/assert"
	"golang.org/x/xerrors"
)

func TestError(t *testing.T) {
	t.Run("DEVICE_UNREACHABLE", func(t *testing.T) {
		err := NewError(model.DeviceUnreachable, "ZZzzzz")
		assert.EqualError(t, err, "[DEVICE_UNREACHABLE] ZZzzzz")
		assert.True(t, xerrors.Is(err, ErrorDeviceUnreachable))
		assert.False(t, xerrors.Is(err, ErrorUnknownError))
	})

	t.Run("DEVICE_BUSY", func(t *testing.T) {
		err := NewError(model.DeviceBusy, "calm down!")
		assert.EqualError(t, err, "[DEVICE_BUSY] calm down!")
		assert.True(t, xerrors.Is(err, ErrorDeviceBusy))
		assert.False(t, xerrors.Is(err, ErrorUnknownError))
	})

	t.Run("DEVICE_NOT_FOUND", func(t *testing.T) {
		err := NewError(model.DeviceNotFound, "there is no device, Neo")
		assert.EqualError(t, err, "[DEVICE_NOT_FOUND] there is no device, Neo")
		assert.True(t, xerrors.Is(err, ErrorDeviceNotFound))
		assert.False(t, xerrors.Is(err, ErrorUnknownError))
	})

	t.Run("INTERNAL_ERROR", func(t *testing.T) {
		err := NewError(model.InternalError, "i'm dead")
		assert.EqualError(t, err, "[INTERNAL_ERROR] i'm dead")
		assert.True(t, xerrors.Is(err, ErrorInternalError))
		assert.False(t, xerrors.Is(err, ErrorUnknownError))
	})

	t.Run("INVALID_ACTION", func(t *testing.T) {
		err := NewError(model.InvalidAction, "stop that!")
		assert.EqualError(t, err, "[INVALID_ACTION] stop that!")
		assert.True(t, xerrors.Is(err, ErrorInvalidAction))
		assert.False(t, xerrors.Is(err, ErrorUnknownError))
	})

	t.Run("INVALID_VALUE", func(t *testing.T) {
		err := NewError(model.InvalidValue, "pi is not 3")
		assert.EqualError(t, err, "[INVALID_VALUE] pi is not 3")
		assert.True(t, xerrors.Is(err, ErrorInvalidValue))
		assert.False(t, xerrors.Is(err, ErrorUnknownError))
	})

	t.Run("NOT_SUPPORTED_IN_CURRENT_MODE", func(t *testing.T) {
		err := NewError(model.NotSupportedInCurrentMode, "not now")
		assert.EqualError(t, err, "[NOT_SUPPORTED_IN_CURRENT_MODE] not now")
		assert.True(t, xerrors.Is(err, ErrorNotSupportedInCurrentMode))
		assert.False(t, xerrors.Is(err, ErrorUnknownError))
	})

	t.Run("DOOR_OPEN", func(t *testing.T) {
		err := NewError(model.DoorOpen, "door open")
		assert.EqualError(t, err, "[DOOR_OPEN] door open")
		assert.True(t, xerrors.Is(err, ErrorDoorOpen))
		assert.False(t, xerrors.Is(err, ErrorInternalError))
	})

	t.Run("LID_OPEN", func(t *testing.T) {
		err := NewError(model.LidOpen, "lid open")
		assert.EqualError(t, err, "[LID_OPEN] lid open")
		assert.True(t, xerrors.Is(err, ErrorLidOpen))
		assert.False(t, xerrors.Is(err, ErrorInternalError))
	})

	t.Run("REMOTE_CONTROL_DISABLED", func(t *testing.T) {
		err := NewError(model.RemoteControlDisabled, "error")
		assert.EqualError(t, err, "[REMOTE_CONTROL_DISABLED] error")
		assert.True(t, xerrors.Is(err, ErrorRemoteControlDisabled))
		assert.False(t, xerrors.Is(err, ErrorInternalError))
	})

	t.Run("NOT_ENOUGH_WATER", func(t *testing.T) {
		err := NewError(model.NotEnoughWater, "error")
		assert.EqualError(t, err, "[NOT_ENOUGH_WATER] error")
		assert.True(t, xerrors.Is(err, ErrorNotEnoughWater))
		assert.False(t, xerrors.Is(err, ErrorInternalError))
	})

	t.Run("LOW_CHARGE_LEVEL", func(t *testing.T) {
		err := NewError(model.LowChargeLevel, "error")
		assert.EqualError(t, err, "[LOW_CHARGE_LEVEL] error")
		assert.True(t, xerrors.Is(err, ErrorLowChargeLevel))
		assert.False(t, xerrors.Is(err, ErrorInternalError))
	})

	t.Run("CONTAINER_FULL", func(t *testing.T) {
		err := NewError(model.ContainerFull, "error")
		assert.EqualError(t, err, "[CONTAINER_FULL] error")
		assert.True(t, xerrors.Is(err, ErrorContainerFull))
		assert.False(t, xerrors.Is(err, ErrorInternalError))
	})

	t.Run("CONTAINER_EMPTY", func(t *testing.T) {
		err := NewError(model.ContainerEmpty, "error")
		assert.EqualError(t, err, "[CONTAINER_EMPTY] error")
		assert.True(t, xerrors.Is(err, ErrorContainerEmpty))
		assert.False(t, xerrors.Is(err, ErrorInternalError))
	})

	t.Run("DRIP_TRAY_FULL", func(t *testing.T) {
		err := NewError(model.DripTrayFull, "error")
		assert.EqualError(t, err, "[DRIP_TRAY_FULL] error")
		assert.True(t, xerrors.Is(err, ErrorDripTrayFull))
		assert.False(t, xerrors.Is(err, ErrorInternalError))
	})

	t.Run("DEVICE_STUCK", func(t *testing.T) {
		err := NewError(model.DeviceStuck, "error")
		assert.EqualError(t, err, "[DEVICE_STUCK] error")
		assert.True(t, xerrors.Is(err, ErrorDeviceStuck))
		assert.False(t, xerrors.Is(err, ErrorInternalError))
	})

	t.Run("DEVICE_OFF", func(t *testing.T) {
		err := NewError(model.DeviceOff, "error")
		assert.EqualError(t, err, "[DEVICE_OFF] error")
		assert.True(t, xerrors.Is(err, ErrorDeviceOff))
		assert.False(t, xerrors.Is(err, ErrorInternalError))
	})

	t.Run("FIRMWARE_OUT_OF_DATE", func(t *testing.T) {
		err := NewError(model.FirmwareOutOfDate, "error")
		assert.EqualError(t, err, "[FIRMWARE_OUT_OF_DATE] error")
		assert.True(t, xerrors.Is(err, ErrorFirmwareOutOfDate))
		assert.False(t, xerrors.Is(err, ErrorInternalError))
	})

	t.Run("NOT_ENOUGH_DETERGENT", func(t *testing.T) {
		err := NewError(model.NotEnoughDetergent, "error")
		assert.EqualError(t, err, "[NOT_ENOUGH_DETERGENT] error")
		assert.True(t, xerrors.Is(err, ErrorNotEnoughDetergent))
		assert.False(t, xerrors.Is(err, ErrorInternalError))
	})

	t.Run("ACCOUNT_LINKING_ERROR", func(t *testing.T) {
		err := NewError(model.AccountLinkingError, "error")
		assert.EqualError(t, err, "[ACCOUNT_LINKING_ERROR] error")
		assert.True(t, xerrors.Is(err, ErrorAccountLinking))
		assert.False(t, xerrors.Is(err, ErrorInternalError))
	})

	t.Run("HUMAN_INVOLVEMENT_NEEDED", func(t *testing.T) {
		err := NewError(model.HumanInvolvementNeeded, "error")
		assert.EqualError(t, err, "[HUMAN_INVOLVEMENT_NEEDED] error")
		assert.True(t, xerrors.Is(err, ErrorHumanInvolvementNeeded))
		assert.False(t, xerrors.Is(err, ErrorInternalError))
	})

	t.Run("UNKNOWN_ERROR", func(t *testing.T) {
		err := NewError(model.UnknownError, "wut?!")
		assert.EqualError(t, err, "[UNKNOWN_ERROR] wut?!")
		assert.True(t, xerrors.Is(err, ErrorUnknownError))
		assert.False(t, xerrors.Is(err, ErrorInternalError))
	})

	t.Run("UNKNOWN_ERROR_2", func(t *testing.T) {
		err := NewError(model.ErrorCode("QUACK_QUACK"), "I'm a duck")
		assert.EqualError(t, err, "[UNKNOWN_ERROR] I'm a duck")
		assert.True(t, xerrors.Is(err, ErrorUnknownError))
		assert.False(t, xerrors.Is(err, ErrorInternalError))
	})

}
