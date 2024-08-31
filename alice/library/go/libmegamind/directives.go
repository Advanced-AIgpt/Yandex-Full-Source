package libmegamind

import "a.yandex-team.ru/alice/megamind/protos/scenarios"

func LEDAnimationDirective(animation LEDAnimation) *scenarios.TDirective {
	return &scenarios.TDirective{
		Directive: &scenarios.TDirective_DrawLedScreenDirective{
			DrawLedScreenDirective: &scenarios.TDrawLedScreenDirective{
				Name: DrawLedScreenDirectiveName,
				DrawItem: []*scenarios.TDrawLedScreenDirective_TDrawItem{
					{
						FrontalLedImage: animation.FrontalImageURL,
						Endless:         animation.IsEndless,
					},
				},
			},
		},
	}
}

func OpenURIDirective(uri string) *scenarios.TDirective {
	return &scenarios.TDirective{
		Directive: &scenarios.TDirective_OpenUriDirective{
			OpenUriDirective: &scenarios.TOpenUriDirective{
				Name: OpenURIDirectiveName,
				Uri:  uri,
			},
		},
	}
}
