package libmegamind

import (
	"context"
	"fmt"

	"a.yandex-team.ru/alice/iot/bulbasaur/model"
	libnlg "a.yandex-team.ru/alice/library/go/nlg"
	"a.yandex-team.ru/alice/library/go/setrace"
	"a.yandex-team.ru/alice/megamind/protos/scenarios"
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/log/ctxlog"
)

type Dispatcher struct {
	Processors []IProcessor
	Logger     log.Logger
	Config     DispatcherConfig
}

type DispatcherConfig struct {
	DefaultScenarioName string
	DefaultIntentName   string
	IrrelevantNLG       libnlg.NLG
	ErrorNLG            libnlg.NLG
}

func (d Dispatcher) Run(ctx context.Context, runRequest *scenarios.TScenarioRunRequest, user model.User) (*scenarios.TScenarioRunResponse, error) {
	for _, p := range d.Processors {
		if p.IsRunnable(ctx, runRequest) {
			message := fmt.Sprintf("`%s` processor has been chosen", p.Name())
			ctxlog.Infof(ctx, d.Logger, message)
			setrace.InfoLogEvent(ctx, d.Logger, message)
			return p.Run(ctx, runRequest, user)
		}
	}
	return d.irrelevant(ctx, d.Logger)
}

func (d Dispatcher) Apply(ctx context.Context, applyRequest *scenarios.TScenarioApplyRequest, user model.User) (*scenarios.TScenarioApplyResponse, error) {
	for _, p := range d.Processors {
		if p.IsApplicable(ctx, applyRequest) {
			return p.Apply(ctx, applyRequest, user)
		}
	}
	return d.errorApplyResponse(ctx, d.Logger)
}

func (d Dispatcher) Continue(ctx context.Context, applyRequest *scenarios.TScenarioApplyRequest, user model.User) (*scenarios.TScenarioContinueResponse, error) {
	for _, p := range d.Processors {
		if p.IsContinuable(ctx, applyRequest) {
			return p.Continue(ctx, applyRequest, user)
		}
	}
	return d.errorContinueResponse(ctx, d.Logger)
}

func (d Dispatcher) irrelevant(ctx context.Context, logger log.Logger) (*scenarios.TScenarioRunResponse, error) {
	irrelevantAsset := d.Config.IrrelevantNLG.RandomAsset(ctx)
	ctxlog.Info(ctx, logger, "dispatcher: returning irrelevant response")
	setrace.InfoLogEvent(ctx, logger, "dispatcher: returning irrelevant response")
	response := NewRunResponse(d.Config.DefaultScenarioName, d.Config.DefaultIntentName).WithIrrelevant().WithLayout(irrelevantAsset.Text(), irrelevantAsset.Speech())
	return response.Build(), nil
}

func (d Dispatcher) errorApplyResponse(ctx context.Context, logger log.Logger) (*scenarios.TScenarioApplyResponse, error) {
	asset := d.Config.ErrorNLG.RandomAsset(ctx)
	response := NewApplyResponse(d.Config.DefaultScenarioName, d.Config.DefaultIntentName).WithLayout(asset.Text(), asset.Speech())
	ctxlog.Info(ctx, logger, fmt.Sprintf("dispatcher: nlgApplyResponse: `%s`", asset.Text()))
	setrace.InfoLogEvent(ctx, logger, fmt.Sprintf("dispatcher: nlgApplyResponse: `%s`", asset.Text()))
	return response.Build(), nil
}

func (d Dispatcher) errorContinueResponse(ctx context.Context, logger log.Logger) (*scenarios.TScenarioContinueResponse, error) {
	asset := d.Config.ErrorNLG.RandomAsset(ctx)
	response := NewContinueResponse(d.Config.DefaultScenarioName, d.Config.DefaultIntentName).WithLayout(asset.Text(), asset.Speech())
	ctxlog.Info(ctx, logger, fmt.Sprintf("dispatcher: nlgContinueResponse: `%s`", asset.Text()))
	setrace.InfoLogEvent(ctx, logger, fmt.Sprintf("dispatcher: nlgContinueResponse: `%s`", asset.Text()))
	return response.Build(), nil
}
