package model

import (
	"a.yandex-team.ru/alice/library/go/timestamp"
)

type DeviceStateType string

type DeviceState struct {
	Type    DeviceStateType
	Updated timestamp.PastTimestamp
}

func (ds DeviceState) ActualStateType(now timestamp.PastTimestamp) DeviceStateType {
	switch ds.Type {
	case BusyDeviceState:
		if ds.Updated.AsTime().Add(StateVerificationThreshold).Before(now.AsTime()) {
			return ReadyDeviceState
		}
		return ds.Type
	default:
		return ds.Type
	}
}
