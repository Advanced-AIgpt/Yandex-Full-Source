package callback

import (
	"fmt"
)

type ErrorCode string

const (
	SubscriptionKeyErrorCode      ErrorCode = "SUBSCRIPTION_KEY_ERROR"
	ExternalUserNotFoundErrorCode ErrorCode = "EXTERNAL_USER_NOT_FOUND_ERROR"
	DiscoveryErrorCode            ErrorCode = "DISCOVERY_ERROR"
	StateTransformationErrorCode  ErrorCode = "STATE_TRANSFORMATION_ERROR"
	SteelixHTTPErrorCode          ErrorCode = "STEELIX_HTTP_ERROR"
)

type Error struct {
	Code ErrorCode
	err  error
}

func NewError(code ErrorCode, err error) error {
	return Error{
		Code: code,
		err:  err,
	}
}

func (e Error) Error() string {
	return fmt.Sprintf("callback error with code %s, wrapped: %v", e.Code, e.err)
}

func (e Error) Unwrap() error {
	return e.err
}
