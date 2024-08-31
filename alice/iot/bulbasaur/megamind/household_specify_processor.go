package megamind

import (
	"context"
	"strings"

	"a.yandex-team.ru/alice/iot/bulbasaur/megamind/common"
	"a.yandex-team.ru/alice/iot/bulbasaur/model"
	"a.yandex-team.ru/alice/library/go/libmegamind"
	"a.yandex-team.ru/alice/megamind/protos/scenarios"
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/log/ctxlog"
	"a.yandex-team.ru/library/go/core/xerrors"
)

type HouseholdSpecifiedProcessor struct {
	Logger           log.Logger
	BegemotProcessor *BegemotProcessor
	FrameRouter      *FrameRouter
}

func (p *HouseholdSpecifiedProcessor) Name() string {
	return SpecifyHouseholdFrame
}

func (p *HouseholdSpecifiedProcessor) IsRunnable(request *scenarios.TScenarioRunRequest) bool {
	callback := request.Input.GetCallback()
	if callback == nil || !strings.HasPrefix(callback.Name, SpecifyHouseholdCallbackName) || request.GetBaseRequest().State == nil {
		return false
	}
	return true
}

func (p *HouseholdSpecifiedProcessor) Run(ctx context.Context, request *scenarios.TScenarioRunRequest, u model.ExtendedUserInfo) (*scenarios.TScenarioRunResponse, error) {
	ctxlog.Infof(ctx, p.Logger, "specify household for user %d", u.User.ID)
	callback := request.Input.GetCallback()
	if callback == nil {
		return nil, xerrors.New("Callback not found")
	}
	householdID := GetHouseholdIDFromPayload(callback.Payload)
	state := request.GetBaseRequest().State

	var specifyRequestState common.SpecifyRequestState
	if err := specifyRequestState.FromBaseRequestState(state); err == nil {
		return p.runFrameProcessorWithSpecifiedHousehold(ctx, request, specifyRequestState, householdID, u)
	}

	begemotDataSource := &scenarios.TDataSource{}
	if err := state.UnmarshalTo(begemotDataSource); err != nil {
		return ErrorRunResponse(ctx, p.Logger, err)
	}
	for i := range begemotDataSource.GetBegemotIotNluResult().GetTypedHypotheses() {
		begemotDataSource.GetBegemotIotNluResult().TypedHypotheses[i].HouseholdIds = append(begemotDataSource.GetBegemotIotNluResult().TypedHypotheses[i].HouseholdIds, householdID)
	}
	return p.BegemotProcessor.processBegemotIOTNluResult(ctx, u, begemotRequestData{
		clientInfo:             libmegamind.NewClientInfo(request.BaseRequest.ClientInfo),
		protoBegemotDataSource: begemotDataSource,
		protoTandemDataSource:  nil,
		sources:                nil,
	})
}

func (p *HouseholdSpecifiedProcessor) runFrameProcessorWithSpecifiedHousehold(ctx context.Context, request *scenarios.TScenarioRunRequest, state common.SpecifyRequestState, householdID string, u model.ExtendedUserInfo) (*scenarios.TScenarioRunResponse, error) {
	householdSlot := libmegamind.Slot{
		Name:  string(common.HouseholdSlotName),
		Type:  string(common.HouseholdSlotType),
		Value: householdID,
	}

	processorContext := common.NewRunProcessorContext(
		ctx,
		u.UserInfo,
		common.NewClientInfo(request.GetBaseRequest().GetClientInfo(), request.GetDataSources()),
		libmegamind.SemanticFrame{
			Frame: state.SemanticFrames[0],
		},
	)

	return p.FrameRouter.RunWithSpecifiedSlots(processorContext, householdSlot)
}
