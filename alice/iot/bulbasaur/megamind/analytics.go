package megamind

import (
	"a.yandex-team.ru/alice/megamind/protos/analytics/scenarios/iot"
)

//this is just a sugar to pass to the functions
type AnalyticsInfo struct {
	Hypotheses         *iot.THypotheses
	SelectedHypotheses *iot.TSelectedHypotheses
}

func formAnalyticsInfo(hypotheses *iot.THypotheses, selectedHypotheses *iot.TSelectedHypotheses) *AnalyticsInfo {
	return &AnalyticsInfo{
		Hypotheses:         hypotheses,
		SelectedHypotheses: selectedHypotheses,
	}
}
