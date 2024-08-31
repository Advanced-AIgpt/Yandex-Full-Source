package libmegamind

import (
	"google.golang.org/protobuf/types/known/anypb"

	"a.yandex-team.ru/alice/megamind/protos/analytics/scenarios/iot"
	"a.yandex-team.ru/alice/megamind/protos/common"
	"a.yandex-team.ru/alice/megamind/protos/scenarios"
	"a.yandex-team.ru/alice/protos/data/language"
	"a.yandex-team.ru/library/go/core/buildinfo"
)

type ApplyResponse struct {
	*scenarios.TScenarioApplyResponse
}

func NewApplyResponse(scenarioName string, intent string) ApplyResponse {
	return ApplyResponse{
		&scenarios.TScenarioApplyResponse{
			Version: buildinfo.Info.SVNRevision,
			Response: &scenarios.TScenarioApplyResponse_ResponseBody{
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

func (ar ApplyResponse) Build() *scenarios.TScenarioApplyResponse {
	return ar.TScenarioApplyResponse
}

func (ar ApplyResponse) WithLayout(text, speech string, directives ...*scenarios.TDirective) ApplyResponse {
	ar.GetResponseBody().Response = &scenarios.TScenarioResponseBody_Layout{
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
	return ar
}

func (ar ApplyResponse) WithNoOutputSpeechLayout(directives ...*scenarios.TDirective) ApplyResponse {
	ar.GetResponseBody().Response = &scenarios.TScenarioResponseBody_Layout{
		Layout: &scenarios.TLayout{
			Directives:   directives,
			ShouldListen: false,
		},
	}
	return ar
}

func (ar ApplyResponse) WithState(state *anypb.Any) ApplyResponse {
	ar.GetResponseBody().State = state
	return ar
}

func (ar ApplyResponse) ResetState() ApplyResponse {
	return ar.WithState(nil)
}

func (ar ApplyResponse) WithDirectives(directives ...*scenarios.TDirective) ApplyResponse {
	layout := ar.GetResponseBody().GetLayout()
	if layout == nil {
		layout = &scenarios.TLayout{}
	}
	layout.Directives = directives
	ar.GetResponseBody().Response = &scenarios.TScenarioResponseBody_Layout{
		Layout: layout,
	}
	return ar
}

func (ar ApplyResponse) WithIoTAnalyticsInfo(hypotheses *iot.THypotheses, selectedHypotheses *iot.TSelectedHypotheses) ApplyResponse {
	ar.GetResponseBody().AnalyticsInfo.Objects = []*scenarios.TAnalyticsInfo_TObject{
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
	return ar
}

func (ar ApplyResponse) WithCallbackFrameAction(action CallbackFrameAction) ApplyResponse {
	nluPhrases := make([]*common.TNluPhrase, 0, len(action.Phrases))
	for _, p := range action.Phrases {
		nluPhrases = append(nluPhrases, &common.TNluPhrase{
			Language: language.ELang_L_RUS,
			Phrase:   p,
		})
	}

	ar.GetResponseBody().FrameActions = map[string]*scenarios.TFrameAction{
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
	return ar
}
