package sdk

import (
	"a.yandex-team.ru/alice/library/go/libmegamind"
	libnlg "a.yandex-team.ru/alice/library/go/nlg"
	"a.yandex-team.ru/alice/megamind/protos/common"
	"a.yandex-team.ru/alice/megamind/protos/scenarios"
	"a.yandex-team.ru/alice/protos/data/language"
	"a.yandex-team.ru/library/go/core/buildinfo"
	"a.yandex-team.ru/library/go/core/xerrors"
)

type NewApplyResponseBuilder interface {
	WithAnalyticsInfo(analyticsInfo AnalyticsInfoBuilder) NewApplyResponseBuilder
	WithAnimation(animation *libmegamind.LEDAnimation) NewApplyResponseBuilder
	WithNLG(nlg libnlg.NLG) NewApplyResponseBuilder
	WithDirectives(directives ...*scenarios.TDirective) NewApplyResponseBuilder
	WithCallbackFrameActions(callbacks ...libmegamind.CallbackFrameAction) NewApplyResponseBuilder
	Build() (*scenarios.TScenarioApplyResponse, error)
}

func ApplyResponse(applyContext ApplyContext) NewApplyResponseBuilder {
	return &applyResponseBuilder{
		ctx:           applyContext,
		analyticsInfo: AnalyticsInfo(),
	}
}

type applyResponseBuilder struct {
	ctx ApplyContext

	// nlg and directives are part of responseBody.Layout
	nlg                  libnlg.NLG
	directives           []*scenarios.TDirective
	callbackFrameActions []libmegamind.CallbackFrameAction

	analyticsInfo AnalyticsInfoBuilder
	animation     *libmegamind.LEDAnimation
}

func (a *applyResponseBuilder) WithAnalyticsInfo(analyticsInfo AnalyticsInfoBuilder) NewApplyResponseBuilder {
	if analyticsInfo != nil {
		a.analyticsInfo = analyticsInfo
	}
	return a
}

func (a *applyResponseBuilder) WithAnimation(animation *libmegamind.LEDAnimation) NewApplyResponseBuilder {
	a.animation = animation
	return a
}

func (a *applyResponseBuilder) WithNLG(nlg libnlg.NLG) NewApplyResponseBuilder {
	a.nlg = nlg
	return a
}

func (a *applyResponseBuilder) WithDirectives(directives ...*scenarios.TDirective) NewApplyResponseBuilder {
	a.directives = directives
	return a
}

func (a *applyResponseBuilder) WithCallbackFrameActions(callbacks ...libmegamind.CallbackFrameAction) NewApplyResponseBuilder {
	a.callbackFrameActions = callbacks
	return a
}

func (a *applyResponseBuilder) Build() (*scenarios.TScenarioApplyResponse, error) {
	builtAnalyticsInfo, err := a.analyticsInfo.Build()
	if err != nil {
		return nil, xerrors.Errorf("failed to build analytics info, %w", err)
	}

	responseBody := &scenarios.TScenarioResponseBody{
		Response: &scenarios.TScenarioResponseBody_Layout{
			Layout: a.buildLayout(),
		},
		FrameActions:  a.buildFrameActions(),
		AnalyticsInfo: builtAnalyticsInfo,
	}

	return &scenarios.TScenarioApplyResponse{
		Version: buildinfo.Info.SVNRevision,
		Response: &scenarios.TScenarioApplyResponse_ResponseBody{
			ResponseBody: responseBody,
		},
	}, nil
}

func (a *applyResponseBuilder) buildLayout() *scenarios.TLayout {
	var layout scenarios.TLayout
	if len(a.nlg) > 0 {
		asset := a.nlg.RandomAsset(a.ctx.Context())
		layout.Cards = []*scenarios.TLayout_TCard{
			{
				Card: &scenarios.TLayout_TCard_Text{
					Text: asset.Text(),
				},
			},
		}
		layout.OutputSpeech = asset.Speech()
	}

	layout.Directives = a.buildDirectives()

	return &layout
}

func (a *applyResponseBuilder) buildDirectives() []*scenarios.TDirective {
	directives := make([]*scenarios.TDirective, 0, len(a.directives))

	directives = append(directives, a.directives...)

	if a.animation != nil {
		directives = append(directives, libmegamind.LEDAnimationDirective(*a.animation))
	}

	return directives
}

func (a *applyResponseBuilder) buildFrameActions() map[string]*scenarios.TFrameAction {
	frameActions := make(map[string]*scenarios.TFrameAction)
	if len(a.callbackFrameActions) == 0 {
		return frameActions
	}

	for _, frameAction := range a.callbackFrameActions {
		nluPhrases := make([]*common.TNluPhrase, 0, len(frameAction.Phrases))
		for _, phrase := range frameAction.Phrases {
			nluPhrases = append(nluPhrases, &common.TNluPhrase{
				Language: language.ELang_L_RUS,
				Phrase:   phrase,
			})
		}

		frameActions[frameAction.FrameSlug] = &scenarios.TFrameAction{
			NluHint: &common.TFrameNluHint{
				FrameName: frameAction.FrameName,
				Instances: nluPhrases,
			},
			Effect: &scenarios.TFrameAction_Callback{
				Callback: &scenarios.TCallbackDirective{
					Name:    string(frameAction.CallbackName),
					Payload: frameAction.CallbackPayload,
				},
			},
		}
	}

	return frameActions
}
