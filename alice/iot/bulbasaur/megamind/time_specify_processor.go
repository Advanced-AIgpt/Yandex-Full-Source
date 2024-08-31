package megamind

import (
	"context"
	"encoding/json"
	"fmt"

	"a.yandex-team.ru/alice/iot/bulbasaur/dto/megamind"
	"a.yandex-team.ru/alice/iot/bulbasaur/megamind/common"
	"a.yandex-team.ru/alice/iot/bulbasaur/model"
	"a.yandex-team.ru/alice/library/go/libmegamind"
	pcommon "a.yandex-team.ru/alice/megamind/protos/common"
	"a.yandex-team.ru/alice/megamind/protos/scenarios"
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/log/ctxlog"
	"a.yandex-team.ru/library/go/core/xerrors"
)

type TimeSpecifiedProcessor struct {
	Logger           log.Logger
	BegemotProcessor *BegemotProcessor
	FrameRouter      *FrameRouter
}

func (t *TimeSpecifiedProcessor) Name() string {
	return SpecifyTimeFrame
}

func (t *TimeSpecifiedProcessor) IsRunnable(request *scenarios.TScenarioRunRequest) bool {
	callback := request.Input.GetCallback()
	if callback == nil || callback.Name != SpecifyTimeCallbackName {
		return false
	}

	if request.GetBaseRequest().State == nil {
		return false
	}

	if input := request.GetInput(); input != nil && len(input.SemanticFrames) > 0 {
		for _, frame := range input.SemanticFrames {
			if frame.Name == SpecifyTimeFrame {
				return true
			}
		}
	}

	return false
}

func (t *TimeSpecifiedProcessor) Run(ctx context.Context, request *scenarios.TScenarioRunRequest, u model.ExtendedUserInfo) (*scenarios.TScenarioRunResponse, error) {
	ctxlog.Info(ctx, t.Logger, fmt.Sprintf("Specify time for user %d", u.User.ID), log.Any("scenario", t.Name()))

	var timeSource megamind.Time
	var timeFrame *pcommon.TSemanticFrame
	if input := request.GetInput(); input != nil && len(input.SemanticFrames) > 0 {
		for _, frame := range input.SemanticFrames {
			if frame.Name != SpecifyTimeFrame {
				continue
			}

			timeFrame = frame
			var err error
			timeSource, err = parseScenariosTimeSpecifyFrame(frame)
			if err != nil {
				ctxlog.Warnf(ctx, t.Logger, "Parsing time slot is failed with error: %v", err)
				return nil, err
			}
		}
	}

	state := request.GetBaseRequest().State

	var specifyRequestState common.SpecifyRequestState
	if specifyRequestState.FromBaseRequestState(state) == nil {
		slot, ok := libmegamind.SemanticFrame{Frame: timeFrame}.Slot("time", "sys.time")
		if !ok {
			return nil, xerrors.New("failed to find time slot in time specify frame")
		}
		return t.runFrameProcessorWithSpecifiedTime(ctx, request, specifyRequestState, slot, u)
	}

	begemotDataSource := &scenarios.TDataSource{}
	if err := state.UnmarshalTo(begemotDataSource); err != nil {
		return ErrorRunResponse(ctx, t.Logger, err)
	}

	return t.BegemotProcessor.processBegemotIOTNluResult(ctx, u, begemotRequestData{
		clientInfo:             libmegamind.NewClientInfo(request.BaseRequest.ClientInfo),
		protoBegemotDataSource: begemotDataSource,
		protoTandemDataSource:  nil,
		sources:                NewAdditionalSources().WithTime(timeSource),
	})
}

func (t *TimeSpecifiedProcessor) runFrameProcessorWithSpecifiedTime(ctx context.Context, request *scenarios.TScenarioRunRequest, state common.SpecifyRequestState, specifiedTimeSlot libmegamind.Slot, u model.ExtendedUserInfo) (*scenarios.TScenarioRunResponse, error) {
	timeSlot := libmegamind.Slot{
		Name:  string(common.TimeSlotName),
		Type:  string(common.TimeSlotType),
		Value: specifiedTimeSlot.Value,
	}

	processorContext := common.NewRunProcessorContext(
		ctx,
		u.UserInfo,
		common.NewClientInfo(request.GetBaseRequest().GetClientInfo(), request.GetDataSources()),
		libmegamind.SemanticFrame{
			Frame: state.SemanticFrames[0],
		},
	)

	return t.FrameRouter.RunWithSpecifiedSlots(processorContext, timeSlot)
}

func parseScenariosTimeSpecifyFrame(frame *pcommon.TSemanticFrame) (megamind.Time, error) {
	slot, ok := libmegamind.SemanticFrame{Frame: frame}.Slot("time", "sys.time")
	if !ok {
		return megamind.Time{}, xerrors.New("run: time slot with type sys.time not found")
	}
	var timeSlot megamind.Time
	if err := json.Unmarshal([]byte(slot.Value), &timeSlot); err != nil {
		return megamind.Time{}, err
	}
	return timeSlot, nil
}
