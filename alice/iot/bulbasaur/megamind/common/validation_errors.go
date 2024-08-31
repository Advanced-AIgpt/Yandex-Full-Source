package common

import (
	"a.yandex-team.ru/alice/iot/bulbasaur/nlg"
	libnlg "a.yandex-team.ru/alice/library/go/nlg"
)

type ValidationError struct {
	Description string
	nlg         libnlg.NLG
}

func (v ValidationError) Error() string {
	return v.Description
}

func (v ValidationError) Is(err error) bool {
	if validationError, ok := err.(ValidationError); ok {
		return v.Description == validationError.Description
	}
	return false
}

func (v ValidationError) NLG() libnlg.NLG {
	if v.nlg != nil {
		return v.nlg
	}
	return nlg.CommonError
}

var (
	PastActionValidationError = ValidationError{
		Description: "the time is in the past",
		nlg:         nlg.PastAction,
	}
	FarFutureValidationError = ValidationError{
		Description: "the time is too far in the future",
		nlg:         nlg.FutureAction,
	}
	TimeIsNotSpecifiedValidationError = ValidationError{
		Description: "the date is specified, but the time is not",
		nlg:         nlg.NoTimeSpecified,
	}
	WeirdTimeRelativityValidationError = ValidationError{
		Description: "the time is relative while the date is not",
		nlg:         nlg.WeirdDateTimeRelativity,
	}
	MultipleHouseholdsInRequestValidationError = ValidationError{
		Description: "multiple households in the request",
		nlg:         nlg.MultipleHouseholdsInRequest,
	}
	MultipleSuitableHouseholdsValidationError = ValidationError{
		Description: "multiple suitable households found",
		nlg:         nlg.NoHouseholdSpecifiedAction,
	}
	CannotPlayVideoStreamInIotAppValidationError = ValidationError{
		Description: "video streams in the iot app are not supported yet",
		nlg:         nil,
	}
	CannotPlayVideoOnDeviceValidationError = ValidationError{
		Description: "no supported video stream protocols in client info",
		nlg:         nlg.CantPlayVideoOnDevice,
	}
	TVIsNotPluggedValidationError = ValidationError{
		Description: "tv is not plugged in",
		nlg:         nlg.TvIsNotPluggedIn,
	}
	TurnOnEverythingIsForbiddenValidationError = ValidationError{
		Description: "turning on all devices is forbidden",
		nlg:         nlg.TurnOnDevicesIsForbidden,
	}
	UnknownModeValidationError = ValidationError{
		Description: "unknown mode value from user",
		nlg:         nlg.InvalidAction,
	}
)
