package megamind

import (
	"context"

	dtopush "a.yandex-team.ru/alice/iot/vulpix/dto/push"
	"a.yandex-team.ru/alice/iot/vulpix/protos"
	"a.yandex-team.ru/alice/megamind/protos/scenarios"
)

type RunProcessor interface {
	Name() string
	IsRunnable(request *scenarios.TScenarioRunRequest) bool
	Run(ctx context.Context, userID uint64, request *scenarios.TScenarioRunRequest) (*scenarios.TScenarioRunResponse, error)
}

type ApplyProcessor interface {
	IsApplicable(request *scenarios.TScenarioApplyRequest, arguments *protos.TApplyArguments) bool
	Apply(ctx context.Context, userID uint64, request *scenarios.TScenarioApplyRequest, arguments *protos.TApplyArguments) (*scenarios.TScenarioApplyResponse, error)
}

type ContinueProcessor interface {
	IsContinuable(request *scenarios.TScenarioApplyRequest, arguments *protos.TContinueArguments) bool
	Continue(ctx context.Context, userID uint64, request *scenarios.TScenarioApplyRequest, arguments *protos.TContinueArguments) (*scenarios.TScenarioContinueResponse, error)
}

type RunApplyProcessor interface {
	RunProcessor
	ApplyProcessor
}

type IPushController interface {
	IoTBroadcastSuccessPush(ctx context.Context, userID uint64, deviceID string, frame dtopush.IoTBroadcastSuccess) error
	IoTBroadcastFailurePush(ctx context.Context, userID uint64, deviceID string, frame dtopush.IoTBroadcastFailure) error
	IoTDiscoverySuccessPush(ctx context.Context, userID uint64, deviceID string, frame dtopush.IoTDiscoverySuccess) error
}
