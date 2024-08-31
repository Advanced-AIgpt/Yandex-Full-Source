package model

import (
	"testing"

	"github.com/stretchr/testify/assert"
)

func TestDeferredEventIsRelevant(t *testing.T) {
	type testCase struct {
		state          EventPropertyState
		deferredEvent  EventValue
		expected       bool
		expectedReason string
	}
	testCases := []testCase{
		{
			state: EventPropertyState{
				Instance: OpenPropertyInstance,
				Value:    OpenedEvent,
			},
			deferredEvent:  OpenedForMinute,
			expected:       true,
			expectedReason: "openPropertyInstance stateValue = opened; expectedValue = opened",
		},
		{
			state: EventPropertyState{
				Instance: OpenPropertyInstance,
				Value:    OpenedEvent,
			},
			deferredEvent:  OpenedFor2Minutes,
			expected:       true,
			expectedReason: "openPropertyInstance stateValue = opened; expectedValue = opened",
		},
		{
			state: EventPropertyState{
				Instance: OpenPropertyInstance,
				Value:    ClosedEvent,
			},
			deferredEvent:  OpenedFor2Minutes,
			expected:       false,
			expectedReason: "openPropertyInstance stateValue = closed; expectedValue = opened",
		},
		{
			state: EventPropertyState{
				Instance: MotionPropertyInstance,
				Value:    DetectedEvent,
			},
			deferredEvent:  NotDetectedWithinMinute,
			expected:       true,
			expectedReason: "deferred event key found for value not_detected_within_1m",
		},
		{
			state: EventPropertyState{
				Instance: MotionPropertyInstance,
				Value:    DetectedEvent,
			},
			deferredEvent:  NotDetectedWithin5Minutes,
			expected:       true,
			expectedReason: "deferred event key found for value not_detected_within_5m",
		},
	}
	for _, tc := range testCases {
		result := tc.deferredEvent.IsDeferredEventRelevant(tc.state)
		assert.Equal(t, tc.expected, result.IsRelevant)
		assert.Equal(t, tc.expectedReason, result.Reason)
	}
}
