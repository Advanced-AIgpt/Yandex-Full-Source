package provider

import (
	"strings"

	"a.yandex-team.ru/alice/iot/bulbasaur/dto/adapter"
	"a.yandex-team.ru/alice/iot/bulbasaur/model"
	"a.yandex-team.ru/alice/megamind/protos/scenarios"
)

type SkillInfo struct {
	ApplicationName   string
	HumanReadableName string
	SkillID           string
	Endpoint          string
	TVMAlias          string
	IsLambda          bool
	Trusted           bool
	Public            bool
}

func (si *SkillInfo) IsDraft() bool {
	return strings.Contains(si.SkillID, "-draft")
}

func (si *SkillInfo) IsIntranet() bool {
	switch si.SkillID {
	case model.YANDEXIO, model.QUASAR, model.TUYA, model.SberSkill, model.XiaomiSkill:
		return true
	default:
		return false
	}
}

type ActionResult struct {
	AdapterResult adapter.ActionResult
	SideEffects   SideEffects
}

type SideEffects struct {
	Directives []Directive
	// todo(galecore): support notificator frames as side effects
}

type Directive struct {
	*scenarios.TDirective
}
