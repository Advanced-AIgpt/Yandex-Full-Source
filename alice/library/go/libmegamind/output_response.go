package libmegamind

import (
	libnlg "a.yandex-team.ru/alice/library/go/nlg"
	"a.yandex-team.ru/alice/megamind/protos/scenarios"
)

type OutputResponse struct {
	NLG       libnlg.NLG
	Animation *LEDAnimation
	Callback  *CallbackFrameAction
	URI       *string
	Video     *scenarios.TVideoPlayDirective

	directives []*scenarios.TDirective
}

func (r *OutputResponse) AddDirectives(directives []*scenarios.TDirective) {
	r.directives = append(r.directives, directives...)
}

func (r OutputResponse) Directives() []*scenarios.TDirective {
	directives := make([]*scenarios.TDirective, 0, len(r.directives))
	directives = append(directives, r.directives...)

	if r.Animation != nil {
		directives = append(directives, LEDAnimationDirective(*r.Animation))
	}
	if r.URI != nil {
		directives = append(directives, OpenURIDirective(*r.URI))
	}
	if r.Video != nil {
		directives = append(directives, &scenarios.TDirective{
			Directive: &scenarios.TDirective_VideoPlayDirective{
				VideoPlayDirective: r.Video,
			},
		})
	}
	return directives
}
