package dialogs

import (
	quasarmetrics "a.yandex-team.ru/alice/library/go/metrics"
	"a.yandex-team.ru/library/go/core/metrics"
)

type signals struct {
	getSkill                 quasarmetrics.RouteSignalsWithTotal
	getSmartHomeSkills       quasarmetrics.RouteSignalsWithTotal
	getSkillCertifiedDevices quasarmetrics.RouteSignalsWithTotal
}

func newSignals(registry metrics.Registry) signals {
	return signals{
		getSkill:                 quasarmetrics.NewRouteSignalsWithTotal(registry.WithTags(map[string]string{"call": "getSkill"}), quasarmetrics.DefaultExponentialBucketsPolicy()),
		getSmartHomeSkills:       quasarmetrics.NewRouteSignalsWithTotal(registry.WithTags(map[string]string{"call": "getSmartHomeSkills"}), quasarmetrics.DefaultExponentialBucketsPolicy()),
		getSkillCertifiedDevices: quasarmetrics.NewRouteSignalsWithTotal(registry.WithTags(map[string]string{"call": "getSkillCertifiedDevices"}), quasarmetrics.DefaultExponentialBucketsPolicy()),
	}
}
