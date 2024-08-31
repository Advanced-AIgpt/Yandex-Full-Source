package iotapi

import (
	"fmt"
)

type HTTPForbiddenError struct{}

func (hfe *HTTPForbiddenError) Error() string {
	return "forbidden"
}

type DeviceNotFoundError struct {
	DeviceID string
}

func (e DeviceNotFoundError) Error() string {
	return fmt.Sprintf("device %s is not found", e.DeviceID)
}

type FeatureNotOnlineError struct{}

func (e FeatureNotOnlineError) Error() string {
	return "feature is not online"
}
