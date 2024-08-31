package megamind

import (
	"a.yandex-team.ru/alice/megamind/protos/scenarios"
)

func getTypeTextDirective(text string) *scenarios.TDirective {
	return &scenarios.TDirective{
		Directive: &scenarios.TDirective_TypeTextDirective{
			TypeTextDirective: &scenarios.TTypeTextDirective{
				Name: "type_text_directive",
				Text: text,
			},
		},
	}
}
