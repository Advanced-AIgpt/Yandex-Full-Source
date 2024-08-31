package libmegamind

import (
	"github.com/golang/protobuf/ptypes/any"

	"a.yandex-team.ru/alice/megamind/protos/common"
	"a.yandex-team.ru/alice/megamind/protos/scenarios"
	"a.yandex-team.ru/alice/protos/data/language"
	"a.yandex-team.ru/library/go/core/buildinfo"
)

type RunResponse struct {
	*scenarios.TScenarioRunResponse
}

func NewRunResponse(scenarioName string, intent string) RunResponse {
	return RunResponse{
		&scenarios.TScenarioRunResponse{
			Version: buildinfo.Info.SVNRevision,
			Response: &scenarios.TScenarioRunResponse_ResponseBody{
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

func (rr RunResponse) Build() *scenarios.TScenarioRunResponse {
	return rr.TScenarioRunResponse
}

func (rr RunResponse) WithIrrelevant() RunResponse {
	rr.Features = &scenarios.TScenarioRunResponse_TFeatures{
		IsIrrelevant: true,
	}
	return rr
}

func (rr RunResponse) WithError(e string, errorType string) RunResponse {
	rr.Response = &scenarios.TScenarioRunResponse_Error{
		Error: &scenarios.TScenarioError{
			Message: e,
			Type:    errorType,
		},
	}
	return rr
}

func (rr RunResponse) WithLayout(text, speech string, directives ...*scenarios.TDirective) RunResponse {
	rr.GetResponseBody().Response = &scenarios.TScenarioResponseBody_Layout{
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
	return rr
}

func (rr RunResponse) WithNoOutputSpeechLayout(directives ...*scenarios.TDirective) RunResponse {
	rr.GetResponseBody().Response = &scenarios.TScenarioResponseBody_Layout{
		Layout: &scenarios.TLayout{
			Directives:   directives,
			ShouldListen: false,
		},
	}
	return rr
}

func (rr RunResponse) WithApplyArguments(a *any.Any) RunResponse {
	rr.Response = &scenarios.TScenarioRunResponse_ApplyArguments{ApplyArguments: a}
	return rr
}

func (rr RunResponse) WithContinueArguments(a *any.Any) RunResponse {
	rr.Response = &scenarios.TScenarioRunResponse_ContinueArguments{ContinueArguments: a}
	return rr
}

func (rr RunResponse) ExpectRequest() RunResponse {
	rr.GetResponseBody().ExpectsRequest = true
	if v, ok := rr.GetResponseBody().Response.(*scenarios.TScenarioResponseBody_Layout); ok {
		v.Layout.ShouldListen = true
	}
	return rr
}

func (rr RunResponse) WithCallbackFrameActions(actions ...CallbackFrameAction) RunResponse {
	for _, action := range actions {
		rr.WithCallbackFrameAction(action)
	}
	return rr
}

// FIXME: make private when BegemotProcessor dies
func (rr RunResponse) WithCallbackFrameAction(action CallbackFrameAction) RunResponse {
	frameActions := rr.GetResponseBody().FrameActions
	if frameActions == nil {
		frameActions = map[string]*scenarios.TFrameAction{}
	}
	nluPhrases := make([]*common.TNluPhrase, 0, len(action.Phrases))
	for _, p := range action.Phrases {
		nluPhrases = append(nluPhrases, &common.TNluPhrase{
			Language: language.ELang_L_RUS,
			Phrase:   p,
		})
	}

	frameActions[action.FrameSlug] = &scenarios.TFrameAction{
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
	}

	rr.GetResponseBody().FrameActions = frameActions
	return rr
}

func (rr RunResponse) WithState(s *any.Any) RunResponse {
	rr.GetResponseBody().State = s
	return rr
}

func (rr RunResponse) WithStackEngine(s *scenarios.TStackEngine) RunResponse {
	rr.GetResponseBody().StackEngine = s
	return rr
}
