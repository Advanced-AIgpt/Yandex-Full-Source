package megamind

import (
	"context"
	"fmt"

	"google.golang.org/protobuf/types/known/anypb"

	"a.yandex-team.ru/alice/iot/bulbasaur/nlg"
	"a.yandex-team.ru/alice/iot/bulbasaur/protos"
	"a.yandex-team.ru/alice/library/go/libmegamind"
	libnlg "a.yandex-team.ru/alice/library/go/nlg"
	"a.yandex-team.ru/alice/library/go/setrace"
	"a.yandex-team.ru/alice/megamind/protos/scenarios"
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/log/ctxlog"
)

func BuildRunResponse(ctx context.Context, logger log.Logger, aaProto *protos.TApplyArguments) (*scenarios.TScenarioRunResponse, error) {
	serialized, err := anypb.New(aaProto)
	if err != nil {
		ctxlog.Warnf(ctx, logger, "could not serialize ApplyArguments: %v", err)
		setrace.ErrorLogEvent(
			ctx, logger,
			fmt.Sprintf("could not serialize ApplyArguments: %v", err),
		)
		return ErrorRunResponse(ctx, logger, err)
	}
	response := libmegamind.NewRunResponse(IoTProductScenarioName, IoTScenarioIntent).WithApplyArguments(serialized)
	return response.Build(), nil
}

func Irrelevant(ctx context.Context, logger log.Logger, reason IrrelevantReason) (*scenarios.TScenarioRunResponse, error) {
	ctxlog.Info(ctx, logger, "Irrelevant", log.Any("reason", reason))
	asset := nlg.CannotDo.RandomAsset(ctx)
	setrace.InfoLogEvent(
		ctx, logger,
		fmt.Sprintf("NlgRunResponse: `%s`", asset.Text()),
		log.Any("reason", reason),
	)
	response := libmegamind.NewRunResponse(IoTProductScenarioName, IoTScenarioIntent).WithLayout(asset.Text(), asset.Speech()).WithIrrelevant()
	return response.Build(), nil
}

func ErrorRunResponse(ctx context.Context, logger log.Logger, e error) (*scenarios.TScenarioRunResponse, error) {
	asset := nlg.CannotDo.RandomAsset(ctx)
	ctxlog.Info(ctx, logger, "returning error run response", log.Any("reason", e.Error()))
	setrace.InfoLogEvent(
		ctx, logger,
		fmt.Sprintf("NlgRunResponse: `%s`", asset.Text()),
		log.Any("reason", e.Error()),
	)
	response := libmegamind.NewRunResponse(IoTProductScenarioName, IoTScenarioIntent).WithLayout(asset.Text(), asset.Speech())
	return response.Build(), nil
}

func SilentRunResponse(ctx context.Context, logger log.Logger) (*scenarios.TScenarioRunResponse, error) {
	ctxlog.Debugf(ctx, logger, "SilentRunResponse")
	setrace.InfoLogEvent(ctx, logger, "SilentRunResponse")
	return libmegamind.NewRunResponse(IoTProductScenarioName, IoTScenarioIntent).Build(), nil
}

func NlgRunResponse(ctx context.Context, logger log.Logger, n libnlg.NLG) (*scenarios.TScenarioRunResponse, error) {
	if n == nil {
		return SilentRunResponse(ctx, logger)
	}
	return NlgRunResponseBuilder(ctx, logger, n).Build(), nil
}

func NlgRunResponseBuilder(ctx context.Context, logger log.Logger, n libnlg.NLG) libmegamind.RunResponse {
	asset := n.RandomAsset(ctx)
	ctxlog.Infof(ctx, logger, "NlgRunResponse: `%s`", asset.Text())
	setrace.InfoLogEvent(
		ctx, logger,
		fmt.Sprintf("NlgRunResponse: `%s`", asset.Text()),
	)
	return libmegamind.NewRunResponse(IoTProductScenarioName, IoTScenarioIntent).WithLayout(asset.Text(), asset.Speech())
}
