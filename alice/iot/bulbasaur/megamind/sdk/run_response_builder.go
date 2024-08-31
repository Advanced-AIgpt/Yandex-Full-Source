package sdk

import (
	"context"

	"google.golang.org/protobuf/types/known/anypb"

	"a.yandex-team.ru/alice/iot/bulbasaur/nlg"
	"a.yandex-team.ru/alice/library/go/libmegamind"
	libnlg "a.yandex-team.ru/alice/library/go/nlg"
	"a.yandex-team.ru/alice/megamind/protos/common"
	"a.yandex-team.ru/alice/megamind/protos/scenarios"
	"a.yandex-team.ru/alice/protos/data/language"
	"a.yandex-team.ru/alice/protos/data/scenario"
	"a.yandex-team.ru/library/go/core/buildinfo"
	"a.yandex-team.ru/library/go/core/xerrors"
)

// RunResponse creates response builder for pure megamind run responses (those that contain ResponseBody).
// The builder can be populated with nlg and/or directives through 'With*' build methods.
func RunResponse(runContext RunContext) NewRunResponseBuilder {
	return &runResponseBuilder{
		ctx:           runContext,
		analyticsInfo: AnalyticsInfo(),
	}
}

// RunApplyResponse creates megamind response with provided apply arguments.
func RunApplyResponse(applyArguments UniversalApplyArguments) (*scenarios.TScenarioRunResponse, error) {
	arguments, err := MarshalApplyArguments(applyArguments)
	if err != nil {
		return nil, xerrors.Errorf("failed to marshal apply arguments: %w", err)
	}
	response := &scenarios.TScenarioRunResponse{
		Response: &scenarios.TScenarioRunResponse_ApplyArguments{
			ApplyArguments: arguments,
		},
		Version: buildinfo.Info.SVNRevision,
	}

	return response, nil
}

// RunContinueResponse creates megamind response with provided continue arguments.
func RunContinueResponse(continueArguments UniversalContinueArguments) (*scenarios.TScenarioRunResponse, error) {
	arguments, err := MarshalContinueArguments(continueArguments)
	if err != nil {
		return nil, xerrors.Errorf("failed to marshal continue arguments: %w", err)
	}
	response := &scenarios.TScenarioRunResponse{
		Response: &scenarios.TScenarioRunResponse_ContinueArguments{
			ContinueArguments: arguments,
		},
		Version: buildinfo.Info.SVNRevision,
	}

	return response, nil
}

func RunSpecifyResponse(ctx RunContext, nlg libnlg.NLG, frame *libmegamind.SemanticFrame) (*scenarios.TScenarioRunResponse, error) {
	response := &scenarios.TScenarioRunResponse{
		Response: &scenarios.TScenarioRunResponse_ResponseBody{
			ResponseBody: &scenarios.TScenarioResponseBody{
				Response: &scenarios.TScenarioResponseBody_Layout{
					Layout: buildLayout(ctx.Context(), nlg, nil, true),
				},
				SemanticFrame: frame.Frame,
			},
		},
		Version: buildinfo.Info.SVNRevision,
	}
	return response, nil
}

// IrrelevantResponse creates megamind response with Irrelevant feature
func IrrelevantResponse(runContext RunContext, msg string) (*scenarios.TScenarioRunResponse, error) {
	runContext.Logger().Errorf("irrelevant: %s", msg)
	response := &scenarios.TScenarioRunResponse{
		Response: &scenarios.TScenarioRunResponse_ResponseBody{
			ResponseBody: &scenarios.TScenarioResponseBody{
				Response: &scenarios.TScenarioResponseBody_Layout{
					Layout: buildLayout(runContext.Context(), nlg.CommonError, nil, false),
				},
			},
		},
		Features: &scenarios.TScenarioRunResponse_TFeatures{
			IsIrrelevant: true,
		},
		Version: buildinfo.Info.SVNRevision,
	}
	return response, nil
}

// NewRunResponseBuilder provides some methods to build pure megamind run responses.
// Use RunResponse to initialize the builder and Build() method to construct *scenarios.TScenarioRunResponse object.
type NewRunResponseBuilder interface {
	WithNLG(nlg libnlg.NLG) NewRunResponseBuilder
	WithDirectives(directives ...*scenarios.TDirective) NewRunResponseBuilder
	WithStackEngine(stackEngine *scenarios.TStackEngine) NewRunResponseBuilder
	WithCallbackFrameActions(callbacks ...libmegamind.CallbackFrameAction) NewRunResponseBuilder
	WithState(state *anypb.Any) NewRunResponseBuilder
	ExpectRequest() NewRunResponseBuilder
	WithScenarioData(scenarioData *scenario.TScenarioData) NewRunResponseBuilder
	Build() (*scenarios.TScenarioRunResponse, error)
}

type runResponseBuilder struct {
	ctx           RunContext
	analyticsInfo AnalyticsInfoBuilder

	// nlg and directives are part of responseBody.Layout
	nlg        libnlg.NLG
	directives []*scenarios.TDirective

	// stackEngine is part of responseBody
	stackEngine *scenarios.TStackEngine

	callbackFrameActions []libmegamind.CallbackFrameAction
	state                *anypb.Any
	expectsRequest       bool

	scenarioData *scenario.TScenarioData
}

func (r *runResponseBuilder) WithNLG(nlg libnlg.NLG) NewRunResponseBuilder {
	r.nlg = nlg
	return r
}

func (r *runResponseBuilder) WithDirectives(directives ...*scenarios.TDirective) NewRunResponseBuilder {
	r.directives = directives
	return r
}

func (r *runResponseBuilder) WithStackEngine(stackEngine *scenarios.TStackEngine) NewRunResponseBuilder {
	r.stackEngine = stackEngine
	return r
}

func (r *runResponseBuilder) WithCallbackFrameActions(callbacks ...libmegamind.CallbackFrameAction) NewRunResponseBuilder {
	r.callbackFrameActions = callbacks
	return r
}

func (r *runResponseBuilder) WithState(state *anypb.Any) NewRunResponseBuilder {
	r.state = state
	return r
}

func (r *runResponseBuilder) ExpectRequest() NewRunResponseBuilder {
	r.expectsRequest = true
	return r
}

func (r *runResponseBuilder) WithScenarioData(scenarioData *scenario.TScenarioData) NewRunResponseBuilder {
	r.scenarioData = scenarioData
	return r
}

func (r *runResponseBuilder) Build() (*scenarios.TScenarioRunResponse, error) {
	builtAnalyticsInfo, err := r.analyticsInfo.Build()
	if err != nil {
		return nil, xerrors.Errorf("failed to build analytics info, %w", err)
	}

	responseBody := &scenarios.TScenarioResponseBody{
		Response: &scenarios.TScenarioResponseBody_Layout{
			Layout: r.buildLayout(),
		},
		StackEngine:    r.stackEngine,
		FrameActions:   r.buildFrameActions(),
		State:          r.state,
		AnalyticsInfo:  builtAnalyticsInfo,
		ExpectsRequest: r.expectsRequest,
		ScenarioData:   r.scenarioData,
	}

	return &scenarios.TScenarioRunResponse{
		Version: buildinfo.Info.SVNRevision,
		Response: &scenarios.TScenarioRunResponse_ResponseBody{
			ResponseBody: responseBody,
		},
	}, nil
}

func (r *runResponseBuilder) buildLayout() *scenarios.TLayout {
	return buildLayout(r.ctx.Context(), r.nlg, r.directives, r.expectsRequest)
}

func (r *runResponseBuilder) buildFrameActions() map[string]*scenarios.TFrameAction {
	frameActions := make(map[string]*scenarios.TFrameAction)
	if len(r.callbackFrameActions) == 0 {
		return frameActions
	}

	for _, frameAction := range r.callbackFrameActions {
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

func buildLayout(ctx context.Context, nlg libnlg.NLG, directives []*scenarios.TDirective, expectsRequest bool) *scenarios.TLayout {
	var layout scenarios.TLayout
	if len(nlg) > 0 {
		asset := nlg.RandomAsset(ctx)
		layout.Cards = []*scenarios.TLayout_TCard{
			{
				Card: &scenarios.TLayout_TCard_Text{
					Text: asset.Text(),
				},
			},
		}
		layout.OutputSpeech = asset.Speech()
	}
	layout.Directives = directives
	layout.ShouldListen = expectsRequest

	return &layout
}
