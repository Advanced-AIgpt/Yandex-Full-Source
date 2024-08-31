package specification

import (
	"context"
	"fmt"

	"google.golang.org/protobuf/types/known/structpb"

	"a.yandex-team.ru/alice/iot/bulbasaur/megamind"
	"a.yandex-team.ru/alice/iot/bulbasaur/megamind/common"
	"a.yandex-team.ru/alice/iot/bulbasaur/megamind/frames"
	"a.yandex-team.ru/alice/iot/bulbasaur/megamind/sdk"
	"a.yandex-team.ru/alice/iot/bulbasaur/model"
	"a.yandex-team.ru/alice/library/go/libmegamind"
	commonpb "a.yandex-team.ru/alice/megamind/protos/common"
	"a.yandex-team.ru/alice/megamind/protos/scenarios"
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/xerrors"
)

var SpecifiedTimeCallbackName libmegamind.CallbackName = "specified_time"

type TimeSpecificationProcessor struct {
	logger      log.Logger
	FrameRouter *megamind.FrameRouter
}

func NewTimeSpecificationProcessor(logger log.Logger, frameRouter *megamind.FrameRouter) *TimeSpecificationProcessor {
	return &TimeSpecificationProcessor{
		logger:      logger,
		FrameRouter: frameRouter,
	}
}

func (p *TimeSpecificationProcessor) SupportedInputs() sdk.SupportedInputs {
	return sdk.SupportedInputs{
		SupportedCallbacks: []libmegamind.CallbackName{
			SpecifiedTimeCallbackName,
		},
	}
}

func (p *TimeSpecificationProcessor) Run(ctx context.Context, frame libmegamind.SemanticFrame, runRequest *scenarios.TScenarioRunRequest, user model.User) (*scenarios.TScenarioRunResponse, error) {
	return p.CoolerRun(sdk.NewRunContext(ctx, p.logger, runRequest, user), sdk.InputFrames(frame))
}

func (p *TimeSpecificationProcessor) CoolerRun(runContext sdk.RunContext, _ sdk.Input) (*scenarios.TScenarioRunResponse, error) {
	user, ok := runContext.User()
	if !ok {
		return nil, xerrors.Errorf("failed to run time specify processor: user is unauthorized")
	}

	runContext.Logger().Info(fmt.Sprintf("specify time for user %d", user.ID))

	inputFrames := runContext.Request().GetInput().GetSemanticFrames()
	if len(inputFrames) == 0 {
		return nil, xerrors.Errorf("failed to run time specify processor: no semantic frames in input")
	}

	timeSlot, err := parseScenariosTimeSpecifyFrame(inputFrames[0])
	if err != nil {
		return nil, xerrors.Errorf("failed to parse time slot: %v", err)
	}

	state := runContext.Request().GetBaseRequest().State

	var specifyRequestState common.SpecifyRequestState
	if specifyRequestState.FromBaseRequestState(state) != nil {
		return nil, xerrors.Errorf("failed to get specifyRequestState: %w", err)
	}

	return p.FrameRouter.RunWithSpecifiedSlotsV2(
		runContext,
		libmegamind.NewSemanticFrames(specifyRequestState.SemanticFrames),
		&timeSlot,
	)
}

func (p *TimeSpecificationProcessor) Name() string {
	return string(frames.SpecifyTimeFrameName)
}

func (p *TimeSpecificationProcessor) IsRunnable(request *scenarios.TScenarioRunRequest) bool {
	callback := request.Input.GetCallback()
	if callback == nil || callback.Name != string(SpecifiedTimeCallbackName) {
		return false
	}

	if request.GetBaseRequest().State == nil {
		return false
	}

	if input := request.GetInput(); input != nil && len(input.SemanticFrames) > 0 {
		for _, frame := range input.SemanticFrames {
			if frame.Name == string(frames.SpecifyTimeFrameName) {
				return true
			}
		}
	}

	return false
}

func parseScenariosTimeSpecifyFrame(frame *commonpb.TSemanticFrame) (frames.ExactTimeSlot, error) {
	slot, ok := libmegamind.SemanticFrame{Frame: frame}.Slot("time", "sys.time")
	if !ok {
		return frames.ExactTimeSlot{}, xerrors.New("run: time slot with type sys.time not found")
	}
	begemotTime := common.BegemotTime{}
	if err := begemotTime.FromValueString(slot.Value); err != nil {
		return frames.ExactTimeSlot{}, err
	}
	timeSlot := frames.ExactTimeSlot{
		Times: []*common.BegemotTime{
			&begemotTime,
		},
	}
	return timeSlot, nil
}

func TimeSpecifyEmptyCallback() libmegamind.CallbackFrameAction {
	return libmegamind.CallbackFrameAction{
		FrameSlug:       string(frames.SpecifyTimeFrameName),
		FrameName:       string(frames.SpecifyTimeFrameName),
		CallbackName:    SpecifiedTimeCallbackName,
		Phrases:         []string{},
		CallbackPayload: &structpb.Struct{Fields: map[string]*structpb.Value{}},
	}
}
