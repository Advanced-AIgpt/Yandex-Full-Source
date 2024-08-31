package model

import (
	"fmt"
	"net/http"

	"a.yandex-team.ru/library/go/core/xerrors"
)

type ErrorCode string

const (
	DiscoveryError                                        ErrorCode = "DISCOVERY_ERROR" // special error between bsaur and yandex app
	DeviceUnreachable                                     ErrorCode = "DEVICE_UNREACHABLE"
	DeviceBusy                                            ErrorCode = "DEVICE_BUSY"
	DeviceNotFound                                        ErrorCode = "DEVICE_NOT_FOUND"
	DeviceUnlinked                                        ErrorCode = "DEVICE_UNLINKED"
	RoomNotFound                                          ErrorCode = "ROOM_NOT_FOUND"
	GroupNotFound                                         ErrorCode = "GROUP_NOT_FOUND"
	IncompatibleDeviceType                                ErrorCode = "INCOMPATIBLE_DEVICE_TYPE"
	SkillNotFound                                         ErrorCode = "SKILL_NOT_FOUND"
	ScenarioNotFound                                      ErrorCode = "SCENARIO_NOT_FOUND"
	ScenarioNotActive                                     ErrorCode = "SCENARIO_NOT_ACTIVE"
	ScenarioLaunchNotFound                                ErrorCode = "SCENARIO_LAUNCH_NOT_FOUND"
	UserNetworkNotFound                                   ErrorCode = "USER_NETWORK_NOT_FOUND"
	HouseholdNotFound                                     ErrorCode = "HOUSEHOLD_NOT_FOUND"
	InternalError                                         ErrorCode = "INTERNAL_ERROR"
	InvalidAction                                         ErrorCode = "INVALID_ACTION"
	InvalidValue                                          ErrorCode = "INVALID_VALUE"
	NotSupportedInCurrentMode                             ErrorCode = "NOT_SUPPORTED_IN_CURRENT_MODE"
	GroupListIsNotEmpty                                   ErrorCode = "GROUP_LIST_IS_NOT_EMPTY"
	UnacceptableTypeSwitching                             ErrorCode = "UNACCEPTABLE_TYPE_SWITCHING"
	TokenNotFound                                         ErrorCode = "TOKEN_NOT_FOUND"
	UnknownError                                          ErrorCode = "UNKNOWN_ERROR"
	DeviceLimitReached                                    ErrorCode = "DEVICE_LIMIT_REACHED"
	UserNetworksLimitReached                              ErrorCode = "USER_NETWORKS_LIMIT_REACHED"
	EventNotFound                                         ErrorCode = "EVENT_NOT_FOUND"
	ActivationPhraseValidationError                       ErrorCode = "ACTIVATION_PHRASE_VALIDATION_ERROR"
	NameIsAlreadyTaken                                    ErrorCode = "NAME_IS_ALREADY_TAKEN"
	VoiceTriggerPhraseAlreadyTaken                        ErrorCode = "VOICE_TRIGGER_PHRASE_IS_ALREADY_TAKEN"
	LocalScenarioSyncFailed                               ErrorCode = "LOCAL_SCENARIO_SYNC_FAILED"
	TimerTriggerScenarioCreationForbidden                 ErrorCode = "TIMER_TRIGGER_SCENARIO_CREATION_FORBIDDEN"
	TriggersLimitReached                                  ErrorCode = "TRIGGERS_LIMIT_REACHED"
	TimetableTriggersLimitReached                         ErrorCode = "TIMETABLE_TRIGGERS_LIMIT_REACHED"
	TimetableHouseholdNotFound                            ErrorCode = "TIMETABLE_HOUSEHOLD_NOT_FOUND"
	TimetableHouseholdHasNoAddress                        ErrorCode = "TIMETABLE_HOUSEHOLD_HAS_NO_ADDRESS"
	DeviceNameAliasesLimitReached                         ErrorCode = "DEVICE_NAME_ALIASES_LIMIT_REACHED"
	DeviceNameAliasesNameAlreadyExists                    ErrorCode = "DEVICE_NAME_ALIASES_NAME_ALREADY_EXISTS"
	GroupNameAliasesLimitReached                          ErrorCode = "GROUP_NAME_ALIASES_LIMIT_REACHED"
	DeviceTypeAliasesUnsupported                          ErrorCode = "DEVICE_TYPE_ALIASES_UNSUPPORTED"
	QuasarServerActionValueScenarioNameCollisionErrorCode ErrorCode = "SCENARIO_QUASAR_SERVER_ACTION_VALUE_SCENARIO_NAME_COLLISION_ERROR"
	QuasarServerActionValueEmptyErrorCode                 ErrorCode = "QUASAR_SERVER_ACTION_EMPTY_ERROR"
	QuasarServerActionValueLengthErrorCode                ErrorCode = "QUASAR_SERVER_ACTION_LENGTH_ERROR"
	QuasarServerActionValueCharErrorCode                  ErrorCode = "QUASAR_SERVER_ACTION_CHAR_ERROR"
	QuasarServerActionValueMinLettersErrorCode            ErrorCode = "QUASAR_SERVER_ACTION_MIN_LETTERS_ERROR"
	TimetableTimeValidationError                          ErrorCode = "TIMETABLE_TIME_VALIDATION_ERROR"
	TimetableSolarCalculationError                        ErrorCode = "TIMETABLE_SOLAR_CALCULATION_ERROR"
	EmptyTriggersFieldValidationError                     ErrorCode = "EMPTY_TRIGGERS_FIELD_VALIDATION_ERROR"
	TimetableWeekdayValidationError                       ErrorCode = "TIMETABLE_WEEKDAY_VALIDATION_ERROR"
	DevicePropertyTypeValidationError                     ErrorCode = "DEVICE_PROPERTY_TYPE_VALIDATION_ERROR"
	DevicePropertyInstanceValidationError                 ErrorCode = "DEVICE_PROPERTY_INSTANCE_VALIDATION_ERROR"
	DeviceCapabilityValidationError                       ErrorCode = "DEVICE_CAPABILITY_VALIDATION_ERROR"
	InvalidPropertyConditionValidationError               ErrorCode = "INVALID_PROPERTY_CONDITION_VALIDATION_ERROR"
	InvalidPropertyTriggerValidationError                 ErrorCode = "INVALID_PROPERTY_TRIGGER_ERROR"
	HouseholdContainsDevicesDeletionError                 ErrorCode = "HOUSEHOLD_CONTAINS_DEVICES_DELETION_ERROR"
	LastHouseholdDeletionError                            ErrorCode = "LAST_HOUSEHOLD_DELETION"
	ActiveHouseholdDeletionError                          ErrorCode = "ACTIVE_HOUSEHOLD_DELETION"
	NoAddedDevices                                        ErrorCode = "NO_ADDED_DEVICES"
	StartTimeOffsetValidationError                        ErrorCode = "START_TIME_OFFSET_VALIDATION_ERROR"
	EndTimeOffsetValidationError                          ErrorCode = "END_TIME_OFFSET_VALIDATION_ERROR"
	EmptyTimeIntervalValidationError                      ErrorCode = "EMPTY_TIME_INTERVAL_VALIDATION_ERROR"
	WeekdaysNotSpecifiedValidationError                   ErrorCode = "WEEKDAYS_NOT_SPECIFIED_VALIDATION_ERROR"
	WeekdayValidationError                                ErrorCode = "WEEKDAYS_VALIDATION_ERROR"
	OverdueScenarioLaunchErrorCode                        ErrorCode = "OVERDUE_SCENARIO_LAUNCH_ERROR"
	IntentStateNotFoundErrorCode                          ErrorCode = "INTENT_STATE_NOT_FOUND"

	HouseholdEmptyAddressErrorCode            ErrorCode = "HOUSEHOLD_EMPTY_ADDRESS_ERROR"
	HouseholdEmptyCoordinatesErrorCode        ErrorCode = "HOUSEHOLD_EMPTY_COORDINATES_ERROR"
	HouseholdInvalidAddressErrorCode          ErrorCode = "HOUSEHOLD_INVALID_ADDRESS_ERROR"
	TriggerTypeInvalidErrorCode               ErrorCode = "TRIGGER_TYPE_INVALID_ERROR"
	UnknownAliceResponseReactionTypeErrorCode ErrorCode = "UNKNOWN_ALICE_RESPONSE_REACTION_TYPE_ERROR"

	RussianAndLatinNameValidationError ErrorCode = "RUSSIAN_AND_LATIN_NAME_VALIDATION_ERROR"
	RussianNameValidationError         ErrorCode = "RUSSIAN_NAME_VALIDATION_ERROR"
	LengthNameValidationError          ErrorCode = "LENGTH_NAME_VALIDATION_ERROR"
	MinLettersNameValidationError      ErrorCode = "MIN_LETTERS_NAME_VALIDATION_ERROR"
	EmptyNameValidationError           ErrorCode = "EMPTY_NAME_VALIDATION_ERROR"

	ScenarioStepsRepeatedDeviceErrorCode    ErrorCode = "SCENARIO_STEPS_REPEATED_DEVICE"
	ScenarioStepsAtLeastOneActionErrorCode  ErrorCode = "SCENARIO_STEPS_AT_LEAST_ONE_ACTION"
	ScenarioStepsDelayLimitReachedErrorCode ErrorCode = "SCENARIO_STEPS_DELAY_LIMIT_REACHED"
	ScenarioStepsConsecutiveDelaysErrorCode ErrorCode = "SCENARIO_STEPS_CONSECUTIVE_DELAY"
	ScenarioStepsDelayLastStepErrorCode     ErrorCode = "SCENARIO_STEPS_DELAY_LAST_STEP"

	SharedDeviceUsedInScenarioErrorCode          ErrorCode = "SHARED_DEVICE_USED_IN_SCENARIO"
	SharedHouseholdOwnerLeavingErrorCode         ErrorCode = "SHARED_HOUSEHOLD_OWNER_LEAVING_ERROR"
	SharingLinkDoesNotExistErrorCode             ErrorCode = "SHARING_LINK_DOES_NOT_EXIST"
	SharingLinkNeedlessAcceptanceErrorCode       ErrorCode = "SHARING_LINK_NEEDLESS_ACCEPTANCE_ERROR"
	SharingInvitationDoesNotExistErrorCode       ErrorCode = "SHARING_INVITATION_DOES_NOT_EXIST"
	SharingInvitationDoesNotOwnedByUserErrorCode ErrorCode = "SHARING_INVITATION_DOES_NOT_OWNED_BY_USER"
	SharingUsersLimitReachedErrorCode            ErrorCode = "SHARING_USERS_LIMIT_REACHED"
	SharingDevicesLimitReachedErrorCode          ErrorCode = "SHARING_DEVICES_LIMIT_REACHED"

	VoiceprintAddingTimeoutErrorCode ErrorCode = "VOICEPRINT_ADDING_TIMEOUT_ERROR_CODE"
	VoiceprintScenarioErrorCode      ErrorCode = "VOICEPRINT_SCENARIO_ERROR"

	DoorOpen               ErrorCode = "DOOR_OPEN"
	LidOpen                ErrorCode = "LID_OPEN"
	RemoteControlDisabled  ErrorCode = "REMOTE_CONTROL_DISABLED"
	NotEnoughWater         ErrorCode = "NOT_ENOUGH_WATER"
	LowChargeLevel         ErrorCode = "LOW_CHARGE_LEVEL"
	ContainerFull          ErrorCode = "CONTAINER_FULL"
	ContainerEmpty         ErrorCode = "CONTAINER_EMPTY"
	DripTrayFull           ErrorCode = "DRIP_TRAY_FULL"
	DeviceStuck            ErrorCode = "DEVICE_STUCK"
	DeviceOff              ErrorCode = "DEVICE_OFF"
	DeviceOffline          ErrorCode = "DEVICE_OFFLINE"
	FirmwareOutOfDate      ErrorCode = "FIRMWARE_OUT_OF_DATE"
	NotEnoughDetergent     ErrorCode = "NOT_ENOUGH_DETERGENT"
	AccountLinkingError    ErrorCode = "ACCOUNT_LINKING_ERROR"
	HumanInvolvementNeeded ErrorCode = "HUMAN_INVOLVEMENT_NEEDED"

	InvalidConfigVersion                               ErrorCode = "INVALID_CONFIG_VERSION"
	DisassembleStereopairBeforeDeviceDeletionErrorCode ErrorCode = "NEED_DISMOUNT_STEREOAIR_BEFORE_DELETE_DEVICE"
	SharingSpeakerError                                ErrorCode = "SHARING_SPEAKER_ERROR"
)

func EC(ec ErrorCode) *ErrorCode { return &ec }

type withHTTPStatusOK struct{}

func (e withHTTPStatusOK) HTTPStatus() int { return http.StatusOK }

type withHTTPStatusNotFound struct{}

func (e withHTTPStatusNotFound) HTTPStatus() int { return http.StatusNotFound }

type withHTTPStatusBadRequest struct{}

func (e withHTTPStatusBadRequest) HTTPStatus() int { return http.StatusBadRequest }

type withValidationError struct{}

func (e withValidationError) Unwrap() error {
	return ValidationError
}

type DiscoveryErrorError struct{ withHTTPStatusOK }

func (d *DiscoveryErrorError) ErrorCode() ErrorCode {
	return DiscoveryError
}

func (d *DiscoveryErrorError) MobileErrorMessage() string {
	return DiscoveryErrorMessage
}

func (d *DiscoveryErrorError) Error() string {
	return string(DiscoveryError)
}

type DiscoveryAuthorizationError struct{ withHTTPStatusOK }

func (d *DiscoveryAuthorizationError) ErrorCode() ErrorCode {
	return DiscoveryError
}

func (d *DiscoveryAuthorizationError) MobileErrorMessage() string {
	return DiscoveryAuthorizationErrorMessage
}

func (d *DiscoveryAuthorizationError) Error() string {
	return string(DiscoveryError)
}

type GroupListIsNotEmptyError struct{ withHTTPStatusOK }

func (g *GroupListIsNotEmptyError) ErrorCode() ErrorCode {
	return GroupListIsNotEmpty
}

func (g *GroupListIsNotEmptyError) MobileErrorMessage() string {
	return DeviceInGroupTypeSwitchError
}

func (g *GroupListIsNotEmptyError) Error() string {
	return string(GroupListIsNotEmpty)
}

type DeviceTypeSwitchError struct{ withHTTPStatusOK }

func (d *DeviceTypeSwitchError) ErrorCode() ErrorCode {
	return UnacceptableTypeSwitching
}

func (d *DeviceTypeSwitchError) MobileErrorMessage() string {
	return DeviceTypeUnacceptableSwitchMessage
}

func (d *DeviceTypeSwitchError) Error() string {
	return string(UnacceptableTypeSwitching)
}

type DeviceUnreachableError struct{ withHTTPStatusOK }

func (d *DeviceUnreachableError) ErrorCode() ErrorCode {
	return DeviceUnreachable
}

func (d *DeviceUnreachableError) Error() string {
	return string(DeviceUnreachable)
}

func (d *DeviceUnreachableError) MobileErrorMessage() string {
	return DeviceUnreachableErrorMessage
}

type InvalidValueError struct{}

func (i *InvalidValueError) Error() string {
	return string(InvalidValue)
}

type DeviceNotFoundError struct{ withHTTPStatusNotFound }

func (d *DeviceNotFoundError) ErrorCode() ErrorCode {
	return DeviceNotFound
}

func (d *DeviceNotFoundError) Error() string {
	return string(DeviceNotFound)
}

func (d *DeviceNotFoundError) MobileErrorMessage() string {
	return DeviceNotFoundErrorMessage
}

type DeviceUnlinkedError struct {
	withHTTPStatusOK
}

func (e *DeviceUnlinkedError) ErrorCode() ErrorCode {
	return DeviceUnlinked
}

func (e *DeviceUnlinkedError) Error() string {
	return string(DeviceUnlinked)
}

func (e *DeviceUnlinkedError) MobileErrorMessage() string {
	return DeviceUnlinkedErrorMessage
}

type QuasarNameCharError struct {
	withHTTPStatusOK
	withValidationError
}

func (q *QuasarNameCharError) ErrorCode() ErrorCode {
	return RussianAndLatinNameValidationError
}

func (q *QuasarNameCharError) MobileErrorMessage() string {
	return RenameToRussianAndLatinErrorMessage
}

func (q *QuasarNameCharError) Error() string {
	return string(RussianAndLatinNameValidationError)
}

type DeviceAliasesLimitReachedError struct {
	withHTTPStatusOK
	withValidationError
}

func (q *DeviceAliasesLimitReachedError) ErrorCode() ErrorCode {
	return DeviceNameAliasesLimitReached
}

func (q *DeviceAliasesLimitReachedError) MobileErrorMessage() string {
	return DeviceNameAliasesLimitReachedErrorMessage
}

func (q *DeviceAliasesLimitReachedError) Error() string {
	return string(DeviceNameAliasesLimitReached)
}

type DeviceAliasesNameAlreadyExistsError struct {
	withHTTPStatusOK
	withValidationError
}

func (q *DeviceAliasesNameAlreadyExistsError) ErrorCode() ErrorCode {
	return DeviceNameAliasesNameAlreadyExists
}

func (q *DeviceAliasesNameAlreadyExistsError) MobileErrorMessage() string {
	return DeviceNameAliasesAlreadyExistsErrorMessage
}

func (q *DeviceAliasesNameAlreadyExistsError) Error() string {
	return string(DeviceNameAliasesNameAlreadyExists)
}

type GroupAliasesLimitReachedError struct {
	withHTTPStatusOK
	withValidationError
}

func (e *GroupAliasesLimitReachedError) ErrorCode() ErrorCode {
	return GroupNameAliasesLimitReached
}

func (e *GroupAliasesLimitReachedError) MobileErrorMessage() string {
	return GroupNameAliasesLimitReachedErrorMessage
}

func (e *GroupAliasesLimitReachedError) Error() string {
	return string(GroupNameAliasesLimitReached)
}

type DeviceTypeAliasesUnsupportedError struct {
	withHTTPStatusOK
	withValidationError
}

func (q *DeviceTypeAliasesUnsupportedError) ErrorCode() ErrorCode {
	return DeviceTypeAliasesUnsupported
}

func (q *DeviceTypeAliasesUnsupportedError) MobileErrorMessage() string {
	return DeviceTypeAliasesUnsupportedErrorMessage
}

func (q *DeviceTypeAliasesUnsupportedError) Error() string {
	return string(DeviceTypeAliasesUnsupported)
}

type NameCharError struct {
	withHTTPStatusOK
	withValidationError
}

func (n *NameCharError) ErrorCode() ErrorCode {
	return RussianNameValidationError
}

func (n *NameCharError) MobileErrorMessage() string {
	return RenameToRussianErrorMessage
}

func (n *NameCharError) Error() string {
	return string(RussianNameValidationError)
}

type TimetableTimeError struct {
	withHTTPStatusBadRequest
	withValidationError
}

func (t *TimetableTimeError) ErrorCode() ErrorCode {
	return TimetableTimeValidationError
}

func (t *TimetableTimeError) Error() string {
	return string(TimetableTimeValidationError)
}

type TimetableSolarError struct {
	withHTTPStatusOK
}

func (t *TimetableSolarError) ErrorCode() ErrorCode {
	return TimetableSolarCalculationError
}

func (t *TimetableSolarError) Error() string {
	return string(TimetableSolarCalculationError)
}

type TimetableWeekdayError struct {
	withHTTPStatusOK
	withValidationError
}

func (n *TimetableWeekdayError) ErrorCode() ErrorCode {
	return TimetableWeekdayValidationError
}

func (n *TimetableWeekdayError) Error() string {
	return string(TimetableWeekdayValidationError)
}

type TimetableUnknownHouseholdError struct {
	withHTTPStatusOK
	withValidationError
}

func (n *TimetableUnknownHouseholdError) ErrorCode() ErrorCode {
	return TimetableHouseholdNotFound
}

func (n *TimetableUnknownHouseholdError) Error() string {
	return string(TimetableHouseholdNotFound)
}

type TimetableHouseholdNoAddressError struct {
	withHTTPStatusOK
	withValidationError
}

func (n *TimetableHouseholdNoAddressError) ErrorCode() ErrorCode {
	return TimetableHouseholdHasNoAddress
}

func (n *TimetableHouseholdNoAddressError) Error() string {
	return string(TimetableHouseholdHasNoAddress)
}

type UnknownPropertyTypeError struct {
	withHTTPStatusBadRequest
	withValidationError
}

func (n *UnknownPropertyTypeError) ErrorCode() ErrorCode {
	return DevicePropertyTypeValidationError
}

func (n *UnknownPropertyTypeError) Error() string {
	return string(DevicePropertyTypeValidationError)
}

type UnknownPropertyInstanceError struct {
	withHTTPStatusBadRequest
	withValidationError
}

func (n *UnknownPropertyInstanceError) ErrorCode() ErrorCode {
	return DevicePropertyInstanceValidationError
}

func (n *UnknownPropertyInstanceError) Error() string {
	return string(DevicePropertyInstanceValidationError)
}

type InvalidPropertyConditionError struct {
	withHTTPStatusOK
	withValidationError
}

func (n *InvalidPropertyConditionError) ErrorCode() ErrorCode {
	return InvalidPropertyConditionValidationError
}

func (n *InvalidPropertyConditionError) Error() string {
	return string(InvalidPropertyConditionValidationError)
}

func (*InvalidPropertyConditionError) MobileErrorMessage() string {
	return InvalidPropertyConditionErrorMessage
}

type NameLengthError struct {
	withHTTPStatusOK
	withValidationError
	Limit int
}

func (n *NameLengthError) ErrorCode() ErrorCode {
	return LengthNameValidationError
}

func (n *NameLengthError) MobileErrorMessage() string {
	return fmt.Sprintf(NameLengthErrorMessage, n.Limit)
}

func (n *NameLengthError) Error() string {
	return string(LengthNameValidationError)
}

type NameMinLettersError struct {
	withHTTPStatusOK
	withValidationError
}

func (n *NameMinLettersError) ErrorCode() ErrorCode {
	return MinLettersNameValidationError
}

func (n *NameMinLettersError) MobileErrorMessage() string {
	return NameMinLettersErrorMessage
}

func (n *NameMinLettersError) Error() string {
	return string(MinLettersNameValidationError)
}

type NameEmptyError struct {
	withHTTPStatusOK
	withValidationError
}

func (n *NameEmptyError) ErrorCode() ErrorCode {
	return EmptyNameValidationError
}

func (n *NameEmptyError) MobileErrorMessage() string {
	return NameEmptyErrorMessage
}

func (n *NameEmptyError) Error() string {
	return string(EmptyNameValidationError)
}

type InvalidConfigVersionError struct{ withHTTPStatusOK }

func (*InvalidConfigVersionError) Error() string {
	return string(InvalidConfigVersion)
}
func (*InvalidConfigVersionError) ErrorCode() ErrorCode {
	return InvalidConfigVersion
}

type DeviceWithoutRoomError struct{}

func (d *DeviceWithoutRoomError) Error() string {
	return "DEVICE_WITHOUT_ROOM_ERROR"
}

type RoomNotFoundError struct{ withHTTPStatusNotFound }

func (r *RoomNotFoundError) ErrorCode() ErrorCode {
	return RoomNotFound
}

func (r *RoomNotFoundError) Error() string {
	return string(RoomNotFound)
}

func (r *RoomNotFoundError) MobileErrorMessage() string {
	return RoomNotFoundErrorMessage
}

type GroupNotFoundError struct{ withHTTPStatusNotFound }

func (g *GroupNotFoundError) ErrorCode() ErrorCode {
	return GroupNotFound
}

func (g *GroupNotFoundError) Error() string {
	return string(GroupNotFound)
}

func (g *GroupNotFoundError) MobileErrorMessage() string {
	return GroupNotFoundErrorMessage
}

type IncompatibleDeviceTypeError struct{ withHTTPStatusBadRequest }

func (g *IncompatibleDeviceTypeError) ErrorCode() ErrorCode {
	return IncompatibleDeviceType
}

func (g *IncompatibleDeviceTypeError) Error() string {
	return string(IncompatibleDeviceType)
}

type NameIsAlreadyTakenError struct {
	withHTTPStatusOK
	withValidationError
}

func (n *NameIsAlreadyTakenError) ErrorCode() ErrorCode {
	return NameIsAlreadyTaken
}

func (n *NameIsAlreadyTakenError) MobileErrorMessage() string {
	return NameIsAlreadyTakenErrorMessage
}

func (n *NameIsAlreadyTakenError) Error() string {
	return string(NameIsAlreadyTaken)
}

type SkillNotFoundError struct{ withHTTPStatusNotFound }

func (s *SkillNotFoundError) ErrorCode() ErrorCode {
	return SkillNotFound
}

func (s *SkillNotFoundError) Error() string {
	return string(SkillNotFound)
}

type UnknownUserError struct{}

func (u *UnknownUserError) Error() string {
	return "UNKNOWN_USER"
}

type ScenarioNotFoundError struct{ withHTTPStatusNotFound }

func (s *ScenarioNotFoundError) ErrorCode() ErrorCode {
	return ScenarioNotFound
}

func (s *ScenarioNotFoundError) Error() string {
	return string(ScenarioNotFound)
}

type ScenarioNotActiveError struct{ withHTTPStatusOK }

func (s *ScenarioNotActiveError) ErrorCode() ErrorCode {
	return ScenarioNotActive
}

func (s *ScenarioNotActiveError) Error() string {
	return string(ScenarioNotActive)
}

type InvalidPropertyTriggerError struct {
	withHTTPStatusBadRequest
	withValidationError
}

func (n *InvalidPropertyTriggerError) ErrorCode() ErrorCode {
	return InvalidPropertyTriggerValidationError
}

func (n *InvalidPropertyTriggerError) Error() string {
	return string(InvalidPropertyTriggerValidationError)
}

type ScenarioLaunchNotFoundError struct{ withHTTPStatusNotFound }

func (s *ScenarioLaunchNotFoundError) ErrorCode() ErrorCode {
	return ScenarioLaunchNotFound
}

func (s *ScenarioLaunchNotFoundError) Error() string {
	return string(ScenarioLaunchNotFound)
}

type ScenarioActionValueError struct {
	withHTTPStatusOK
	withValidationError
}

func (n *ScenarioActionValueError) ErrorCode() ErrorCode {
	return InvalidAction
}

func (n *ScenarioActionValueError) MobileErrorMessage() string {
	return ScenarioActionValueErrorMessage
}

func (n *ScenarioActionValueError) Error() string {
	return string(InvalidAction)
}

type UserNetworkNotFoundError struct{ withHTTPStatusOK }

func (s *UserNetworkNotFoundError) ErrorCode() ErrorCode {
	return UserNetworkNotFound
}
func (s *UserNetworkNotFoundError) Error() string {
	return string(UserNetworkNotFound)
}

type TokenNotFoundError struct{}

func (t *TokenNotFoundError) Error() string {
	return string(TokenNotFound)
}

type DeviceLimitReachedError struct{}

func (d *DeviceLimitReachedError) Error() string {
	return string(DeviceLimitReached)
}

type UserNetworksLimitReachedError struct{}

func (un *UserNetworksLimitReachedError) Error() string {
	return string(UserNetworksLimitReached)
}

type EventNotFoundError struct{ withHTTPStatusOK }

func (e *EventNotFoundError) ErrorCode() ErrorCode {
	return EventNotFound
}

func (e *EventNotFoundError) Error() string {
	return string(EventNotFound)
}

type EventDeviceNotFoundError struct{ withHTTPStatusOK }

func (d *EventDeviceNotFoundError) ErrorCode() ErrorCode {
	return DeviceNotFound
}

func (d *EventDeviceNotFoundError) Error() string {
	return string(DeviceNotFound)
}

type ScenarioTextServerActionNameError struct {
	withHTTPStatusOK
	withValidationError
}

func (e *ScenarioTextServerActionNameError) ErrorCode() ErrorCode {
	return QuasarServerActionValueScenarioNameCollisionErrorCode
}

func (e *ScenarioTextServerActionNameError) MobileErrorMessage() string {
	return ScenarioTextServerActionNameErrorMessage
}

func (e *ScenarioTextServerActionNameError) Error() string {
	return string(QuasarServerActionValueScenarioNameCollisionErrorCode)
}

type ScenarioTextServerActionInternalValidationError struct {
	withHTTPStatusOK
	withValidationError
}

func (e *ScenarioTextServerActionInternalValidationError) ErrorCode() ErrorCode {
	return InternalError
}

func (e *ScenarioTextServerActionInternalValidationError) MobileErrorMessage() string {
	return UnknownErrorMessage
}

func (e *ScenarioTextServerActionInternalValidationError) Error() string {
	return string(InternalError)
}

type DoorOpenError struct{ withHTTPStatusOK }

func (d *DoorOpenError) ErrorCode() ErrorCode {
	return DoorOpen
}

func (d *DoorOpenError) MobileErrorMessage() string {
	return DoorOpenErrorMessage
}

func (d *DoorOpenError) Error() string {
	return string(DoorOpen)
}

type LidOpenError struct{ withHTTPStatusOK }

func (l *LidOpenError) ErrorCode() ErrorCode {
	return LidOpen
}

func (l *LidOpenError) MobileErrorMessage() string {
	return LidOpenErrorMessage
}

func (l *LidOpenError) Error() string {
	return string(LidOpen)
}

type RemoteControlDisabledError struct{ withHTTPStatusOK }

func (r *RemoteControlDisabledError) ErrorCode() ErrorCode {
	return RemoteControlDisabled
}

func (r *RemoteControlDisabledError) MobileErrorMessage() string {
	return RemoteControlDisabledErrorMessage
}

func (r *RemoteControlDisabledError) Error() string {
	return string(RemoteControlDisabled)
}

type NotEnoughWaterError struct{ withHTTPStatusOK }

func (n *NotEnoughWaterError) ErrorCode() ErrorCode {
	return NotEnoughWater
}

func (n *NotEnoughWaterError) MobileErrorMessage() string {
	return NotEnoughWaterErrorMessage
}

func (n *NotEnoughWaterError) Error() string {
	return string(NotEnoughWater)
}

type LowChargeLevelError struct{ withHTTPStatusOK }

func (l *LowChargeLevelError) ErrorCode() ErrorCode {
	return LowChargeLevel
}

func (l *LowChargeLevelError) MobileErrorMessage() string {
	return LowChargeLevelErrorMessage
}

func (l *LowChargeLevelError) Error() string {
	return string(LowChargeLevel)
}

type ContainerFullError struct{ withHTTPStatusOK }

func (c *ContainerFullError) ErrorCode() ErrorCode {
	return ContainerFull
}

func (c *ContainerFullError) MobileErrorMessage() string {
	return ContainerFullErrorMessage
}

func (c *ContainerFullError) Error() string {
	return string(ContainerFull)
}

type ContainerEmptyError struct{ withHTTPStatusOK }

func (c *ContainerEmptyError) ErrorCode() ErrorCode {
	return ContainerEmpty
}

func (c *ContainerEmptyError) MobileErrorMessage() string {
	return ContainerEmptyErrorMessage
}

func (c *ContainerEmptyError) Error() string {
	return string(ContainerEmpty)
}

type DripTrayFullError struct{ withHTTPStatusOK }

func (d *DripTrayFullError) ErrorCode() ErrorCode {
	return DripTrayFull
}

func (d *DripTrayFullError) MobileErrorMessage() string {
	return DripTrayFullErrorMessage
}

func (d *DripTrayFullError) Error() string {
	return string(DripTrayFull)
}

type DeviceStuckError struct{ withHTTPStatusOK }

func (d *DeviceStuckError) ErrorCode() ErrorCode {
	return DeviceStuck
}

func (d *DeviceStuckError) MobileErrorMessage() string {
	return DeviceStuckErrorMessage
}

func (d *DeviceStuckError) Error() string {
	return string(DeviceStuck)
}

type DeviceOffError struct{ withHTTPStatusOK }

func (d *DeviceOffError) ErrorCode() ErrorCode {
	return DeviceOff
}

func (d *DeviceOffError) MobileErrorMessage() string {
	return DeviceOffErrorMessage
}

func (d *DeviceOffError) Error() string {
	return string(DeviceOff)
}

type FirmwareOutOfDateError struct{ withHTTPStatusOK }

func (f *FirmwareOutOfDateError) ErrorCode() ErrorCode {
	return FirmwareOutOfDate
}

func (f *FirmwareOutOfDateError) MobileErrorMessage() string {
	return FirmwareOutOfDateErrorMessage
}

func (f *FirmwareOutOfDateError) Error() string {
	return string(FirmwareOutOfDate)
}

type NotEnoughDetergentError struct{ withHTTPStatusOK }

func (n *NotEnoughDetergentError) ErrorCode() ErrorCode {
	return NotEnoughDetergent
}

func (n *NotEnoughDetergentError) MobileErrorMessage() string {
	return NotEnoughDetergentErrorMessage
}

func (n *NotEnoughDetergentError) Error() string {
	return string(NotEnoughDetergent)
}

type AccountLinkingErrorError struct{ withHTTPStatusOK }

func (a *AccountLinkingErrorError) ErrorCode() ErrorCode {
	return AccountLinkingError
}

func (a *AccountLinkingErrorError) MobileErrorMessage() string {
	return AccountLinkingErrorErrorMessage
}

func (a *AccountLinkingErrorError) Error() string {
	return string(AccountLinkingError)
}

type HumanInvolvementNeededError struct{ withHTTPStatusOK }

func (h *HumanInvolvementNeededError) ErrorCode() ErrorCode {
	return HumanInvolvementNeeded
}

func (h *HumanInvolvementNeededError) MobileErrorMessage() string {
	return HumanInvolvementNeededErrorMessage
}

func (h *HumanInvolvementNeededError) Error() string {
	return string(HumanInvolvementNeeded)
}

type TriggerTypeInvalidError struct {
	withHTTPStatusBadRequest
	withValidationError
}

func (t *TriggerTypeInvalidError) ErrorCode() ErrorCode {
	return TriggerTypeInvalidErrorCode
}

func (t *TriggerTypeInvalidError) Error() string {
	return string(TriggerTypeInvalidErrorCode)
}

type VoiceTriggerPhraseAlreadyTakenError struct {
	withHTTPStatusOK
	withValidationError
}

func (e *VoiceTriggerPhraseAlreadyTakenError) ErrorCode() ErrorCode {
	return VoiceTriggerPhraseAlreadyTaken
}

func (e *VoiceTriggerPhraseAlreadyTakenError) MobileErrorMessage() string {
	return VoiceTriggerPhraseAlreadyTakenErrorMessage
}

func (e *VoiceTriggerPhraseAlreadyTakenError) Error() string {
	return string(VoiceTriggerPhraseAlreadyTaken)
}

type DisassembleStereopairBeforeDeviceDeletionError struct{ withHTTPStatusOK }

func (*DisassembleStereopairBeforeDeviceDeletionError) Error() string {
	return string(DisassembleStereopairBeforeDeviceDeletionErrorCode)
}

func (*DisassembleStereopairBeforeDeviceDeletionError) ErrorCode() ErrorCode {
	return DisassembleStereopairBeforeDeviceDeletionErrorCode
}

func (*DisassembleStereopairBeforeDeviceDeletionError) MobileErrorMessage() string {
	return DisassembleStereopairBeforeDeviceDeletionErrorMessage
}

type NoAddedDevicesError struct{ withHTTPStatusOK }

func (err *NoAddedDevicesError) ErrorCode() ErrorCode {
	return NoAddedDevices
}

func (err *NoAddedDevicesError) Error() string {
	return string(NoAddedDevices)
}

func (err *NoAddedDevicesError) MobileErrorMessage() string {
	return NoAddedDevicesErrorMessage
}

type TimerTriggerScenarioCreationError struct{ withHTTPStatusBadRequest }

func (e *TimerTriggerScenarioCreationError) ErrorCode() ErrorCode {
	return TimerTriggerScenarioCreationForbidden
}

func (e *TimerTriggerScenarioCreationError) Error() string {
	return string(TimerTriggerScenarioCreationForbidden)
}

type LocalScenarioSyncError struct{ withHTTPStatusOK }

func (e *LocalScenarioSyncError) ErrorCode() ErrorCode {
	return LocalScenarioSyncFailed
}

func (e *LocalScenarioSyncError) Error() string {
	return string(LocalScenarioSyncFailed)
}

func (e *LocalScenarioSyncError) MobileErrorMessage() string {
	return LocalScenarioSyncFailedErrorMessage
}

type TooManyVoiceTriggersError struct {
	withHTTPStatusOK
	withValidationError
}

func (e *TooManyVoiceTriggersError) ErrorCode() ErrorCode {
	return TriggersLimitReached
}

func (e *TooManyVoiceTriggersError) MobileErrorMessage() string {
	return TriggersLimitReachedErrorMessage
}

func (e *TooManyVoiceTriggersError) Error() string {
	return string(TriggersLimitReached)
}

type EmptyTriggersFieldError struct {
	withHTTPStatusBadRequest
	withValidationError
}

func (n *EmptyTriggersFieldError) ErrorCode() ErrorCode {
	return EmptyTriggersFieldValidationError
}

func (n *EmptyTriggersFieldError) Error() string {
	return string(EmptyTriggersFieldValidationError)
}

type EmptyTimeIntervalError struct {
	withHTTPStatusBadRequest
	withValidationError
}

func (n *EmptyTimeIntervalError) ErrorCode() ErrorCode {
	return EmptyTimeIntervalValidationError
}

func (n *EmptyTimeIntervalError) Error() string {
	return string(EmptyTimeIntervalValidationError)
}

type WeekdaysNotSpecifiedError struct {
	withHTTPStatusBadRequest
	withValidationError
}

func (n *WeekdaysNotSpecifiedError) ErrorCode() ErrorCode {
	return WeekdaysNotSpecifiedValidationError
}

func (n *WeekdaysNotSpecifiedError) Error() string {
	return string(WeekdaysNotSpecifiedValidationError)
}

type InvalidWeekdayError struct {
	withHTTPStatusBadRequest
	withValidationError
}

func (n *InvalidWeekdayError) ErrorCode() ErrorCode {
	return WeekdayValidationError
}

func (n *InvalidWeekdayError) Error() string {
	return string(WeekdayValidationError)
}

type InvalidEndTimeOffsetError struct {
	withHTTPStatusBadRequest
	withValidationError
}

func (n *InvalidEndTimeOffsetError) ErrorCode() ErrorCode {
	return EndTimeOffsetValidationError
}

func (n *InvalidEndTimeOffsetError) Error() string {
	return string(EndTimeOffsetValidationError)
}

type InvalidStartTimeOffsetError struct {
	withHTTPStatusBadRequest
	withValidationError
}

func (n *InvalidStartTimeOffsetError) ErrorCode() ErrorCode {
	return StartTimeOffsetValidationError
}

func (n *InvalidStartTimeOffsetError) Error() string {
	return string(StartTimeOffsetValidationError)
}

type TooManyTimetableTriggersError struct {
	withHTTPStatusOK
	withValidationError
}

func (e *TooManyTimetableTriggersError) ErrorCode() ErrorCode {
	return TimetableTriggersLimitReached
}

func (e *TooManyTimetableTriggersError) MobileErrorMessage() string {
	return TimetableTriggersLimitReachedErrorMessage
}

func (e *TooManyTimetableTriggersError) Error() string {
	return string(TimetableTriggersLimitReached)
}

type QuasarServerActionValueEmptyError struct {
	withHTTPStatusOK
	withValidationError
}

func (e *QuasarServerActionValueEmptyError) ErrorCode() ErrorCode {
	return QuasarServerActionValueEmptyErrorCode
}

func (e *QuasarServerActionValueEmptyError) MobileErrorMessage() string {
	return QuasarServerActionValueEmptyErrorMessage
}

func (e *QuasarServerActionValueEmptyError) Error() string {
	return string(QuasarServerActionValueEmptyErrorCode)
}

type QuasarServerActionValueLengthError struct {
	withHTTPStatusOK
	withValidationError
	Limit int
}

func (e *QuasarServerActionValueLengthError) ErrorCode() ErrorCode {
	return QuasarServerActionValueLengthErrorCode
}

func (e *QuasarServerActionValueLengthError) MobileErrorMessage() string {
	return fmt.Sprintf(QuasarServerActionValueLengthErrorMessage, e.Limit)
}

func (e *QuasarServerActionValueLengthError) Error() string {
	return string(QuasarServerActionValueLengthErrorCode)
}

type QuasarServerActionValueCharError struct {
	withHTTPStatusOK
	withValidationError
}

func (e *QuasarServerActionValueCharError) ErrorCode() ErrorCode {
	return QuasarServerActionValueCharErrorCode
}

func (e *QuasarServerActionValueCharError) MobileErrorMessage() string {
	return QuasarServerActionValueCharErrorMessage
}

func (e *QuasarServerActionValueCharError) Error() string {
	return string(QuasarServerActionValueCharErrorCode)
}

type QuasarServerActionValueMinLettersError struct {
	withHTTPStatusOK
	withValidationError
}

func (e *QuasarServerActionValueMinLettersError) ErrorCode() ErrorCode {
	return QuasarServerActionValueMinLettersErrorCode
}

func (e *QuasarServerActionValueMinLettersError) MobileErrorMessage() string {
	return QuasarServerActionValueMinLettersErrorMessage
}

func (e *QuasarServerActionValueMinLettersError) Error() string {
	return string(QuasarServerActionValueMinLettersErrorCode)
}

type UnknownAliceResponseReactionTypeError struct{ withHTTPStatusBadRequest }

func (e *UnknownAliceResponseReactionTypeError) Error() string {
	return string(UnknownAliceResponseReactionTypeErrorCode)
}

func (e *UnknownAliceResponseReactionTypeError) ErrorCode() ErrorCode {
	return UnknownAliceResponseReactionTypeErrorCode
}

type UserHouseholdNotFoundError struct{ withHTTPStatusNotFound }

func (e *UserHouseholdNotFoundError) ErrorCode() ErrorCode {
	return HouseholdNotFound
}
func (e *UserHouseholdNotFoundError) Error() string {
	return string(HouseholdNotFound)
}
func (e *UserHouseholdNotFoundError) MobileErrorMessage() string {
	return HouseholdNotFoundErrorMessage
}

type UserHouseholdContainsDevicesError struct{ withHTTPStatusOK }

func (e *UserHouseholdContainsDevicesError) ErrorCode() ErrorCode {
	return HouseholdContainsDevicesDeletionError
}
func (e *UserHouseholdContainsDevicesError) Error() string {
	return string(HouseholdContainsDevicesDeletionError)
}
func (e *UserHouseholdContainsDevicesError) MobileErrorMessage() string {
	return HouseholdContainsDevicesDeletionErrorMessage
}

type UserHouseholdLastDeletionError struct{ withHTTPStatusOK }

func (e *UserHouseholdLastDeletionError) ErrorCode() ErrorCode {
	return LastHouseholdDeletionError
}
func (e *UserHouseholdLastDeletionError) Error() string {
	return string(LastHouseholdDeletionError)
}
func (e *UserHouseholdLastDeletionError) MobileErrorMessage() string {
	return LastHouseholdDeletionErrorMessage
}

type UserHouseholdActiveDeletionError struct{ withHTTPStatusOK }

func (e *UserHouseholdActiveDeletionError) ErrorCode() ErrorCode {
	return ActiveHouseholdDeletionError
}
func (e *UserHouseholdActiveDeletionError) Error() string {
	return string(ActiveHouseholdDeletionError)
}
func (e *UserHouseholdActiveDeletionError) MobileErrorMessage() string {
	return ActiveHouseholdDeletionErrorMessage
}

type UserHouseholdEmptyAddressError struct {
	withHTTPStatusOK
	withValidationError
}

func (e *UserHouseholdEmptyAddressError) Error() string {
	return string(HouseholdEmptyAddressErrorCode)
}

func (e *UserHouseholdEmptyAddressError) ErrorCode() ErrorCode {
	return HouseholdEmptyAddressErrorCode
}

func (e *UserHouseholdEmptyAddressError) MobileErrorMessage() string {
	return HouseholdEmptyAddressErrorMessage
}

type UserHouseholdEmptyCoordinatesError struct {
	withHTTPStatusOK
	withValidationError
}

func (e *UserHouseholdEmptyCoordinatesError) Error() string {
	return string(HouseholdEmptyCoordinatesErrorCode)
}

func (e *UserHouseholdEmptyCoordinatesError) ErrorCode() ErrorCode {
	return HouseholdEmptyCoordinatesErrorCode
}

func (e *UserHouseholdEmptyCoordinatesError) MobileErrorMessage() string {
	return HouseholdEmptyLocationErrorMessage
}

type UserHouseholdInvalidAddressError struct {
	withHTTPStatusOK
	withValidationError
}

func (e *UserHouseholdInvalidAddressError) Error() string {
	return string(HouseholdInvalidAddressErrorCode)
}

func (e *UserHouseholdInvalidAddressError) ErrorCode() ErrorCode {
	return HouseholdInvalidAddressErrorCode
}

func (e *UserHouseholdInvalidAddressError) MobileErrorMessage() string {
	return HouseholdInvalidAddressErrorMessage
}

type UnknownCapabilityError struct {
	withHTTPStatusBadRequest
	withValidationError
}

func (n *UnknownCapabilityError) ErrorCode() ErrorCode {
	return DeviceCapabilityValidationError
}

func (n *UnknownCapabilityError) Error() string {
	return string(DeviceCapabilityValidationError)
}

type ScenarioStepsRepeatedDeviceError struct {
	withHTTPStatusOK
	withValidationError
}

func (e *ScenarioStepsRepeatedDeviceError) Error() string {
	return string(ScenarioStepsRepeatedDeviceErrorCode)
}

func (e *ScenarioStepsRepeatedDeviceError) ErrorCode() ErrorCode {
	return ScenarioStepsRepeatedDeviceErrorCode
}

func (e *ScenarioStepsRepeatedDeviceError) MobileErrorMessage() string {
	return ScenarioStepsRepeatedDeviceErrorMessage
}

type ScenarioStepsAtLeastOneActionError struct {
	withHTTPStatusOK
	withValidationError
}

func (e *ScenarioStepsAtLeastOneActionError) Error() string {
	return string(ScenarioStepsAtLeastOneActionErrorCode)
}

func (e *ScenarioStepsAtLeastOneActionError) ErrorCode() ErrorCode {
	return ScenarioStepsAtLeastOneActionErrorCode
}

func (e *ScenarioStepsAtLeastOneActionError) MobileErrorMessage() string {
	return ScenarioStepsAtLeastOneActionErrorMessage
}

type ScenarioStepsDelayLimitReachedError struct {
	withHTTPStatusOK
	withValidationError
}

func (e *ScenarioStepsDelayLimitReachedError) Error() string {
	return string(ScenarioStepsDelayLimitReachedErrorCode)
}

func (e *ScenarioStepsDelayLimitReachedError) ErrorCode() ErrorCode {
	return ScenarioStepsDelayLimitReachedErrorCode
}

func (e *ScenarioStepsDelayLimitReachedError) MobileErrorMessage() string {
	return ScenarioStepsDelayLimitReachedErrorMessage
}

type ScenarioStepsConsecutiveDelaysError struct {
	withHTTPStatusOK
	withValidationError
}

func (e *ScenarioStepsConsecutiveDelaysError) Error() string {
	return string(ScenarioStepsConsecutiveDelaysErrorCode)
}

func (e *ScenarioStepsConsecutiveDelaysError) ErrorCode() ErrorCode {
	return ScenarioStepsConsecutiveDelaysErrorCode
}

func (e *ScenarioStepsConsecutiveDelaysError) MobileErrorMessage() string {
	return ScenarioStepsConsecutiveDelaysErrorMessage
}

type ScenarioStepsDelayLastStepError struct {
	withHTTPStatusOK
	withValidationError
}

func (e *ScenarioStepsDelayLastStepError) Error() string {
	return string(ScenarioStepsDelayLastStepErrorCode)
}

func (e *ScenarioStepsDelayLastStepError) ErrorCode() ErrorCode {
	return ScenarioStepsDelayLastStepErrorCode
}

func (e *ScenarioStepsDelayLastStepError) MobileErrorMessage() string {
	return ScenarioStepsDelayLastStepErrorMessage
}

type SpeakerDiscoveryInternalError struct{ withHTTPStatusOK }

func (e *SpeakerDiscoveryInternalError) Error() string {
	return string(InternalError)
}

func (e *SpeakerDiscoveryInternalError) ErrorCode() ErrorCode {
	return InternalError
}

type IntentStateNotFoundError struct{ withHTTPStatusNotFound }

func (e *IntentStateNotFoundError) Error() string {
	return string(IntentStateNotFoundErrorCode)
}

func (e *IntentStateNotFoundError) ErrorCode() ErrorCode {
	return IntentStateNotFoundErrorCode
}

type SharedDeviceUsedInScenarioError struct {
	withHTTPStatusOK
	withValidationError
}

func (e *SharedDeviceUsedInScenarioError) Error() string {
	return string(SharedDeviceUsedInScenarioErrorCode)
}

func (e *SharedDeviceUsedInScenarioError) ErrorCode() ErrorCode {
	return SharedDeviceUsedInScenarioErrorCode
}

func (e *SharedDeviceUsedInScenarioError) MobileErrorMessage() string {
	return SharedDeviceUsedInScenarioErrorMessage
}

type SharedHouseholdOwnerLeavingError struct {
	withHTTPStatusOK
	withValidationError
}

func (e *SharedHouseholdOwnerLeavingError) Error() string {
	return string(SharedHouseholdOwnerLeavingErrorCode)
}

func (e *SharedHouseholdOwnerLeavingError) ErrorCode() ErrorCode {
	return SharedHouseholdOwnerLeavingErrorCode
}

func (e *SharedHouseholdOwnerLeavingError) MobileErrorMessage() string {
	return SharedHouseholdOwnerLeavingErrorMessage
}

type SharingLinkDoesNotExistError struct {
	withHTTPStatusOK
}

func (e *SharingLinkDoesNotExistError) Error() string {
	return string(SharingLinkDoesNotExistErrorCode)
}

func (e *SharingLinkDoesNotExistError) ErrorCode() ErrorCode {
	return SharingLinkDoesNotExistErrorCode
}

func (e *SharingLinkDoesNotExistError) MobileErrorMessage() string {
	return SharingLinkDoesNotExistErrorMessage
}

type SharingLinkNeedlessAcceptanceError struct {
	withHTTPStatusOK
}

func (e *SharingLinkNeedlessAcceptanceError) Error() string {
	return string(SharingLinkNeedlessAcceptanceErrorCode)
}

func (e *SharingLinkNeedlessAcceptanceError) ErrorCode() ErrorCode {
	return SharingLinkNeedlessAcceptanceErrorCode
}

func (e *SharingLinkNeedlessAcceptanceError) MobileErrorMessage() string {
	return SharingLinkNeedlessAcceptanceErrorMessage
}

type SharingInvitationDoesNotExistError struct {
	withHTTPStatusOK
}

func (e *SharingInvitationDoesNotExistError) Error() string {
	return string(SharingInvitationDoesNotExistErrorCode)
}

func (e *SharingInvitationDoesNotExistError) ErrorCode() ErrorCode {
	return SharingInvitationDoesNotExistErrorCode
}

func (e *SharingInvitationDoesNotExistError) MobileErrorMessage() string {
	return SharingInvitationDoesNotExistErrorMessage
}

type SharingInvitationDoesNotOwnedByUserError struct {
	withHTTPStatusOK
}

func (e *SharingInvitationDoesNotOwnedByUserError) Error() string {
	return string(SharingInvitationDoesNotOwnedByUserErrorCode)
}

func (e *SharingInvitationDoesNotOwnedByUserError) ErrorCode() ErrorCode {
	return SharingInvitationDoesNotOwnedByUserErrorCode
}

func (e *SharingInvitationDoesNotOwnedByUserError) MobileErrorMessage() string {
	return SharingInvitationDoesNotOwnedByUserErrorMessage
}

type SharingUsersLimitReachedError struct {
	withHTTPStatusOK
}

func (e *SharingUsersLimitReachedError) Error() string {
	return string(SharingUsersLimitReachedErrorCode)
}

func (e *SharingUsersLimitReachedError) ErrorCode() ErrorCode {
	return SharingUsersLimitReachedErrorCode
}

func (e *SharingUsersLimitReachedError) MobileErrorMessage() string {
	return SharingUsersLimitReachedErrorMessage
}

type SharingDevicesLimitReachedError struct {
	withHTTPStatusOK
}

func (e *SharingDevicesLimitReachedError) Error() string {
	return string(SharingDevicesLimitReachedErrorCode)
}

func (e *SharingDevicesLimitReachedError) ErrorCode() ErrorCode {
	return SharingDevicesLimitReachedErrorCode
}

func (e *SharingDevicesLimitReachedError) MobileErrorMessage() string {
	return SharingDevicesLimitReachedErrorMessage
}

func NewSharingSpeakerRawError(err error) *SharingSpeakerRawError {
	return &SharingSpeakerRawError{
		internalError: err,
	}
}

// SharingSpeakerRawError is an error with wrapping raw error message
// used only for sharing testing - must be replaced during sharing integration with UI
type SharingSpeakerRawError struct {
	internalError error
}

func (e *SharingSpeakerRawError) HTTPStatus() int {
	if xerrors.Is(e.internalError, &DeviceNotFoundError{}) {
		return http.StatusNotFound
	} else {
		return http.StatusInternalServerError
	}
}

func (e *SharingSpeakerRawError) Error() string {
	return fmt.Sprintf("%s", e.internalError)
}

func (e *SharingSpeakerRawError) MobileErrorMessage() string {
	return fmt.Sprintf("%v", e.internalError)
}

func (e *SharingSpeakerRawError) ErrorCode() ErrorCode {
	return SharingSpeakerError
}
