package mobile

import (
	"github.com/stretchr/testify/assert"
	"testing"
	"time"

	"a.yandex-team.ru/alice/library/go/solomonapi"
)

func TestParseGridDuration(t *testing.T) {
	testCases := []struct {
		name     string
		input    string
		hasError bool
		expected time.Duration
	}{
		{
			name:     "empty",
			input:    "",
			hasError: true,
		},
		{
			name:     "unknown format",
			input:    "1z",
			hasError: true,
		},
		{
			name:     "unknown format 2",
			input:    "1mh",
			hasError: true,
		},
		{
			name:     "unknown format 3",
			input:    "1smhd",
			hasError: true,
		},
		{
			name:     "seconds",
			input:    "2324s",
			hasError: false,
			expected: 2324 * time.Second,
		},
		{
			name:     "1 second",
			input:    "1s",
			hasError: false,
			expected: 1 * time.Second,
		},
		{
			name:     "1 minute",
			input:    "1m",
			hasError: false,
			expected: 1 * time.Minute,
		},
		{
			name:     "234 minute",
			input:    "234m",
			hasError: false,
			expected: 234 * time.Minute,
		},
		{
			name:     "5 hours",
			input:    "5h",
			hasError: false,
			expected: 5 * time.Hour,
		},
		{
			name:     "1 day",
			input:    "1d",
			hasError: false,
			expected: 24 * time.Hour,
		},
		{
			name:     "negative 1 day",
			input:    "-1d",
			hasError: true,
		},
		{
			name:     "31d",
			input:    "31d",
			hasError: false,
			expected: 31 * 24 * time.Hour,
		},
		{
			name:     "32d",
			input:    "32d",
			hasError: true,
		},
	}

	for _, tc := range testCases {
		t.Run(tc.name, func(t *testing.T) {
			actual, err := parseGridDuration(tc.input)
			if tc.hasError {
				assert.Error(t, err)
				assert.Equal(t, time.Duration(0), actual)
			} else {
				assert.NoError(t, err)
				assert.Equal(t, tc.expected, actual)
			}
		})
	}
}

func TestParseGapFilling(t *testing.T) {
	testCases := []struct {
		name     string
		input    string
		hasError bool
		expected solomonapi.GapFillingType
	}{
		{
			name:     "empty",
			input:    "",
			hasError: false,
			expected: solomonapi.NullGapFillingType,
		},
		{
			name:     "previous value",
			input:    "previous",
			hasError: false,
			expected: solomonapi.PreviousGapFillingType,
		},
		{
			name:     "null value",
			input:    "null",
			hasError: false,
			expected: solomonapi.NullGapFillingType,
		},
		{
			name:     "none value",
			input:    "none",
			hasError: false,
			expected: solomonapi.NoneGapFillingType,
		},
		{
			name:     "unknown value",
			input:    "some-unknown",
			hasError: true,
		},
	}

	for _, tc := range testCases {
		t.Run(tc.name, func(t *testing.T) {
			actual, err := parseGapFillingType(tc.input)
			if tc.hasError {
				assert.Error(t, err)
			} else {
				assert.NoError(t, err)
				assert.Equal(t, tc.expected, actual)
			}
		})
	}
}
