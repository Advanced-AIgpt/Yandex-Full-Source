package model

import (
	"testing"
	"time"

	"a.yandex-team.ru/alice/library/go/timestamp"
	"github.com/stretchr/testify/assert"
)

func TestKnownEventsMapConsistency(t *testing.T) {
	for eventKey, event := range KnownEvents {
		assert.Equal(t, eventKey.Value, event.Value)
	}
}

func TestEventPropertyStateStatus(t *testing.T) {
	motion := MakePropertyByType(EventPropertyType)
	motion.SetParameters(EventPropertyParameters{
		Instance: MotionPropertyInstance,
		Events: Events{
			KnownEvents[EventKey{Instance: MotionPropertyInstance, Value: DetectedEvent}],
			KnownEvents[EventKey{Instance: MotionPropertyInstance, Value: NotDetectedEvent}],
		},
	})
	type testCase struct {
		retrievable    bool
		stateChangedAt timestamp.PastTimestamp
		eventValue     EventValue
		expectedStatus PropertyStatus
	}

	nowTimestamp := timestamp.Now()
	testCases := []testCase{
		{
			retrievable:    true,
			eventValue:     DetectedEvent,
			expectedStatus: DangerStatus,
		},
		{
			retrievable:    true,
			eventValue:     NotDetectedEvent,
			stateChangedAt: nowTimestamp.Add(-1 * time.Minute),
			expectedStatus: WarningStatus,
		},
		{
			retrievable:    true,
			eventValue:     NotDetectedEvent,
			stateChangedAt: nowTimestamp.Add(-40 * time.Minute),
			expectedStatus: NormalStatus,
		},
		{
			eventValue:     NotDetectedEvent,
			stateChangedAt: nowTimestamp.Add(-5 * time.Minute),
			expectedStatus: DangerStatus,
		},
		{
			eventValue:     NotDetectedEvent,
			stateChangedAt: nowTimestamp.Add(-20 * time.Minute),
			expectedStatus: WarningStatus,
		},
		{
			eventValue:     NotDetectedEvent,
			stateChangedAt: nowTimestamp.Add(-50 * time.Minute),
			expectedStatus: NormalStatus,
		},
	}
	for i, tc := range testCases {
		motion.SetRetrievable(tc.retrievable)
		motion.SetState(EventPropertyState{Instance: MotionPropertyInstance, Value: tc.eventValue})
		motion.SetStateChangedAt(tc.stateChangedAt)
		assert.NotNil(t, motion.Status(nowTimestamp))
		assert.Equal(t, PS(tc.expectedStatus), motion.Status(nowTimestamp), "test case %d failed", i)
	}
}
