package megamind

import (
	"a.yandex-team.ru/alice/iot/bulbasaur/dto/megamind"
)

type AdditionalSources struct {
	time *megamind.Time
}

func NewAdditionalSources() *AdditionalSources {
	return &AdditionalSources{}
}

func (a *AdditionalSources) WithTime(t megamind.Time) *AdditionalSources {
	a.time = &t
	return a
}

func (a *AdditionalSources) Time() (megamind.Time, bool) {
	if a.time == nil {
		return megamind.Time{}, false
	}
	return *a.time, true
}
