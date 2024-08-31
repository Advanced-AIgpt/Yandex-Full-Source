package megamind

import (
	"context"

	"google.golang.org/protobuf/types/known/anypb"

	"a.yandex-team.ru/alice/iot/bulbasaur/megamind/common"
	"a.yandex-team.ru/alice/iot/bulbasaur/megamind/sdk"
	"a.yandex-team.ru/alice/iot/bulbasaur/model"
	"a.yandex-team.ru/alice/library/go/libmegamind"
	"a.yandex-team.ru/alice/megamind/protos/scenarios"
)

type NamedFrameProcessor interface {
	Name() string
}

type FrameRunProcessor interface {
	NamedFrameProcessor
	SupportedInputs() sdk.SupportedInputs

	Run(ctx context.Context, frame libmegamind.SemanticFrame, runRequest *scenarios.TScenarioRunRequest, user model.User) (*scenarios.TScenarioRunResponse, error)
	CoolerRun(runContext sdk.RunContext, input sdk.Input) (*scenarios.TScenarioRunResponse, error)
}

type FrameApplyProcessor interface {
	NamedFrameProcessor
	IsApplicable(applyArguments *anypb.Any) bool
	Apply(ctx context.Context, applyRequest *scenarios.TScenarioApplyRequest, user model.User) (*scenarios.TScenarioApplyResponse, error)
}

type FrameContinueProcessor interface {
	NamedFrameProcessor
	IsContinuable(continueArguments *anypb.Any) bool
	Continue(ctx context.Context, continueRequest *scenarios.TScenarioApplyRequest, user model.User) (*scenarios.TScenarioContinueResponse, error)
}

type FrameRunApplyProcessor interface {
	NamedFrameProcessor
	FrameRunProcessor
	FrameApplyProcessor
}

type FrameRunContinueProcessor interface {
	NamedFrameProcessor
	FrameRunProcessor
	FrameContinueProcessor
}

type SpecifySupportingProcessor interface {
	FrameRunProcessor
	// TODO(aaulayev) remove RunWithSpecifiedSlots when action processor is updated
	RunWithSpecifiedSlots(processorContext common.RunProcessorContext, specifiedSlots ...libmegamind.Slot) (*scenarios.TScenarioRunResponse, error)
	RunWithSpecifiedSlotsV2(runContext sdk.RunContext, input sdk.Input, specifiedSlots ...sdk.GranetSlot) (*scenarios.TScenarioRunResponse, error)
}
