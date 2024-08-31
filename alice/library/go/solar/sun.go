package solar

import (
	"time"

	"github.com/nathan-osman/go-sunrise"

	"a.yandex-team.ru/library/go/core/xerrors"
)

type Coordinates struct {
	Latitude  float64
	Longitude float64
}

var NoSunriseSunsetError = xerrors.New("no sunset or sunrise on the given day")

// SunriseUTC calculates sunrise time on the given day at given place
// returns NoSunriseSunsetError if day has no sunrise (on polar day or night)
func SunriseUTC(coordinates Coordinates, year int, month time.Month, day int) (time.Time, error) {
	rise, _ := sunrise.SunriseSunset(
		coordinates.Latitude, coordinates.Longitude,
		year, month, day,
	)

	if rise.IsZero() {
		return time.Time{}, xerrors.Errorf("no sunrise on day %d-%d-%d: %w", year, month, day, NoSunriseSunsetError)
	}

	return rise, nil
}

// SunsetUTC calculates sunset time on the given day at given place
// returns NoSunriseSunsetError if day has no sunset (on polar day or night)
func SunsetUTC(coordinates Coordinates, year int, month time.Month, day int) (time.Time, error) {
	_, set := sunrise.SunriseSunset(
		coordinates.Latitude, coordinates.Longitude,
		year, month, day,
	)

	if set.IsZero() {
		return time.Time{}, xerrors.Errorf("no sunset on day %d-%d-%d: %w", year, month, day, NoSunriseSunsetError)
	}

	return set, nil
}
