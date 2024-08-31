package model

import (
	"testing"
	"time"

	"a.yandex-team.ru/alice/library/go/timestamp"
	"github.com/stretchr/testify/assert"
)

func TestActualDeviceState(t *testing.T) {
	timestamper := timestamp.NewMockTimestamper()
	deviceState := DeviceState{
		Type:    BusyDeviceState,
		Updated: timestamper.CurrentTimestamp(),
	}
	currentTime := timestamper.CurrentTimestamp().AsTime().Add(time.Minute * 6)
	assert.Equal(t, ReadyDeviceState, deviceState.ActualStateType(timestamp.FromTime(currentTime)))

	olderTime := timestamper.CurrentTimestamp().AsTime().Add(time.Minute * 2)
	assert.Equal(t, BusyDeviceState, deviceState.ActualStateType(timestamp.FromTime(olderTime)))
}
