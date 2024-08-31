package megamind

import (
	"context"
	"fmt"

	"a.yandex-team.ru/alice/iot/bulbasaur/nlg"
	"a.yandex-team.ru/alice/library/go/libmegamind"
	"a.yandex-team.ru/alice/library/go/setrace"
	"a.yandex-team.ru/alice/megamind/protos/scenarios"
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/log/ctxlog"
)

func ErrorApplyResponse(ctx context.Context, logger log.Logger, err error) (*scenarios.TScenarioApplyResponse, error) {
	asset := nlg.CommonError.RandomAsset(ctx)
	response := libmegamind.NewApplyResponse(IoTProductScenarioName, IoTScenarioIntent).
		WithLayout(asset.Text(), asset.Speech()).
		Build()
	ctxlog.Warnf(ctx, logger, "ErrorApplyResponse: %v", err)
	setrace.ErrorLogEvent(
		ctx, logger,
		fmt.Sprintf("ErrorApplyResponse: %v", err),
	)
	return response, nil
}

func NlgApplyResponse(ctx context.Context, logger log.Logger, r libmegamind.OutputResponse, ai *AnalyticsInfo) (*scenarios.TScenarioApplyResponse, error) {
	response := libmegamind.NewApplyResponse(IoTProductScenarioName, IoTScenarioIntent)
	if r.NLG != nil {
		asset := r.NLG.RandomAsset(ctx)
		ctxlog.Debugf(ctx, logger, "NlgApplyResponse: text %q, speech %q, isTextOnly: %v", asset.Text(), asset.Speech(), asset.IsTextOnly())
		setrace.InfoLogEvent(
			ctx, logger,
			fmt.Sprintf("NlgApplyResponse: text %q, speech %q, isTextOnly: %v", asset.Text(), asset.Speech(), asset.IsTextOnly()),
		)
		response.WithLayout(asset.Text(), asset.Speech())
	} else {
		ctxlog.Debugf(ctx, logger, "silentApplyResponse")
		setrace.InfoLogEvent(
			ctx, logger,
			"silentApplyResponse",
		)
	}
	if directives := r.Directives(); len(directives) > 0 {
		response.WithDirectives(directives...)
	}
	if r.Callback != nil {
		response.WithCallbackFrameAction(*r.Callback)
	}
	if ai != nil {
		response.WithIoTAnalyticsInfo(ai.Hypotheses, ai.SelectedHypotheses)
	}
	return response.Build(), nil
}
