package solar

import (
	"github.com/stretchr/testify/assert"
	"testing"
	"time"
)

func TestSunsetUTC(t *testing.T) {
	testCases := []struct {
		name          string
		coordinates   Coordinates
		year          int
		month         time.Month
		day           int
		expectedTime  time.Time
		expectedError error
	}{
		{
			name: "moscow sunset",
			coordinates: Coordinates{
				Latitude:  55.7522,
				Longitude: 37.6156,
			},
			year:          2022,
			month:         time.April,
			day:           13,
			expectedTime:  time.Date(2022, time.April, 13, 16, 29, 49, 00, time.UTC),
			expectedError: nil,
		},
		{
			name: "spb sunset",
			coordinates: Coordinates{
				Latitude:  59.9386,
				Longitude: 30.3141,
			},
			year:          2022,
			month:         time.June,
			day:           22,
			expectedTime:  time.Date(2022, time.June, 22, 19, 25, 54, 00, time.UTC),
			expectedError: nil,
		},
		{
			name: "murmansk no sunset",
			coordinates: Coordinates{
				Latitude:  68.9792,
				Longitude: 33.0925,
			},
			year:          2022,
			month:         time.June,
			day:           22,
			expectedTime:  time.Time{},
			expectedError: NoSunriseSunsetError,
		},
		{
			name: "murmansk last sunset",
			coordinates: Coordinates{
				Latitude:  68.9792,
				Longitude: 33.0925,
			},
			year:          2022,
			month:         time.May,
			day:           21,
			expectedTime:  time.Date(2022, time.May, 21, 21, 30, 42, 00, time.UTC),
			expectedError: nil,
		},
		{
			name: "murmansk first polar day",
			coordinates: Coordinates{
				Latitude:  68.9792,
				Longitude: 33.0925,
			},
			year:          2022,
			month:         time.May,
			day:           22,
			expectedTime:  time.Time{},
			expectedError: NoSunriseSunsetError,
		},
	}

	for _, tc := range testCases {
		t.Run(tc.name, func(t *testing.T) {
			actualTime, err := SunsetUTC(tc.coordinates, tc.year, tc.month, tc.day)
			if tc.expectedError != nil {
				assert.ErrorIs(t, err, tc.expectedError)
			} else {
				assert.NoError(t, err)
				assert.Equal(t, tc.expectedTime, actualTime)
			}
		})
	}
}

func TestSunriseUTC(t *testing.T) {
	testCases := []struct {
		name          string
		coordinates   Coordinates
		year          int
		month         time.Month
		day           int
		expectedTime  time.Time
		expectedError error
	}{
		{
			name: "moscow sunrise",
			coordinates: Coordinates{
				Latitude:  55.7522,
				Longitude: 37.6156,
			},
			year:          2022,
			month:         time.April,
			day:           13,
			expectedTime:  time.Date(2022, time.April, 13, 2, 29, 59, 00, time.UTC),
			expectedError: nil,
		},
		{
			name: "spb sunrise",
			coordinates: Coordinates{
				Latitude:  59.9386,
				Longitude: 30.3141,
			},
			year:          2022,
			month:         time.June,
			day:           22,
			expectedTime:  time.Date(2022, time.June, 22, 0, 35, 24, 00, time.UTC),
			expectedError: nil,
		},
		{
			name: "murmansk no sunset",
			coordinates: Coordinates{
				Latitude:  68.9792,
				Longitude: 33.0925,
			},
			year:          2022,
			month:         time.June,
			day:           22,
			expectedTime:  time.Time{},
			expectedError: NoSunriseSunsetError,
		},
		{
			name: "murmansk last sunrise",
			coordinates: Coordinates{
				Latitude:  68.9792,
				Longitude: 33.0925,
			},
			year:          2022,
			month:         time.May,
			day:           21,
			expectedTime:  time.Date(2022, time.May, 20, 21, 58, 0, 00, time.UTC),
			expectedError: nil,
		},
		{
			name: "murmansk first polar day",
			coordinates: Coordinates{
				Latitude:  68.9792,
				Longitude: 33.0925,
			},
			year:          2022,
			month:         time.May,
			day:           22,
			expectedTime:  time.Time{},
			expectedError: NoSunriseSunsetError,
		},
		{
			name: "murmansk first sunrise after polar night",
			coordinates: Coordinates{
				Latitude:  68.9792,
				Longitude: 33.0925,
			},
			year:          2022,
			month:         time.January,
			day:           11,
			expectedTime:  time.Date(2022, time.January, 11, 9, 43, 17, 00, time.UTC),
			expectedError: nil,
		},
	}

	for _, tc := range testCases {
		t.Run(tc.name, func(t *testing.T) {
			actualTime, err := SunriseUTC(tc.coordinates, tc.year, tc.month, tc.day)
			if tc.expectedError != nil {
				assert.ErrorIs(t, err, tc.expectedError)
			} else {
				assert.NoError(t, err)
				assert.Equal(t, tc.expectedTime, actualTime)
			}
		})
	}
}
