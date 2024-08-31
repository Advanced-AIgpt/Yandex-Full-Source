package libmegamind

import (
	"a.yandex-team.ru/alice/megamind/protos/analytics/scenarios/iot"
	"a.yandex-team.ru/alice/megamind/protos/common"
	"a.yandex-team.ru/alice/megamind/protos/scenarios"
	"a.yandex-team.ru/alice/protos/data/language"
	"a.yandex-team.ru/library/go/core/buildinfo"
)

type ContinueResponse struct {
	*scenarios.TScenarioContinueResponse
}

func NewContinueResponse(scenarioName string, intent string) ContinueResponse {
	return ContinueResponse{
		TScenarioContinueResponse: &scenarios.TScenarioContinueResponse{
			Version: buildinfo.Info.SVNRevision,
			Response: &scenarios.TScenarioContinueResponse_ResponseBody{
				ResponseBody: &scenarios.TScenarioResponseBody{
					AnalyticsInfo: &scenarios.TAnalyticsInfo{
						ProductScenarioName: scenarioName,
						Intent:              intent,
					},
				},
			},
		},
	}
}

func (cr ContinueResponse) Build() *scenarios.TScenarioContinueResponse {
	return cr.TScenarioContinueResponse
}

func (cr ContinueResponse) WithLayout(text, speech string, directives ...*scenarios.TDirective) ContinueResponse {
	cr.GetResponseBody().Response = &scenarios.TScenarioResponseBody_Layout{
		Layout: &scenarios.TLayout{
			Cards: []*scenarios.TLayout_TCard{
				{
					Card: &scenarios.TLayout_TCard_Text{
						Text: text,
					},
				},
			},
			OutputSpeech: speech,
			Directives:   directives,
		},
	}
	return cr
}

func (cr ContinueResponse) WithDirectives(directives ...*scenarios.TDirective) ContinueResponse {
	layout := cr.GetResponseBody().GetLayout()
	if layout == nil {
		layout = &scenarios.TLayout{}
	}
	layout.Directives = directives
	cr.GetResponseBody().Response = &scenarios.TScenarioResponseBody_Layout{
		Layout: layout,
	}
	return cr
}

func (cr ContinueResponse) WithIoTAnalyticsInfo(hypotheses *iot.THypotheses, selectedHypotheses *iot.TSelectedHypotheses) ContinueResponse {
	cr.GetResponseBody().AnalyticsInfo.Objects = []*scenarios.TAnalyticsInfo_TObject{
		{
			Id: "hypotheses",
			Payload: &scenarios.TAnalyticsInfo_TObject_Hypotheses{
				Hypotheses: hypotheses,
			},
		}, {
			Id: "selected_hypotheses",
			Payload: &scenarios.TAnalyticsInfo_TObject_SelectedHypotheses{
				SelectedHypotheses: selectedHypotheses,
			},
		},
	}
	return cr
}

func (cr ContinueResponse) WithCallbackFrameAction(action CallbackFrameAction) ContinueResponse {
	nluPhrases := make([]*common.TNluPhrase, 0, len(action.Phrases))
	for _, p := range action.Phrases {
		nluPhrases = append(nluPhrases, &common.TNluPhrase{
			Language: language.ELang_L_RUS,
			Phrase:   p,
		})
	}

	cr.GetResponseBody().FrameActions = map[string]*scenarios.TFrameAction{
		action.FrameSlug: {
			NluHint: &common.TFrameNluHint{
				FrameName: action.FrameName,
				Instances: nluPhrases,
			},
			Effect: &scenarios.TFrameAction_Callback{
				Callback: &scenarios.TCallbackDirective{
					Name:    string(action.CallbackName),
					Payload: action.CallbackPayload,
				},
			},
		},
	}
	return cr
}
