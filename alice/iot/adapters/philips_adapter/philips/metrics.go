package philips

import (
	quasarmetrics "a.yandex-team.ru/alice/library/go/metrics"
	"a.yandex-team.ru/library/go/core/metrics"
)

type Signals struct {
	whileListLinkButton quasarmetrics.RouteSignals
	getUserName         quasarmetrics.RouteSignals
	getAllLights        quasarmetrics.RouteSignals
	setLightState       quasarmetrics.RouteSignals
}

func newSignals(registry metrics.Registry, policy quasarmetrics.BucketsGenerationPolicy) Signals {
	return Signals{
		whileListLinkButton: quasarmetrics.NewRouteSignals(registry.WithTags(map[string]string{"type": "philips", "api": "white_list_link_button"}), policy()),
		getUserName:         quasarmetrics.NewRouteSignals(registry.WithTags(map[string]string{"type": "philips", "api": "get_username"}), policy()),
		getAllLights:        quasarmetrics.NewRouteSignals(registry.WithTags(map[string]string{"type": "philips", "api": "get_all_lights"}), policy()),
		setLightState:       quasarmetrics.NewRouteSignals(registry.WithTags(map[string]string{"type": "philips", "api": "set_light_state"}), policy()),
	}
}
