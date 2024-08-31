package timetable

import (
	"github.com/stretchr/testify/assert"
	"math/rand"
	"testing"
	"time"
)

func TestRandomJitter(t *testing.T) {
	testCases := []struct {
		name            string
		leftBorder      time.Duration
		rightBorder     time.Duration
		applyToPatterns []int
	}{
		{
			name:        "apply jitter",
			leftBorder:  1 * time.Second,
			rightBorder: 5 * time.Second,
		},
		{
			name:        "apply another jitter",
			leftBorder:  5 * time.Second,
			rightBorder: 10 * time.Second,
		},
		{
			name:        "jitter before planned time",
			leftBorder:  -10 * time.Second,
			rightBorder: -5 * time.Second,
		},
	}
	rand.Seed(time.Now().UTC().UnixNano())

	for _, tc := range testCases {
		t.Run(tc.name, func(t *testing.T) {
			jitter := &RandomJitter{
				leftBorder:  tc.leftBorder,
				rightBorder: tc.rightBorder,
			}

			lag := jitter.Jit()
			assert.NotEqual(t, time.Duration(0), lag)
			assert.GreaterOrEqual(t, lag, tc.leftBorder)
			assert.Less(t, lag, tc.rightBorder)
		})
	}
}
