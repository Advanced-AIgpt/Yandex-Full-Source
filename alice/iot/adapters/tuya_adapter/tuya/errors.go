package tuya

import (
	"fmt"
	"net/http"

	"a.yandex-team.ru/alice/iot/bulbasaur/model"
)

const (
	RemoteMatchingTimeout    = "REMOTE_MATCHING_TIMEOUT"
	CodeLearningTimeout      = "CODE_LEARNING_TIMEOUT"
	UnknownFirmwareStatus    = "UNKNOWN_FIRMWARE_STATUS"
	FirmwareUpgradeException = "FIRMWARE_UPGRADE_EXCEPTION"
	CustomControlNotFound    = "CUSTOM_CONTROL_NOT_FOUND"
	CustomButtonNotFound     = "CUSTOM_BUTTON_NOT_FOUND"
	LastCustomButtonDeletion = "LAST_CUSTOM_BUTTON_DELETION"
)

type ErrUnknownIRCategory struct {
	Category IrCategoryID
}

func (e *ErrUnknownIRCategory) Error() string {
	return fmt.Sprintf("unknown IR category id %s", e.Category)
}

func (e *ErrUnknownIRCategory) HTTPStatus() int {
	return http.StatusBadRequest
}

func (e *ErrUnknownIRCategory) ErrorCode() model.ErrorCode {
	return "BAD_REQUEST"
}

type ErrUnknownMatchingType struct {
	MatchingType model.DeviceType
}

func (e *ErrUnknownMatchingType) Error() string {
	return fmt.Sprintf("unknown matching device type %s", e.MatchingType)
}

func (e *ErrUnknownMatchingType) HTTPStatus() int {
	return http.StatusBadRequest
}

func (e *ErrUnknownMatchingType) ErrorCode() model.ErrorCode {
	return "BAD_REQUEST"
}

type ErrIRMatchingRemotesTimeout struct{}

func (e *ErrIRMatchingRemotesTimeout) Error() string {
	return RemoteMatchingTimeout
}

func (e *ErrIRMatchingRemotesTimeout) HTTPStatus() int {
	return http.StatusOK
}

func (e *ErrIRMatchingRemotesTimeout) ErrorCode() model.ErrorCode {
	return RemoteMatchingTimeout
}

type ErrIRLearningCodeTimeout struct{}

func (e *ErrIRLearningCodeTimeout) Error() string {
	return CodeLearningTimeout
}

func (e *ErrIRLearningCodeTimeout) HTTPStatus() int {
	return http.StatusOK
}

func (e *ErrIRLearningCodeTimeout) ErrorCode() model.ErrorCode {
	return CodeLearningTimeout
}

type ErrUnknownFirmwareStatus struct {
	FirmwareStatus FirmwareUpgradeStatus
}

func (e *ErrUnknownFirmwareStatus) Error() string {
	return fmt.Sprintf("unknown firmware status from tuya: %d", e.FirmwareStatus)
}

func (e *ErrUnknownFirmwareStatus) HTTPStatus() int {
	return http.StatusOK
}

func (e *ErrUnknownFirmwareStatus) ErrorCode() model.ErrorCode {
	return UnknownFirmwareStatus
}

func (e *ErrUnknownFirmwareStatus) MobileErrorMessage() string {
	return UnableToGetFirmwareErrorMessage
}

type ErrModuleTypeNotExist struct{}

func (e *ErrModuleTypeNotExist) Error() string {
	return UnknownFirmwareStatus
}

func (e *ErrModuleTypeNotExist) HTTPStatus() int {
	return http.StatusOK
}

func (e *ErrModuleTypeNotExist) ErrorCode() model.ErrorCode {
	return UnknownFirmwareStatus
}

func (e *ErrModuleTypeNotExist) MobileErrorMessage() string {
	return UnableToGetFirmwareErrorMessage
}

type ErrFirmwareUpgradeException struct {
	ModuleType FirmwareModuleType
}

func (e *ErrFirmwareUpgradeException) Error() string {
	return fmt.Sprintf("upgrade exception on module %d", e.ModuleType)
}

func (e *ErrFirmwareUpgradeException) HTTPStatus() int {
	return http.StatusOK
}

func (e *ErrFirmwareUpgradeException) ErrorCode() model.ErrorCode {
	return FirmwareUpgradeException
}

func (e *ErrFirmwareUpgradeException) MobileErrorMessage() string {
	return FirmwareUpgradeExceptionErrorMessage
}

type ErrCustomControlNotFound struct{}

func (e *ErrCustomControlNotFound) Error() string {
	return CustomControlNotFound
}

func (e *ErrCustomControlNotFound) HTTPStatus() int {
	return http.StatusNotFound
}

func (e *ErrCustomControlNotFound) ErrorCode() model.ErrorCode {
	return CustomControlNotFound
}

func (e *ErrCustomControlNotFound) MobileErrorMessage() string {
	return CustomControlNotFound
}

type ErrCustomButtonNotFound struct{}

func (e *ErrCustomButtonNotFound) Error() string {
	return CustomButtonNotFound
}

func (e *ErrCustomButtonNotFound) HTTPStatus() int {
	return http.StatusNotFound
}

func (e *ErrCustomButtonNotFound) ErrorCode() model.ErrorCode {
	return CustomButtonNotFound
}

func (e *ErrCustomButtonNotFound) MobileErrorMessage() string {
	return CustomButtonNotFound
}

type ErrCustomButtonNameIsTaken struct{}

func (e *ErrCustomButtonNameIsTaken) Error() string {
	return string(model.NameIsAlreadyTaken)
}

func (e *ErrCustomButtonNameIsTaken) HTTPStatus() int {
	return http.StatusOK
}

func (e *ErrCustomButtonNameIsTaken) ErrorCode() model.ErrorCode {
	return model.NameIsAlreadyTaken
}

func (e *ErrCustomButtonNameIsTaken) MobileErrorMessage() string {
	return model.NameIsAlreadyTakenErrorMessage
}

type ErrLastCustomButtonDeletion struct{}

func (e *ErrLastCustomButtonDeletion) Error() string {
	return LastCustomButtonDeletion
}

func (e *ErrLastCustomButtonDeletion) HTTPStatus() int {
	return http.StatusOK
}

func (e *ErrLastCustomButtonDeletion) ErrorCode() model.ErrorCode {
	return LastCustomButtonDeletion
}

func (e *ErrLastCustomButtonDeletion) MobileErrorMessage() string {
	return LastCustomButtonDeletionErrorMessage
}

type ErrDeviceNotFound struct{}

func (e *ErrDeviceNotFound) Error() string {
	return string(model.DeviceNotFound)
}

func (e *ErrDeviceNotFound) HTTPStatus() int {
	return http.StatusOK
}

func (e *ErrDeviceNotFound) ErrorCode() model.ErrorCode {
	return model.DeviceNotFound
}
