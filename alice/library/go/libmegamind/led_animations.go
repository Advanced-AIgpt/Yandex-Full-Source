package libmegamind

import (
	"fmt"
	"math"
)

type LEDAnimation struct {
	FrontalImageURL string
	IsEndless       bool
}

var (
	ScenarioOKLedAnimation = LEDAnimation{
		FrontalImageURL: "https://static-alice.s3.yandex.net/led-production/iot/success.gif",
		IsEndless:       false,
	}
)

func PercentLEDAnimation(percent float64) (LEDAnimation, bool) {
	percent = math.Round(percent)
	if percent < 0 || percent > 100 {
		return LEDAnimation{}, false
	}
	return LEDAnimation{
		FrontalImageURL: fmt.Sprintf("https://static-alice.s3.yandex.net/led-production/iot/percents/%d.gif", int64(percent)),
		IsEndless:       false,
	}, true
}

func TemperatureLEDAnimation(temperature float64) (LEDAnimation, bool) {
	temperature = math.Round(temperature)
	if temperature < -200 || temperature > 200 {
		return LEDAnimation{}, false
	}
	var gifPrefix string
	if temperature < 0 {
		gifPrefix = "-"
	}
	return LEDAnimation{
		FrontalImageURL: fmt.Sprintf("https://static-alice.s3.yandex.net/led-production/iot/temperature/%s%d.gif", gifPrefix, int64(temperature)),
		IsEndless:       false,
	}, true
}
